#pragma once

// 設置主菜單。FrameScene 的 ENTER 長按觸發 push 到棧。
// UP/DOWN 短按移動光標,ENTER 短按 push 子頁,ENTER 長按 pop 回 FrameScene。
// 條目超過 MenuList::kVisibleRows(6)時啓用視口滾動,光標越界自動滾屏。

#include <memory>

#include "scenes/core/scene.h"
#include "ui/status_bar.h"

class MenuList;

class SettingsScene : public Scene {
   public:
    SettingsScene();
    // ~SettingsScene 必須在 .cc 實現:這裏 unique_ptr<MenuList> 默認析構需要
    // 看到 MenuList 的完整定義,如果讓編譯器自動隱式 inline 析構,frame_scene.cc
    // 在 std::make_unique<SettingsScene>() 處會因 menu_list.h 沒被 include 而
    // sizeof(MenuList) 失敗。
    ~SettingsScene() override;

    const char* Name() const override {
        return "settings";
    }
    bool IsSettings() const override {
        return true;
    }
    void      OnEnter(SceneContext& ctx) override;
    void      OnExit(SceneContext& ctx) override;
    void      OnEvent(SceneContext& ctx, const UiEvent& e) override;
    lv_obj_t* Root() override {
        return root_;
    }

   private:
    lv_obj_t*                  root_ = nullptr;
    std::unique_ptr<StatusBar> status_bar_;
    std::unique_ptr<MenuList>  menu_;
    int                        saved_cursor_ = 0;  // 子頁 pop 回來時恢復光標位置
};
