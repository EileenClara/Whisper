/**
 * @file wifi_manager.cpp
 * @brief Whisper WiFi 连接管理 — 实现
 */

#include "wifi_manager.h"
#include "config.h"
#include "storage.h"
#include <esp_wifi.h>

extern Storage g_storage;  // 全局存储对象（在 main.cpp 中定义）

WiFiManager::WiFiManager() {}

WiFiManager::~WiFiManager() {
    disconnect();
}

bool WiFiManager::connect(const char* ssid, const char* password, unsigned long timeoutMs) {
    const char* useSSID = ssid;
    const char* usePass = password;

    // 如果没有提供凭据，尝试从 NVS 读取已保存的
    if (!useSSID || strlen(useSSID) == 0) {
        String savedSSID = g_storage.getWiFiSSID();
        if (savedSSID.length() > 0) {
            useSSID = savedSSID.c_str();
            String savedPass = g_storage.getWiFiPassword();
            usePass = savedPass.c_str();
            Serial.printf("[WiFi] 使用已保存凭据: %s\n", useSSID);
        }
    }

    // 还没有凭据 → 无可用WiFi
    if (!useSSID || strlen(useSSID) == 0) {
        Serial.println("[WiFi] 无可用WiFi凭据");
        return false;
    }

    return _doConnect(useSSID, usePass, timeoutMs);
}

bool WiFiManager::_doConnect(const char* ssid, const char* password, unsigned long timeoutMs) {
    Serial.printf("[WiFi] 连接中: %s ...\n", ssid);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, password);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        _connected = true;
        _reconnectCount = 0;
        _reconnectDelay = _reconnectBase;

        // 保存凭据到 NVS
        g_storage.setWiFiCreds(ssid, password);

        Serial.printf("[WiFi] ✅ 已连接! IP: %s, RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        return true;
    } else {
        _connected = false;
        Serial.printf("[WiFi] ❌ 连接失败: %s\n", getStatusText(WiFi.status()).c_str());
        return false;
    }
}

void WiFiManager::disconnect() {
    WiFi.disconnect(true);
    _connected = false;
    Serial.println("[WiFi] 已断开");
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int WiFiManager::getRSSI() {
    return WiFi.RSSI();
}

String WiFiManager::getLocalIP() {
    return WiFi.localIP().toString();
}

String WiFiManager::getStatusText(wl_status_t status) {
    switch (status) {
        case WL_IDLE_STATUS:    return "空闲";
        case WL_NO_SSID_AVAIL:  return "无SSID";
        case WL_SCAN_COMPLETED: return "扫描完成";
        case WL_CONNECTED:      return "已连接";
        case WL_CONNECT_FAILED: return "连接失败";
        case WL_CONNECTION_LOST:return "连接丢失";
        case WL_DISCONNECTED:   return "已断开";
        default:                return "未知";
    }
}

bool WiFiManager::startConfigPortal(const char* apName, unsigned long timeoutSec) {
    Serial.printf("[WiFi] 启动配网AP: %s (超时%lu秒)\n", apName, timeoutSec);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apName);

    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("[WiFi] AP IP: %s\n", apIP.toString().c_str());
    Serial.println("[WiFi] 用手机连接此WiFi热点进行配网（暂不支持Web配置，请串口设置凭据）");

    // TODO: 后续可接入 WiFiManager 库的 Web 配网页面
    // 目前先返回，让调用者在 loop 中通过其他方式设置凭据

    return true;
}

void WiFiManager::loop() {
    // 检查是否需要重连
    if (!_connected && WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt >= _reconnectDelay) {
            _lastReconnectAttempt = now;
            if (_tryReconnect()) {
                _reconnectCount = 0;
                _reconnectDelay = _reconnectBase;
            } else {
                _reconnectCount++;
                // 指数退避
                _reconnectDelay = min(_reconnectBase * (1 << min(_reconnectCount, 7)),
                                      _reconnectMax);
                Serial.printf("[WiFi] 下次重连: %u 秒后 (尝试 %d)\n",
                              _reconnectDelay / 1000, _reconnectCount);
            }
        }
    }

    // 更新连接状态
    _connected = (WiFi.status() == WL_CONNECTED);
}

bool WiFiManager::_tryReconnect() {
    String ssid = g_storage.getWiFiSSID();
    if (ssid.length() == 0) {
        return false;
    }
    String pass = g_storage.getWiFiPassword();

    Serial.printf("[WiFi] 重连中 #%d: %s\n", _reconnectCount + 1, ssid.c_str());

    if (_reconnectCount >= _maxReconnectAttempts) {
        Serial.println("[WiFi] 已达最大重连次数，停止重连。请重启设备。");
        return false;
    }

    return _doConnect(ssid.c_str(), pass.c_str(), WIFI_CONNECT_TIMEOUT_MS);
}
