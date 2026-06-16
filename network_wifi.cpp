/*
 * network_wifi.cpp — WiFi 非阻塞实现
 */

#include "network_wifi.h"
#include "storage_prefs.h"

WiFiState NetworkWiFi::_state = WiFiState::DISCONNECTED;
unsigned long NetworkWiFi::_lastAttempt = 0;
String NetworkWiFi::_ssid = "";
String NetworkWiFi::_pwd = "";

void NetworkWiFi::begin(const char* ssid, const char* pwd) {
    _ssid = String(ssid);
    _pwd  = String(pwd);
    _connect(ssid, pwd);
}

void NetworkWiFi::begin() {
    _ssid = StoragePrefs::getWiFiSSID();
    _pwd  = StoragePrefs::getWiFiPassword();
    if (_ssid.length() > 0) {
        _connect(_ssid.c_str(), _pwd.c_str());
    }
}

void NetworkWiFi::_connect(const char* ssid, const char* pwd) {
    _state = WiFiState::CONNECTING;
    _lastAttempt = millis();

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(ssid, pwd);

    Serial.printf("[WiFi] Connecting to %s...\n", ssid);
}

void NetworkWiFi::loop() {
    wl_status_t status = WiFi.status();

    switch (_state) {
        case WiFiState::CONNECTING:
            if (status == WL_CONNECTED) {
                _state = WiFiState::CONNECTED;
                Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
                // 保存凭证
                StoragePrefs::setWiFiCredentials(_ssid, _pwd);
            } else if (millis() - _lastAttempt > WIFI_CONNECT_TIMEOUT_MS) {
                _state = WiFiState::DISCONNECTED;
                Serial.println("[WiFi] Connect timeout");
            }
            break;

        case WiFiState::CONNECTED:
            if (status != WL_CONNECTED) {
                _state = WiFiState::RECONNECTING;
                _lastAttempt = millis();
                Serial.println("[WiFi] Lost connection, reconnecting...");
            }
            break;

        case WiFiState::RECONNECTING:
            if (status == WL_CONNECTED) {
                _state = WiFiState::CONNECTED;
                Serial.printf("[WiFi] Reconnected! IP: %s\n", WiFi.localIP().toString().c_str());
            } else if (millis() - _lastAttempt > WIFI_RECONNECT_INTERVAL_MS) {
                WiFi.reconnect();
                _lastAttempt = millis();
            }
            break;

        case WiFiState::DISCONNECTED:
            // 重试连接
            if (millis() - _lastAttempt > WIFI_RECONNECT_INTERVAL_MS) {
                _connect(_ssid.c_str(), _pwd.c_str());
            }
            break;
    }
}

bool NetworkWiFi::isConnected() {
    return _state == WiFiState::CONNECTED && WiFi.status() == WL_CONNECTED;
}

WiFiState NetworkWiFi::state() { return _state; }
String NetworkWiFi::localIP() { return WiFi.localIP().toString(); }
String NetworkWiFi::ssid() { return _ssid; }
