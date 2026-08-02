#pragma once

// 啓動佔位屏 + 配對/等待顯示。事件驅動狀態機:
//   - cred::Load 失敗 → kProvisioning(顯示 AP SSID)
//   - kBootStage event 切到對應階段(連 Wi-Fi / 對時 / 註冊中 / 配對碼 ...)
//   - kBound → kAwaitingGroup;kUnbound → kAwaitingPair(載新碼)
//   - kSyncProgress → 幀級下載進度;kSyncFinished{ok=false} → 網絡異常
//   - kCachedGroupReady / kSyncedGroupReady → RequestReplace 切 FrameScene
//
// 配對碼 6 字符用 montserrat_48 大字號居中顯示,確保用户對屏抄碼一眼能看清。

#include <freertos/FreeRTOS.h>

#include <cstdint>

#include "scenes/core/scene.h"

class SplashScene : public Scene {
   public:
    enum class State : uint8_t {
        kInitializing = 0,
        kProvisioning,
        kWifiConnecting,
        kWifiFailed,
        kSntp,
        kRegistering,
        kServerUnreachable,
        kAwaitingPair,
        kAwaitingGroup,
        kNetError,
        kSyncProgress,
    };

    const char* Name() const override {
        return "splash";
    }
    void      OnEnter(SceneContext& ctx) override;
    void      OnExit(SceneContext& ctx) override;
    void      OnEvent(SceneContext& ctx, const UiEvent& e) override;
    lv_obj_t* Root() override {
        return root_;
    }

   private:
    void CreateLayout();
    void RenderContent();
    void Render(SceneContext& ctx);

    State   state_             = State::kInitializing;
    char    ssid_[33]          = {0};
    char    pair_code_[8]      = {0};
    char    progress_name_[48] = {0};
    uint8_t progress_cur_      = 0;
    uint8_t progress_total_    = 0;

    lv_obj_t* root_       = nullptr;
    lv_obj_t* text_label_ = nullptr;  // 主文案(中文,Zfull)
    lv_obj_t* code_label_ = nullptr;  // 配對碼大字(montserrat_48,僅 kAwaitingPair 顯示)
    lv_obj_t* hint_label_ = nullptr;  // 底部應急逃生 hint

    // 進度節流:啓動期下載幾十幀時,SyncProgress 高頻觸發,需要節流避免
    // EPD 累計 8 次 partial 自動升 full 閃屏。
    uint8_t    last_progress_current_ = 0xFF;
    TickType_t last_progress_tick_    = 0;
};
