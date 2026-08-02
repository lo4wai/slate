#include "power/minute_boundary_ticker.h"

#include <esp_log.h>
#include <sys/time.h>
#include <ctime>

#include "events/event_bus.h"

namespace {
constexpr char kTag[] = "minute_boundary";

// 觸發點落在邊界之後一點點，避免因調度抖動在邊界前幾毫秒觸發、讀到上一分鐘。
constexpr int64_t kBoundaryEpsilonMs = 50;
constexpr int64_t kMinuteMs          = 60'000;

// 距下一分鐘邊界的毫秒數（含 epsilon）。牆鍾未同步時也能給出 ~60s 的穩定節拍。
int64_t MsToNextBoundary() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    const int64_t ms_into_min = (static_cast<int64_t>(tv.tv_sec) % 60) * 1000 + tv.tv_usec / 1000;
    int64_t       delay       = kMinuteMs - ms_into_min + kBoundaryEpsilonMs;
    if (delay <= kBoundaryEpsilonMs)
        delay += kMinuteMs;  // 已過/正好在邊界，推到下一分鐘
    return delay;
}
}  // namespace

MinuteBoundaryTicker::~MinuteBoundaryTicker() {
    Stop();
}

void MinuteBoundaryTicker::Start() {
    if (timer_)
        return;
    esp_timer_create_args_t args = {};
    args.callback                = &MinuteBoundaryTicker::TickCb;
    args.arg                     = this;
    args.dispatch_method         = ESP_TIMER_TASK;
    args.name                    = "minute_boundary";
    ESP_ERROR_CHECK(esp_timer_create(&args, &timer_));
    ArmNextBoundary();
}

void MinuteBoundaryTicker::ArmNextBoundary() {
    if (!timer_)
        return;
    esp_timer_start_once(timer_, MsToNextBoundary() * 1000);
}

void MinuteBoundaryTicker::Stop() {
    if (!timer_)
        return;
    esp_err_t err = esp_timer_stop(timer_);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(kTag, "timer stop failed err=%s", esp_err_to_name(err));
    }
    esp_timer_delete(timer_);
    timer_ = nullptr;
}

void MinuteBoundaryTicker::TickCb(void* arg) {
    auto* self = static_cast<MinuteBoundaryTicker*>(arg);

    time_t now = time(nullptr);
    if (now >= 1577836800) {  // 2020-01-01 之後才視為 SNTP 已同步；之前不發 tick，只續 arm
        struct tm tm;
        localtime_r(&now, &tm);
        int last = self->last_minute_.load(std::memory_order_acquire);
        if (tm.tm_min != last && self->last_minute_.compare_exchange_strong(last, tm.tm_min, std::memory_order_acq_rel,
                                                                            std::memory_order_acquire)) {
            evt::PostSimple(UiEventKind::kMinuteTick, evt::kNoWait);
        }
    }
    // 續 arm 到下一分鐘邊界。SNTP 校時若發生在本拍之前，這裏用新牆鍾重新對齊。
    self->ArmNextBoundary();
}
