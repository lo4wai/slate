#include "power/sleep_manager.h"

#include <driver/rtc_io.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <array>
#include <utility>

#include "bsp/board.h"
#include "bsp/charge_status.h"
#include "bsp/config.h"
#include "drivers/display/epd_ssd1683.h"
#include "drivers/display/framebuffer_ops.h"
#include "events/event_bus.h"
#include "power/power_state.h"
#include "power/shutdown.h"
#include "utils/gpio_util.h"
#include "utils/time_utils.h"

namespace {
constexpr char kTag[] = "sleep";

// 進 deep sleep 前等 EPD 刷新結束的最大時長。低温或 full cleanup 可接近 5s；
// 超時過短會在白相階段切 EPD 電源，留下整屏白。
constexpr int kEpdFlushTimeoutMs = 8000;

constexpr int kMaxBatteryPct = 100;
constexpr int kMinBatteryPct = 0;

int ClampBatteryPct(int pct) {
    if (pct < kMinBatteryPct)
        return kMinBatteryPct;
    if (pct > kMaxBatteryPct)
        return kMaxBatteryPct;
    return pct;
}

// 把單個 GPIO 配成 RTC 數字輸入 + 上拉 + hold,適合做 EXT1 ANY_LOW 喚醒源。
// 必須用 rtc_gpio_set_direction —— 僅 rtc_gpio_init 不改 direction,GPIO 仍可能
// 處於 ADC/iot_button 之前留下的非數字輸入態,EXT1 感知不到電平。
void PrepareWakeupGpio(gpio_num_t pin) {
    rtc_gpio_init(pin);
    rtc_gpio_set_direction(pin, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_dis(pin);
    rtc_gpio_pullup_en(pin);
    rtc_gpio_hold_en(pin);
}

// VBAT_PWR (GPIO17) 是軟鎖存,拉低=整機斷電(BOOT 喚醒無效=變磚)。
// ESP_SLEEP_GPIO_RESET_WORKAROUND 開了後,普通 GPIO 在 deep sleep 期間會被
// 強制復位。必須切到 RTC GPIO 域、顯式 rtc_gpio_hold_en 才能真正 hold 高電平。
void LockVbatPowerHigh() {
    auto pin = static_cast<gpio_num_t>(VBAT_PWR_PIN);
    gpio_hold_dis(pin);  // 先釋放普通 GPIO 域 hold,RTC GPIO 才能接管
    rtc_gpio_init(pin);
    rtc_gpio_set_direction(pin, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_pulldown_dis(pin);
    rtc_gpio_pullup_dis(pin);
    rtc_gpio_set_level(pin, 1);
    rtc_gpio_hold_en(pin);
}

void SaveStatusBarSnapshot(EpdSsd1683* epd) {
    if (!epd)
        return;
    std::array<uint8_t, epd::kStatusBarSnapshotBytes> snapshot{};
    if (!epd->ReadPreviousRaw1bpp(0, 0, epd::kStatusBarSnapshotWidth, epd::kStatusBarSnapshotHeight, snapshot.data(),
                                  snapshot.size())) {
        ESP_LOGW(kTag, "status snapshot skipped reason=previous_buffer_not_synced");
        power_state::ClearStatusBarSnapshot();
        return;
    }
    power_state::SaveStatusBarSnapshot(snapshot.data(), snapshot.size());
}

}  // namespace

void SleepManager::Init(Policy p) {
    idle_timeout_min_ = p.idle_timeout_min;
    unbound_grace_ms_ = p.unbound_grace_ms;
    low_battery_pct_  = p.low_battery_pct;
    last_active_ms_.store(time_utils::NowMs());
    {
        std::lock_guard<std::mutex> lock(unbound_mutex_);
        unbound_state_ = {};
    }
    enabled_.store(!p.disabled);
    ESP_LOGD(kTag, "init idle_min=%u disabled=%d unbound_grace_ms=%lld low_battery_pct=%d",
             static_cast<unsigned>(idle_timeout_min_), p.disabled ? 1 : 0, static_cast<long long>(unbound_grace_ms_),
             low_battery_pct_);
}

void SleepManager::SetSleepBlocker(std::function<bool()> blocks_sleep) {
    blocks_sleep_ = std::move(blocks_sleep);
}

void SleepManager::Disable() {
    enabled_.store(false);
}

void SleepManager::OnEvent(const UiEvent& e) {
    switch (e.kind) {
        case UiEventKind::kButtonShort:
        case UiEventKind::kButtonLong:
        case UiEventKind::kButtonDouble:
            ESP_LOGD(kTag, "activity kind=button event=%d btn=%d", static_cast<int>(e.kind),
                     static_cast<int>(e.u.button.btn));
            last_active_ms_.store(time_utils::NowMs());
            break;
        case UiEventKind::kChargeChanged:
            ESP_LOGD(kTag, "charge changed present=%d charging=%d full=%d no_battery=%d", e.u.charge.present ? 1 : 0,
                     e.u.charge.charging ? 1 : 0, e.u.charge.full ? 1 : 0, e.u.charge.no_battery ? 1 : 0);
            paused_.store(e.u.charge.present);
            if (e.u.charge.present) {
                last_active_ms_.store(time_utils::NowMs());
            }
            break;
        case UiEventKind::kBound: {
            std::lock_guard<std::mutex> lock(unbound_mutex_);
            unbound_state_.unbound  = false;
            unbound_state_.since_ms = 0;
        } break;
        case UiEventKind::kUnbound:
            // 僅首次進入 unbound 時記錄起始 ts,重複事件不重置(否則 2h 兜底永不觸發)。
            if (MarkUnboundIfNeeded(time_utils::NowMs())) {
                ESP_LOGW(kTag, "unbound grace hours=%lld", (long long)(kUnboundGraceMs / (60 * 60 * 1000)));
            }
            break;
        case UiEventKind::kBatteryUpdated: {
            std::lock_guard<std::mutex> lock(unbound_mutex_);
            unbound_state_.battery_pct = ClampBatteryPct(e.u.battery.pct);
        } break;
        default:
            break;
    }
}

bool SleepManager::InUnboundGrace(int64_t now_ms) const {
    std::lock_guard<std::mutex> lock(unbound_mutex_);
    if (!unbound_state_.unbound)
        return false;
    if (unbound_state_.battery_pct < low_battery_pct_)
        return false;
    return now_ms - unbound_state_.since_ms < unbound_grace_ms_;
}

bool SleepManager::MarkUnboundIfNeeded(int64_t now_ms) {
    std::lock_guard<std::mutex> lock(unbound_mutex_);
    if (unbound_state_.unbound)
        return false;
    unbound_state_.unbound  = true;
    unbound_state_.since_ms = now_ms;
    return true;
}

uint32_t SleepManager::ComputeConfiguredNextWakeSec() const {
    return power_state::ComputeNextWakeSec();
}

bool SleepManager::BlocksSleep() const {
    return blocks_sleep_ && blocks_sleep_();
}

void SleepManager::Tick(int64_t now_ms) {
    if (!enabled_.load())
        return;
    // 充電 / unbound 寬限是合法的「先別睡」，不計入看門狗（它們不是卡死），重置計時。
    if (paused_.load()) {
        blocked_since_ms_.store(0);
        return;
    }
    if (InUnboundGrace(now_ms)) {
        blocked_since_ms_.store(0);
        return;
    }

    bool forced = false;
    if (BlocksSleep()) {
        int64_t since = blocked_since_ms_.load();
        if (since == 0) {
            blocked_since_ms_.store(now_ms);
            since = now_ms;
            ESP_LOGI(kTag, "sleep blocked begin");
        }
        if (now_ms - since < kMaxBlockedMs)
            return;  // 給語音/同步收尾時間
        // 看門狗超時：強制走一次帶守衞的深睡嘗試。TryEnterDeepSleep → WaitForEpdAndShutdown
        // 會停掉語音(SuspendForSleep)與同步(SyncService::Stop)，打斷卡死的 blocker。
        ESP_LOGW(kTag, "sleep blocked timeout elapsed_ms=%lld limit_ms=%lld action=force_sleep",
                 (long long)(now_ms - since), (long long)kMaxBlockedMs);
        forced = true;
    } else {
        blocked_since_ms_.store(0);
        const int64_t idle_ms      = now_ms - last_active_ms_.load();
        const int64_t threshold_ms = static_cast<int64_t>(idle_timeout_min_) * 60 * 1000;
        if (idle_ms < threshold_ms)
            return;
        ESP_LOGI(kTag, "idle timeout idle_ms=%lld threshold_ms=%lld action=deep_sleep", (long long)idle_ms,
                 (long long)threshold_ms);
    }

    const auto decision = TryEnterDeepSleep();
    if (decision.outcome != SleepOutcome::kSlept) {
        // TryEnterDeepSleep can be refused by charge/unbound/disabled guards. Treat
        // that refusal as activity so Tick() does not spin the full sleep path every second.
        last_active_ms_.store(now_ms);
        // 強制嘗試被拒：重置看門狗計時，給 blocker 再一個完整窗口，避免每秒重複強制。
        if (forced)
            blocked_since_ms_.store(0);
    }
}

SleepManager::SleepDecision SleepManager::TryEnterDeepSleep() {
    power_state::RestoreCurrentFrameScheduleFromCache();
    const uint32_t next_sec = ComputeConfiguredNextWakeSec();
    if (!enabled_.load()) {
        return {SleepOutcome::kDisabled, next_sec};
    }
    const int64_t now_ms = time_utils::NowMs();
    if (InUnboundGrace(now_ms)) {
        return {SleepOutcome::kUnboundGrace, next_sec};
    }
    // paused_ 由 kChargeChanged 事件驅動,timer wake 路徑下可能尚未消化此事件。
    // 同時現場查詢硬件確保新插入的電源也能即時攔截。
    const bool power_present = Board::Get().charge()->Get().power_present;
    if (power_present || paused_.load()) {
        if (power_present)
            paused_.store(true);
        return {SleepOutcome::kPausedByCharge, next_sec};
    }
    ESP_LOGI(kTag, "deep sleep prepare");

    // 1) 停後台 task,避免在 rail 關閉後還有 I²C / 網絡寫操作，並等待已有 EPD 刷新完成。
    const bool epd_ready = power_shutdown::WaitForEpdAndShutdown(kEpdFlushTimeoutMs);

    // 2) 不主動製造一輪全刷。墨水屏內容本來可保留；
    //    靜態幀 idle 進睡眠時如果這裏再全刷一次，會白白耗電。
    if (auto* epd = Board::Get().epd()) {
        if (!epd_ready) {
            ESP_LOGW(kTag, "status snapshot skipped reason=epd_pending elapsed_ms=%d", kEpdFlushTimeoutMs);
            power_state::ClearStatusBarSnapshot();
        } else {
            SaveStatusBarSnapshot(epd);
        }
    }

    // 3) 關 EPD rail (GPIO6)。墨水屏像素雙穩態保留,controller 寄存器/電荷泵失效,
    //    醒來 EpdInit 重做時序就好。
    GpioWriteHold(EPD_PWR_PIN, 0);

    // 4) 關 audio rail (GPIO42)。**必須在 I²C 操作完成之後**:這條 rail 一關,
    //    R45/R46 上拉死,後續任何 I²C 都失敗。NVS / 其他後台任務清理已在前面完成。
    GpioWriteHold(AUDIO_PWR_PIN, 0);

    // 5) **關鍵防變磚**:VBAT_PWR (GPIO17) 必須保持高,否則整機斷電,BOOT 也喚不醒。
    //    用 RTC GPIO API 切到 RTC 域顯式 hold,繞開 GPIO_RESET_WORKAROUND。
    LockVbatPowerHigh();

    // 6) 配置 EXT1 喚醒源 GPIO,iot_button 之前佔用過 GPIO0/18 的 IO MUX,
    //    必須 rtc_gpio_set_direction(INPUT_ONLY) 復位回數字輸入,EXT1 才能感知。
    PrepareWakeupGpio(static_cast<gpio_num_t>(BOOT_BUTTON_GPIO));    // ENTER
    PrepareWakeupGpio(static_cast<gpio_num_t>(DOWN_BUTTON_GPIO));    // 下鍵(GPIO 39 UP 不是 RTC IO 用不了)
    PrepareWakeupGpio(static_cast<gpio_num_t>(CHARGE_DETECT_GPIO));  // 插 USB 自動喚醒

    constexpr uint64_t kWakeupMask =
        (1ULL << BOOT_BUTTON_GPIO) | (1ULL << DOWN_BUTTON_GPIO) | (1ULL << CHARGE_DETECT_GPIO);
    esp_sleep_enable_ext1_wakeup(kWakeupMask, ESP_EXT1_WAKEUP_ANY_LOW);

    // 7) RTC timer 只服務當前動態幀。靜態幀不會自己變更，靠 timer wake
    //    週期性聯網只會空耗電；遠端靜態內容變化等用户按鍵/插電喚醒後再同步。
    if (next_sec > 0) {
        esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(next_sec) * 1'000'000ULL);
        ESP_LOGI(kTag, "deep sleep start wake_mask=0x%llx timer_sec=%u", (unsigned long long)kWakeupMask,
                 static_cast<unsigned>(next_sec));
    } else {
        // 靜態幀不配定時喚醒。但自動 light sleep(CONFIG_PM_ENABLE/tickless idle)會把
        // RTC timer 喚醒源留在 esp_sleep 配置裏,不顯式清掉的話 esp_deep_sleep_start 會繼承
        // 這個「幻影 timer」,導致 timer_sec=0 的深睡約 1s 後就被 wake=rtc_timer 喚醒、
        // 每分鐘空醒重啓(實測)。必須顯式禁用 timer 喚醒源,確保靜態幀只靠 EXT1/插電醒。
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        ESP_LOGI(kTag, "deep sleep start wake_mask=0x%llx timer_sec=0", (unsigned long long)kWakeupMask);
    }
    esp_deep_sleep_start();
    __builtin_unreachable();
}
