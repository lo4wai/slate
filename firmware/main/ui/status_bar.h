#pragma once

// 頂部 24px 小狀態欄：左 WiFi 圖標 / 中標題 / 右電量圖標。
// 不持有刷屏策略，只 set_text；調用方（FrameScene）決定何時調
// epd->RequestUrgentPartialRefresh()。

#include <lvgl.h>

#include <string>

class StatusBar {
   public:
    explicit StatusBar(lv_obj_t* parent);

    // 返回 true 表示有任何字段實際變化（調用方據此決定是否刷屏）。
    bool SetWifi(bool connected, int rssi);
    // charging / full 互斥(ChargeStatus 保證):
    //   full=true     → 滿電圖標 + "100%"(物理充滿,不再走 ADC 估算)
    //   charging=true → BOLT 圖標 + "--"(ADC 端電壓被充電 IC 拉高,pct 不可信)
    //   都 false      → 按 pct 顯示真實電量
    bool SetBattery(int pct, bool charging, bool full);
    bool SetCaption(const std::string& text);
    bool SetCaptionIcon(const char* icon);

    void Show();
    void Hide();

   private:
    void LayoutTitle();

    lv_obj_t* root_             = nullptr;
    lv_obj_t* wifi_label_       = nullptr;
    lv_obj_t* battery_label_    = nullptr;
    lv_obj_t* battery_pct_lbl_  = nullptr;  // 電池圖標左側的百分比文字
    lv_obj_t* title_icon_label_ = nullptr;
    lv_obj_t* title_label_      = nullptr;

    std::string shown_wifi_;
    std::string shown_battery_;
    // 直接緩存最終文本(可能為空,空 = 充電中只剩圖標),避免再用 sentinel int 區分
    // "未知"/"隱藏"/"具體百分比"。
    std::string shown_pct_text_;
    std::string shown_title_icon_;
    std::string shown_title_;
};
