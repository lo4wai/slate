#pragma once

// 場景棧：活躍態通常以 FrameScene 為根；配網和後台刷新也可作為啓動根場景。
// 其他 Scene（設置頁 / 子菜單）push 在當前根場景之上。
// 所有同步切換方法（Push/Pop/Replace）只能由 ui_loop task 調用，否則 LVGL 不安全。
// Scene::OnEvent 內若需切換，應調 RequestX；ui_loop 在 Dispatch 後調 ApplyPending。

#include <cstdint>
#include <memory>
#include <vector>

#include "scenes/core/scene.h"

class SceneStack {
   public:
    SceneStack() = default;

    void SetContext(const SceneContext& ctx) {
        ctx_ = ctx;
    }
    SceneContext& Context() {
        return ctx_;
    }

    // 同步切換（僅 ui_loop 調）
    void Push(std::unique_ptr<Scene> s);
    void Pop();
    void Replace(std::unique_ptr<Scene> s);

    Scene* Top() const {
        return stack_.empty() ? nullptr : stack_.back().get();
    }
    bool Empty() const {
        return stack_.empty();
    }

    // 給 Scene::OnEvent 內用的 deferred 切換。Apply 時 ui_loop 取出執行。
    void RequestPush(std::unique_ptr<Scene> s);
    void RequestPop();
    void RequestReplace(std::unique_ptr<Scene> s);

    // ui_loop 每次 Dispatch 後調一次。
    void ApplyPending();

    // 把事件分發給棧頂。階段 1 僅給 Top()；狀態類事件（charge/wifi/sync）也是
    // 給 Top() 即可，因為子 Scene 不可見時本來就不該處理。
    void Dispatch(const UiEvent& e);

   private:
    enum class PendingKind { kPush, kPop, kReplace };
    struct PendingOp {
        PendingKind            kind;
        std::unique_ptr<Scene> scene;
    };

    void ResetRootRetry();

    SceneContext                        ctx_;
    std::vector<std::unique_ptr<Scene>> stack_;
    std::vector<PendingOp>              pending_ops_;
    Scene*                              root_retry_scene_   = nullptr;
    int64_t                             next_root_retry_ms_ = 0;
    uint8_t                             root_retry_count_   = 0;
};
