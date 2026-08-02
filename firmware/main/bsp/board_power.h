#pragma once

// 系統級電源管理,管三條 audio 子系統相關 pin + VBAT 軟鎖存:
//   - VBAT 軟鎖存(GPIO17 拉低=關機的唯一手段)
//   - AVDD_3V3 / Audio rail(GPIO42,關掉=I²C 死、ES8311/PA/MIC 失能)
//   - PA CTRL(GPIO46,PA 數字使能,必須先於 audio rail 通電時被驅動 LOW)
// EPD_PWR(GPIO6) 由 EpdSsd1683 自管。
//
// 為什麼 PA pin 也歸這裏管:audio rail 一通電 PA U5 就吃電,如果此時 PA CTRL
// 浮空可能被讀為 HIGH → PA 放大 ES8311 默認 DC bias → 喇叭"啵"。所以 PA pin
// 必須跟 audio rail 在同一構造裏 init,確保 PowerAudioOn 時 CTRL 已穩是 LOW。
// 後續由 AudioPlayer::EnsureCodecOpen 在 100 ms DAC 穩定窗後再拉高出聲。
//
// LED(GPIO3):階段 1 起不再走充電狀態閃爍(屏保取消 → 改由狀態欄指示)。
// 這裏只在 InitLed() 把 GPIO3 配成 OUTPUT 並熄滅,之後不再驅動。

class BoardPowerBsp {
   public:
    BoardPowerBsp(int audioPowerPin, int audioAmpPin, int vbatPowerPin);
    ~BoardPowerBsp() = default;

    // 一次性把 LED 配成 OUTPUT 並熄滅。可在 Init 序列任意點調，冪等。
    void InitLed();

    void PowerAudioOn();
    void PowerAudioOff();
    void VbatPowerOn();
    void VbatPowerOff();

   private:
    const int audioPowerPin_;
    const int audioAmpPin_;  // PA CTRL,構造時鎖定 LOW
    const int vbatPowerPin_;
};
