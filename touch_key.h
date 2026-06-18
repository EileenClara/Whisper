#ifndef TOUCH_KEY_H
#define TOUCH_KEY_H

#include <Arduino.h>

#define PIN_TP_KEY  5

enum TapType { TAP_NONE = 0, TAP_SHORT, TAP_LONG };

class TouchKey {
public:
    static void begin();
    static TapType check();   // 每 30ms 调用

private:
    static bool _pressing;          // 当前是否按下
    static bool _wasPressing;       // 上一帧
    static bool _longFired;         // 本次按压已触发过长按
    static unsigned long _downTime; // 按下时刻
    static int _baseline;
    static int _threshold;
};

#endif
