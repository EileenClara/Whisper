/**
 * @file display.h
 * @brief Whisper TFT 屏幕驱动 + UI 绘制 (ST7789 172x320)
 *
 * UI 布局（单时钟版）:
 *  ┌──────────────────────────┐
 *  │  🔊        📶░░░  🔋     │ ← 状态栏（静音/WiFi/电量）
 *  │                          │
 *  │        14:30             │ ← 单行大字时钟
 *  │     6月10日 周三          │
 *  │                          │
 *  │  北京 🌤 32°  新加坡 🌤 28°│ ← 双城天气
 *  │                          │
 *  │  💕 vv 正在想你          │ ← 对方状态
 *  │──────────────────────────│
 *  │  💬 想你了💕  2分钟前     │ ← 最新消息
 *  │──────────────────────────│
 *  │  摇一摇 ❤️   敲三下 消息  │ ← 操作提示
 *  └──────────────────────────┘
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>

class Display {
public:
    Display();
    ~Display();

    bool begin();

    /** 绘制主屏幕（单时钟 + 双城天气） */
    void drawHomeScreen(
        const char* localTime,      // "14:30"
        const char* localDate,      // "6月10日 周三"
        const char* localCity,      // "北京"
        float localTemp,            // 32
        const char* localIcon,      // "01d"
        const char* partnerCity,    // "新加坡"
        float partnerTemp,          // 28
        const char* partnerIcon,    // "01d"
        const char* partnerStatus,  // "💕 想你"
        const char* lastMsg,        // 最新消息
        const char* lastMsgTime,    // 消息时间
        int rssi,                   // WiFi信号
        bool charging,              // 充电中？
        bool muted                  // 静音？
    );

    /** 心跳模式提示 */
    void drawHeartbeatPrompt();

    /** ❤️ 动画 */
    void drawHeartAnimation();

    /** 快捷消息菜单 */
    void drawMessageMenu(int selectedIndex, const char* messages[], int count);

    /** 配网提示 */
    void drawWiFiSetupPrompt(const char* apName, const char* ip);

    /** 连接状态 */
    void drawConnectingScreen(const char* status);

    /** 背光亮度 (0-255) */
    void setBrightness(uint8_t brightness);

    void clear();

    TFT_eSPI& getTFT();

private:
    TFT_eSPI _tft;
    uint8_t _brightness = 255;

    void _drawStatusBar(int rssi, bool charging, bool muted);
    void _drawBottomBar();
    const char* _getWeatherIcon(const char* iconCode);
    void _drawWeatherCity(int y, const char* city, float temp, const char* iconCode);
};

#endif
