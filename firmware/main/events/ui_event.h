#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

enum class ButtonId : uint8_t { kEnter = 0, kUp, kDown };

// boot 階段枚舉;splash 用此切文案。順序對應 splash 狀態機典型路徑。
enum class BootStage : uint8_t {
    kInitializing = 0,
    kProvisioning,       // 無 cred,captive portal 模式
    kWifiConnecting,     // 試連 STA(載荷帶 ssid)
    kWifiFailed,         // 試連超時/認證失敗
    kSntp,               // 等系統時間對齊
    kRegistering,        // 調 /devices
    kServerUnreachable,  // 服務器 30s 無響應
    kAwaitingPair,       // 註冊完畢,等待 Web 端 claim(載荷帶 pair_code)
    kAwaitingGroup,      // 已 bound,等待管理端分配內容組
    kNetError,           // 其它網絡異常
};

enum class GroupSyncStatusMode : uint8_t {
    kCycleTarget = 0,          // 已拿到主動切換目標
    kCycleCacheHit,            // 主動切換目標命中本地緩存
    kCycleDownloading,         // 正在下載主動切換目標
    kCurrentGroupUpdating,     // 後台刷新正在更新當前內容組
    kInitialGroupDownloading,  // 啓動/普通同步正在下載目標內容組
    kTargetGroupSaving,        // 下載後正在保存目標內容組緩存
    kCurrentGroupSaving,       // 下載後正在保存當前內容組緩存
    kCycleFailed,              // 主動切換失敗，保留當前內容組
};

namespace evt {
namespace limits {
inline constexpr size_t kGroupIdBytes         = 32;
inline constexpr size_t kGroupSyncNameBytes   = 48;
inline constexpr size_t kGroupNameBytes       = 64;
inline constexpr size_t kWifiSsidBytes        = 33;
inline constexpr size_t kPairCodeBytes        = 8;
inline constexpr int    kMaxGroupContentCount = 100;
}  // namespace limits
}  // namespace evt

enum class UiEventKind : uint8_t {
    kButtonShort,       // u.button.btn
    kButtonLong,        // u.button.btn
    kButtonDouble,      // u.button.btn
    kChargeChanged,     // u.charge
    kBatteryUpdated,    // u.battery
    kWifiStateChanged,  // u.wifi
    kSyncStarted,
    kSyncProgress,      // u.progress { current, total }  幀級下載進度
    kGroupSyncStatus,   // u.group_sync  內容組切換/下載/更新狀態
    kSyncFinished,      // u.sync
    kCachedGroupReady,  // u.group
    kSyncedGroupReady,  // u.group
    kMinuteTick,
    kIdleTimeout,
    // 啓動階段進度,由 app.cc TryConnectAndSetup 各步 emit;splash 用 stage 切文案。
    kBootStage,  // u.boot_stage
    // 設備從 unbound 翻 bound:Web 端用户輸入了配對碼。splash 切「等待內容組」。
    kBound,
    // 設備從 bound 翻 unbound:Web 端主動解綁。任何場景需 RequestReplace 回 splash。
    kUnbound,  // u.unbound { pair_code[8] }
    // poll 收到 401:secret 失效,固件 self-reset 流(清 NVS secret + 重啓)。
    kSecretInvalid,
    // RTC timer 喚醒後台刷新場景完成/放棄，App 可立即進入下一輪 deep sleep。
    kBgRefreshDone,
    // Xiaozhi 子系統狀態變化。Scene 收到後從 XiaozhiService 讀取最新快照。
    kXiaozhiChanged,
    // Xiaozhi 網絡/服務端主動關閉。App 收到後轉交 XiaozhiService 收束對應會話。
    kXiaozhiChannelClosed,  // u.xiaozhi_channel.token
};

struct UiEvent {
    UiEventKind kind;
    union U {
        struct {
            ButtonId btn;
        } button;
        struct {
            uint8_t state;  // ChargeStatus::State
            bool    present;
            bool    charging;
            bool    full;
            bool    no_battery;
        } charge;
        struct {
            int mv;
            int pct;
        } battery;
        struct {
            bool connected;
            int  rssi;
        } wifi;
        struct {
            bool ok;
            bool group_changed;
        } sync;
        struct {
            uint8_t current;
            uint8_t total;
        } progress;
        struct {
            char                gid[evt::limits::kGroupIdBytes];
            char                name[evt::limits::kGroupSyncNameBytes];
            GroupSyncStatusMode mode;
            uint8_t             current;
            uint8_t             total;
        } group_sync;
        struct {
            char gid[evt::limits::kGroupIdBytes];
            char name[evt::limits::kGroupNameBytes];  // 當前組名（UTF-8），用於狀態欄 / boot splash 文案
            int  content_count;
            // true = 本輪 sync 真下載了新 frame(內容變化);false = fast-path/304,只是確認狀態。
            // FrameScene 用它決定是否觸發 EPD full refresh,避免 30s 心跳每輪都閃屏。
            bool content_changed;
        } group;
        struct {
            BootStage stage;
            char      ssid[evt::limits::kWifiSsidBytes];       // kWifiConnecting 時設 STA SSID
            char      pair_code[evt::limits::kPairCodeBytes];  // kAwaitingPair 時設 6 位 + nul
        } boot_stage;
        struct {
            char pair_code[evt::limits::kPairCodeBytes];
        } unbound;
        struct {
            uint32_t token;
        } xiaozhi_channel;
        U() : group{} {
        }
    } u;

    UiEvent() : kind(UiEventKind::kMinuteTick), u() {
    }
};

static_assert(std::is_trivially_copyable_v<UiEvent>, "UiEvent must stay byte-copyable for FreeRTOS queues");
static_assert(std::is_standard_layout_v<UiEvent>, "UiEvent must stay layout-stable for FreeRTOS queues");
static_assert(std::is_trivially_destructible_v<UiEvent>, "UiEvent must not own resources in FreeRTOS queues");
static_assert(sizeof(UiEvent) <= 128, "UiEvent queue item grew unexpectedly");
