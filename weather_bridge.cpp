/*
 * weather_bridge.cpp — 天气桥梁实现
 */

#include "weather_bridge.h"
#include "network_mqtt.h"
#include "network_wifi.h"
#include "storage_prefs.h"
#include <ArduinoJson.h>

WeatherData WeatherBridge::_bj;
WeatherData WeatherBridge::_sg;
bool WeatherBridge::_valid = false;
unsigned long WeatherBridge::_lastRequest = 0;
String WeatherBridge::_lastReqId = "";

void WeatherBridge::begin() {
    // 尝试从缓存加载
    String cache = StoragePrefs::getWeatherCache();
    if (cache.length() > 0) {
        handleResponse(cache);
    }
    // 启动时立即请求一次
    _lastRequest = millis() - WEATHER_INTERVAL_MS;
}

void WeatherBridge::loop() {
    if (!NetworkWiFi::isConnected() || !NetworkMQTT::isConnected()) return;

    // 定时刷新
    if (millis() - _lastRequest > WEATHER_INTERVAL_MS) {
        _request();
        _lastRequest = millis();
    }
}

void WeatherBridge::_request() {
    _lastReqId = String("wr_") + String(millis());
    bool ok = NetworkMQTT::publishWeatherReq(_lastReqId.c_str());
    Serial.printf("[Weather] Requesting (req=%s) ok=%d MQTT=%d WiFi=%d\n",
        _lastReqId.c_str(), ok, NetworkMQTT::isConnected(), NetworkWiFi::isConnected());
}

void WeatherBridge::forceRefresh() {
    _request();
    _lastRequest = millis();
}

void WeatherBridge::handleResponse(const String& json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);

    if (err) {
        Serial.printf("[Weather] JSON parse error: %s\n", err.c_str());
        return;
    }

    // 解析北京
    JsonObject bj = doc["beijing"];
    if (!bj.isNull()) {
        _bj.temp        = bj["temp"] | 0;
        _bj.humidity    = bj["humidity"] | 0;
        _bj.weatherCode = bj["weather_code"] | 99;
        _bj.weatherText = bj["weather"].as<String>();
        _bj.city        = bj["city"].as<String>();
        _bj.fetchedAt   = millis();
    }

    // 解析新加坡
    JsonObject sg = doc["singapore"];
    if (!sg.isNull()) {
        _sg.temp        = sg["temp"] | 0;
        _sg.humidity    = sg["humidity"] | 0;
        _sg.weatherCode = sg["weather_code"] | 99;
        _sg.weatherText = sg["weather"].as<String>();
        _sg.city        = sg["city"].as<String>();
        _sg.fetchedAt   = millis();
    }

    _valid = true;

    // 缓存
    StoragePrefs::setWeatherCache(json);
    StoragePrefs::setWeatherCacheTime(millis());

    Serial.printf("[Weather] BJ: %s %dC %d%% code=%d\n",
        _bj.city.c_str(), _bj.temp, _bj.humidity, _bj.weatherCode);
    Serial.printf("[Weather] SG: %s %dC %d%% code=%d\n",
        _sg.city.c_str(), _sg.temp, _sg.humidity, _sg.weatherCode);
}

const WeatherData& WeatherBridge::beijing()   { return _bj; }
const WeatherData& WeatherBridge::singapore() { return _sg; }
bool WeatherBridge::isValid() { return _valid; }
