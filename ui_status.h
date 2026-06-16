/*
 * ui_status.h — 双人状态显示 + 全屏状态选择页
 * 触摸交互: 摸一下切到下一个, 3秒不动自动确认
 */

#ifndef UI_STATUS_H
#define UI_STATUS_H

#include "config.h"
#include "display_screen.h"

enum UIPage {
    PAGE_MAIN = 0,
    PAGE_MOOD_PICKER = 1,
    PAGE_IDENTITY = 2
};

class UIStatus {
public:
    static void begin();
    static void drawStatusBar();

    // 状态选择页 (触摸驱动)
    static void showMoodPicker();
    static void hideMoodPicker();
    static bool isPickerVisible();
    static void pickerNext();           // 摸一下切到下一个
    static bool pickerShouldConfirm();  // 3秒未触摸 → true
    static void pickerConfirm();        // 确认当前选择

    // 串口兼容
    static int hitPicker(uint16_t tx, uint16_t ty);

    // 状态管理
    static void setOwnMood(uint8_t mood);
    static void setPartnerMood(uint8_t mood);
    static void setPartnerOnline(bool online);
    static uint8_t ownMood();
    static uint8_t partnerMood();
    static bool partnerOnline();
    static void publishOwnMood();

private:
    static void _drawPicker();

    static uint8_t _ownMood;
    static uint8_t _partnerMood;
    static bool    _partnerOnline;
    static bool    _pickerVisible;
    static int     _pickerIdx;          // 当前高亮选项
    static unsigned long _pickerLastTap;// 上次触摸时间
};

extern UIPage currentPage;
void drawMainScreen();  // 在 .ino 中定义

#endif
