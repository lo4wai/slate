# Slate / Firmware

ESP-IDF 5.5.x 固件，目標芯片 ESP32-S3。當前只支持 **ZecTrix_Note4_V1.0**（極趣實驗室「Ai 便利貼」）：4.2 英寸黑白墨水屏、ES8311 音頻、MEMS 麥、3 個按鍵（確認 / 上 / 下）、單節鋰電池。

本目錄是獨立 ESP-IDF 工程，不屬於 Bun workspace。

## 構建與燒錄

```bash
source $IDF_PATH/export.sh
idf.py -C firmware build
idf.py -C firmware -p <serial> flash monitor
```

CI 使用 ESP-IDF v5.5.2 構建：

```bash
idf.py build
idf.py merge-bin -o slate-full.bin
cp build/slate.bin build/slate-ota.bin
```

target、Flash、PSRAM、分區表已在 `sdkconfig.defaults` 固化，無需手動 `idf.py set-target`。

## 工程結構

```text
firmware/
├── CMakeLists.txt
├── partitions.csv              4 MB factory app + 12 MB LittleFS storage
├── sdkconfig.defaults          ESP32-S3 / Flash / PSRAM / PM / TLS / Slate 配置
├── tools/                      字體生成工具
└── main/
    ├── app/                    App 生命週期編排
    ├── bsp/                    板級 GPIO、電源、I2C、EPD、按鍵、ADC、充電狀態
    ├── drivers/
    │   ├── audio/              ES8311 + I2S duplex 音頻播放/錄音
    │   ├── bus/                I2C 設備封裝、總線鎖、電源自救 hook
    │   ├── display/            SSD2683/SSD1683-compatible EPD 驅動
    │   └── input/              按鍵封裝
    ├── events/                 boot stage、group sync status、UI 事件與事件總線
    ├── network/                Wi-Fi、SNTP、DNS hijack、captive portal、憑據存儲
    ├── power/                  power state、sleep manager、重啓/關機、分鐘級時鐘
    ├── resources/              captive portal HTML 與內置字體
    ├── scenes/                 BootSplash、BgRefresh、Frame、Xiaozhi、Settings 及子頁
    ├── startup/                boot mode、首次啓動/註冊流程
    ├── storage/                LittleFS cache、NVS schema
    ├── sync/                   Slate backend HTTP API client 與 SyncService
    ├── ui/                     狀態欄、frame view、menu list、主題
    ├── utils/                  JSON、時間、字節、鎖 helper
    └── xiaozhi/                小智配置、協議、MCP、對話服務
```

## 硬件規格

| 項 | 規格 |
| --- | --- |
| MCU | ESP32-S3-WROOM-1 N16R8V，16 MB QIO Flash + 8 MB Octal PSRAM |
| 顯示 | 4.2" 黑白 EPD，400 x 300，SSD2683 控制器，命令兼容 SSD1683 |
| 音頻 | ES8311 codec，單聲道揚聲器，MEMS 麥，差分 D 類 PA |
| 傳感 | PCF8563 RTC、GT23SC6699 NFC |
| 電源 | 單節 4.2 V 鋰電，軟鎖存主電源，獨立 EPD rail 與音頻/I2C rail |
| 按鍵 | GPIO0 確認 / BOOT，GPIO39 上，GPIO18 下 / 開機，EN 硬復位 |
| 接口 | USB-C CDC/JTAG、喇叭座、調試座 |

Flash 是 QIO，PSRAM 是 Octal。`CONFIG_ESPTOOLPY_OCT_FLASH=n` 與 `CONFIG_SPIRAM_MODE_OCT=y` 是正確組合。

## GPIO

