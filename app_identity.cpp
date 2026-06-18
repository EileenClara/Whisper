/*
 * app_identity.cpp — 设备身份实现
 */

#include "app_identity.h"
#include "storage_prefs.h"
#include "display_screen.h"

DeviceID AppIdentity::_id = ID_NONE;

void AppIdentity::begin() {
    uint8_t v = StoragePrefs::getIdentity();
    if (v == 1) _id = ID_GUGU;
    else if (v == 2) _id = ID_VV;
    else _id = ID_NONE;
}

bool AppIdentity::isSet() { return _id != ID_NONE; }

DeviceID AppIdentity::get() { return _id; }

const char* AppIdentity::name() {
    return (_id == ID_GUGU) ? "yg" : "vv";
}

const char* AppIdentity::nameCN() {
    return (_id == ID_GUGU) ? "yg" : "vv";
}

const char* AppIdentity::partnerName() {
    return (_id == ID_GUGU) ? "vv" : "yg";
}

const char* AppIdentity::partnerNameCN() {
    return (_id == ID_GUGU) ? "vv" : "yg";
}

void AppIdentity::set(DeviceID id) {
    _id = id;
    StoragePrefs::setIdentity(id == ID_GUGU ? 1 : 2);
}

// ===== 首次开机选择界面 =====
void AppIdentity::drawPicker() {
    TFT_eSPI& tft = DisplayScreen::tft();

    tft.fillScreen(TFT_BLACK);

    // 用默认字体 (ASCII)
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(30, 30);
    tft.print("Who are you?");

    // yougo 按钮
    tft.drawRoundRect(60, 80, 120, 50, 10, TFT_GREEN);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(3);
    tft.setCursor(55, 92);
    tft.print("yg");

    // vv 按钮
    tft.drawRoundRect(60, 150, 120, 50, 10, 0xF81F);
    tft.setTextColor(0xF81F, TFT_BLACK);
    tft.setCursor(100, 162);
    tft.print("VV");

    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(25, 220);
    tft.print("Serial: 1=yougo  2=vv");
}

DeviceID AppIdentity::hitPicker(uint16_t tx, uint16_t ty) {
    // 果果按钮区域
    if (tx >= 60 && tx <= 180 && ty >= 80 && ty <= 135) {
        return ID_GUGU;
    }
    // vv 按钮区域
    if (tx >= 60 && tx <= 180 && ty >= 150 && ty <= 205) {
        return ID_VV;
    }
    return ID_NONE;
}
