/**
 * @file storage.h
 * @brief Whisper NVS 配置存储模块
 *
 * 使用 ESP32 NVS（非易失存储）持久化设备配置：
 * - 设备名、对方名、城市、时区
 * - WiFi 凭据（由 WiFiManager 写入）
 * - 上次状态
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include <Preferences.h>

class Storage {
public:
    Storage();
    ~Storage();

    /** 初始化 NVS */
    bool begin();

    /** 保存/读取：设备名字 */
    void setDeviceName(const String& name);
    String getDeviceName(const String& defaultName = "");

    /** 保存/读取：对方名字 */
    void setPartnerName(const String& name);
    String getPartnerName(const String& defaultName = "");

    /** 保存/读取：城市名 */
    void setCity(const String& city);
    String getCity(const String& defaultCity = "");

    /** 保存/读取：时区字符串 */
    void setTimezone(const String& tz);
    String getTimezone(const String& defaultTZ = "");

    /** 保存/读取：WiFi SSID / Password */
    void setWiFiCreds(const String& ssid, const String& pass);
    String getWiFiSSID();
    String getWiFiPassword();

    /** 保存/读取：上次选中的状态 */
    void setLastStatus(const String& status);
    String getLastStatus(const String& defaultStatus = "love");

    /** 清空所有配置 */
    void clear();

    /** 关闭 NVS */
    void end();

private:
    Preferences _prefs;
    bool _opened = false;
};

#endif // STORAGE_H