```text
GPIO0   KEY_ENTER       確認 + BOOT，低有效
GPIO1   STDBY_H         充電 IC 滿電狀態
GPIO2   CHRG_L          充電 IC 充電狀態
GPIO3   LED_G           綠色 LED，低有效
GPIO4   ADC_BAT         VBAT 1:2 分壓
GPIO5   RTC_INT         PCF8563 INT#
GPIO6   EPD_PWR_EN      EPD rail
GPIO7   NFC_FD          NFC 場檢測
GPIO8   EPD_BUSY        active-low，低=忙，高=空閒
GPIO9   EPD_NRES
GPIO10  EPD_NDC
GPIO11  EPD_NCS         軟件控制 CS
GPIO12  EPD_SCK
GPIO13  EPD_SDA         SPI MOSI
GPIO14  I2S_MCLK
GPIO15  I2S_SCLK
GPIO16  I2S_ASDOUT      MIC DIN
GPIO17  PWR_ON          主電源軟鎖存，高=保持供電
GPIO18  KEY_DET / PGDN  下鍵 + 開機反饋
GPIO19  USB_DN
GPIO20  USB_DP
GPIO21  NFC_PWR
GPIO38  I2S_LRCK
GPIO39  KEY_PGUP        上鍵，非 RTC IO，不能 ext1 喚醒
GPIO42  PA_PWR_EN       AVDD_3V3：音頻 + I2C 上拉
GPIO43  TXD0
GPIO44  RXD0
GPIO45  I2S_DSDIN       喇叭 DOUT
GPIO46  PA_CTRL         PA enable，高=出聲
GPIO47  I2C_SDA
GPIO48  I2C_SCL
```

GPIO 26-37 被 Octal PSRAM 佔用，不能用作普通 GPIO。

## 電源

主電源軟鎖存：

```text
USB/VBAT -> Q5 PMOS -> VIN -> Buck -> 3V3
              ^
              ├─ SW1 / GPIO18 下鍵拉低柵極：按住開機
              └─ GPIO17 PWR_ON 自鎖：固件拉高後鬆手不斷電
```

三條關鍵 rail：

| Rail | 控制 | 説明 |
| --- | --- | --- |
| 主電源 | GPIO17 | 拉低會整機斷電；deep sleep 前必須 RTC GPIO hold 高 |
| EPD 3V3 | GPIO6 | 關閉後屏幕內容保留，但 controller/電荷泵失效；醒來需完整 init |
| AVDD_3V3 | GPIO42 | 音頻供電 + I2C 上拉；任何 I2C 操作前必須打開並 hold |

開機階段必須等待 GPIO18 鬆開後再交給按鍵驅動，否則下鍵會被誤識別為一次普通按鍵。

## 總線

| 總線 | 端口 | 引腳 | 設備 |
| --- | --- | --- | --- |
| I2C | `I2C_NUM_0` | SDA=47, SCL=48 | ES8311 0x18、PCF8563 0x51、GT23SC6699 0x55 |
| SPI | `SPI3_HOST` | SCK=12, MOSI=13, CS=11, DC=10, RST=9, BUSY=8 | EPD，40 MHz mode 0 |
| I2S | `I2S_NUM_0` | MCLK=14, BCLK=15, WS=38, DIN=16, DOUT=45 | ES8311 duplex |
| ADC | ADC1 CH3 | GPIO4 | VBAT 分壓 |

## 分區

[partitions.csv](partitions.csv)：

```text
nvs      0x9000    0x6000
phy_init 0xf000    0x1000
factory  0x10000   0x400000
storage  0x410000  0xBF0000
```

`storage` 是 LittleFS，約 12 MB。按每幀 15 KB image + 可選 PCM 估算，可緩存約數百幀。

## 啓動模式

`boot_mode::Decide()` 根據憑據和喚醒原因決定：

| 模式 | 條件 | 行為 |
| --- | --- | --- |
| `kPortal` | 沒有 Wi-Fi 憑據 | 啓動 SoftAP captive portal |
| `kBackgroundRefresh` | RTC timer 喚醒、已有 device secret、且有緩存內容組 | 後台刷新當前動態幀，完成後繼續 deep sleep |
| `kFullActive` | 冷啓動、按鍵喚醒、充電喚醒或其他情況 | 顯示 UI，聯網同步，允許用户操作 |

