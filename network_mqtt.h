/*
 * network_mqtt.h — MQTT 客户端
 * 先不用 TLS，等 VPS 部署好再加证书
 */

#ifndef NETWORK_MQTT_H
#define NETWORK_MQTT_H

#include <WiFiClient.h>
#include <PubSubClient.h>
#include "config.h"

// MQTT 消息回调
typedef void (*MqttCallback)(const String& topic, const String& payload);

class NetworkMQTT {
public:
    static void begin(const char* deviceId);
    static void loop();
    static bool isConnected();

    // 发布
    static bool publish(const char* topic, const String& payload, bool retained = false);
    static bool publishStatus(bool online);
    static bool publishMood(uint8_t mood);
    static bool publishHeartSend(int count, const char* to);
    static bool publishHeartAck(const char* to);
    static bool publishWeatherReq(const char* reqId);

    // 订阅
    static void subscribe(const char* topic);
    static void onMessage(MqttCallback cb);

    static const char* deviceId();
    static const char* partnerId();  // 对方的 ID

private:
    static void _mqttCallback(char* topic, byte* payload, unsigned int length);
    static bool _connect();

    static WiFiClient   _wifiClient;
    static PubSubClient _mqtt;
    static const char*  _deviceId;
    static bool         _connected;
    static MqttCallback _userCallback;
    static unsigned long _lastReconnectAttempt;

    // 主题前缀
    static String _topicStatus();
    static String _topicMood();
    static String _topicHeartSend();
    static String _topicHeartAck();
    static String _topicWeatherReq();
};

#endif
