/*
 * touch_key.cpp — 自适应电容触摸
 * 开机时采样基线，阈值 = 基线 + 500
 */

#include "touch_key.h"

bool TouchKey::_pressing = false;
bool TouchKey::_wasPressing = false;
bool TouchKey::_longFired = false;
unsigned long TouchKey::_downTime = 0;
int TouchKey::_baseline = 0;
int TouchKey::_threshold = 0;

void TouchKey::begin() {
    _pressing = _wasPressing = _longFired = false;

    // 采样 1 秒建立基线
    long sum = 0;
    int count = 0;
    unsigned long start = millis();
    while (millis() - start < 1000) {
        sum += touchRead(PIN_TP_KEY);
        count++;
        delay(5);
    }
    _baseline = sum / count;
    _threshold = _baseline + 500;  // 摸到时上升约 500-800

    Serial.printf("[Touch] baseline=%d threshold=%d\n", _baseline, _threshold);
}

TapType TouchKey::check() {
    uint32_t val = touchRead(PIN_TP_KEY);
    _pressing = (val > _threshold);

    unsigned long now = millis();
    TapType result = TAP_NONE;

    if (_pressing && !_wasPressing) {
        _downTime = now;
        _longFired = false;
    }

    if (_pressing && !_longFired && (now - _downTime > 800)) {
        _longFired = true;
        result = TAP_LONG;
    }

    if (!_pressing && _wasPressing) {
        if (!_longFired) result = TAP_SHORT;
    }

    _wasPressing = _pressing;

    static unsigned long lastDbg = 0;
    if (now - lastDbg > 3000) {
        lastDbg = now;
        Serial.printf("[Touch] val=%d thr=%d press=%d\n", val, _threshold, _pressing);
    }

    return result;
}