`wake_reason` 會隨 poll 上報給後端：

```text
timer | button | power_on | charge | other
```

## 啓動流程

`App::Init()` 當前順序：

```text
nvs_flash_init + LittleFS mount
  -> Board::Init()
  -> AudioPlayer::Init()
  -> evt::Init()
  -> xiaozhi::XiaozhiService::Start()
  -> SceneStack::SetContext()
  -> load credentials + boot_mode::Decide()
  -> SleepManager::Init()
  -> StartUiLoop()
  -> AttachInputs()
  -> StartMinuteTick()
  -> StartSleep()
  -> 按 boot mode 啓動 captive portal 或 Wi-Fi + SyncService
  -> esp_pm_configure(80-240 MHz DFS)
```

`Run()` 直接刪除 main task，讓 `ui_loop`、`slate_sync`、`audio_play`、EPD refresh 等後台 task 接管。

## Captive Portal

沒有 Wi-Fi 憑據時：

- 啓動 SoftAP：`{SLATE_AP_SSID_PREFIX}-{MAC後2字節}`，默認 `Slate-XXXX`。
- DNS hijack 所有查詢到 `192.168.4.1`。
- HTTP portal 提供兩步表單：Wi-Fi SSID/password 與 backend `server_url`。
- 提交後先 `Wifi::TryConnect()` 驗證，再保存 NVS 並重啓。

憑據結構：

```cpp
std::string wifi_ssid;
std::string wifi_pwd;
std::string server_url;
std::string device_id;
std::string device_secret;
```

首次配網後 `device_secret` 為空，重啓進入註冊流程。工廠重置會清空憑據，下次開機重新進入 portal。

## 設備註冊與綁定

聯網後：

1. SNTP 對時，HTTPS 需要有效系統時間。
2. `api::Init(server_url, mac, device_secret)`。
3. 如果 NVS 沒有 `device_secret`，調用：

```text
POST /api/v1/devices
```

4. 保存後端返回的 `device_id` 與 64 字符 `device_secret`。
5. 屏幕顯示 `pair_code`，等待 Web claim。

後續所有受保護設備 API 都使用：

```text
Authorization: Bearer <device_secret>
```

如果連續 5 次收到 401，固件會投遞 `kSecretInvalid`，在 UI 主線程清除 secret 並重啓，走重新註冊流程。

## 同步協議

`SyncService` 運行在 `slate_sync` task，事件位包括：

- 普通 poll
- 手動 trigger
- next/prev 切組
- timer wake background refresh
- stop

輪詢週期：

| 狀態 | 週期 |
| --- | --- |
| 已綁定 | 60 秒 |
| 未綁定前 10 分鐘 | 10 秒 |
| 未綁定 10-30 分鐘 | 30 秒 |
| 未綁定 30 分鐘後 | 60 秒 |

poll telemetry：

```json
{
  "telemetry": {
    "battery_pct": 85,
    "rssi_dbm": -56,
    "fw_version": "0.1.0",
    "wake_reason": "timer",
    "current_group": "gid",
    "current_content_seq": 3,
    "current_content_etag": "etag",
    "manifest_etag": "etag"
  }
}
```

同步策略：

- manifest etag 未變：只寫當前 state，觸發緩存命中 UI。
- manifest etag 變化：`GET /groups/:gid/manifest`，再增量下載缺失 image/audio。
- 每個資源請求帶 `If-None-Match`，後端命中返回 304。
- timer wake 且 manifest 未變時，如果後端返回 `current_content`，只更新當前幀。
- 切組調用 `/devices/current/group/next` 或 `/prev`，然後同步目標組。

## LittleFS 緩存

佈局：

