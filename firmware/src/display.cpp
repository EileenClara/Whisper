/**
 * @file display.cpp
 * @brief Whisper ST7789 172×320 — 实现
 */

#include "display.h"
#include "config.h"

Display::Display() {}
Display::~Display() {}

bool Display::begin() {
    _tft.init();
    _tft.setRotation(TFT_ROTATION);
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextWrap(false);
    ledcSetup(0, 5000, 8);
    ledcAttachPin(TFT_BL, 0);
    setBrightness(_brightness);
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString("Whisper", TFT_WIDTH / 2, TFT_HEIGHT / 2 - 20, 4);
    _tft.setTextDatum(TL_DATUM);
    _tft.drawString("启动中...", 10, TFT_HEIGHT / 2 + 20, 2);
    Serial.println("[Display] ✅ 就绪");
    return true;
}

// ============================================================
// 主屏幕
// ============================================================

void Display::drawHomeScreen(
    const char* localTime, const char* localDate,
    const char* localCity, float localTemp, const char* localIcon,
    const char* partnerCity, float partnerTemp, const char* partnerIcon,
    const char* myStatusEmoji, const char* myStatusLabel,
    const char* partnerStatus, const char* lastHeartbeat,
    int rssi, bool charging, bool muted)
{
    _tft.fillScreen(TFT_BLACK);

    // ---- 顶部状态栏 ----
    _drawStatusBar(rssi, charging, muted);

    // ---- 大字时间 ----
    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString(localTime ? localTime : "--:--", TFT_WIDTH / 2, 30, 7);
    _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString(localDate ? localDate : "", TFT_WIDTH / 2, 68, 2);
    _tft.setTextDatum(TL_DATUM);

    // ---- 双城天气（紧凑一行）----
    int y = 88;
    _tft.drawLine(10, y, TFT_WIDTH - 10, y, TFT_DARKGREY);
    y += 6;

    // 左侧：本地
    const char* lIcon = _getWeatherIcon(localIcon);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString(localCity, 12, y, 1);
    _tft.drawString(lIcon ? lIcon : "?", 12, y + 14, 3);
    char t[12];
    snprintf(t, sizeof(t), "%.0f°", localTemp);
    _tft.drawString(t, 46, y + 18, 2);

    // 右侧：对方
    int rx = TFT_WIDTH / 2 + 6;
    _tft.drawString(partnerCity, rx, y, 1);
    const char* pIcon = _getWeatherIcon(partnerIcon);
    _tft.drawString(pIcon ? pIcon : "?", rx, y + 14, 3);
    snprintf(t, sizeof(t), "%.0f°", partnerTemp);
    _tft.drawString(t, rx + 34, y + 18, 2);

    // ---- 双状态并排 ----
    y = 140;
    _tft.drawLine(10, y, TFT_WIDTH - 10, y, TFT_DARKGREY);
    y += 8;
    _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString("我", TFT_WIDTH / 4 - 4, y, 1);
    _tft.drawString("TA", TFT_WIDTH * 3 / 4 - 8, y, 1);
    y += 14;

    // 我的状态卡片
    int cardW = (TFT_WIDTH - 30) / 2;
    int cardH = 48;
    _tft.fillRoundRect(6, y, cardW, cardH, 6, TFT_DARKGREEN);
    _tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    _tft.drawString(myStatusEmoji ? myStatusEmoji : "?", 14, y + 4, 3);
    _tft.drawString(myStatusLabel ? myStatusLabel : "", 50, y + 10, 2);

    // 对方状态卡片
    int cx = 6 + cardW + 6;
    _tft.fillRoundRect(cx, y, cardW, cardH, 6, TFT_NAVY);
    _tft.setTextColor(TFT_WHITE, TFT_NAVY);
    if (partnerStatus && strlen(partnerStatus) > 0) {
        _tft.drawString(partnerStatus, cx + 8, y + 10, 2);
    } else {
        _tft.drawString("?", cx + 8, y + 10, 2);
    }

    // ---- 心跳区 ----
    y += cardH + 16;
    _tft.drawLine(10, y, TFT_WIDTH - 10, y, TFT_DARKGREY);
    y += 8;
    _tft.setTextColor(TFT_RED, TFT_BLACK);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString("❤️", TFT_WIDTH / 2, y, 4);
    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    if (lastHeartbeat && strlen(lastHeartbeat) > 0) {
        _tft.drawString(lastHeartbeat, TFT_WIDTH / 2, y + 30, 1);
    }
    _tft.setTextDatum(TL_DATUM);

    // ---- 底部操作提示 ----
    _drawBottomBar();
}

// ============================================================
// 特殊界面
// ============================================================

void Display::drawHeartbeatPrompt() {
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_RED, TFT_BLACK);
    _tft.drawString("摇一摇发送 ❤️", TFT_WIDTH / 2, TFT_HEIGHT / 2 - 20, 4);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString("(3秒内摇动确认)", TFT_WIDTH / 2, TFT_HEIGHT / 2 + 20, 2);
    _tft.setTextDatum(TL_DATUM);
    setBrightness(255);
}

