/*
 * network_ntp.h — NTP 北京时间同步
 */

#ifndef NETWORK_NTP_H
#define NETWORK_NTP_H

#include <WiFiUdp.h>
#include <TimeLib.h>
#include "config.h"

class NetworkNTP {
public:
    static void begin();
    static void loop();          // 定期同步
    static bool isSynced();

    // 北京时间获取
    static time_t now();         // UTC+8 时间戳
    static int    hour();
    static int    minute();
    static int    second();
    static int    day();
    static int    month();
    static int    year();
    static int    weekday();     // 1=周日 .. 7=周六（TimeLib 标准）

    // 格式化
    static String timeStr();     // "14:30:25"
    static String dateStr();     // "6月15日"
    static String weekStr();     // "周日" ~ "周六"

    static void forceSync();

private:
    static time_t _getNtpTime();
    static void   _sendPacket(IPAddress& addr);

    static WiFiUDP _udp;
    static bool    _synced;
    static unsigned long _lastSync;
    static byte    _packetBuf[48];
    static const char* _weekNames[7];
};

#endif
