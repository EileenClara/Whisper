/*
 * backlight.cpp — LEDC PWM 实现
 */

#include "backlight.h"

uint8_t Backlight::_pct = 80;

void Backlight::begin() {
    ledcSetup(LEDC_CH_BL, LEDC_FREQ_BL, LEDC_RES_BL);
    ledcAttachPin(PIN_TFT_BL, LEDC_CH_BL);
    set(_pct);
}

void Backlight::set(uint8_t pct) {
    _pct = constrain(pct, 0, 100);
    // 老板的配置: TFT_BACKLIGHT_ON HIGH → 高电平亮
    // duty = pct * 255 / 100
    uint32_t duty = (_pct * 255) / 100;
    ledcWrite(LEDC_CH_BL, duty);
}

uint8_t Backlight::get() {
    return _pct;
}
