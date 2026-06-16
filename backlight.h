/*
 * backlight.h — 背光 LEDC PWM 控制
 * ESP32-S3 不支持 analogWrite，必须用 LEDC
 */

#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include <Arduino.h>
#include "config.h"

class Backlight {
public:
    static void begin();
    static void set(uint8_t pct);       // 0-100
    static uint8_t get();               // 返回当前百分比

private:
    static uint8_t _pct;
};

#endif
