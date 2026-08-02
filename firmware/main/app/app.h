#pragma once

// 頂層 App。把所有子系統按依賴順序串起來：
//   Storage → Board → Audio → EventBus → SceneStack → ui_loop task →
//   Inputs(按鍵/充電→EventBus) → MinuteBoundaryTicker → Network → SleepManager → PM
//
// Run() 等同 vTaskDelete(NULL)：把 main task 的 8 KB 棧讓出來，
// 由各後台 task（ui_loop / sync / charge_tick / audio / epd_refresh）繼續跑。

#include <atomic>
#include <memory>
#include <string>

#include "drivers/input/up_down_combo.h"
#include "events/event_bus.h"
#include "network/cred_store.h"
#include "power/minute_boundary_ticker.h"
#include "power/sleep_manager.h"
#include "scenes/core/scene_stack.h"
#include "startup/boot_mode.h"

class CaptivePortal;

class App {
   public:
    App();
    ~App();
    void Init();
    void Run();

   private:
    void InitStorage();
    void InitDevices();
    void InitEventBus();
    void InitSceneStack();
    void StartUiLoop();
    void AttachInputs();
    void StartMinuteBoundaryTicker();
    bool InitWifiAndSync(cred::Credentials& creds, bool background_refresh);
    bool ReadBattery(int* mv, int* pct);
    void StartSleep();
    void FinalizePm();
    // 按當前供電/模式決定是否啓用自動 light sleep，並應用 PM 配置。充電狀態變化時重調。
    void ConfigurePm(bool light_sleep_enable);
    bool ShouldEnableLightSleep(bool power_present) const;

    void StartPortal();
    void PostWakeupKeyEvent(uint64_t ext1_mask);
    void PromoteToFrameSceneFromCache();
    bool HandleSecretInvalid(const UiEvent& e);
    bool HandleBackgroundRefreshDone(const UiEvent& e);
    bool HandleXiaozhiChannelClosed(const UiEvent& e);
    bool HandleInitialGroupReady(const UiEvent& e);
    bool HandleEnterDoubleClick(const UiEvent& e);

    static void UiLoopEntry(void* arg);
    void        UiLoopTask();

    SceneStack                     scene_stack_;
    SleepManager                   sleep_mgr_;
    MinuteBoundaryTicker           minute_ticker_;
    UpDownComboController          up_down_combo_;
    std::unique_ptr<CaptivePortal> portal_;
    std::atomic<bool>              ui_loop_running_{false};
    boot_mode::Decision            decision_;
};
