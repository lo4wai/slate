#pragma once

// NVS 憑據存儲:
//
// 1. 配網憑據 slate.net { ssid / pwd / url }: captive portal 提交後由 Save() 寫入。
//
// 2. 內容服務端設備身份 slate.net { dev_id / dev_sec }:
//    register 響應裏下發,SaveSecret() 單獨寫一次
//    並 commit,保證跨重啓可見。後續所有受保護 API 用 Authorization: Bearer <device_secret>。
//    poll 收到 401 (secret 失效) 時調 ClearSecret() 讓設備重啓走 register 流,
//    不擦 wifi/server,體驗上是"內部修復"而非"重新配網"。

#include <string>

namespace cred {

struct Credentials {
    std::string wifi_ssid;
    std::string wifi_pwd;
    std::string server_url;
    std::string device_id;
    std::string device_secret;
};

// Load 把 NVS 裏所有字段讀出來。返回 true 表示至少有 wifi_ssid + server_url (= 配網完整)。
// device_id/device_secret 可能為空 —— 表示首次啓動或 self-reset 後,需要走 register。
bool Load(Credentials& out);

// 寫配網憑據 (wifi + server_url)。captive portal submit 後調用。
// 不動 device_id/device_secret,保持身份與配網解耦。
bool Save(const Credentials& c);

// 獨立 commit 設備身份。register 響應解析成功後立即調用一次。
// 半寫保護:nvs_open RW → set 兩個 key → commit → close,失敗返回 false 由調用方決定 panic。
bool SaveSecret(const std::string& device_id, const std::string& device_secret);

// 清掉 device_id + device_secret,保留 wifi。下次啓動會走 register 重新拿。
// 觸發場景:poll 收到 401 (後端 reset 了我們 / DB 異常)。
void ClearSecret();

// 便利:從 NVS 讀 server_url
std::string GetServerUrl();

// 工廠重置:清整個 namespace。下次啓動進入配網模式。
void Clear();

}  // namespace cred
