/*
 * storage_prefs.cpp — Preferences 实现
 */

#include "storage_prefs.h"

Preferences StoragePrefs::_prefs;
const char* StoragePrefs::_ns = "sdd";

void StoragePrefs::begin() {
    _prefs.begin(_ns, false);
}

void StoragePrefs::end() {
    _prefs.end();
}

// ---- WiFi ----
String StoragePrefs::getWiFiSSID() {
    return _prefs.getString("wifi_ssid", "");
}

String StoragePrefs::getWiFiPassword() {
    return _prefs.getString("wifi_pwd", "");
}

void StoragePrefs::setWiFiCredentials(const String& ssid, const String& pass) {
    _prefs.putString("wifi_ssid", ssid);
    _prefs.putString("wifi_pwd", pass);
}

void StoragePrefs::clearWiFiCredentials() {
    _prefs.remove("wifi_ssid");
    _prefs.remove("wifi_pwd");
}

// ---- 设备身份 ----
bool StoragePrefs::isIdentitySet() {
    return _prefs.getUChar("identity", 0) != 0;
}

uint8_t StoragePrefs::getIdentity() {
    return _prefs.getUChar("identity", 0);  // 0=未设置, 1=果果, 2=vv
}

void StoragePrefs::setIdentity(uint8_t id) {
    _prefs.putUChar("identity", id);
}

// ---- 背光 ----
uint8_t StoragePrefs::getBrightness() {
    return _prefs.getUChar("bl", BL_DEFAULT_PCT);
}

void StoragePrefs::setBrightness(uint8_t pct) {
    _prefs.putUChar("bl", constrain(pct, 5, 100));
}

// ---- 屏幕方向 ----
uint8_t StoragePrefs::getRotation() {
    return _prefs.getUChar("rot", 0);
}

void StoragePrefs::setRotation(uint8_t rot) {
    _prefs.putUChar("rot", constrain(rot, 0, 3));
}

// ---- 状态 ----
uint8_t StoragePrefs::getLastMood() {
    return _prefs.getUChar("mood", MOOD_FREE);
}

void StoragePrefs::setLastMood(uint8_t mood) {
    _prefs.putUChar("mood", constrain(mood, 0, MOOD_COUNT - 1));
}

// ---- 天气 ----
uint8_t StoragePrefs::getWeatherInterval() {
    return _prefs.getUChar("wx_iv", 10);  // 默认 10 分钟
}

void StoragePrefs::setWeatherInterval(uint8_t mins) {
    _prefs.putUChar("wx_iv", constrain(mins, 5, 60));
}

// ---- 爱心冷却 ----
unsigned long StoragePrefs::getLastHeartTime() {
    return _prefs.getULong("ht_time", 0);
}

void StoragePrefs::setLastHeartTime(unsigned long t) {
    _prefs.putULong("ht_time", t);
}

// ---- 天气缓存 ----
String StoragePrefs::getWeatherCache() {
    return _prefs.getString("wx_cache", "");
}

void StoragePrefs::setWeatherCache(const String& json) {
    _prefs.putString("wx_cache", json);
}

unsigned long StoragePrefs::getWeatherCacheTime() {
    return _prefs.getULong("wx_time", 0);
}

void StoragePrefs::setWeatherCacheTime(unsigned long t) {
    _prefs.putULong("wx_time", t);
}

// ---- 恢复出厂 ----
void StoragePrefs::factoryReset() {
    _prefs.clear();
}
