/**
 * @file display.h
 * @brief Whisper TFT ST7789 172×320 — 主屏 + 状态菜单 + 动画
 *
 * 主屏布局:
 *  ┌──────────────────────────┐
 *  │  🔇          📶░░░  🔋   │  状态栏
 *  │                          │
 *  │        14:30             │  大字时钟
 *  │     6月10日 周三          │
 *  │                          │
 *  │  北京 🌤 32°  新加坡 🌤 28°│  双城天气
 *  │                          │
 *  │  ┌─────────┐ ┌─────────┐ │
 *  │  │💕 想你  │ │📚 学习中 │ │  我的状态 | 对方状态
 *  │  └─────────┘ └─────────┘ │
 *  │                          │
 *  │        💕                 │  上次心跳
 *  │       刚刚                │
 *  │──────────────────────────│
 *  │  摇一摇 ❤️    敲四下 状态  │  操作提示
 *  └──────────────────────────┘
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <TFT_eSPI.h>

class Display {
public:
    Display(); ~Display();
    bool begin();

    /** 主屏幕：双状态并排 + 天气 + 时钟 */
    void drawHomeScreen(
        const char* localTime,      // "14:30"
        const char* localDate,      // "6月10日 周三"
        const char* localCity,      // "北京"
        float localTemp,            // 32
        const char* localIcon,      // "01d"
        const char* partnerCity,    // "新加坡"
        float partnerTemp,          // 28
        const char* partnerIcon,    // "01d"
        const char* myStatusEmoji,  // "💕"
        const char* myStatusLabel,  // "想你"
        const char* partnerStatus,  // "📚 学习中"
        const char* lastHeartbeat,  // "刚刚" / "3分钟前" / ""
        int rssi, bool charging, bool muted
    );

    void drawHeartbeatPrompt();
    void drawHeartAnimation();

    /** 状态选择菜单 */
    void drawStatusMenu(int selectedIndex, const char* emojis[], const char* labels[], int count);

    void drawWiFiSetupPrompt(const char* apName, const char* ip);
    void drawConnectingScreen(const char* status);
    void setBrightness(uint8_t b);
    void clear();
    TFT_eSPI& getTFT();

private:
    TFT_eSPI _tft;
    uint8_t _brightness = 255;
    void _drawStatusBar(int rssi, bool charging, bool muted);
    void _drawBottomBar();
    const char* _getWeatherIcon(const char* iconCode);
};

#endif
