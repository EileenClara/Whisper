/**
 * @file display.cpp
 * @brief Whisper TFT 屏幕驱动 — 实现
 *
 * 注意：TFT_eSPI 需要预先配置 User_Setup.h 或在 platformio.ini 中通过
 * build_flags 设置引脚。本项目使用 config.h 中的引脚定义，
 * 需要在 TFT_eSPI 库中做对应设置。
 *
 * 简化方案：在 platformio.ini build_flags 中传入 TFT 参数
 * 或修改 .pio/libdeps/.../TFT_eSPI/User_Setup_Select.h 使用自定义设置
 */

#include "display.h"
#include "config.h"

Display::Display() {}

Display::~Display() {}

bool Display::begin() {
    // TFT_eSPI 初始化（引脚在 User_Setup.h 或 build_flags 中配置）
    _tft.init();
    _tft.setRotation(TFT_ROTATION);
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextWrap(false);

    // 背光PWM
    ledcSetup(0, 5000, 8);  // 通道0, 5kHz, 8-bit
    ledcAttachPin(TFT_BL, 0);
    setBrightness(_brightness);

    // 显示启动画面
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString("Whisper", TFT_WIDTH / 2, TFT_HEIGHT / 2 - 20, 4);
    _tft.setTextDatum(TL_DATUM);
    _tft.drawString("启动中...", 10, TFT_HEIGHT / 2 + 20, 2);

    Serial.println("[Display] ✅ ST7789 172x320 就绪");
    return true;
}

// ============================================================
// 主屏幕绘制
// ============================================================

void Display::drawHomeScreen(
    const char* localTime, const char* localDate, const char* localCity,
    float localTemp, const char* localIcon,
    const char* partnerTime, const char* partnerDate, const char* partnerCity,
    float partnerTemp, const char* partnerIcon,
    const char* partnerStatus, const char* lastMsg, const char* lastMsgTime,
    int rssi)
{
    _tft.fillScreen(TFT_BLACK);

    // 顶部状态栏
    _drawStatusBar(rssi, false);

    int y = 24;

    // ---- 本地时间+天气 ----
    _drawWeatherRow(y, localTime, localDate, localCity, localTemp, localIcon);
    y += 48;

    // 分隔线
    _tft.drawLine(16, y, TFT_WIDTH - 16, y, TFT_DARKGREY);
    y += 4;

    // ---- 对方时间+天气 ----
    _drawWeatherRow(y, partnerTime, partnerDate, partnerCity, partnerTemp, partnerIcon);
    y += 52;

    // 分隔线
    _tft.drawLine(16, y, TFT_WIDTH - 16, y, TFT_DARKGREY);
    y += 6;

    // ---- 对方状态 ----
    _tft.setTextColor(TFT_GOLD, TFT_BLACK);
    _tft.drawString("状态:", 8, y, 2);
    if (partnerStatus && strlen(partnerStatus) > 0) {
        _tft.setTextColor(TFT_WHITE, TFT_BLACK);
        _tft.drawString(partnerStatus, 56, y, 2);
    }
    y += 28;

    // 状态卡片背景
    _tft.fillRoundRect(6, y - 4, TFT_WIDTH - 12, 24, 4, TFT_DARKGREY);

    // ---- 最新消息 ----
    y += 4;
    _tft.setTextColor(TFT_CYAN, TFT_BLACK);
    _tft.drawString("消息:", 8, y, 2);

    if (lastMsg && strlen(lastMsg) > 0) {
        _tft.setTextColor(TFT_WHITE, TFT_BLACK);

        // 截断过长消息
        char msgBuf[32];
        int len = strlen(lastMsg);
        if (len > 14) {
            // UTF-8 安全截断：简单处理，取前14个字节
            memcpy(msgBuf, lastMsg, 14);
            msgBuf[14] = '.';
            msgBuf[15] = '.';
            msgBuf[16] = '\0';
        } else {
            strcpy(msgBuf, lastMsg);
        }
        _tft.drawString(msgBuf, 56, y, 2);
    }
    y += 22;

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

    _tft.setTextColor(TFT_RED, TFT_BLACK);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString("摇一摇发送 ❤️", TFT_WIDTH / 2, TFT_HEIGHT / 2 - 20, 4);

    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString("(3秒内摇动确认)", TFT_WIDTH / 2, TFT_HEIGHT / 2 + 20, 2);
    _tft.setTextDatum(TL_DATUM);

    setBrightness(255);  // 全亮
}

void Display::drawHeartAnimation() {
    // 简单的❤️动画：从小到大显示爱心
    _tft.fillScreen(TFT_BLACK);

    int sizes[] = {2, 3, 4, 5, 6, 5, 4, 3, 2};
    for (int i = 0; i < 9; i++) {
        _tft.fillScreen(TFT_BLACK);
        _tft.setTextColor(TFT_RED, TFT_BLACK);
        _tft.setTextDatum(MC_DATUM);
        _tft.drawString("❤️", TFT_WIDTH / 2, TFT_HEIGHT / 2, sizes[i]);
        _tft.setTextDatum(TL_DATUM);
        delay(150);
    }

    // 显示2秒
    delay(1500);
}

