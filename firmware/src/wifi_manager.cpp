/**
 * @file wifi_manager.cpp
 * @brief Whisper WiFi 连接 + WiFiManager 配网
 *
 * 配网流程:
 *  1. 开机 → 尝试连接已保存的 WiFi
 *  2. 失败 → 自动开启 AP 热点 "Whisper-Setup"
 *  3. 手机连接热点 → 弹出配网页面 → 输入WiFi+密码
 *  4. 设备保存凭据 → 自动重启
 */

#include "wifi_manager.h"
#include "config.h"
#include "storage.h"
#include <WiFiManager.h>  // tzapu/WiFiManager
#include <esp_wifi.h>

extern Storage g_storage;

WiFiManager::WiFiManager() {}

WiFiManager::~WiFiManager() {
    disconnect();
}

bool WiFiManager::connect(const char* ssid, const char* password, unsigned long timeoutMs) {
    // 优先用传入的凭据
    const char* useSSID = ssid;
    const char* usePass = password;

    // 如果没传，尝试从 NVS 读取已保存的
    if (!useSSID || strlen(useSSID) == 0) {
        String savedSSID = g_storage.getWiFiSSID();
        if (savedSSID.length() > 0) {
            useSSID = savedSSID.c_str();
            String savedPass = g_storage.getWiFiPassword();
            usePass = savedPass.c_str();
            Serial.printf("[WiFi] 使用已保存凭据: %s\n", useSSID);
        }
    }

    if (!useSSID || strlen(useSSID) == 0) {
        Serial.println("[WiFi] 无可用的WiFi凭据");
        return false;
    }

    return _doConnect(useSSID, usePass, timeoutMs);
}

bool WiFiManager::_doConnect(const char* ssid, const char* password, unsigned long timeoutMs) {
    Serial.printf("[WiFi] 正在连接: %s ...\n", ssid);

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

        // 保存到 NVS
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
    Serial.printf("[WiFi] 启动配网AP: %s (超时 %lu 秒)\n", apName, timeoutSec);

    // 使用 WiFiManager 库的 captive portal
    WiFiManager wm;
    wm.setConfigPortalTimeout(timeoutSec);
    wm.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));

    // 尝试连接已保存的网络，如果失败就进入配网模式
    String savedSSID = g_storage.getWiFiSSID();
    String savedPass = g_storage.getWiFiPassword();

    if (savedSSID.length() > 0) {
        // 有已保存凭据，先尝试自动连接
        Serial.printf("[WiFi] 尝试自动连接: %s\n", savedSSID.c_str());
        wm.setConnectTimeout(10);  // 10秒超时
        bool connected = wm.autoConnect(apName);
        if (connected) {
            _connected = true;
            _reconnectCount = 0;
            g_storage.setWiFiCreds(
                WiFi.SSID().c_str(),
                WiFi.psk().c_str()
            );
            Serial.printf("[WiFi] ✅ 自动连接成功! IP: %s\n", WiFi.localIP().toString().c_str());
            return true;
        }
    }

    // 启动配网门户
    Serial.println("[WiFi] 进入配网模式，请用手机连接热点...");
    bool connected = wm.startConfigPortal(apName);

    if (connected) {
        // 配网成功，保存凭据
        g_storage.setWiFiCreds(
            WiFi.SSID().c_str(),
            WiFi.psk().c_str()
        );
        _connected = true;
        _reconnectCount = 0;
        Serial.printf("[WiFi] ✅ 配网成功! IP: %s\n", WiFi.localIP().toString().c_str());
        return true;
    } else {
        Serial.println("[WiFi] ⚠️ 配网超时或失败");
        return false;
    }
}

void WiFiManager::loop() {
    // WiFi 断线自动重连
    if (!_connected && WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt >= _reconnectDelay) {
            _lastReconnectAttempt = now;
            if (_tryReconnect()) {
                _reconnectCount = 0;
                _reconnectDelay = _reconnectBase;
            } else {
                _reconnectCount++;
                _reconnectDelay = min(_reconnectBase * (1 << min(_reconnectCount, 7)), _reconnectMax);
                Serial.printf("[WiFi] 下次重连: %u 秒后 (尝试 %d)\n",
                              _reconnectDelay / 1000, _reconnectCount);
            }
        }
    }
    _connected = (WiFi.status() == WL_CONNECTED);
}

bool WiFiManager::_tryReconnect() {
    String ssid = g_storage.getWiFiSSID();
    if (ssid.length() == 0) return false;
    String pass = g_storage.getWiFiPassword();
    if (_reconnectCount >= _maxReconnectAttempts) {
        Serial.println("[WiFi] 重连次数耗尽，将重启进入配网模式");
        ESP.restart();
        return false;
    }
    return _doConnect(ssid.c_str(), pass.c_str(), WIFI_CONNECT_TIMEOUT_MS);
}
