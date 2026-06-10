/**
 * @file display.h
 * @brief Whisper TFT 屏幕驱动 + UI 绘制 (ST7789 172x320)
 *
 * 使用 TFT_eSPI 库驱动 ST7789 屏幕
 * UI 布局:
 *  ┌──────────────────────────┐
 *  │  🕐 14:30           🌤 32°│ ← 本地时间 + 本地天气
 *  │  上海                     │
 *  │  🕐 15:30           ⛈ 28°│ ← 对方时间 + 对方天气
 *  │  新加坡                    │
 *  │  ┌─────────────────────┐  │
 *  │  │ 💕 想你             │  │ ← 对方状态
 *  │  └─────────────────────┘  │
 *  │  ┌─────────────────────┐  │
 *  │  │ 💬 想你了💕  2分钟前│  │ ← 最新消息
 *  │  └─────────────────────┘  │
 *  │  摇一摇 ❤️  |  双摇 消息  │ ← 底部操作提示
 *  └──────────────────────────┘
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>

/** 模拟的天气图标（Unicode字符） */
struct WeatherIcon {
    const char* code;   // OpenWeatherMap icon code 前缀
    const char* icon;   // Unicode 图标
};
static const WeatherIcon WEATHER_ICONS[] = {
    {"01d", "☀️"}, {"01n", "🌙"},
    {"02d", "⛅"}, {"02n", "☁️"},
    {"03d", "☁️"}, {"03n", "☁️"},
    {"04d", "☁️"}, {"04n", "☁️"},
    {"09d", "🌧️"}, {"09n", "🌧️"},
    {"10d", "🌦️"}, {"10n", "🌧️"},
    {"11d", "⛈️"}, {"11n", "⛈️"},
    {"13d", "🌨️"}, {"13n", "🌨️"},
    {"50d", "🌫️"}, {"50n", "🌫️"},
};

class Display {
public:
    Display();
    ~Display();

    /** 初始化屏幕 */
    bool begin();

    /** 绘制主屏幕 */
    void drawHomeScreen(
        const char* localTime,      // 本地时间 "14:30"
        const char* localDate,      // 本地日期 "6月10日 周三"
        const char* localCity,      // 本地城市
        float localTemp,            // 本地温度
        const char* localIcon,      // 本地天气图标代码
        const char* partnerTime,    // 对方时间
        const char* partnerDate,    // 对方日期
        const char* partnerCity,    // 对方城市
        float partnerTemp,          // 对方温度
        const char* partnerIcon,    // 对方天气图标代码
        const char* partnerStatus,  // 对方状态 "💕 想你"
        const char* lastMsg,        // 最新消息内容
        const char* lastMsgTime,    // 消息时间
        int rssi                    // WiFi 信号强度
    );

    /** 绘制心跳模式提示 */
    void drawHeartbeatPrompt();

    /** 绘制❤️动画（收到对方心跳时） */
    void drawHeartAnimation();

    /** 绘制快捷消息菜单（摇动切换） */
    void drawMessageMenu(int selectedIndex, const char* messages[], int count);

    /** 绘制配网提示 */
    void drawWiFiSetupPrompt(const char* apName);

    /** 绘制连接状态（WiFi/MQTT 连接中...） */
    void drawConnectingScreen(const char* status);

    /** 设置屏幕背光亮度 (0-255) */
    void setBrightness(uint8_t brightness);

    /** 清屏 */
    void clear();

    /** 获取 TFT 对象引用 */
    TFT_eSPI& getTFT();

private:
    TFT_eSPI _tft;
    uint8_t _brightness = 255;

    /** 根据天气图标代码获取 Unicode 天气图标 */
    const char* _getWeatherIcon(const char* iconCode);

    /** 在指定位置绘制天气信息行 */
    void _drawWeatherRow(int y, const char* timeStr, const char* dateStr,
                         const char* city, float temp, const char* iconCode);

    /** 绘制顶部状态栏（WiFi信号 + 电池等） */
    void _drawStatusBar(int rssi, bool charging);

    /** 绘制底部操作提示 */
    void _drawBottomBar();

    /** 将 UTF-8 字符串写到屏幕（TFT_eSPI 基础 drawString 支持 ASCII） */
    void _drawUTF8String(const char* str, int x, int y);
};

#endif // DISPLAY_H
