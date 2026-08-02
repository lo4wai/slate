#include "scenes/settings/settings_scene.h"

#include <esp_log.h>
#include <memory>
#include <utility>

#include "drivers/display/epd_ssd1683.h"
#include "events/event_bus.h"
#include "events/ui_event_log.h"
#include "scenes/core/scene_stack.h"
#include "scenes/settings/settings_pages.h"
#include "ui/menu_list.h"
#include "ui/theme.h"

namespace {
constexpr char kTag[] = "settings";
}

SettingsScene::SettingsScene()  = default;
SettingsScene::~SettingsScene() = default;

void SettingsScene::OnEnter(SceneContext& ctx) {
    ESP_LOGD(kTag, "enter saved_cursor=%d", saved_cursor_);
    if (!ctx.epd->Lock(2000)) {
        ESP_LOGW(kTag, "enter failed reason=epd_lock_timeout");
        return;
    }

    root_ = CreateFullscreenRoot();

    status_bar_ = std::make_unique<StatusBar>(root_);
    status_bar_->SetCaption("設置");

    // 三段語義分組(MenuList 不顯式畫分隔,靠順序傳達):
    //   偏好    音量調節
    //   信息    設備信息
    //   危險    重啓設備 / 恢復出廠(永遠末尾,避免誤觸)
    auto*                       stack = ctx.stack;
    std::vector<MenuList::Item> items = {
        {"音量調節", [stack]() { stack->RequestPush(std::make_unique<VolumePage>()); }},
        {"設備信息", [stack]() { stack->RequestPush(std::make_unique<DeviceInfoPage>()); }},
        {"重啓設備", [stack]() { stack->RequestPush(std::make_unique<RestartDevicePage>()); }},
        {"恢復出廠", [stack]() { stack->RequestPush(std::make_unique<FactoryResetPage>()); }},
    };
    menu_ = std::make_unique<MenuList>(root_, std::move(items), saved_cursor_);

    RefreshStatusBarFromSensors(ctx, *status_bar_);

    lv_refr_now(NULL);
    ctx.epd->Unlock();
    // 子頁 OnEnter 走 partial:UI ↔ UI 切換 diff 小,EPD 自己看 diff>=30% 兜底升 full。
    // 從 frame 進來 diff 必然 >30% 自動 full;settings ↔ 子頁之間 partial 即可。
    ctx.epd->RequestUrgentPartialRefresh();
    ESP_LOGD(kTag, "enter done root=%p", root_);
}

void SettingsScene::OnExit(SceneContext& ctx) {
    ESP_LOGD(kTag, "exit root=%p cursor=%d", root_, menu_ ? menu_->Cursor() : -1);
    DestroyRoot(ctx, root_, [this]() {
        // push 子頁時 OnExit 觸發,記錄光標位置 — 子頁 pop 回來 OnEnter 恢復。
        if (menu_)
            saved_cursor_ = menu_->Cursor();
        menu_.reset();
        status_bar_.reset();
    });
}

void SettingsScene::OnEvent(SceneContext& ctx, const UiEvent& e) {
    if (evt::log::DebugEnabled(kTag)) {
        char detail[128];
        evt::log::Describe(e, detail, sizeof(detail));
        ESP_LOGD(kTag, "event kind=%s detail=%s root=%p menu=%p cursor=%d", evt::log::KindName(e.kind), detail, root_,
                 menu_.get(), menu_ ? menu_->Cursor() : -1);
    }
    if (!root_ || !menu_) {
        ESP_LOGW(kTag, "event ignored reason=no_root root=%p menu=%p kind=%s", root_, menu_.get(),
                 evt::log::KindName(e.kind));
        return;
    }
    switch (e.kind) {
        case UiEventKind::kButtonShort: {
            switch (e.u.button.btn) {
                case ButtonId::kUp:
                    SyncRender(
                        ctx,
                        [this]() {
                            const int before = menu_->Cursor();
                            menu_->OnUp();
                            ESP_LOGD(kTag, "button short btn=up action=menu_up from=%d to=%d", before, menu_->Cursor());
                        },
                        false);
                    break;
                case ButtonId::kDown:
                    SyncRender(
                        ctx,
                        [this]() {
                            const int before = menu_->Cursor();
                            menu_->OnDown();
                            ESP_LOGD(kTag, "button short btn=down action=menu_down from=%d to=%d", before,
                                     menu_->Cursor());
                        },
                        false);
                    break;
                case ButtonId::kEnter:
                    ESP_LOGD(kTag, "button short btn=enter action=menu_enter cursor=%d", menu_->Cursor());
                    menu_->OnEnter();
                    break;  // RequestPush 由 callback 走,ApplyPending 處理
            }
            break;
        }
        case UiEventKind::kButtonLong: {
            if (e.u.button.btn == ButtonId::kEnter) {
                ESP_LOGD(kTag, "button long btn=enter action=pop");
                ctx.stack->RequestPop();
            }
            break;
        }
        default:
            break;
    }
}
