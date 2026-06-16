/*
 * network_wifi.cpp
 * 每 5s 尝试原 WiFi, 30s 后同时开 portal 作为后备
 */

#include "network_wifi.h"
#include "storage_prefs.h"
#include "display_screen.h"

WiFiState NetworkWiFi::_state = WiFiState::DISCONNECTED;
unsigned long NetworkWiFi::_lastRetry = 0;
unsigned long NetworkWiFi::_bootTime = 0;
String NetworkWiFi::_ssid = "";
String NetworkWiFi::_pwd = "";
WiFiManager NetworkWiFi::_wm;
bool NetworkWiFi::_portalOpen = false;

void NetworkWiFi::begin(const char* fallbackSSID, const char* fallbackPWD) {
    _ssid = StoragePrefs::getWiFiSSID();
    _pwd  = StoragePrefs::getWiFiPassword();
    if (_ssid.length() == 0) { _ssid = fallbackSSID; _pwd = fallbackPWD; }

    WiFi.mode(WIFI_STA);
    _state = WiFiState::CONNECTING;
    _bootTime = millis();
    _lastRetry = 0;

    WiFi.begin(_ssid.c_str(), _pwd.c_str());
    Serial.printf("[WiFi] Connecting to %s...\n", _ssid.c_str());
}

void NetworkWiFi::loop() {
    wl_status_t status = WiFi.status();
    unsigned long now = millis();

    if (_state == WiFiState::CONNECTED) {
        if (status != WL_CONNECTED) {
            _state = WiFiState::CONNECTING;
            _lastRetry = 0;
            WiFi.begin(_ssid.c_str(), _pwd.c_str());
            Serial.println("[WiFi] Lost — reconnecting...");
        }
        if (_portalOpen) {
            _wm.stopConfigPortal();
            _portalOpen = false;
        }
        return;
    }

    // ==== 未连接 ====

    // 连上了 → 切到 CONNECTED
    if (status == WL_CONNECTED) {
        _state = WiFiState::CONNECTED;
        _ssid = WiFi.SSID();
        _pwd  = WiFi.psk();
        StoragePrefs::setWiFiCredentials(_ssid, _pwd);
        Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        return;
    }

    // 每 5s 重试原 WiFi (WiFi.begin 会触发重连)
    if (now - _lastRetry > 5000) {
        _lastRetry = now;
        WiFi.begin(_ssid.c_str(), _pwd.c_str());
        Serial.printf("[WiFi] Retry %s...\n", _ssid.c_str());
    }

    // 30s 后还没连上 → 开 portal (并行, 不阻塞)
    if (!_portalOpen && (now - _bootTime > 30000)) {
        startPortal();
    }

    // portal 运行中 → 处理
    if (_portalOpen) {
        _wm.process();
        // portal 设置了新 WiFi → 立刻尝试
        if (WiFi.SSID() != _ssid && WiFi.SSID().length() > 0) {
            _ssid = WiFi.SSID();
            _pwd  = WiFi.psk();
            StoragePrefs::setWiFiCredentials(_ssid, _pwd);
            Serial.printf("[WiFi] Portal set new SSID: %s\n", _ssid.c_str());
        }
    }
}

void NetworkWiFi::startPortal() {
    _portalOpen = true;

    TFT_eSPI& tft = DisplayScreen::tft();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(30, 50);
    tft.print("WiFi Setup");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(30, 90);
    tft.print("Connect phone to:");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(60, 110);
    tft.print("AutoConnectAP");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(40, 140);
    tft.print("Still trying saved");
    tft.setCursor(40, 158);
    tft.print("WiFi in background...");

    _wm.resetSettings();
    _wm.setConfigPortalTimeout(600);  // 10 分钟
    _wm.startConfigPortal("AutoConnectAP");
    Serial.println("[WiFi] Portal opened (background)");
}

bool NetworkWiFi::isConnected() {
    return _state == WiFiState::CONNECTED && WiFi.status() == WL_CONNECTED;
}
WiFiState NetworkWiFi::state() { return _state; }
String NetworkWiFi::localIP() { return WiFi.localIP().toString(); }
