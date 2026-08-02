#pragma once

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <lvgl.h>

#include <atomic>
#include <functional>
#include <vector>

#include "drivers/display/framebuffer_ops.h"

// SSD1683 類驅動 4.2" 黑白 EPD（400×300，1bpp）+ LVGL 集成。
// SPI 寫幀 + 異步 refresh_task（300 ms 節流，防過頻刷新損傷 EPD）+ LVGL flush_cb
// 閾值化 RGB565→1bpp。RequestUrgentFullRefresh() 立即觸發全幀刷新。
class EpdSsd1683 {
   public:
    static constexpr int kWidth     = 400;
    static constexpr int kHeight    = 300;
    static constexpr int kBufferLen = ((kWidth + 7) / 8) * kHeight;

    EpdSsd1683();
    ~EpdSsd1683();

    void Init();

    bool IsRefreshPending();
    // 阻塞輪詢直到刷新結束或超時。返回 true=已空閒，false=超時仍 pending。
    // 進深睡 / 關 rail / 後台刷新結束前用它確保不在刷新中途切斷 EPD 電源。
    bool WaitForRefreshIdle(int timeout_ms);
    void RequestUrgentPartialRefresh();  // partial(~1s 殘影)
    void RequestUrgentFullRefresh();     // full(~5s 乾淨)

    // 直接把 1bpp 原始數據寫入 framebuffer，繞過 LVGL 管線。
    // bit=1=白，bit=0=黑（與服務端下發格式一致，無需反轉）。
    // 調用後自動 notify refresh_task；調用方再發 RequestUrgentXxxRefresh 設 urgent 標誌。
    void WriteRaw1bpp(int x, int y, int w, int h, const uint8_t* data, size_t len);

    // 把已知的當前物理畫面種到 buffer_/prev_snapshot_，不觸發刷新。
    // deep sleep 喚醒後內存丟失，但 EPD 物理像素仍保持；timer 自動刷新要先用
    // 睡前緩存重建 previous snapshot，後續才能做真正 partial 而不是首次 full 清屏。
    void SeedPreviousRaw1bpp(int x, int y, int w, int h, const uint8_t* data, size_t len);

    // 讀取上次已刷到物理屏的 framebuffer 快照。只有 prev_snapshot_ 已同步時返回 true。
    bool ReadPreviousRaw1bpp(int x, int y, int w, int h, uint8_t* out, size_t len);

    bool Lock(int timeout_ms = 0);
    void Unlock();

    lv_display_t* lvgl_display() {
        return lvgl_display_;
    }

   private:
    spi_device_handle_t spi_        = nullptr;
    bool                spi_inited_ = false;

    uint8_t*             buffer_          = nullptr;  // 實時 framebuffer（LVGL flush 寫入）
    uint8_t*             snapshot_        = nullptr;  // refresh_task 凍結的本輪快照
    uint8_t*             prev_snapshot_   = nullptr;  // 上次已刷到 EPD 的快照
    uint8_t*             lvgl_render_buf_ = nullptr;
    std::vector<uint8_t> epd_line_;

    // 200 而非 128：LVGL anti-alias 字體邊緣的灰度像素被劃入「黑」，
    // 字體看起來粗實清晰。128 中性二值化會讓灰邊判白丟失,字體發虛。
    static constexpr uint8_t kBwThreshold = 200;

    lv_display_t* lvgl_display_ = nullptr;
    static void   LvglFlushCb(lv_display_t* disp, const lv_area_t* area, uint8_t* color_p);

    SemaphoreHandle_t    dirty_mutex_  = nullptr;
    TaskHandle_t         refresh_task_ = nullptr;
    SemaphoreHandle_t    refresh_exit_ = nullptr;
    epd::Rect            dirty_;
    bool                 pending_                  = false;
    bool                 urgent_refresh_           = false;
    bool                 force_full_refresh_       = false;
    bool                 refresh_task_stop_        = false;
    bool                 prev_snapshot_synced_     = false;
    bool                 refresh_in_progress_      = false;
    TickType_t           last_sample_tick_         = 0;
    TickType_t           last_flush_tick_          = 0;  // LVGL 最後一次 flush_cb 的時刻,用於等待靜默
    int                  sample_interval_ms_       = 300;
    int                  partial_since_full_       = 0;  // 累積多少次 partial 後強制 full 清殘影
    static constexpr int kPartialBeforeFullCleanup = 8;

    void        StartRefreshTask();
    static void RefreshTaskEntry(void* arg);
    void        RefreshTaskLoop();
    bool        RefreshTaskShouldStop();
    void        DebounceRefreshNotify();
    bool        TakeRefreshRequest(bool& urgent, bool& force_full);
    bool        ThrottleRefreshSampling(bool urgent, bool force_full);
    bool        CaptureRefreshSnapshot(bool force_full, epd::DiffResult& diff, bool& prev_synced);
    bool        ShouldUseFullRefresh(const epd::DiffResult& diff, bool force_full, bool prev_synced) const;
    void        RunRefresh(bool full_refresh);
    void        FinishRefreshSnapshot();
    void        MarkRefreshIdle();

    void AssertRefreshTaskContext() const;
    void SpiPortInit();    // 發送模式（DI 當 MOSI，40 MHz）
    void SpiPortRxInit();  // 接收模式（DI 反向當 MISO，8 MHz）—— 讀温度寄存器
    void SpiGpioInit();
    void EpdInit();
    void EpdDisplayFull();
    void EpdDisplayPartial();
    void EpdTurnOnDisplay();
    // 讀屏內温度寄存器(0x40)→映射 5 檔 booster 寫 0xE0/0xE6,Full/Partial 共用。
    // 60 s 內重複刷新會複用上次温度避免每次 5~10 ms 切換 SPI 模式開銷。
    void    ApplyTemperatureBoost();
    int64_t last_temp_read_ms_ = 0;
    uint8_t cached_booster_    = 0;
    void    EpdSendCommand(uint8_t c);
    void    EpdSendData(uint8_t d);
    uint8_t EpdRecvData();  // refresh_task only:切到 RX 模式讀 1 字節,讀完切回 TX
    void    WriteBytes(const uint8_t* buf, int len);
    void    ReadBusy();
    void    EpdPowerOn();  // 主動管 EPD_PWR_PIN(含 hold_dis/set/hold_en)
    void    EpdPowerOff();

    // pin 緩存（來自 bsp/config.h）
    gpio_num_t        cs_, dc_, rst_, busy_, mosi_, sclk_;
    spi_host_device_t spi_host_;
};
