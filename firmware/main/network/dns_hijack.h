#pragma once

// SoftAP 配網期間的 DNS 劫持:監聽 UDP 53,把所有 DNS 查詢偽造響應指向
// 192.168.4.1(AP gateway),配合 DHCP 推 DNS = AP IP,實現 OS 自動彈出
// captive portal 配網頁(手機/筆記本連上 AP 後探測 connectivitycheck.* 等
// → 解析到本機 → HTTP 200/302 → OS 彈頁)。
//
// 實現照搬 esp32-eink/managed_components/78__esp-wifi-connect/dns_server.cc 思路,
// 改成本項目命名空間。

#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <atomic>
#include <mutex>

class DnsHijack {
   public:
    DnsHijack() = default;
    ~DnsHijack();

    void Start(esp_ip4_addr_t gateway, uint16_t port = 53);
    void Stop();  // 阻塞直到 task 真正退出,保證 ~DnsHijack 之後無懸掛回調

   private:
    void Run();
    bool StopLocked(TickType_t wait_ticks);

    esp_ip4_addr_t    gateway_ = {};
    std::atomic<int>  fd_{-1};
    std::atomic<bool> running_{false};
    TaskHandle_t      task_handle_ = nullptr;
    SemaphoreHandle_t exit_sem_    = nullptr;  // task 退出時 give,Stop 等它
    uint16_t          port_        = 53;
    std::mutex        lifecycle_mutex_;
};
