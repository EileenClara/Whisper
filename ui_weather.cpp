/*
 * ui_weather.cpp — 纯文字版天气面板（图标暂时移除）
 * 北京 x=2..118  | 新加坡 x=122..238
 */

#include "ui_weather.h"
int UIWeather::_lastBjTemp = -999;
int UIWeather::_lastSgTemp = -999;

void UIWeather::begin() { _lastBjTemp = -999; _lastSgTemp = -999; draw(); }

void UIWeather::update() {
    if (!WeatherBridge::isValid()) return;
    int bt = WeatherBridge::beijing().temp;
    int st = WeatherBridge::singapore().temp;
    if (bt != _lastBjTemp || st != _lastSgTemp) {
        draw(); _lastBjTemp = bt; _lastSgTemp = st;
    }
}

void UIWeather::draw() {
    if (!WeatherBridge::isValid()) return;
    _drawPanel(2, WeatherBridge::beijing());
    _drawPanel(122, WeatherBridge::singapore());
}

void UIWeather::_drawPanel(int x, const WeatherData& wd) {
    TFT_eSPI& tft = DisplayScreen::tft();
    // 清除旧区域
    tft.fillRect(x, 0, 116, 74, TFT_BLACK);

    // 城市名 16px
    tft.setTextSize(2);
    tft.setTextColor(0xFFE0, TFT_BLACK);  // 黄色醒目
    tft.setCursor(x, 2);
    tft.print(wd.city);

    // 温度 16px 白色
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(x, 24);
    tft.print(wd.temp);
    tft.print("C");

    // 湿度 16px
    tft.setCursor(x + 60, 24);
    tft.print(wd.humidity);
    tft.print("%");

    // 天气描述 8px (长文本不换行)
    tft.setTextSize(1);
    tft.setTextColor(0x7E7E, TFT_BLACK);
    tft.setCursor(x, 50);
    tft.print(wd.weatherText.substring(0, 18));  // 最多 18 字符
}

bool UIWeather::hitBeijing(uint16_t tx, uint16_t ty)   { return tx < 120 && ty < 75; }
bool UIWeather::hitSingapore(uint16_t tx, uint16_t ty) { return tx >= 120 && ty < 75; }
uint16_t UIWeather::_humidityColor(int h) { return 0x0AFF; }
