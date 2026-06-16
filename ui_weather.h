/*
 * ui_weather.h — 双城市天气面板渲染
 * 北京 (x=0..85) | 新加坡 (x=85..170) 在屏幕顶部
 */

#ifndef UI_WEATHER_H
#define UI_WEATHER_H

#include "config.h"
#include "display_screen.h"
#include "weather_bridge.h"

class UIWeather {
public:
    static void begin();
    static void update();   // 数据更新时重绘
    static void draw();     // 强制全量绘制

    // 点击检测
    static bool hitBeijing(uint16_t x, uint16_t y);
    static bool hitSingapore(uint16_t x, uint16_t y);

private:
    static void _drawPanel(int baseX, const WeatherData& wd);
    static void _drawWeatherIcon(int x, int y, int code);
    static uint16_t _humidityColor(int h);

    static int _lastBjTemp;
    static int _lastSgTemp;
};

#endif
