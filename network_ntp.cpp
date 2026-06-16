/*
 * network_ntp.cpp — NTP 实现
 */

#include "network_ntp.h"
#include "network_wifi.h"

WiFiUDP NetworkNTP::_udp;
bool NetworkNTP::_synced = false;
unsigned long NetworkNTP::_lastSync = 0;
byte NetworkNTP::_packetBuf[48];

const char* NetworkNTP::_weekNames[7] = {"周日","周一","周二","周三","周四","周五","周六"};

void NetworkNTP::begin() {
    _udp.begin(8000);
    setSyncInterval(NTP_SYNC_INTERVAL);
}

void NetworkNTP::loop() {
    if (!NetworkWiFi::isConnected()) return;

    if (!_synced || millis() - _lastSync > (NTP_SYNC_INTERVAL * 1000UL)) {
        time_t t = _getNtpTime();
        if (t > 1000000000) {  // 2020 年以后
            setTime(t);
            _synced = true;
            _lastSync = millis();
            Serial.printf("[NTP] Synced: %s %s\n", dateStr().c_str(), timeStr().c_str());
        }
    }
}

bool NetworkNTP::isSynced() { return _synced; }

time_t NetworkNTP::now()  { return ::now(); }
int NetworkNTP::hour()    { return ::hour(); }
int NetworkNTP::minute()  { return ::minute(); }
int NetworkNTP::second()  { return ::second(); }
int NetworkNTP::day()     { return ::day(); }
int NetworkNTP::month()   { return ::month(); }
int NetworkNTP::year()    { return ::year(); }
int NetworkNTP::weekday() { return ::weekday(); }

String NetworkNTP::timeStr() {
    char buf[9];
    snprintf(buf, 9, "%02d:%02d:%02d", hour(), minute(), second());
    return String(buf);
}

String NetworkNTP::dateStr() {
    char buf[16];
    snprintf(buf, 16, "%d月%d日", month(), day());
    return String(buf);
}

String NetworkNTP::weekStr() {
    int w = weekday() - 1;
    if (w < 0) w = 6;
    return String(_weekNames[w]);
}

void NetworkNTP::forceSync() {
    _synced = false;
    _lastSync = 0;
}

// ---- NTP 协议 ----
time_t NetworkNTP::_getNtpTime() {
    IPAddress ntpIP;
    WiFi.hostByName(NTP_SERVER, ntpIP);

    while (_udp.parsePacket() > 0);  // 清空残留

    _sendPacket(ntpIP);

    uint32_t start = millis();
    while (millis() - start < 1500) {
        int size = _udp.parsePacket();
        if (size >= 48) {
            _udp.read(_packetBuf, 48);
            unsigned long secs = (unsigned long)_packetBuf[40] << 24;
            secs |= (unsigned long)_packetBuf[41] << 16;
            secs |= (unsigned long)_packetBuf[42] << 8;
            secs |= (unsigned long)_packetBuf[43];
            return secs - 2208988800UL + NTP_TIMEZONE * 3600UL;
        }
        delay(10);
    }
    return 0;
}

void NetworkNTP::_sendPacket(IPAddress& addr) {
    memset(_packetBuf, 0, 48);
    _packetBuf[0] = 0b11100011;
    _packetBuf[1] = 0;
    _packetBuf[2] = 6;
    _packetBuf[3] = 0xEC;
    _packetBuf[12] = 49;
    _packetBuf[13] = 0x4E;
    _packetBuf[14] = 49;
    _packetBuf[15] = 52;
    _udp.beginPacket(addr, 123);
    _udp.write(_packetBuf, 48);
    _udp.endPacket();
}