```text
/littlefs/state.json
/littlefs/groups/{gid}/manifest.json
/littlefs/groups/{gid}/frames/{idx}.img
/littlefs/groups/{gid}/frames/{idx}.img.etag
/littlefs/groups/{gid}/frames/{idx}.pcm
/littlefs/groups/{gid}/frames/{idx}.pcm.etag
/littlefs/groups/{gid}/frames/{idx}.meta
```

`FrameMeta` 包含：

```cpp
status_bar_text
content_etag
image_etag
audio_etag
has_ttl
ttl_sec
```

完整 manifest 同步使用 per-group staging area：

1. `BeginFrameStage(gid)`
2. 下載所有缺失 image/audio 到 stage
3. 寫 staged meta
4. 全部成功後逐幀 `CommitStagedFrame`
5. 寫 manifest 與 state
6. 清理舊幀與舊音頻

這樣失敗同步不會把已提交緩存寫成半更新狀態。同步後會按空閒空間和組數量清理舊組，當前配置為至少保留 1 MB，最多緩存 4 個組。

## UI 與按鍵

唯一 UI 消費者是 `ui_loop` task。事件通過 FreeRTOS queue 傳遞，`UiEvent` 必須保持 trivially-copyable，不允許放 `std::string` 或 owning 容器。

按鍵：

| 操作 | 行為 |
| --- | --- |
| UP 短按 | 上一幀 |
| UP 長按 | 上一個內容組 |
| DOWN 短按 | 下一幀 |
| DOWN 長按 | 下一個內容組 |
| ENTER 短按 | 下一幀 |
| ENTER 長按 | 設置頁 |
| ENTER 雙擊 | 小智語音頁 |
| UP + DOWN 同時按 | 緊急全屏刷新，清殘影 |

Scene：

```text
BootSplashScene      啓動、配網、註冊、等待綁定/內容組
BgRefreshScene       timer wake 後台刷新
FrameScene           常規顯示與翻頁
XiaozhiScene         小智語音對話
SettingsScene        設置頁
  ├─ VolumePage      音量調節
  ├─ DeviceInfoPage
  ├─ RestartDevicePage
  └─ FactoryResetPage
```

## EPD

關鍵參數：

- 400 x 300，1bpp packed 幀為 15000 bytes。
- SSD2683 實際刷屏需要 2bpp 傳輸，1bpp 會膨脹成 30 KB SPI payload。
- 全刷約 2-3 秒，局刷約 0.3-0.6 秒。
- 累計多次 partial 後強制 full cleanup，減少殘影。
- BUSY 極性是 active-low：低=忙，高=空閒。
- 讀屏內温度後寫温度補償寄存器，60 秒內複用温度，避免每次 RX 切換開銷。

deep sleep 前只等待已有 EPD refresh 完成，不主動全刷；屏幕內容依靠墨水屏雙穩態保留。

## 音頻

`AudioPlayer` 初始化 I2S0 duplex：

- 16 kHz
- mono
- 16-bit
- MCLK = 256 x fs
- TX 用於內容音頻和小智下行播放
- RX 用於小智麥克風上行

ES8311 使用 lazy open：

1. 初始化時創建 codec handle，但不立即 open。
2. 第一次播放或語音進入時 open codec。
3. DAC bias 等 100 ms 後再拉高 GPIO46 PA。
4. 切歌前靜音並等待 DMA 殘留，減少爆音。

內容音頻格式與後端一致：

```text
16 kHz mono signed 16-bit little-endian raw PCM
```

內容播放和小智對話共用同一個音量設置。

## 小智語音

`xiaozhi/` 子系統包含：

- `config/`：`activation_client` / `settings`，向小智配置服務獲取 MQTT/WebSocket 配置與 activation 信息，並保存 UUID、協議配置、音量等 NVS 設置。
- `mcp/`：`mcp_dispatcher` / `mcp_tools`，MCP 協議分發與工具註冊。
- `protocol/`：`protocol` / `mqtt_protocol` / `websocket_protocol` / `audio_stream_packet`，對話協議。
- `service/`：`xiaozhi_service`、`xiaozhi_phase`、`audio_service`、`message_handler`，對話狀態機、麥克風、播放、語音處理。

