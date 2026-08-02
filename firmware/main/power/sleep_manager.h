#pragma once

// 閒置自動深睡。Tick() 檢測閒置時長 ≥ 閾值 + 不在充電 + 啓用狀態時,
// 直接 esp_deep_sleep_start。醒來由 ext1 wakeup(GPIO 0/18 任一拉低)或重啓觸發,
// app_main 重新跑。
//
// 硬件限制:ESP32-S3 RTC GPIO 範圍 0-21,GPIO 39(UP 鍵)不是 RTC IO,不能 ext1 喚醒。
// 只能 BOOT(GPIO0) / DOWN(GPIO18) 醒來。用户想看上一幀需要先按 DOWN/BOOT 醒,
// 再按 UP 翻。
//
// Unbound grace 窗口:設備未綁定時禁 deep sleep,讓 SyncService 快輪詢,
// 用户在 Web 端輸碼後屏切「等待內容組」。輪詢間隔階梯退避(10s→30s→60s),
// 窗口最長 2h,過期或低電量(<20%)後回退正常省電策略,避免耗光電池。

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>

struct UiEvent;

class SleepManager {
   public:
    enum class SleepOutcome {
        kSlept,
        kPausedByCharge,
        kDisabled,
        kUnboundGrace,
    };

    // unbound 狀態保持禁睡的最長窗口。超過則即便仍 unbound 也允許 deep sleep。
    static constexpr int64_t kUnboundGraceMs = 2LL * 60 * 60 * 1000;
    // 電量低於此閾值時強制允許 deep sleep,無視 unbound 狀態。
    static constexpr int kLowBatteryPct = 20;

    // 看門狗：語音/同步等 blocker 連續阻止深睡超過此時長時，強制走一次帶守衞的
    // 深睡嘗試（內部會停掉語音/同步）。兜底「會話/下載卡死 → 永不睡 → 耗光電池」。
    static constexpr int64_t kMaxBlockedMs = 15LL * 60 * 1000;

    struct SleepDecision {
        SleepOutcome outcome;
        uint32_t     configured_next_wake_sec;
    };

    struct Policy {
        int     idle_timeout_min = 5;
        int64_t unbound_grace_ms = kUnboundGraceMs;
        int     low_battery_pct  = kLowBatteryPct;
        bool    disabled         = false;
    };

    void Init(Policy p);
    void SetSleepBlocker(std::function<bool()> blocks_sleep);
    void Disable();  // captive portal 等場景禁用 deep sleep

    void OnEvent(const UiEvent& e);
    void Tick(int64_t now_ms);

    // 主動進 deep sleep。**正常情況不返回**；若被 paused_(充電中)/enabled_=false 短路，
    // 會立刻 return,調用方應轉入正常 active 模式(例如把 cache 中的內容組 push 成 FrameScene)。
    SleepDecision TryEnterDeepSleep();

   private:
    // 當前是否處於 unbound 加速窗口(unbound + 未超 2h + 電量充足)。
    bool     InUnboundGrace(int64_t now_ms) const;
    bool     MarkUnboundIfNeeded(int64_t now_ms);
    uint32_t ComputeConfiguredNextWakeSec() const;
    bool     BlocksSleep() const;

    std::atomic<bool>    enabled_{false};
    std::atomic<int64_t> last_active_ms_{0};
    // blocker(語音/同步)開始連續阻止深睡的時刻；0 表示當前未被阻止。看門狗據此計時。
    std::atomic<int64_t> blocked_since_ms_{0};
    std::atomic<bool>    paused_{false};
    int                  idle_timeout_min_ = 5;
    int64_t              unbound_grace_ms_ = kUnboundGraceMs;
    int                  low_battery_pct_  = kLowBatteryPct;

    struct UnboundState {
        bool    unbound     = false;
        int     battery_pct = 100;
        int64_t since_ms    = 0;
    };

    mutable std::mutex unbound_mutex_;
    UnboundState       unbound_state_;

    std::function<bool()> blocks_sleep_;
};
