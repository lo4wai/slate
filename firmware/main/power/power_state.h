#pragma once

// RTC slow memory 持久化的電源狀態。深睡跨越保留，掉電後清零。
//
// 用途：
//   - 記錄當前幀是否有服務端下發的動態刷新間隔
//   - 避免每次 wake 都走完整 onboarding（cold boot 時數值為 0，按默認策略）
//
// 選 RTC slow RAM 而不是 NVS 的原因：寫次數高（每次睡都更新）、不耐 flash 壽命。
//
// 當前幀調度狀態機：
//   - FrameScene / BgRefreshScene 展示某幀後調用 SetCurrentFrameFromMeta(seq, meta)，
//     同時更新 RTC slow memory 與 LittleFS 中的 current_frame_seq。
//   - SleepManager 睡前調用 ComputeNextWakeSec()；只有 meta.ttl_sec 有效的動態幀才配置
//     RTC timer，靜態幀只靠按鍵/插電喚醒後再同步。
//   - Timer wake 的後台刷新路徑先 RestoreCurrentFrameScheduleFromCache()，再只上報/刷新當前幀。
//   - 組被清空或內容不可用時 ClearCurrentFrame() 把序號和動態調度都回到靜態默認態。

#include <cstddef>
#include <cstdint>

#include "storage/cache/cache.h"

namespace power_state {

struct CurrentFrameSchedule {
    bool     dynamic         = false;
    uint32_t server_sync_sec = 0;
};

// Explicitly reset RTC slow-memory state on cold boot. Deep-sleep wake keeps it.
void Init(bool cold_boot);

// 當前展示幀的刷新策略。FrameScene::LoadFrame 寫入；RTC timer 喚醒後台同步當前
// 動態幀時,SyncService 也會更新這裏的 next_wake_sec。靜態幀不會自己變更,
// 不配置定時喚醒,避免為了無意義同步空耗電。
CurrentFrameSchedule GetCurrentFrameSchedule();
void                 SetCurrentFrameSchedule(const CurrentFrameSchedule& schedule);

int  GetCurrentFrameSeq();
void SetCurrentFrameSeq(int seq);
bool CurrentFrameNeedsTimerWake();
void SetCurrentFrameFromMeta(int seq, const cache::FrameMeta& meta);
void ClearCurrentFrame();

// 從 LittleFS cache 恢復當前幀序號和動態刷新策略。用於 deep sleep 前兜底，
// 防止 RTC slow memory 因 reset/cold boot 丟失後下一輪 timer wake 被關閉。
bool RestoreCurrentFrameScheduleFromCache();

// 當前動態幀的下次 RTC timer wakeup 間隔（秒）。0 表示當前幀沒有動態刷新間隔。
// 已疊加不可達退避：連續 timer wake 聯繫不上服務器時指數拉長，封頂 ~1h。
uint32_t ComputeNextWakeSec();

// 記錄一次 timer wake 的同步結果。success=true 清零退避計數；false 遞增（封頂）。
// 由後台刷新路徑調用：網絡建立失敗 / sync 完成(ok 或失敗) 時各上報一次。
void RecordTimerWakeResult(bool success);

// 睡前最後一次刷到物理屏上的狀態欄 1bpp 快照。用於 timer wake 後重建
// prev_buffer_ 的 0~24 行，讓後台 partial refresh 的 old/new 輸入真實一致。
bool SaveStatusBarSnapshot(const uint8_t* data, size_t len);
bool LoadStatusBarSnapshot(uint8_t* out, size_t len);
void ClearStatusBarSnapshot();

}  // namespace power_state
