/*
 * ui_clock.cpp — 时钟
 * HH:MM FONT6 48px, :SS 后面小一点, 日期 16px
 */

#include "ui_clock.h"
#include "network_ntp.h"

int UIClock::_lastHour = -1;
int UIClock::_lastMin  = -1;
int UIClock::_lastSec  = -1;
int UIClock::_lastDay  = -1;

void UIClock::begin() {
    _lastHour = -1; _lastMin = -1; _lastSec = -1; _lastDay = -1;
    draw();
}

void UIClock::update() {
    if (!NetworkNTP::isSynced()) return;
    int h = NetworkNTP::hour(), m = NetworkNTP::minute();
    int s = NetworkNTP::second(), d = NetworkNTP::day();
    if (h != _lastHour || m != _lastMin || s != _lastSec) {
        _drawTime(h, m, s);
        _lastHour = h; _lastMin = m; _lastSec = s;
    }
    if (d != _lastDay) {
        _drawDate();
        _lastDay = d;
    }
}

void UIClock::draw() {
    _drawTime(NetworkNTP::hour(), NetworkNTP::minute(), NetworkNTP::second());
    _drawDate();
}

void UIClock::_drawTime(int h, int m, int s) {
    TFT_eSPI& tft = DisplayScreen::tft();

    // 清除
    tft.fillRect(0, 80, 240, 55, TFT_BLACK);

    // HH:MM — FONT6 48px
    tft.setTextFont(6);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    char buf[6];
    snprintf(buf, 6, "%02d:%02d", h, m);
    int x = (240 - 120) / 2;
    tft.setCursor(x, 84);
    tft.print(buf);

    // :SS — 跟在后面, 小一号 (FONT2 16px, 但用 setTextSize(2) 变 32px)
    // 实际: FONT2 + size 2 = 32px, 跟 48px 差太多。改用 FONT4 (26px)
    tft.setTextFont(4);   // 26px
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF, TFT_BLACK);
    tft.setCursor(x + 120, 96);
    tft.printf(":%02d", s);

    tft.setTextFont(1);  // 恢复
}

void UIClock::_drawDate() {
    TFT_eSPI& tft = DisplayScreen::tft();
    tft.fillRect(0, 138, 240, 18, TFT_BLACK);
    tft.setTextSize(2);  // 16px
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(4, 140);

    const char* ms[] = {"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    const char* ws[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    int wd = NetworkNTP::weekday() - 1; if (wd < 0) wd = 6;
    tft.printf("%s %d  %s", ms[NetworkNTP::month()], NetworkNTP::day(), ws[wd]);
}
