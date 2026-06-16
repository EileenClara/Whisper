#ifndef NETWORK_WIFI_H
#define NETWORK_WIFI_H

#include <WiFi.h>
#include <WiFiManager.h>
#include "config.h"

enum class WiFiState { DISCONNECTED, CONNECTING, CONNECTED, PORTAL };

class NetworkWiFi {
public:
    static void begin(const char* fallbackSSID, const char* fallbackPWD);
    static void loop();
    static bool isConnected();
    static WiFiState state();
    static String localIP();
    static void startPortal();

private:
    static WiFiState _state;
    static unsigned long _lastAttempt;
    static String _ssid, _pwd;
    static WiFiManager _wm;
    static bool _portalRunning;
};

#endif
