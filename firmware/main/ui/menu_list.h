#pragma once

// EPD 友好的列表控件:每項一個 lv_label,光標位置用左側黑色 bar 標記。
// items > kVisibleRows 時右側出現一個細 thumb 標記當前視口在總長度裏的位置,
// 不畫 track(1bpp 灰階有限,track + thumb 都用純黑會糊在一起)。
//
// 屏幕 400×300 - status bar 24 = root 276,對稱 pad 12,行高 42 -> 視口 6 行
// (12 + 6×42 + 12 = 276 完美填滿,thumb track 區也自然對稱)。
// 滾動一律走 partial refresh(EPD 自身按 dirty 比例自決是否升 full),
// 不主動 full,犧牲一點殘影換響應速度。OnUp/OnDown 仍返回是否發生 viewport
// 平移,留給調用方擴展用。

#include <functional>
#include <string>
#include <vector>

#include <lvgl.h>

class MenuList {
   public:
    struct Item {
        std::string           title;
        std::function<void()> on_enter;  // ENTER 短按觸發
    };

    static constexpr int kVisibleRows = 6;

    MenuList(lv_obj_t* parent, std::vector<Item> items, int initial_cursor = 0);

    // 返回 true 表示視口發生了滾動(調用方應走 full refresh 而非 partial)。
    bool OnUp();
    bool OnDown();
    void OnEnter();  // 觸發當前項

    int Cursor() const {
        return cursor_;
    }
    lv_obj_t* root() {
        return root_;
    }

   private:
    void Redraw();
    // 調整 viewport_top_ 使 cursor_ 落在 [viewport_top_, viewport_top_ + kVisibleRows)。
    // 返回 true 表示 viewport_top_ 發生變化。
    bool EnsureCursorVisible();

    std::vector<Item>      items_;
    int                    cursor_       = 0;
    int                    viewport_top_ = 0;
    lv_obj_t*              root_         = nullptr;
    std::vector<lv_obj_t*> rows_;  // 行容器,滾動時調整 y / hidden
    std::vector<lv_obj_t*> cursor_bars_;
    // 右側 scrollbar thumb,items > kVisibleRows 時才創建。
    // 高度 = visible/total × track_h(下界 theme::kScrollbarThumbMinH),y 跟隨 viewport_top_。
    lv_obj_t* thumb_ = nullptr;
};
