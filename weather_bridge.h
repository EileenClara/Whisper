/*
 * weather_bridge.h — 天气数据模型 + MQTT 桥梁
 * ESP32 → MQTT weather/req → VPS → MQTT weather/resp → 解析
 */

#ifndef WEATHER_BRIDGE_H
#define WEATHER_BRIDGE_H

#include <Arduino.h>
#include "config.h"

struct WeatherData {
    int    temp;
    int    humidity;
    int    weatherCode;   // 天气图标编号 (对应 tianqi/tX.h)
    String weatherText;   // 中文天气描述
    String city;
    unsigned long fetchedAt;
};

class WeatherBridge {
public:
    static void begin();
    static void loop();

    // 缓存访问
    static const WeatherData& beijing();
    static const WeatherData& singapore();
    static bool isValid();

    // MQTT 消息处理
    static void handleResponse(const String& json);

    // 强制刷新
    static void forceRefresh();

private:
    static void _request();

    static WeatherData _bj;
    static WeatherData _sg;
    static bool        _valid;
    static unsigned long _lastRequest;
    static String      _lastReqId;
};

#endif
