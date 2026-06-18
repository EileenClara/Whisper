/*
 * ui_status.cpp — 状态实现
 * 状态选择: 摸一下切到下一个, 3秒不摸自动确认
 */

#include "ui_status.h"
#include "display_screen.h"
#include "network_mqtt.h"
#include "storage_prefs.h"
#include "app_identity.h"

uint8_t UIStatus::_ownMood      = MOOD_FREE;
uint8_t UIStatus::_partnerMood  = MOOD_FREE;
bool    UIStatus::_partnerOnline = false;
bool    UIStatus::_pickerVisible = false;
int     UIStatus::_pickerIdx     = 0;
unsigned long UIStatus::_pickerLastTap = 0;
UIPage  currentPage = PAGE_MAIN;

void UIStatus::begin() {
    _ownMood = StoragePrefs::getLastMood();
    _pickerVisible = false;
}

// ===== 主屏状态栏 (y=160~180) =====
void UIStatus::drawStatusBar() {
    TFT_eSPI& tft = DisplayScreen::tft();
    const char* moodEn[] = {"Free","Play","Busy","MissU","Eat","Study","Sleep","Soccer"};

    // yougo (左半边)
    uint16_t mc = MOOD_COLORS[_ownMood];
    tft.fillRect(0, 160, 120, 22, TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(mc, TFT_BLACK);
    tft.setCursor(2, 162);
    tft.print(AppIdentity::name());
    tft.print(":");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.print(moodEn[_ownMood]);

    // vv (右半边)
    mc = _partnerOnline ? MOOD_COLORS[_partnerMood] : 0x7BEF;
    tft.fillRect(120, 160, 120, 22, TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(mc, TFT_BLACK);
    tft.setCursor(124, 162);
    tft.print(AppIdentity::partnerName());
    tft.print(":");
    if (_partnerOnline) {
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.print(moodEn[_partnerMood]);
    } else {
        tft.setTextColor(0x7BEF, TFT_BLACK);
        tft.print("Off");
    }
}

// ===== 全屏状态选择页 (摸一下切一个, 3秒不动自动确认) =====
void UIStatus::showMoodPicker() {
    _pickerVisible = true;
    _pickerIdx = _ownMood;  // 从当前状态开始
    _pickerLastTap = millis();
    currentPage = PAGE_MOOD_PICKER;

    _drawPicker();
}

void UIStatus::hideMoodPicker() {
    _pickerVisible = false;
    currentPage = PAGE_MAIN;
}

bool UIStatus::isPickerVisible() { return _pickerVisible; }

// 摸一下 → 切到下一个
void UIStatus::pickerNext() {
    _pickerIdx = (_pickerIdx + 1) % MOOD_COUNT;
    _pickerLastTap = millis();
    _drawPicker();
}

// 检查是否 3 秒未触摸 → 自动确认
bool UIStatus::pickerShouldConfirm() {
    return (millis() - _pickerLastTap > 3000);
}

void UIStatus::pickerConfirm() {
    _ownMood = _pickerIdx;
    StoragePrefs::setLastMood(_ownMood);
    NetworkMQTT::publishMood(_ownMood);
    hideMoodPicker();
    drawMainScreen();
}

void UIStatus::_drawPicker() {
    TFT_eSPI& tft = DisplayScreen::tft();
    tft.fillScreen(TFT_BLACK);

    const char* moodEn[] = {"Free","Play","Busy","MissU","Eat","Study","Sleep","Soccer"};

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(30, 5);
    tft.print("Tap to change");
    tft.setTextSize(1);
    tft.setCursor(10, 28);
    tft.print("3s no tap = confirm");

    // 8 个格子: 2列 x 4行
    for (int i = 0; i < MOOD_COUNT; i++) {
        int col = i % 2;
        int row = i / 2;
        int px = col * 120 + 5;
        int py = row * 48 + 45;

        uint16_t mc = MOOD_COLORS[i];
        bool selected = (i == _pickerIdx);

        uint16_t bg = selected ? mc : TFT_BLACK;
        tft.fillRoundRect(px, py, 110, 42, 8, bg);
        if (!selected) {
            tft.drawRoundRect(px, py, 110, 42, 8, mc);
        }

        tft.setTextColor(selected ? TFT_BLACK : mc, bg);
        tft.setTextSize(2);
        tft.setCursor(px + 10, py + 10);
        tft.print(moodEn[i]);

        // 当前高亮标记
        if (selected) {
            tft.setTextColor(TFT_BLACK, mc);
            tft.setCursor(px + 85, py + 2);
            tft.print(">");
        }
    }
}

// ===== 串口兼容 =====
int UIStatus::hitPicker(uint16_t tx, uint16_t ty) { return -1; }

void UIStatus::setOwnMood(uint8_t mood) {
    _ownMood = constrain(mood, 0, MOOD_COUNT - 1);
    StoragePrefs::setLastMood(_ownMood);
    DisplayScreen::markDirty(ZONE_STATUS);
}

void UIStatus::setPartnerMood(uint8_t mood) {
    _partnerMood = constrain(mood, 0, MOOD_COUNT - 1);
    DisplayScreen::markDirty(ZONE_STATUS);
}

void UIStatus::setPartnerOnline(bool online) {
    _partnerOnline = online;
    DisplayScreen::markDirty(ZONE_STATUS);
}

void UIStatus::publishOwnMood() {
    NetworkMQTT::publishMood(_ownMood);
}

uint8_t UIStatus::ownMood()     { return _ownMood; }
uint8_t UIStatus::partnerMood() { return _partnerMood; }
bool UIStatus::partnerOnline()  { return _partnerOnline; }
