/*
 * touch_key.cpp — 极简可靠版
 * 短按: 摸 (<800ms 释放) → TAP_SHORT
 * 长按: 按住 (>800ms) → TAP_LONG (触发一次)
 * 冷却: 释放后 400ms 内不触发新事件
 */

#include "touch_key.h"

bool TouchKey::_pressing = false;
bool TouchKey::_wasPressing = false;
bool TouchKey::_longFired = false;
unsigned long TouchKey::_downTime = 0;
int TouchKey::_baseline = 0;

void TouchKey::begin() {
    _pressing = false;
    _wasPressing = false;
    _longFired = false;
    Serial.println("[Touch] ready, threshold=15800");
}

TapType TouchKey::check() {
    uint32_t val = touchRead(PIN_TP_KEY);

    _pressing = (val > 15800);

    unsigned long now = millis();
    TapType result = TAP_NONE;

    if (_pressing && !_wasPressing) {
        // 刚按下
        _downTime = now;
        _longFired = false;
    }

    if (_pressing && !_longFired && (now - _downTime > 800)) {
        // 长按 >800ms, 触发一次
        _longFired = true;
        result = TAP_LONG;
    }

    if (!_pressing && _wasPressing) {
        // 刚释放
        if (!_longFired) {
            // 没触发过长按 → 短按
            result = TAP_SHORT;
        }
    }

    _wasPressing = _pressing;

    // 调试
    static unsigned long lastDbg = 0;
    if (now - lastDbg > 3000) {
        lastDbg = now;
        Serial.printf("[Touch] val=%d thresh=15800 press=%d\n", val, _pressing);
    }

    return result;
}
