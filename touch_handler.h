/*
 * touch_handler.h — XPT2046 触摸驱动
 * 触摸检测 + 去抖 + 区域判断
 */

#ifndef TOUCH_HANDLER_H
#define TOUCH_HANDLER_H

#include <Arduino.h>
#include "config.h"

enum TouchZone {
    TZ_NONE = 0,
    TZ_WEATHER_BJ,      // 北京天气
    TZ_WEATHER_SG,      // 新加坡天气
    TZ_STATUS_GUGU,     // 果果状态 (左半边)
    TZ_STATUS_VV,       // vv状态 (右半边)
    TZ_HEART_AREA,      // 爱心区
    TZ_MOOD_PICKER,     // 状态选择页上的选项
    TZ_IDENTITY_GUGU,   // 首次开机选果果
    TZ_IDENTITY_VV      // 首次开机选vv
};

class TouchHandler {
public:
    static void begin();
    static void loop();

    static bool isPressed();     // 当前是否按下
    static bool justPressed();   // 刚按下 (上升沿，已去抖)
    static bool justReleased();  // 刚释放

    static uint16_t x();
    static uint16_t y();
    static TouchZone zone();     // 当前触摸区域

    // 不同页面的区域检测
    static TouchZone detectMainScreen(uint16_t tx, uint16_t ty);
    static TouchZone detectMoodPicker(uint16_t tx, uint16_t ty);
    static TouchZone detectIdentityScreen(uint16_t tx, uint16_t ty);

private:
    static void _readRaw();
    static bool _wasPressed;
    static bool _pressed;
    static uint16_t _x, _y;
    static unsigned long _lastEvent;
};

#endif
