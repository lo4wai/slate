#pragma once

// 集中字體 / 顏色 / 間距，避免每個控件自己 LV_FONT_DECLARE。
//
// 字體清單（全固件統一一份中文字體，避免風格不一致）：
//   - Zfull_16           16px Zfull-GB 墨水屏優化位圖，GB2312 + 符號。
//                        所有中文顯示統一用這個（BootSplash + 狀態欄標題）。
//   - Zfull_12           12px Zfull-GB，ASCII 子集，狀態欄百分比數字。
//   - font_awesome_14_1  14px FontAwesome 圖標，wifi/電池用。
//
// 狀態欄 24px 高 = 16px line_height + 上下 4px 邊距。

#include <lvgl.h>

#include <font_awesome.h>

LV_FONT_DECLARE(Zfull_16);
LV_FONT_DECLARE(Zfull_12);
LV_FONT_DECLARE(font_awesome_14_1);
LV_FONT_DECLARE(font_awesome_30_1);

namespace theme {
constexpr int kStatusBarHeight = 24;

// 右側 scrollbar thumb 幾何,MenuList 與 DeviceInfoPage 共用。
constexpr int kScrollbarTrackPadTop    = 12;
constexpr int kScrollbarTrackPadBottom = 12;
constexpr int kScrollbarThumbW         = 2;
constexpr int kScrollbarThumbRightPad  = 6;
constexpr int kScrollbarThumbMinH      = 14;

// Settings menu geometry.
constexpr int kMenuRowHeight   = 42;
constexpr int kMenuRowPadLeft  = 32;
constexpr int kMenuRowPadRight = 24;
constexpr int kMenuCursorBarW  = 4;
constexpr int kMenuCursorBarH  = 22;

// Device info page.
constexpr int kDeviceInfoScrollStep = 80;
}  // namespace theme
