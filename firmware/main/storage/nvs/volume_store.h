#pragma once

// 音量持久化:NVS namespace "slate.audio"。
// 用户感知是 0-10 檔,codec 實際接收 0-100 → ToCodec(v) = v * 10。
// 默認 9 檔(=codec 90)。

namespace vol {

constexpr int kDefault = 9;
constexpr int kMax     = 10;

int  Get();  // 音量,0..10,首次讀返回 kDefault
void Set(int level);
int  ToCodec(int level);  // level * 10,給 esp_codec_dev_set_out_vol 用

}  // namespace vol
