/*
 * touch_handler.cpp — XPT2046 触摸实现
 * 触摸屏共用 TFT SPI 总线, CS=5, IRQ=4
 */

#include "touch_handler.h"
#include <SPI.h>

bool TouchHandler::_wasPressed = false;
bool TouchHandler::_pressed = false;
uint16_t TouchHandler::_x = 0;
uint16_t TouchHandler::_y = 0;
unsigned long TouchHandler::_lastEvent = 0;

void TouchHandler::begin() {
    pinMode(PIN_TOUCH_CS, OUTPUT);
    digitalWrite(PIN_TOUCH_CS, HIGH);

    if (PIN_TOUCH_IRQ >= 0) {
        pinMode(PIN_TOUCH_IRQ, INPUT_PULLUP);
    }
}

void TouchHandler::loop() {
    _wasPressed = _pressed;

    // 简单检测: 拉低 CS, 读 X/Y
    // XPT2046 需要在低速 SPI 下读取
    if (PIN_TOUCH_IRQ >= 0) {
        if (digitalRead(PIN_TOUCH_IRQ) == HIGH) {
            _pressed = false;
            return;
        }
    }

    // 有触摸 → 读取坐标
    _readRaw();

    // 去抖
    unsigned long now = millis();
    if (_x > 0 && _y > 0) {
        if (!_pressed && (now - _lastEvent > TOUCH_DEBOUNCE_MS)) {
            _pressed = true;
            _lastEvent = now;
        }
    } else {
        _pressed = false;
    }
}

void TouchHandler::_readRaw() {
    // XPT2046 读取 (简化版, 使用 TFT 的 SPI)
    SPI.beginTransaction(SPISettings(2500000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_TOUCH_CS, LOW);

    // 读 X
    SPI.transfer(0xD0);  // 控制字: 读 X, 12-bit, 差分
    uint8_t xh = SPI.transfer(0);
    uint8_t xl = SPI.transfer(0);
    uint16_t rawX = ((xh << 8) | xl) >> 3;

    // 读 Y
    SPI.transfer(0x90);  // 控制字: 读 Y, 12-bit, 差分
    uint8_t yh = SPI.transfer(0);
    uint8_t yl = SPI.transfer(0);
    uint16_t rawY = ((yh << 8) | yl) >> 3;

    digitalWrite(PIN_TOUCH_CS, HIGH);
    SPI.endTransaction();

    // 映射到 0..240 (需要根据实际屏幕校准)
    // 典型 XPT2046: raw 200~3800 → 0~240
    _x = constrain(map(rawX, 300, 3800, 0, 240), 0, 240);
    _y = constrain(map(rawY, 300, 3800, 0, 240), 0, 240);
}

bool TouchHandler::isPressed()     { return _pressed; }
bool TouchHandler::justPressed()   { return _pressed && !_wasPressed; }
bool TouchHandler::justReleased()  { return !_pressed && _wasPressed; }
uint16_t TouchHandler::x()         { return _x; }
uint16_t TouchHandler::y()         { return _y; }

TouchZone TouchHandler::zone() {
    return detectMainScreen(_x, _y);
}

// ===== 主屏幕区域检测 =====
TouchZone TouchHandler::detectMainScreen(uint16_t tx, uint16_t ty) {
    // 天气区 (y < 80)
    if (ty < 80) {
        if (tx < 85)  return TZ_WEATHER_BJ;
        if (tx < 170) return TZ_WEATHER_SG;
    }

    // 状态区 (y 150~175)
    if (ty >= 150 && ty < 175) {
        if (tx < 120) return TZ_STATUS_GUGU;
        return TZ_STATUS_VV;
    }

    // 爱心区 (y >= 175)
    if (ty >= 175) return TZ_HEART_AREA;

    return TZ_NONE;
}

// ===== 状态选择页 (2列 x 4行) =====
TouchZone TouchHandler::detectMoodPicker(uint16_t tx, uint16_t ty) {
    // 8 个格子: 每格 100x50, 2 列
    if (ty < 40 || ty > 240) return TZ_NONE;

    int row = (ty - 40) / 50;
    int col = tx < 120 ? 0 : 1;

    // 返回选项编号 0-7, 存储在 TZ 里
    // 左上(空闲=0), 右上(游戏=1), 第二行左(忙碌=2)...
    return TZ_MOOD_PICKER;
}

// ===== 身份选择页 =====
TouchZone TouchHandler::detectIdentityScreen(uint16_t tx, uint16_t ty) {
    if (ty < 80 || ty > 200) return TZ_NONE;
    if (tx < 60 || tx > 180) return TZ_NONE;

    if (ty < 140) return TZ_IDENTITY_GUGU;
    return TZ_IDENTITY_VV;
}
