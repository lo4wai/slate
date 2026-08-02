#pragma once

#include <driver/i2c_master.h>
#include <memory>

class BatteryAdc;
class BoardPowerBsp;
class ChargeStatus;
class Button;
class EpdSsd1683;

// 板級單例:i2c bus / 電源 / 充電狀態 / 按鍵 / EPD+LVGL / 電池 ADC。
// 按 Init() 內的順序構建依賴,順序要求詳見 Init() 裏的註釋。
class Board {
   public:
    static Board& Get();

    void Init();

    BoardPowerBsp* power() {
        return power_.get();
    }
    ChargeStatus* charge() {
        return charge_.get();
    }
    Button* up_btn() {
        return up_btn_.get();
    }
    Button* down_btn() {
        return down_btn_.get();
    }
    Button* boot_btn() {
        return boot_btn_.get();
    }
    EpdSsd1683* epd() {
        return epd_.get();
    }
    i2c_master_bus_handle_t i2c_bus() {
        return i2c_bus_;
    }

    // 單節鋰電池電壓 + 百分比。失敗原因:ADC 未 ready / 沒裝電池(charge 狀態機説)。
    bool ReadBattery(uint16_t* voltage_mv, uint8_t* percent);

   private:
    Board() = default;
    void InitPower();
    void InitI2c();
    void InitChargeStatus();
    void InitButtons();
    void InitEpd();
    void InitBatteryAdc();

    std::unique_ptr<BoardPowerBsp> power_;
    std::unique_ptr<ChargeStatus>  charge_;
    std::unique_ptr<BatteryAdc>    battery_adc_;
    std::unique_ptr<Button>        up_btn_;
    std::unique_ptr<Button>        down_btn_;
    std::unique_ptr<Button>        boot_btn_;
    std::unique_ptr<EpdSsd1683>    epd_;
    i2c_master_bus_handle_t        i2c_bus_ = nullptr;
};
