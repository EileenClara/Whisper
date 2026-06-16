#ifndef NETWORK_WIFI_H
#define NETWORK_WIFI_H

#include <WiFi.h>
#include "config.h"

enum class WiFiState {
    DISCONNECTED, CONNECTING, CONNECTED, PORTAL
};

class NetworkWiFi {
public:
    static void begin(const char* fallbackSSID, const char* fallbackPWD);
    static void loop();
    static bool isConnected();
    static WiFiState state();
    static String localIP();
    static void startPortal();

private:
    static void _connect(const char* ssid, const char* pwd);
    static WiFiState _state;
    static unsigned long _lastAttempt;
    static String _ssid;
    static String _pwd;
};

#endif
