/**
 * @file storage.cpp
 * @brief Whisper NVS 配置存储 — 实现
 */

#include "storage.h"
#include "config.h"

Storage::Storage() {}

Storage::~Storage() {
    end();
}

bool Storage::begin() {
    _opened = _prefs.begin(NVS_NAMESPACE, false);
    if (!_opened) {
        Serial.println("[Storage] NVS 初始化失败！尝试格式化...");
        // 尝试格式化 NVS 分区再试
        _opened = _prefs.begin(NVS_NAMESPACE, false);
    }
    Serial.println(_opened ? "[Storage] NVS 就绪" : "[Storage] NVS 彻底失败");
    return _opened;
}

void Storage::end() {
    if (_opened) {
        _prefs.end();
        _opened = false;
    }
}

// ---- 设备名字 ----
void Storage::setDeviceName(const String& name) {
    _prefs.putString(NVS_KEY_DEVICE_NAME, name);
}

String Storage::getDeviceName(const String& defaultName) {
    return _prefs.getString(NVS_KEY_DEVICE_NAME, defaultName);
}

// ---- 对方名字 ----
void Storage::setPartnerName(const String& name) {
    _prefs.putString(NVS_KEY_PARTNER_NAME, name);
}

String Storage::getPartnerName(const String& defaultName) {
    return _prefs.getString(NVS_KEY_PARTNER_NAME, defaultName);
}

// ---- 城市 ----
void Storage::setCity(const String& city) {
    _prefs.putString(NVS_KEY_CITY, city);
}

String Storage::getCity(const String& defaultCity) {
    return _prefs.getString(NVS_KEY_CITY, defaultCity);
}

// ---- 时区 ----
void Storage::setTimezone(const String& tz) {
    _prefs.putString(NVS_KEY_TZ, tz);
}

String Storage::getTimezone(const String& defaultTZ) {
    return _prefs.getString(NVS_KEY_TZ, defaultTZ);
}

// ---- WiFi 凭据 ----
void Storage::setWiFiCreds(const String& ssid, const String& pass) {
    _prefs.putString(NVS_KEY_WIFI_SSID, ssid);
    _prefs.putString(NVS_KEY_WIFI_PASS, pass);
}

String Storage::getWiFiSSID() {
    return _prefs.getString(NVS_KEY_WIFI_SSID, "");
}

String Storage::getWiFiPassword() {
    return _prefs.getString(NVS_KEY_WIFI_PASS, "");
}

// ---- 上次状态 ----
void Storage::setLastStatus(const String& status) {
    _prefs.putString("last_status", status);
}

String Storage::getLastStatus(const String& defaultStatus) {
    return _prefs.getString("last_status", defaultStatus);
}

// ---- 清空 ----
void Storage::clear() {
    _prefs.clear();
    Serial.println("[Storage] 所有配置已清空");
}
