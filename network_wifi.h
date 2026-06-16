#ifndef NETWORK_WIFI_H
#define NETWORK_WIFI_H

#include <WiFi.h>
#include "config.h"

enum class WiFiState { DISCONNECTED, CONNECTING, CONNECTED };

class NetworkWiFi {
public:
    static void begin(const char* fallbackSSID, const char* fallbackPWD);
    static void loop();
    static bool isConnected();
    static WiFiState state();
    static String localIP();

private:
    static WiFiState _state;
    static unsigned long _lastRetry, _bootTime;
    static String _ssid, _pwd;
    static bool _portalOpen;
};

#endif
