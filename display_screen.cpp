/*
 * display_screen.cpp — TFT 实现
 */

#include "display_screen.h"

TFT_eSPI DisplayScreen::_tft = TFT_eSPI();
TFT_eSprite DisplayScreen::_spr = TFT_eSprite(&_tft);
uint8_t DisplayScreen::_dirty = ZONE_ALL;

// JPG 解码回调
bool tftJpgOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= DisplayScreen::tft().height()) return 0;
    DisplayScreen::tft().pushImage(x, y, w, h, bitmap);
    return 1;
}

void DisplayScreen::begin() {
    _tft.begin();
    _tft.setRotation(2);    // 0=默认, 2=翻转180度 (如果字倒了就改: 0→2, 2→0)
    _tft.fillScreen(TFT_BLACK);
    _tft.invertDisplay(1);  // 颜色反转 (黑变白白变黑就改成 0)

    // JPG 解码器
    TJpgDec.setJpgScale(1);
    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(tftJpgOutput);

    // 精灵
    _spr.setColorDepth(8);
}

TFT_eSPI& DisplayScreen::tft() { return _tft; }
TFT_eSprite& DisplayScreen::sprite() { return _spr; }

void DisplayScreen::markDirty(uint8_t zone) { _dirty |= zone; }
bool DisplayScreen::isDirty(uint8_t zone) { return _dirty & zone; }
void DisplayScreen::clearDirty() { _dirty = 0; }
uint8_t DisplayScreen::dirtyZones() { return _dirty; }

void DisplayScreen::fillZone(int x, int y, int w, int h, uint16_t color) {
    _tft.fillRect(x, y, w, h, color);
}