進入方式：ENTER 雙擊打開 `XiaozhiScene`。如果尚無協議配置，會先走配置/激活流程；配置完成後進入待機。語音活動、配置任務或播放中會阻止 deep sleep。

## 休眠與喚醒

`SleepManager` 策略：

- 默認閒置 10 分鐘 deep sleep，可由 `SLATE_IDLE_DEEP_SLEEP_MIN` 配置。
- captive portal 模式禁用 deep sleep。
- USB/充電存在時暫停 deep sleep。
- 未綁定後 2 小時內阻止 deep sleep，方便用户在 Web claim 後設備快速響應；低電量會退出 grace。
- 小智活動中阻止 deep sleep。

喚醒源：

- ENTER / GPIO0
- DOWN / GPIO18
- CHARGE_DETECT / GPIO2
- RTC timer（僅當前動態幀需要定時刷新時啓用）

GPIO39 上鍵不是 RTC IO，不能作為 deep sleep ext1 喚醒源。

進入 deep sleep 前：

1. 停止小智、同步和音頻。
2. 等待 EPD 當前刷新完成，保存狀態欄快照。
3. 關閉 EPD rail。
4. 關閉音頻/I2C rail。
5. 使用 RTC GPIO hold 住 GPIO17 主電源。
6. 配置 ext1 和可選 timer。

## 配置項

[main/Kconfig.projbuild](main/Kconfig.projbuild)：

| 項 | 默認 | 説明 |
| --- | --- | --- |
| `SLATE_DEFAULT_SERVER_URL` | 空 | captive portal 服務端 URL 預填值 |
| `SLATE_AP_SSID_PREFIX` | `Slate` | SoftAP SSID 前綴 |
| `SLATE_DEFAULT_TIMEZONE` | `CST-8` | SNTP 後設置的 POSIX TZ |
| `SLATE_IDLE_DEEP_SLEEP_MIN` | `10` | 閒置多少分鐘進 deep sleep |

`sdkconfig.defaults` 還固化：

- ESP32-S3 target
- 16 MB QIO Flash
- 8 MB Octal PSRAM
- LittleFS 分區表
- LVGL 9.5
- TLS root CA bundle
- mbedTLS dynamic buffer 與 external mem alloc
- DFS 80-240 MHz
- tickless idle
- deep sleep leakage workaround

## 依賴

[main/idf_component.yml](main/idf_component.yml)：

```yaml
espressif/button: ~4.1.5
espressif/esp_codec_dev: ~1.5.6
78/esp-ml307: ~3.6.5
espressif/esp_audio_codec: ~2.4.1
espressif/esp_audio_effects: ~1.2.1
lvgl/lvgl: ~9.5.0
espressif/esp_lvgl_port: ~2.7.2
joltwallet/littlefs: ~1.16.0
idf: ">=5.5"
```

## 字體

固件內置字體在 `main/resources/fonts/`：

- `zfull_16.c`
- `zfull_12.c`
- `font_awesome_14_1.c`
- `font_awesome_30_1.c`

生成腳本：

```bash
firmware/tools/gen_zfull_fonts.sh
```

## 調試要點

- HTTP base URL 接受 `http://` 和 `https://`；authenticated HTTP 會打印警告。
- HTTPS 需要 SNTP 時間同步，否則證書校驗可能失敗。
- `/api/v1` 前綴寫死在 `sync/api_client.cc`，要與 shared/backend 保持一致。
- EPD BUSY 是低忙高閒，調試新屏或新板時不要按 SSD1683 datasheet 默認極性判斷。
- AVDD_3V3 關閉後 I2C 上拉消失，任何 I2C 操作都會失敗。
- deep sleep 前 GPIO17 必須切 RTC GPIO hold 高，否則會整機斷電，按鍵喚不醒。
