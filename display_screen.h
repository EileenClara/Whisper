/*
 * display_screen.h — TFT 显示初始化与基础操作
 */

#ifndef DISPLAY_SCREEN_H
#define DISPLAY_SCREEN_H

#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include "config.h"

// 脏区域标记 (位掩码)
enum ZoneMask : uint8_t {
    ZONE_CLOCK       = 0x01,
    ZONE_DATE        = 0x02,
    ZONE_WEATHER_BJ  = 0x04,
    ZONE_WEATHER_SG  = 0x08,
    ZONE_STATUS      = 0x10,
    ZONE_HEART       = 0x20,
    ZONE_ALL         = 0xFF
};

class DisplayScreen {
public:
    static void begin();
    static TFT_eSPI& tft();
    static TFT_eSprite& sprite();

    // 脏区域管理
    static void markDirty(uint8_t zone);
    static bool isDirty(uint8_t zone);
    static void clearDirty();
    static uint8_t dirtyZones();

    // 快捷绘制
    static void fillZone(int x, int y, int w, int h, uint16_t color);

private:
    static TFT_eSPI _tft;
    static TFT_eSprite _spr;
    static uint8_t _dirty;
};

// JPG 解码回调
bool tftJpgOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap);

#endif