void Display::drawHeartAnimation() {
    int sizes[] = {2, 3, 4, 5, 6, 5, 4, 3, 2};
    for (int i = 0; i < 9; i++) {
        _tft.fillScreen(TFT_BLACK);
        _tft.setTextColor(TFT_RED, TFT_BLACK);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString("❤️", TFT_WIDTH / 2, TFT_HEIGHT / 2, sizes[i]);
        _tft.setTextDatum(TL_DATUM);
        delay(150);
    }
    delay(1500);
}

void Display::drawStatusMenu(int selectedIndex, const char* emojis[], const char* labels[], int count) {
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    _tft.drawString("设置状态", 6, 8, 2);
    _tft.drawLine(6, 30, TFT_WIDTH - 6, 30, TFT_DARKGREY);
    int y = 42;
    for (int i = 0; i < count; i++) {
        if (i == selectedIndex) {
            _tft.fillRoundRect(6, y - 2, TFT_WIDTH - 12, 30, 4, TFT_MAGENTA);
            _tft.setTextColor(TFT_WHITE, TFT_MAGENTA);
            _tft.drawString("> ", 12, y + 4, 2);
        } else {
            _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            _tft.drawString("  ", 12, y + 4, 2);
        }
        char buf[40];
        snprintf(buf, sizeof(buf), "%s  %s", emojis[i], labels[i]);
        _tft.drawString(buf, 36, y + 4, 2);
        y += 36;
    }
    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString("静置3秒 = 确认", 6, TFT_HEIGHT - 20, 1);
    setBrightness(255);
}

void Display::drawWiFiSetupPrompt(const char* apName, const char* ip) {
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft.drawString("WiFi 未配置", 8, 16, 2);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString("手机连接热点:", 8, 50, 2);
    _tft.setTextColor(TFT_CYAN, TFT_BLACK);
    _tft.drawString(apName, 8, 78, 3);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString("浏览器打开:", 8, 120, 2);
    _tft.setTextColor(TFT_GREEN, TFT_BLACK);
    _tft.drawString(ip, 8, 146, 3);
    _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString("选择WiFi并输入密码", 8, 180, 2);
}

void Display::drawConnectingScreen(const char* status) {
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextDatum(MC_DATUM);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString("Whisper", TFT_WIDTH / 2, TFT_HEIGHT / 2 - 30, 4);
    _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString(status, TFT_WIDTH / 2, TFT_HEIGHT / 2 + 20, 2);
    _tft.setTextDatum(TL_DATUM);
}

// ---- 辅助 ----

void Display::_drawStatusBar(int rssi, bool charging, bool muted) {
    _tft.setTextColor(muted ? TFT_RED : TFT_GREEN, TFT_BLACK);
    _tft.drawString(muted ? "🔇" : "🔊", 4, 2, 1);
    _tft.setTextColor(charging ? TFT_GREEN : TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString(charging ? "⚡" : "🔋", TFT_WIDTH - 52, 2, 1);
    int bars = 0;
    if (rssi > -50) bars = 4;
    else if (rssi > -65) bars = 3;
    else if (rssi > -75) bars = 2;
    else if (rssi > -85) bars = 1;
    uint16_t bc = (bars >= 3) ? TFT_GREEN : (bars >= 1) ? TFT_YELLOW : TFT_RED;
    for (int i = 0; i < 4; i++) {
        int h = 3 + i * 3;
        _tft.fillRect(TFT_WIDTH - 28 + i * 5, 8 + (10 - h), 3, h, i < bars ? bc : TFT_DARKGREY);
    }
}

void Display::_drawBottomBar() {
    _tft.drawLine(6, TFT_HEIGHT - 22, TFT_WIDTH - 6, TFT_HEIGHT - 22, TFT_DARKGREY);
    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString("摇一摇 ❤️", 6, TFT_HEIGHT - 18, 1);
    _tft.drawString("敲四下 状态", TFT_WIDTH - 72, TFT_HEIGHT - 18, 1);
}

const char* Display::_getWeatherIcon(const char* code) {
    if (!code) return "?";
    if (strcmp(code, "01d") == 0) return "☀️";
    if (strcmp(code, "01n") == 0) return "🌙";
    if (strcmp(code, "02d") == 0) return "⛅";
    if (strcmp(code, "02n") == 0 || strcmp(code, "03d") == 0 || strcmp(code, "03n") == 0 ||
        strcmp(code, "04d") == 0 || strcmp(code, "04n") == 0) return "☁️";
    if (strcmp(code, "09d") == 0 || strcmp(code, "09n") == 0 ||
        strcmp(code, "10d") == 0 || strcmp(code, "10n") == 0) return "🌧️";
    if (strcmp(code, "11d") == 0 || strcmp(code, "11n") == 0) return "⛈️";
    if (strcmp(code, "13d") == 0 || strcmp(code, "13n") == 0) return "🌨️";
    if (strcmp(code, "50d") == 0 || strcmp(code, "50n") == 0) return "🌫️";
    return "?";
}

void Display::setBrightness(uint8_t b) { _brightness = b; ledcWrite(0, b); }
void Display::clear() { _tft.fillScreen(TFT_BLACK); }
TFT_eSPI& Display::getTFT() { return _tft; }
