#pragma once

// SNTP 對時:連上 STA 後立即調,後台同步時間。狀態欄 %H:%M 用。

#include <string>

namespace sntp {

void Init();                                   // 啓動 SNTP,設默認 timezone (Kconfig CST-8)
bool TimeSynced();                             // 是否已成功同步過(time(nullptr) > 2020 年視為成功)
void ApplyServerTime(const std::string& iso);  // SNTP 不可用或偏差明顯時用服務端時間兜底

}  // namespace sntp
