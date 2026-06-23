/*
 * network_wifi.cpp
 * 每 5s 重试 WiFi, 30s 后开 AutoConnectAP 热点作后备
 */

#include "network_wifi.h"
#include "storage_prefs.h"
#include "display_screen.h"
#include <WebServer.h>

WiFiState NetworkWiFi::_state = WiFiState::DISCONNECTED;
unsigned long NetworkWiFi::_lastRetry = 0;
unsigned long NetworkWiFi::_bootTime = 0;
String NetworkWiFi::_ssid = "";
String NetworkWiFi::_pwd = "";
bool NetworkWiFi::_portalOpen = false;
int NetworkWiFi::_wifiIdx = 0;
static bool _justConnected = false;

static WebServer* portalServer = nullptr;

void handlePortalRoot() {
    String html = "<html><body style='background:#1a1a2e;color:#fff;font-size:16px;text-align:center;'>";
    html += "<h2>WiFi Setup</h2>";
    if (portalServer->hasArg("ssid")) {
        String s = portalServer->arg("ssid");
        String p = portalServer->arg("pwd");
        StoragePrefs::setWiFiCredentials(s, p);
        html += "<p>Saved! Restarting...</p>";
        portalServer->send(200, "text/html", html);
        delay(500);
        ESP.restart();
        return;
    }
    html += "<form method='GET'>";
    html += "SSID:<br><input name='ssid'><br>";
    html += "Password:<br><input name='pwd' type='password'><br><br>";
    html += "<input type='submit' value='Save & Restart'>";
    html += "</form></body></html>";
    portalServer->send(200, "text/html", html);
}

void NetworkWiFi::begin() {
    _wifiIdx = 0;
    _ssid = StoragePrefs::getWiFiSSID(0);
    _pwd  = StoragePrefs::getWiFiPassword(0);

    WiFi.mode(WIFI_STA);
    _state = WiFiState::CONNECTING;
    _bootTime = millis();
    _lastRetry = 0;
    _portalOpen = false;

    WiFi.begin(_ssid.c_str(), _pwd.c_str());
    Serial.printf("[WiFi] Trying #%d: %s\n", _wifiIdx, _ssid.c_str());
}

void NetworkWiFi::loop() {
    wl_status_t status = WiFi.status();
    unsigned long now = millis();

    // 连上了
    if (status == WL_CONNECTED && _state != WiFiState::CONNECTED) {
        _state = WiFiState::CONNECTED;
        _ssid = WiFi.SSID();
        _pwd  = WiFi.psk();
        StoragePrefs::setWiFiCredentials(_ssid, _pwd);
        _justConnected = true;
        if (_portalOpen) {
            if (portalServer) { portalServer->stop(); delete portalServer; portalServer = nullptr; }
            WiFi.softAPdisconnect(true);
            _portalOpen = false;
        }
        Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        return;
    }

    // 断连
    if (_state == WiFiState::CONNECTED && status != WL_CONNECTED) {
        _state = WiFiState::CONNECTING;
        _lastRetry = 0;
        Serial.println("[WiFi] Lost — reconnecting...");
    }

    if (_state == WiFiState::CONNECTED) return;

    // ==== 未连接 ====

    // 每 8s 换下一个 WiFi 尝试
    if (now - _lastRetry > 8000) {
        _lastRetry = now;
        _wifiIdx = (_wifiIdx + 1) % StoragePrefs::wifiCount();
        _ssid = StoragePrefs::getWiFiSSID(_wifiIdx);
        _pwd  = StoragePrefs::getWiFiPassword(_wifiIdx);
        if (_portalOpen) WiFi.mode(WIFI_AP_STA);
        WiFi.begin(_ssid.c_str(), _pwd.c_str());
        Serial.printf("[WiFi] Trying #%d: %s\n", _wifiIdx, _ssid.c_str());
    }

    // 30s 还没连上 → 开热点 (AP+STA 模式, 原 WiFi 继续在后台尝试)
    if (!_portalOpen && (now - _bootTime > 30000)) {
        _portalOpen = true;
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP("AutoConnectAP");
        Serial.println("[WiFi] AP opened: AutoConnectAP (192.168.4.1)");

        portalServer = new WebServer(80);
        portalServer->on("/", handlePortalRoot);
        portalServer->begin();

        TFT_eSPI& tft = DisplayScreen::tft();
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.setTextSize(2);
        tft.setCursor(30, 40);
        tft.print("WiFi Setup");
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(20, 80);
        tft.print("1. Connect to:");
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setCursor(55, 100);
        tft.print("AutoConnectAP");
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(20, 130);
        tft.print("2. Open:");
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setCursor(65, 150);
        tft.print("192.168.4.1");
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(20, 180);
        tft.print("Retrying saved WiFi");
        tft.setCursor(20, 198);
        tft.print("in background...");
    }

    if (_portalOpen && portalServer) {
        portalServer->handleClient();
    }
}

bool NetworkWiFi::isConnected() { return WiFi.status() == WL_CONNECTED; }
bool NetworkWiFi::justConnected() { bool r = _justConnected; _justConnected = false; return r; }
WiFiState NetworkWiFi::state() { return _state; }
String NetworkWiFi::localIP() { return WiFi.localIP().toString(); }
