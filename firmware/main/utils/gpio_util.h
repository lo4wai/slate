#pragma once

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "utils/scoped_mutex_lock.h"

inline SemaphoreHandle_t GpioHoldMutex() {
    static StaticSemaphore_t s_mutex_buf;
    static SemaphoreHandle_t s_mutex = xSemaphoreCreateMutexStatic(&s_mutex_buf);
    return s_mutex;
}

// 把"hold_dis → set_level → hold_en"三段式打包成一個 inline。
// 板上很多 rail 用 hold_en 鎖電平避免 deep/light sleep 期間被 IO MUX 拉低,
// 改電平時必須先 hold_dis,否則 set_level 不生效;再 hold_en 重新鎖回去。
//
// 多個 task 可能同時改 rail/PA pin；這裏用全局短臨界區避免三段式交錯。
inline void GpioWriteHold(gpio_num_t pin, int level) {
    ScopedMutexLock lock(GpioHoldMutex());
    gpio_hold_dis(pin);
    gpio_set_level(pin, level);
    gpio_hold_en(pin);
}

inline void GpioWriteHold(int pin, int level) {
    GpioWriteHold(static_cast<gpio_num_t>(pin), level);
}
