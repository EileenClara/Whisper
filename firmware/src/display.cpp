/**
 * @file display.cpp
 * @brief Whisper ST7789 172×320 屏幕驱动 — 实现
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

    Serial.println("[Display] ✅ ST7789 172x320 就绪");
    return true;
}

// ============================================================
// 主屏幕
// ============================================================

void Display::drawHomeScreen(
    const char* localTime, const char* localDate, const char* localCity,
    float localTemp, const char* localIcon,
    const char* partnerCity, float partnerTemp, const char* partnerIcon,
    const char* partnerStatus, const char* lastMsg, const char* lastMsgTime,
    int rssi, bool charging, bool muted)
{
    _tft.fillScreen(TFT_BLACK);

    // ---- 顶部状态栏 ----
    _drawStatusBar(rssi, charging, muted);

    // ---- 单行大字时钟 ----
    _tft.setTextDatum(MC_DATUM);
    if (localTime && strlen(localTime) > 0) {
        _tft.setTextColor(TFT_WHITE, TFT_BLACK);
        _tft.drawString(localTime, TFT_WIDTH / 2, 40, 7);  // 大字时间
    }
    if (localDate && strlen(localDate) > 0) {
        _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        _tft.drawString(localDate, TFT_WIDTH / 2, 78, 2);
    }
    _tft.setTextDatum(TL_DATUM);

    // ---- 双城天气（同行显示） ----
    int y = 100;
    _tft.drawLine(10, y, TFT_WIDTH - 10, y, TFT_DARKGREY);
    y += 6;

    // 左侧：本地城市天气
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString(localCity, 12, y, 2);
    y += 22;

    const char* lIcon = _getWeatherIcon(localIcon);
    _tft.drawString(lIcon ? lIcon : "?", 14, y, 3);
    char tempStr[12];
    snprintf(tempStr, sizeof(tempStr), "%.0f°", localTemp);
    _tft.drawString(tempStr, 50, y + 4, 2);

    // 右侧：对方城市天气
    int rx = TFT_WIDTH / 2 + 6;
    y = 106;
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString(partnerCity, rx, y, 2);
    y += 22;

    const char* pIcon = _getWeatherIcon(partnerIcon);
    _tft.drawString(pIcon ? pIcon : "?", rx + 2, y, 3);
    snprintf(tempStr, sizeof(tempStr), "%.0f°", partnerTemp);
    _tft.drawString(tempStr, rx + 38, y + 4, 2);

    // 分隔线
    y = 152;
    _tft.drawLine(10, y, TFT_WIDTH - 10, y, TFT_DARKGREY);
    y += 6;

    // ---- 对方状态 ----
    _tft.setTextColor(TFT_GOLD, TFT_BLACK);
    if (partnerStatus && strlen(partnerStatus) > 0) {
        _tft.drawString(partnerStatus, 8, y, 2);
    } else {
        _tft.drawString("💕", 8, y, 2);
    }
    y += 26;

    // 分隔线
    _tft.drawLine(10, y, TFT_WIDTH - 10, y, TFT_DARKGREY);
    y += 6;

    // ---- 最新消息 ----
    _tft.setTextColor(TFT_CYAN, TFT_BLACK);
    _tft.drawString("消息:", 8, y, 2);

    if (lastMsg && strlen(lastMsg) > 0) {
        _tft.setTextColor(TFT_WHITE, TFT_BLACK);
        char msgBuf[20];
        int len = strlen(lastMsg);
        if (len > 16) {
            memcpy(msgBuf, lastMsg, 16);
            msgBuf[16] = '.';
            msgBuf[17] = '.';
            msgBuf[18] = '\0';
        } else {
            strcpy(msgBuf, lastMsg);
        }
        _tft.drawString(msgBuf, 56, y, 2);
    }
    y += 20;

    if (lastMsgTime && strlen(lastMsgTime) > 0) {
        _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        _tft.drawString(lastMsgTime, 56, y, 1);
    }

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

void Display::drawMessageMenu(int selectedIndex, const char* messages[], int count) {
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_CYAN, TFT_BLACK);
    _tft.drawString("快捷消息 (敲击切换)", 6, 8, 2);
    _tft.drawLine(6, 30, TFT_WIDTH - 6, 30, TFT_DARKGREY);

    int y = 42;
    for (int i = 0; i < count; i++) {
        if (i == selectedIndex) {
            _tft.fillRoundRect(6, y - 2, TFT_WIDTH - 12, 30, 4, TFT_BLUE);
            _tft.setTextColor(TFT_WHITE, TFT_BLUE);
            _tft.drawString("> ", 12, y + 4, 2);
            _tft.drawString(messages[i], 36, y + 4, 2);
        } else {
            _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            _tft.drawString("  ", 12, y + 4, 2);
            _tft.drawString(messages[i], 36, y + 4, 2);
        }
        y += 36;
    }
    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString("静置3秒 = 发送", 6, TFT_HEIGHT - 20, 1);
    setBrightness(255);
}

void Display::drawStatusMenu(int selectedIndex, const char* emojis[], const char* labels[], int count) {
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    _tft.drawString("设置状态 (敲击切换)", 6, 8, 2);
    _tft.drawLine(6, 30, TFT_WIDTH - 6, 30, TFT_DARKGREY);

    int y = 42;
    for (int i = 0; i < count; i++) {
        if (i == selectedIndex) {
            _tft.fillRoundRect(6, y - 2, TFT_WIDTH - 12, 30, 4, TFT_MAGENTA);
            _tft.setTextColor(TFT_WHITE, TFT_MAGENTA);
        } else {
            _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        }
        char buf[40];
        snprintf(buf, sizeof(buf), "%s  %s", emojis[i], labels[i]);
        _tft.drawString(i == selectedIndex ? "> " : "  ", 12, y + 4, 2);
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
    _tft.drawString("请手机连接热点:", 8, 50, 2);
    _tft.setTextColor(TFT_CYAN, TFT_BLACK);
    _tft.drawString(apName, 8, 78, 3);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString("浏览器打开:", 8, 120, 2);
    _tft.setTextColor(TFT_GREEN, TFT_BLACK);
    _tft.drawString(ip, 8, 146, 3);
    _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString("在页面中输入WiFi密码", 8, 180, 2);
}

void Display::drawConnectingScreen(const char* status) {
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString("Whisper", TFT_WIDTH / 2, TFT_HEIGHT / 2 - 30, 4);
    _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString(status, TFT_WIDTH / 2, TFT_HEIGHT / 2 + 20, 2);
    _tft.setTextDatum(TL_DATUM);
}

// ============================================================
// 辅助
// ============================================================

void Display::_drawStatusBar(int rssi, bool charging, bool muted) {
    // 静音图标（左上）
    _tft.setTextColor(muted ? TFT_RED : TFT_GREEN, TFT_BLACK);
    _tft.drawString(muted ? "🔇" : "🔊", 4, 2, 1);

    // 电量/充电
    _tft.setTextColor(charging ? TFT_GREEN : TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString(charging ? "⚡" : "🔋", TFT_WIDTH - 52, 2, 1);

    // WiFi信号（右上）
    int bars = 0;
    if (rssi > -50) bars = 4;
    else if (rssi > -65) bars = 3;
    else if (rssi > -75) bars = 2;
    else if (rssi > -85) bars = 1;
    uint16_t barColor = (bars >= 3) ? TFT_GREEN : (bars >= 1) ? TFT_YELLOW : TFT_RED;
    for (int i = 0; i < 4; i++) {
        int h = 3 + i * 3;
        uint16_t c = (i < bars) ? barColor : TFT_DARKGREY;
        _tft.fillRect(TFT_WIDTH - 28 + i * 5, 8 + (10 - h), 3, h, c);
    }
}

void Display::_drawBottomBar() {
    _tft.drawLine(6, TFT_HEIGHT - 22, TFT_WIDTH - 6, TFT_HEIGHT - 22, TFT_DARKGREY);
    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString("摇一摇 ❤️", 4, TFT_HEIGHT - 18, 1);
    _tft.drawString("敲3=消息 敲4=状态", TFT_WIDTH - 110, TFT_HEIGHT - 18, 1);
}

const char* Display::_getWeatherIcon(const char* iconCode) {
    if (!iconCode) return "?";
    if (strcmp(iconCode, "01d") == 0) return "☀️";
    if (strcmp(iconCode, "01n") == 0) return "🌙";
    if (strcmp(iconCode, "02d") == 0) return "⛅";
    if (strcmp(iconCode, "02n") == 0 || strcmp(iconCode, "03d") == 0 || strcmp(iconCode, "03n") == 0 ||
        strcmp(iconCode, "04d") == 0 || strcmp(iconCode, "04n") == 0) return "☁️";
    if (strcmp(iconCode, "09d") == 0 || strcmp(iconCode, "09n") == 0 ||
        strcmp(iconCode, "10d") == 0 || strcmp(iconCode, "10n") == 0) return "🌧️";
    if (strcmp(iconCode, "11d") == 0 || strcmp(iconCode, "11n") == 0) return "⛈️";
    if (strcmp(iconCode, "13d") == 0 || strcmp(iconCode, "13n") == 0) return "🌨️";
    if (strcmp(iconCode, "50d") == 0 || strcmp(iconCode, "50n") == 0) return "🌫️";
    return "?";
}

void Display::setBrightness(uint8_t b) {
    _brightness = b;
    ledcWrite(0, b);
}

void Display::clear() { _tft.fillScreen(TFT_BLACK); }

TFT_eSPI& Display::getTFT() { return _tft; }
