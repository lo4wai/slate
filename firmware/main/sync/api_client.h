#pragma once

// HTTP client 封裝(所有 server 通訊走這裏)。默認門面函數委託給 DefaultClient()。
//
// 端點:
//   POST /api/v1/devices
//   POST /api/v1/devices/current/poll
//   POST /api/v1/devices/current/group/next|prev
//   PUT  /api/v1/devices/current/group
//   GET  /api/v1/groups/:gid/manifest
//   GET  /api/v1/contents/:contentId/image|audio

#include <esp_http_client.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace api {

struct ContentMeta {
    int         seq = 0;
    std::string id;
    std::string content_etag;
    std::string device_status_bar_text;
    std::string image_etag;
    std::string audio_etag;
    int         image_size = 0;
    int         audio_size = 0;
    std::string kind;
    bool        has_next_wake_sec = false;
    int         next_wake_sec     = 0;
};

struct DeviceState {
    std::string id;
    std::string device_name;
    bool        bound = false;
    std::string pair_code;
    std::string server_time;

    bool        has_group = false;
    std::string group_id;
    std::string group_name;
    std::string structure_etag;
    std::string manifest_etag;
    int         content_count    = 0;
    int         group_sort_order = 0;
    int         position_current = 0;
    int         position_total   = 0;

    bool        has_current_content = false;
    ContentMeta current_content;
};

struct RegisterResult {
    std::string id;
    std::string device_secret;
    std::string pair_code;
};

struct Manifest {
    std::string              group_id;
    std::string              group_name;
    std::string              manifest_etag;
    std::vector<ContentMeta> contents;
};

struct Telemetry {
    int         battery_pct = -1;
    int         rssi_dbm    = 0;
    std::string fw_version;
    std::string wake_reason;
    std::string current_group;
    int         current_content_seq = -1;
    std::string current_content_etag;
    std::string manifest_etag;
};

using UnauthorizedCb = std::function<void()>;

class ApiClient {
   public:
    ApiClient() = default;
    ApiClient(std::string server_url, std::string mac, std::string device_secret);

    void Configure(const std::string& server_url, const std::string& mac, const std::string& device_secret);
    void SetServerUrl(const std::string& url);
    void SetSecret(const std::string& secret);
    void SetUnauthorizedHandler(UnauthorizedCb cb);

    // 關閉並釋放持久 HTTP 連接(回收 mbedTLS 內存)。在一次 sync 突發結束後調用,
    // 這樣連接只在單次突發內(poll/cycle → manifest → 各幀 image/audio)被複用。
    void ResetConnection();

    bool Register(RegisterResult& out);
    bool Poll(const Telemetry& tel, DeviceState& out);
    bool CycleGroup(const std::string& direction, DeviceState& out);
    bool SelectGroup(const std::string& gid, DeviceState& out);

    bool GetManifest(const std::string& group_id, const std::string& if_none_match, Manifest& out, bool& not_modified);

    bool DownloadContentImage(const std::string& id, const std::string& if_none_match, std::vector<uint8_t>& out,
                              bool& not_modified);
    bool DownloadContentAudio(const std::string& id, const std::string& if_none_match, std::vector<uint8_t>& out,
                              bool& not_modified);

   private:
    bool DoRequest(const std::string& path, esp_http_client_method_t method, const std::string& body_in,
                   std::vector<uint8_t>& body_out, int* status_out, const std::string& if_none_match,
                   std::string* etag_out, bool need_auth, int timeout_ms);
    bool DoRequestJson(const std::string& path, esp_http_client_method_t method, const std::string& body_in,
                       std::string& body_out_str, bool need_auth);
    bool DownloadBinary(const std::string& path, const std::string& if_none_match, std::vector<uint8_t>& out,
                        bool& not_modified);

    // 複用 conn_。conn_base_url_ 與請求 base 不一致時重建。調用者需持 conn_mutex_。
    esp_http_client_handle_t EnsureClientLocked(const std::string& base_url);
    // 銷燬 conn_(連接進入異常態時調用,強制下次重建)。調用者需持 conn_mutex_。
    void DropConnectionLocked();

    std::string server_url_;
    std::string mac_;
    std::string secret_;

    mutable std::mutex state_mutex_;
    UnauthorizedCb     unauthorized_cb_;
    std::atomic<int>   consecutive_401_{0};
    std::atomic<bool>  warned_http_auth_{false};

    // 持久 HTTP 連接:在一次 sync 突發內複用,避免每請求重做 TLS 握手。
    // conn_mutex_ 串行化整個請求事務並保護 conn_/conn_base_url_。
    std::mutex               conn_mutex_;
    esp_http_client_handle_t conn_ = nullptr;
    std::string              conn_base_url_;
};

ApiClient& DefaultClient();

void Init(const std::string& server_url, const std::string& mac, const std::string& device_secret);
void SetServerUrl(const std::string& url);
void SetSecret(const std::string& secret);
void SetUnauthorizedHandler(UnauthorizedCb cb);
void ResetConnection();

bool Register(RegisterResult& out);
bool Poll(const Telemetry& tel, DeviceState& out);
bool CycleGroup(const std::string& direction, DeviceState& out);
bool SelectGroup(const std::string& gid, DeviceState& out);

bool GetManifest(const std::string& group_id, const std::string& if_none_match, Manifest& out, bool& not_modified);

bool DownloadContentImage(const std::string& id, const std::string& if_none_match, std::vector<uint8_t>& out,
                          bool& not_modified);
bool DownloadContentAudio(const std::string& id, const std::string& if_none_match, std::vector<uint8_t>& out,
                          bool& not_modified);

}  // namespace api
