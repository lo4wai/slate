#pragma once

// SoftAP captive portal:開機 NVS 無憑據時,設備開 AP "Slate-XXXX",
// 手機連進來後瀏覽器打開 192.168.4.1 看到兩段表單(WiFi/服務端)。
// POST /submit 接 JSON {ssid, password, server_url}
// → 寫 NVS namespace "slate" → 切 STA 試連 → 成功後退 AP。
// 設備名(name)不在配網階段填,綁定後由 Web 端 PUT /devices/:id 設置。
//
// 用法:
//   CaptivePortal portal;
//   portal.Start();             // 同步 起 AP + HTTP server
//   while (!portal.Submitted()) vTaskDelay(...);  // 或註冊 OnSubmit 回調
//   portal.Stop();

#include <esp_http_server.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include "network/dns_hijack.h"

class CaptivePortal {
   public:
    struct Submission {
        std::string ssid;
        std::string password;
        std::string server_url;
    };

    bool Start();  // 啓 SoftAP + HTTP + DNS,立即返回
    void Stop();

    // 用户提交配網信息時的回調。回調內做"試連驗證"(wifi.TryConnect),
    // 不能直接 portal.Stop()或重啓,只驗證 + 寫 NVS。
    // - return true:提交合法 ssid/pwd,/submit 回 {success:true},
    //   隨後 portal 內部啓 task 延 2s 調 OnFinished(true)
    // - return false:驗證失敗,out_error 填可讀中文,/submit 回
    //   {success:false,error:msg},表單保留給用户改密碼重提
    using SubmitCb   = std::function<bool(const Submission&, std::string& out_error)>;
    using FinishedCb = std::function<void(bool success)>;

    void OnSubmit(SubmitCb cb);
    void OnFinished(FinishedCb cb);

    bool Running() const {
        return running_.load();
    }

    ~CaptivePortal();

   private:
    std::atomic<bool>                  running_{false};
    httpd_handle_t                     server_ = nullptr;
    SubmitCb                           on_submit_;
    FinishedCb                         on_finished_;
    std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);
    DnsHijack                          dns_;
    std::string                        ap_url_ = "http://192.168.4.1/";

    static esp_err_t HandleRoot(httpd_req_t* req);
    static esp_err_t HandleScan(httpd_req_t* req);
    static esp_err_t HandleSubmit(httpd_req_t* req);
    static esp_err_t HandleDone(httpd_req_t* req);
    static esp_err_t HandleExit(httpd_req_t* req);
    // captive portal "萬能" handler:任何未知 URL 都重定向到 /
    static esp_err_t HandleCatchAll(httpd_req_t* req);
    static void      FinishTask(void* arg);
    static void      StopTask(void* arg);
};
