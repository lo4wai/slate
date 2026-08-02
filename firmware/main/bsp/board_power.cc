#include "bsp/board_power.h"

#include <driver/gpio.h>

#include "bsp/config.h"
#include "utils/gpio_util.h"

namespace {
constexpr gpio_num_t kLedPin = GPIO_NUM_3;  // 板上唯一一顆綠色 LED，低有效
}  // namespace

BoardPowerBsp::BoardPowerBsp(int audioPowerPin, int audioAmpPin, int vbatPowerPin)
    : audioPowerPin_(audioPowerPin), audioAmpPin_(audioAmpPin), vbatPowerPin_(vbatPowerPin) {
    // VBAT_PWR(GPIO17): 系統軟鎖存,拉高=自鎖,拉低=斷電(關機唯一手段)
    // Audio_PWR(GPIO42): AVDD_3V3 rail,關掉 = I²C 死(R45/R46 上拉在這條 rail)
    // Audio_AMP(GPIO46): PA U5 數字使能 + ES8311 PA_PIN。
    //
    // PA pin 必須跟 audio rail 在同一 gpio_config 裏 init —— gpio_config 一返回
    // mode=OUTPUT level=0 立即生效,PA CTRL 穩是 LOW。後面 PowerAudioOn 拉高
    // GPIO42 給 PA U5 通電,CTRL 已經穩定 LOW,PA 不會放大 ES8311 默認 DC bias。
    // 這是消除開機"啵"聲的關鍵時序點。AudioPlayer 後續負責在 codec dev open
    // + DAC 穩定 100 ms 後再拉高 PA（出聲）。
    gpio_config_t cfg = {};
    cfg.intr_type     = GPIO_INTR_DISABLE;
    cfg.mode          = GPIO_MODE_OUTPUT;
    cfg.pin_bit_mask  = (1ULL << audioPowerPin_) | (1ULL << audioAmpPin_) | (1ULL << vbatPowerPin_);
    cfg.pull_up_en    = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&cfg));
    // 顯式拉低 + hold_en:gpio_config 默認 level=0 但穩起見寫一遍,且 hold 讓
    // 電平在 deep sleep / 復位過程中也不丟(不然醒來到 audio init 之間又是浮空)。
    gpio_set_level(static_cast<gpio_num_t>(audioAmpPin_), 0);
    gpio_hold_en(static_cast<gpio_num_t>(audioAmpPin_));
}

void BoardPowerBsp::InitLed() {
    // GPIO3 是 strapping pin（高=不打 ROM log）。板上 R35 已經把它上拉到 3V3，
    // 所以復位瞬間 LED 滅、ROM 不打 log。這裏再配置成 OUTPUT 並寫高（繼續滅）。
    gpio_config_t cfg = {};
    cfg.intr_type     = GPIO_INTR_DISABLE;
    cfg.mode          = GPIO_MODE_OUTPUT;
    cfg.pin_bit_mask  = 1ULL << kLedPin;
    cfg.pull_up_en    = GPIO_PULLUP_DISABLE;
    cfg.pull_down_en  = GPIO_PULLDOWN_DISABLE;
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&cfg));
    GpioWriteHold(kLedPin, 1);
}

void BoardPowerBsp::PowerAudioOn() {
    GpioWriteHold(audioPowerPin_, 1);
}
void BoardPowerBsp::PowerAudioOff() {
    GpioWriteHold(audioPowerPin_, 0);
}
void BoardPowerBsp::VbatPowerOn() {
    GpioWriteHold(vbatPowerPin_, 1);
}
void BoardPowerBsp::VbatPowerOff() {
    GpioWriteHold(vbatPowerPin_, 0);
}

extern "C" void BoardI2cForcePowerOn() {
    GpioWriteHold(AUDIO_PWR_PIN, AUDIO_PWR_FORCE_LEVEL);
}
