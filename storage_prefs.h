/*
 * storage_prefs.h — Preferences (NVS) 持久化存储
 * ESP32-S3 用 Preferences 替代 EEPROM
 */

#ifndef STORAGE_PREFS_H
#define STORAGE_PREFS_H

#include <Preferences.h>
#include "config.h"

class StoragePrefs {
public:
    static void begin();
    static void end();

    // ---- WiFi (最多 2 个) ----
    static String getWiFiSSID(int n = 0);
    static String getWiFiPassword(int n = 0);
    static void   setWiFiCredentials(const String& ssid, const String& pass, int n = 0);
    static void   clearWiFiCredentials();
    static int    wifiCount();  // 存了几个 WiFi

    // ---- 设备身份 ----
    static bool   isIdentitySet();           // 是否已选择身份
    static uint8_t getIdentity();            // 0=未设置, 1=果果, 2=vv
    static void   setIdentity(uint8_t id);

    // ---- 背光 ----
    static uint8_t getBrightness();          // 0-100
    static void    setBrightness(uint8_t pct);

    // ---- 屏幕方向 ----
    static uint8_t getRotation();
    static void    setRotation(uint8_t rot);

    // ---- 状态 ----
    static uint8_t getLastMood();            // 上次的状态
    static void    setLastMood(uint8_t mood);

    // ---- 天气刷新间隔 (分钟) ----
    static uint8_t getWeatherInterval();
    static void    setWeatherInterval(uint8_t mins);

    // ---- 爱心状态 ----
    static unsigned long getLastHeartTime();
    static void          setLastHeartTime(unsigned long t);
    static int           getHeartCount();        // 爱心计数
    static void          setHeartCount(int c);
    static int           getHeartRole();         // 0=NONE 1=SENDER 2=RECEIVER
    static void          setHeartRole(int r);
    static void          clearHeartState();      // 清空所有爱心状态

    // ---- 天气缓存 ----
    static String getWeatherCache();
    static void   setWeatherCache(const String& json);
    static unsigned long getWeatherCacheTime();
    static void          setWeatherCacheTime(unsigned long t);

    // ---- 恢复出厂 ----
    static void factoryReset();

private:
    static Preferences _prefs;
    static const char* _ns;
};

#endif
