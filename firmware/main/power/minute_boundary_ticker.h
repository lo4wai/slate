#pragma once

// 跨分鐘時 evt::Post(kMinuteTick)。單次定時器，每次對齊到「下一個分鐘邊界」再觸發，
// 觸發後重新計算並續 arm。相比舊的每秒輪詢，空閒時 CPU 每分鐘只被喚醒一次（其餘
// 59 次省掉），讓自動 light sleep 能睡滿；對齊用牆鍾，SNTP 校時後下一拍自動歸位。

#include <esp_timer.h>

#include <atomic>

class MinuteBoundaryTicker {
   public:
    ~MinuteBoundaryTicker();

    void Start();
    void Stop();

   private:
    static void TickCb(void* arg);
    // 計算到下一分鐘邊界的延時並 arm 單次定時器（帶小 epsilon 確保落在邊界之後）。
    void ArmNextBoundary();

    esp_timer_handle_t timer_ = nullptr;
    std::atomic<int>   last_minute_{-1};
};
