#pragma once

// 後台同步：POST /api/v1/devices/current/poll 後按 manifest 增量拉 frame image / audio。
// 狀態/進度通過 EventBus 反饋：
//   - SyncStarted       每輪開始
//   - SyncFinished{ok}  每輪結束(含 304 noop)
//   - kSyncedGroupReady{gid,content_count}  當 selected_group 內容就緒

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "storage/cache/cache.h"
#include "sync/api_client.h"

class SyncService {
   public:
    static SyncService& Get();

    enum class InitialSync {
        kNone,
        kUserActive,
        kBackgroundRefresh,
    };

    void Start(std::string wake_reason, InitialSync initial_sync = InitialSync::kNone);
    void Stop();

    // 主動觸發一次前台 poll。用於後台刷新被充電/解綁寬限打斷後轉入 active 模式。
    void RequestUserActiveSync();

    // 設備主動 cycle 切組(scene 按鍵 callback 調)。
    // 內部置 BIT_CYCLE_NEXT/PREV,Loop 在喚醒時調 api::CycleGroup 然後立即 SyncOnce。
    void CycleNext();
    void CyclePrev();

    // 當前已就緒的 group_id(Scene::OnEnter 時讀)
    std::string CurrentGroupId() const;

    // 是否正在執行一次 sync 突發(poll/cycle → manifest → 拉幀)。SleepManager 用它阻止
    // idle deep sleep 在大文件下載中途打斷同步。只在單輪 SyncOnce/DoCycle 期間為真,
    // 突發之間(輪詢間隔)為假,因此不會讓設備永不睡(卡死由看門狗兜底)。
    bool IsBusy() const {
        return in_flight_.load(std::memory_order_acquire);
    }

   private:
    SyncService() = default;
    static void TaskEntry(void* arg);
    void        Loop();
    int         NextIntervalSec() const;
    enum class SyncMode { kUserActive, kBackgroundRefresh };
    enum class SyncReason { kUserActive, kBackgroundRefresh, kCycle };
    void               SyncOnce(SyncMode mode);
    void               Trigger(SyncMode mode);
    void               DoCycle(const std::string& direction);
    static const char* SyncModeName(SyncMode mode);
    bool SyncBackground(const api::DeviceState& state, const api::Telemetry& telemetry, bool& group_changed);
    bool SyncUserActive(const api::DeviceState& state, bool& group_changed);
    bool ShouldStop() const;
    bool SyncManifestAndFrames(const std::string& gid, const std::string& expected_etag, const std::string& group_name,
                               int expected_content_count, SyncReason reason, bool& group_changed);
    bool SyncCurrentContent(const std::string& gid, const api::ContentMeta& content, bool& changed);
    bool ClearSelectedGroup();
    bool HandleCachedManifestHit(const std::string& gid, const std::string& expected_etag,
                                 const std::string& status_name, int content_count, const std::string& previous_current,
                                 const std::string& selected_group_id, SyncReason reason);
    bool HandleNotModifiedManifest(const std::string& gid, const cache::ManifestMeta& cached_meta,
                                   const std::string& status_name, int expected_content_count,
                                   const std::string& previous_current, const std::string& selected_group_id,
                                   SyncReason reason);
    bool DownloadFramesToStage(cache::CacheWriter& writer, const std::string& gid, const api::Manifest& manifest,
                               const std::string& status_name, const std::string& previous_current,
                               const std::string& selected_group_id, SyncReason reason, int& total_updates);
    bool CommitStagedFrames(cache::CacheWriter& writer, const std::string& gid, const api::Manifest& manifest,
                            const std::string& group_name, const std::string& selected_group_id,
                            bool current_group_update, SyncReason reason, int total_updates, int old_content_count);
    void PostSyncedGroupReady(const std::string& gid, const std::string& name, int content_count, bool content_changed);
    std::string CurrentGroupSnapshot() const;
    void        SetCurrentGroup(const std::string& gid);
    void        ClearCurrentGroup();

    std::atomic<bool>    running_{false};
    std::atomic<bool>    in_flight_{false};
    mutable std::mutex   task_mutex_;
    mutable std::mutex   current_group_mutex_;
    EventGroupHandle_t   event_group_ = nullptr;
    SemaphoreHandle_t    exit_sem_    = nullptr;
    TaskHandle_t         task_handle_ = nullptr;
    mutable std::string  current_group_;
    std::string          wake_reason_;
    std::vector<uint8_t> download_buf_;
    enum class BoundState : uint8_t { kUnknown, kBound, kUnbound };
    // 跟蹤 bound 翻轉。Unknown 初始態保證首輪 unbound 也會 emit kUnbound。
    std::atomic<BoundState> was_bound_{BoundState::kUnknown};
    // 進入 unbound 狀態的時刻,用於階梯退避輪詢間隔。
    std::atomic<int64_t> unbound_since_ms_{0};
};
