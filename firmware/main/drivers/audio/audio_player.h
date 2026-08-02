#pragma once

// I2S + ES8311 audio owner。內容使用異步播放接口；小智進入對話時通過
// BeginXiaozhi/EndXiaozhi 獨佔同一套 codec/I2S，避免兩套業務各自初始化硬件。
//
// 服務端約定：16 kHz mono 16-bit raw PCM（.pcm 二進制）。
// 數據來源:cache::ReadFrameAudio(gid, idx, vec<uint8_t>) 讀 LittleFS。
// frame 切換時調 Play() 中斷當前播放,立即播新 PCM。
//
// 抄襲 esp32-eink/refs/zectrix-original/main/audio/codecs/es8311_audio_codec.cc 的
// I2S+ES8311 配置序列(去 AudioCodec 父類繼承,只保留 output 部分,不要 mic)。

#include <cstddef>
#include <cstdint>

#include <driver/i2c_master.h>
#include <driver/i2s_std.h>
#include <esp_codec_dev.h>
#include <esp_codec_dev_defaults.h>
#include <esp_pm.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <atomic>

class AudioPlayer {
   public:
    static AudioPlayer& Get();

    // 一次性初始化 I2S + codec。需 Board::I2c() 已 ready,sample_rate 16000。
    // 失敗返回 false(I2C 通訊失敗/codec 未識別等)。
    bool Init(i2c_master_bus_handle_t i2c_bus);

    // 異步播放：替換共享 PCM，通知 task 立即播。中斷當前播放（若有）。
    // pcm = 16 kHz mono 16-bit signed，len_bytes 必須是偶數。
    void Play(const uint8_t* pcm_bytes, size_t len_bytes);

    // 停止當前播放(若有);保持 codec 通電,下次 Play 立即可用。
    void Stop();

    // 0..100,默認 90。改 codec output volume 寄存器。
    void SetVolume(int v);

    // 小智對話獨佔音頻硬件。
    bool BeginXiaozhi();
    void EndXiaozhi();
    bool ReadXiaozhiPcm(int16_t* dest, size_t samples);
    bool WriteXiaozhiPcm(const int16_t* data, size_t samples);
    bool IsXiaozhiActive() const {
        return xiaozhi_active_.load(std::memory_order_relaxed);
    }

   private:
    AudioPlayer() = default;
    static void TaskEntry(void* arg);
    void        TaskLoop();
    // 第一次 Play 時同步打開 codec(lazy)。開機不 open 是為了消除 codec lib
    // 的 enable→DAC start→PA on 時序在喇叭上的"啵"聲。
    bool EnsureCodecOpen();
    void CleanupInitResources();
    // 音頻活躍期持有 NO_LIGHT_SLEEP 鎖，防止自動 light sleep 停時鐘導致 I2S 欠載卡頓。
    // acquire/release 必須配對；內部判空，PM 鎖創建失敗時為 no-op。
    void AcquireAudioPmLock();
    void ReleaseAudioPmLock();

    bool              initialized_ = false;
    std::atomic<bool> codec_opened_{false};  // lazy 標誌
    // 是否已經播過至少一段 PCM。用於 TaskLoop 判斷"切歌"場景需要先靜音再寫,
    // 避免舊 PCM 末尾和新 PCM 開頭波形跳變產生"啵"。Init 後第一段不算切歌。
    std::atomic<bool>            codec_in_progress_{false};
    i2s_chan_handle_t            tx_handle_ = nullptr;
    i2s_chan_handle_t            rx_handle_ = nullptr;
    const audio_codec_data_if_t* data_if_   = nullptr;
    const audio_codec_ctrl_if_t* ctrl_if_   = nullptr;
    const audio_codec_if_t*      codec_if_  = nullptr;
    const audio_codec_gpio_if_t* gpio_if_   = nullptr;
    esp_codec_dev_handle_t       dev_       = nullptr;
    std::atomic<int>             volume_{90};

    // 共享 PCM:Play 寫入,task 讀取並播。簡單 swap,不做 ring buffer
    // (本場景 frame 切換 = 整段重播,不是流式追加)。
    SemaphoreHandle_t shared_mutex_ = nullptr;
    SemaphoreHandle_t codec_mutex_  = nullptr;
    SemaphoreHandle_t notify_       = nullptr;  // binary semaphore,Play 時 give,task wait
    uint8_t*          pending_pcm_  = nullptr;  // 由 Play 拷貝;task 取走置 null
    size_t            pending_len_  = 0;
    std::atomic<bool> stop_flag_{false};
    std::atomic<bool> xiaozhi_active_{false};
    TaskHandle_t      task_ = nullptr;

    esp_pm_lock_handle_t no_light_sleep_lock_ = nullptr;
};
