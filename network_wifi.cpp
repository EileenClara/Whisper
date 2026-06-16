/*
 * network_wifi.cpp — WiFi + WiFiManager 配网
 * 连不上已保存的 WiFi → 自动开热点 "AutoConnectAP"
 */

#include "network_wifi.h"
#include "storage_prefs.h"
#include "display_screen.h"

WiFiState NetworkWiFi::_state = WiFiState::DISCONNECTED;
unsigned long NetworkWiFi::_lastAttempt = 0;
String NetworkWiFi::_ssid = "";
String NetworkWiFi::_pwd = "";
WiFiManager NetworkWiFi::_wm;
bool NetworkWiFi::_portalRunning = false;

void NetworkWiFi::begin(const char* fallbackSSID, const char* fallbackPWD) {
    // 先从 Preferences 加载，没有就用 fallback
    _ssid = StoragePrefs::getWiFiSSID();
    _pwd  = StoragePrefs::getWiFiPassword();

    if (_ssid.length() == 0) {
        _ssid = fallbackSSID;
        _pwd  = fallbackPWD;
    }

    WiFi.mode(WIFI_STA);
    _state = WiFiState::CONNECTING;
    _lastAttempt = millis();

    WiFi.begin(_ssid.c_str(), _pwd.c_str());
    Serial.printf("[WiFi] Connecting to %s...\n", _ssid.c_str());
}

void NetworkWiFi::loop() {
    wl_status_t status = WiFi.status();
    unsigned long now = millis();

    switch (_state) {
        case WiFiState::CONNECTING:
            if (status == WL_CONNECTED) {
                _state = WiFiState::CONNECTED;
                StoragePrefs::setWiFiCredentials(_ssid, _pwd);
                Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            } else if (now - _lastAttempt > 25000) {
                // 25 秒没连上 → 开 portal
                Serial.println("[WiFi] Timeout — opening portal...");
                startPortal();
            }
            break;

        case WiFiState::PORTAL:
            _wm.process();
            if (WiFi.status() == WL_CONNECTED) {
                _state = WiFiState::CONNECTED;
                _ssid = WiFi.SSID();
                _pwd  = WiFi.psk();
                StoragePrefs::setWiFiCredentials(_ssid, _pwd);
                _portalRunning = false;
                Serial.printf("[WiFi] Portal success! IP: %s\n", WiFi.localIP().toString().c_str());
            }
            // portal 3 分钟超时 → 重试原 WiFi
            if (now - _lastAttempt > 185000) {
                WiFi.mode(WIFI_STA);
                Serial.println("[WiFi] Portal timeout — retrying saved WiFi...");
                _state = WiFiState::CONNECTING;
                _lastAttempt = now;
                WiFi.begin(_ssid.c_str(), _pwd.c_str());
            }
            break;

        case WiFiState::CONNECTED:
            if (status != WL_CONNECTED) {
                _state = WiFiState::CONNECTING;
                _lastAttempt = now;
                WiFi.begin(_ssid.c_str(), _pwd.c_str());
                Serial.println("[WiFi] Lost — reconnecting...");
            }
            break;

        case WiFiState::DISCONNECTED:
            break;
    }
}

void NetworkWiFi::startPortal() {
    _state = WiFiState::PORTAL;
    _portalRunning = true;
    _lastAttempt = millis();  // 记录 portal 开始时间

    // 屏幕提示
    TFT_eSPI& tft = DisplayScreen::tft();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(20, 60);
    tft.print("WiFi Setup");
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(30, 100);
    tft.print("Connect to WiFi:");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(55, 120);
    tft.print("AutoConnectAP");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(30, 150);
    tft.print("Then open browser");
    tft.setCursor(65, 170);
    tft.print("192.168.4.1");

    _wm.resetSettings();
    _wm.setConfigPortalTimeout(180);  // 3 分钟超时

    // 非阻塞模式
    _wm.startConfigPortal("AutoConnectAP");
}

bool NetworkWiFi::isConnected() {
    return _state == WiFiState::CONNECTED && WiFi.status() == WL_CONNECTED;
}
WiFiState NetworkWiFi::state() { return _state; }
String NetworkWiFi::localIP() { return WiFi.localIP().toString(); }
