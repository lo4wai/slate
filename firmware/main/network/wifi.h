#pragma once

// Wi-Fi 單例：STA 與 SoftAP 模式互斥，內部按需創建/銷燬 netif。
// 設計參考 xiaozhi-esp32 的 wifi-connect 組件:
//   - cfg.nvs_enable=false 禁用 ESP-IDF wifi 內置的 NVS 持久化,防止
//     mode/config 殘留在 NVS 讓重啓後狀態錯亂
//   - StartAp/Connect 互斥:進任一邊之前徹底停掉另一邊
//   - Stop = 註銷 event handler instance + esp_wifi_stop + destroy netif
//
// 重連策略:
//   1. 快速:STA_DISCONNECTED 立即 esp_wifi_connect,最多 5 次。
//   2. 慢速:5 次用完轉 esp_timer + 主動 esp_wifi_scan_start 全信道掃描,
//      指數退避 10s → 20 → 40 → 80 → 120 → 120s。scan 找到 SSID 即拿
//      bssid+channel 設回 wc.sta 再 connect。
//   3. IP_GOT_IP 後清 fail_count_ + 重置 backoff_idx_,timer 停。
//   4. want_reconnect_=false (Disconnect / TryConnect 主動斷 / StopAp) 時
//      整套機制不觸發,純靜默。
//
// 用法:Wifi::Get().Init() 一次,之後 Connect/StartAp/...

#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <lwip/inet.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

#include "network/wifi_reconnect_manager.h"

class Wifi {
    friend class WifiReconnectManager;

   public:
    enum class State {
        Idle,          // 還沒連過
        Connecting,    // 正在連接(快速 retry / 慢速 scan-then-connect 期間)
        Connected,     // 已連
        Disconnected,  // 已斷開,慢速重連定時器在排隊下一次嘗試(若 want_reconnect_)
    };

    static Wifi& Get();

    // 一次性的最小底座:esp_netif_init + 默認 event loop + esp_wifi_init(nvs_enable=false)。
    // 不創建任何 netif,不調 wifi_start。NVS 由 App::InitStorage 統一初始化。
    void Init();

    // 進 STA 模式並連接。如果當前在 AP 模式,會先徹底停掉 AP。
    // 返回 false 時:Connect 同步等待超時(快速 retry 全失敗),want_reconnect_
    // 已被置 false,慢速重連機制不會啓動。調用方典型走 captive portal fallback。
    bool Connect(const std::string& ssid, const std::string& password, int timeout_ms = 15000);

    // 僅"試連":必須在 AP 模式下調用(captive portal /submit 用)。
    // 期間 wifi 處於 APSTA,STA 臨時連接驗證密碼,驗證完 disconnect 但 AP 保留。
    // 失敗時 out_reason 填可讀中文。本函數不開自動重連。
    bool TryConnect(const std::string& ssid, const std::string& password, int timeout_ms, std::string& out_reason);

    // 主動斷開 STA(不下 wifi),後續不重連
    void Disconnect();

    bool  IsConnected() const;
    State state() const {
        return state_.load();
    }
    int8_t      GetRssi() const;
    std::string GetIp() const;

    // 啓動 SoftAP(給 captive portal 用,SSID = prefix-{MAC後兩位})。
    // 如果當前在 STA 模式,會先徹底停掉 STA。共存模式 APSTA 讓 TryConnect
    // 能在配網期間複用同一個 wifi。
    bool StartAp(const std::string& ssid_prefix);

    // 徹底停 AP(esp_wifi_stop + destroy netif),不切回 STA。
    // 調用方(captive portal)Stop 後 App 會重啓,啓動時再走 Connect。
    void StopAp();

    // 註冊事件回調
    using DisconnectCb = std::function<void(int reason_code)>;
    void OnDisconnected(DisconnectCb cb);

   private:
    enum class Mode {
        Off,
        Station,
        AccessPoint,
    };

    Wifi() = default;

    // 內部:進/出某個 mode。互斥狀態機的核心。
    void StartStationInternal();
    void StartApInternal(const std::string& ssid_prefix);
    void StopStationInternal();
    void StopApInternal();

    void RegisterEventHandlers();
    void UnregisterEventHandlers();

    static void EventHandler(void* arg, esp_event_base_t base, int32_t id, void* data);
    void        SetIpString(const char* ip);
    void        ClearIpString();
    bool        ReconnectAllowed() const;
    bool        StationModeActive() const;
    void        MarkSlowReconnectConnecting();
    void        ResetFastFailCount();

    // 僅 Init 調一次的底座
    std::mutex init_mutex_;
    bool       inited_ = false;

    // 當前模式。主線程修改，慢速重連 timer/event handler 讀取。
    std::atomic<Mode> mode_{Mode::Off};

    // 按需創建/銷燬
    esp_netif_t* sta_netif_ = nullptr;
    esp_netif_t* ap_netif_  = nullptr;

    // 用 instance 句柄註冊,Stop 時精確註銷;mode 切換前後必須保證已註銷
    esp_event_handler_instance_t handler_wifi_ = nullptr;
    esp_event_handler_instance_t handler_ip_   = nullptr;

    // STA 狀態
    std::atomic<State>   state_{State::Idle};
    std::atomic<int>     fail_count_{0};
    int                  max_fast_fail_ = 5;
    std::atomic<bool>    want_reconnect_{false};
    mutable std::mutex   callback_mutex_;
    DisconnectCb         on_disconnect_;
    mutable std::mutex   ip_mutex_;
    char                 ip_str_[INET6_ADDRSTRLEN] = {0};
    std::atomic<int>     last_disconnect_reason_{0};
    WifiReconnectManager reconnect_{this};
};