void Display::drawMessageMenu(int selectedIndex, const char* messages[], int count) {
    _tft.fillScreen(TFT_BLACK);

    _tft.setTextColor(TFT_CYAN, TFT_BLACK);
    _tft.drawString("快捷消息 (摇动切换)", 8, 10, 2);

    _tft.drawLine(8, 34, TFT_WIDTH - 8, 34, TFT_DARKGREY);

    int y = 50;
    for (int i = 0; i < count; i++) {
        if (i == selectedIndex) {
            // 高亮选中项
            _tft.fillRoundRect(8, y - 2, TFT_WIDTH - 16, 32, 4, TFT_BLUE);
            _tft.setTextColor(TFT_WHITE, TFT_BLUE);
            _tft.drawString("> ", 14, y + 4, 2);
            _tft.drawString(messages[i], 40, y + 4, 2);
        } else {
            _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            _tft.drawString("  ", 14, y + 4, 2);
            _tft.drawString(messages[i], 40, y + 4, 2);
        }
        y += 38;
    }

    // 底部提示
    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString("静置3秒=发送", 8, TFT_HEIGHT - 20, 1);

    setBrightness(255);
}

void Display::drawWiFiSetupPrompt(const char* apName) {
    _tft.fillScreen(TFT_BLACK);
    _tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    _tft.drawString("WiFi 未配置", 10, 20, 2);

    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString("请用手机连接热点:", 10, 60, 2);
    _tft.drawString(apName, 10, 90, 2);

    _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    _tft.drawString("IP: 192.168.4.1", 10, 130, 2);
    _tft.drawString("浏览器打开配置页面", 10, 160, 2);
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
// 辅助方法
// ============================================================

void Display::_drawWeatherRow(int y, const char* timeStr, const char* dateStr,
                               const char* city, float temp, const char* iconCode)
{
    // 左侧：时间
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    if (timeStr && strlen(timeStr) > 0) {
        _tft.drawString(timeStr, 6, y, 4);  // 大号时间
    }

    // 日期
    if (dateStr && strlen(dateStr) > 0) {
        _tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        _tft.drawString(dateStr, 6, y + 28, 1);
    }

    // 城市
    if (city && strlen(city) > 0) {
        _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        _tft.drawString(city, 6, y + 40, 1);
    }

    // 右侧：天气图标 + 温度
    const char* icon = _getWeatherIcon(iconCode);
    if (icon) {
        _tft.setTextColor(TFT_WHITE, TFT_BLACK);
        _tft.drawString(icon, TFT_WIDTH - 60, y + 4, 2);
    }

    // 温度
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.0f°", temp);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.drawString(tempStr, TFT_WIDTH - 60, y + 28, 2);
}

void Display::_drawStatusBar(int rssi, bool charging) {
    // WiFi 信号图标
    int bars = 0;
    if (rssi > -50) bars = 4;
    else if (rssi > -65) bars = 3;
    else if (rssi > -75) bars = 2;
    else if (rssi > -85) bars = 1;

    uint16_t barColor = (bars >= 3) ? TFT_GREEN :
                        (bars >= 1) ? TFT_YELLOW : TFT_RED;

    for (int i = 0; i < 4; i++) {
        int h = 3 + i * 3;
        uint16_t c = (i < bars) ? barColor : TFT_DARKGREY;
        _tft.fillRect(TFT_WIDTH - 30 + i * 5, 10 + (12 - h), 3, h, c);
    }
}

void Display::_drawBottomBar() {
    // 底部操作提示线
    _tft.drawLine(8, TFT_HEIGHT - 22, TFT_WIDTH - 8, TFT_HEIGHT - 22, TFT_DARKGREY);

    _tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    _tft.drawString("摇一摇 ❤️", 8, TFT_HEIGHT - 18, 1);
    _tft.drawString("双摇 消息", TFT_WIDTH - 72, TFT_HEIGHT - 18, 1);
}

const char* Display::_getWeatherIcon(const char* iconCode) {
    if (!iconCode) return "❓";

    // 提取前缀匹配（如 "01d" → 匹配 "01d"）
    for (auto& wi : WEATHER_ICONS) {
        if (strcmp(wi.code, iconCode) == 0) {
            return wi.icon;
        }
    }

    // Fallback：根据是否是白天
    if (strstr(iconCode, "d")) return "☀️";
    if (strstr(iconCode, "n")) return "🌙";
    return "❓";
}

// ---- 亮度控制 ----
void Display::setBrightness(uint8_t brightness) {
    _brightness = brightness;
    ledcWrite(0, brightness);
}

void Display::clear() {
    _tft.fillScreen(TFT_BLACK);
}

TFT_eSPI& Display::getTFT() {
    return _tft;
}

void Display::_drawUTF8String(const char* str, int x, int y) {
    // TFT_eSPI 的 drawString 对 ASCII 和部分 Latin1 支持良好
    // 对于中文和 Emoji，需要额外的字体支持
    // 目前简化处理：直接调用 drawString
    _tft.drawString(str, x, y);
}
