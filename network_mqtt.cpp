/*
 * network_mqtt.cpp — MQTT 实现
 * 主题格式: sdd/{device_id}/...
 */

#include "network_mqtt.h"
#include "network_wifi.h"
#include <ArduinoJson.h>

WiFiClient   NetworkMQTT::_wifiClient;
PubSubClient NetworkMQTT::_mqtt(_wifiClient);
const char*  NetworkMQTT::_deviceId = nullptr;
bool         NetworkMQTT::_connected = false;
MqttCallback NetworkMQTT::_userCallback = nullptr;
unsigned long NetworkMQTT::_lastReconnectAttempt = 0;

// ===== 主题构建 =====
String NetworkMQTT::_topicStatus()    { return String("sdd/") + _deviceId + "/status"; }
String NetworkMQTT::_topicMood()      { return String("sdd/") + _deviceId + "/mood"; }
String NetworkMQTT::_topicHeartSend() { return String("sdd/") + _deviceId + "/heart/send"; }
String NetworkMQTT::_topicHeartAck()  { return String("sdd/") + _deviceId + "/heart/ack"; }
String NetworkMQTT::_topicWeatherReq(){ return String("sdd/") + _deviceId + "/weather/req"; }

// ===== 初始化 =====
void NetworkMQTT::begin(const char* deviceId) {
    _deviceId = deviceId;
    _mqtt.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
    _mqtt.setCallback(_mqttCallback);
    _mqtt.setKeepAlive(MQTT_KEEPALIVE);
    _lastReconnectAttempt = 0;
}

// ===== 主循环 =====
void NetworkMQTT::loop() {
    if (!NetworkWiFi::isConnected()) return;

    if (!_mqtt.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > 5000) {
            _lastReconnectAttempt = now;
            if (_connect()) {
                _connected = true;
            }
        }
    } else {
        _mqtt.loop();
    }
}

bool NetworkMQTT::_connect() {
    String id = String(_deviceId) + "_" + String(random(1000));
    Serial.printf("[MQTT] Connecting as %s...\n", id.c_str());

    // LWT: 断线时自动发布 offline
    String lwtPayload;
    {
        JsonDocument doc;
        doc["online"] = false;
        doc["device"] = _deviceId;
        serializeJson(doc, lwtPayload);
    }

    Serial.printf("[MQTT] Packet limit=%d\n", MQTT_MAX_PACKET_SIZE);

    if (_mqtt.connect(id.c_str(),
                      MQTT_USERNAME, MQTT_PASSWORD,
                      _topicStatus().c_str(), 1, true, lwtPayload.c_str())) {

        Serial.println("[MQTT] Connected!");

        // 发布上线
        publishStatus(true);

        // 订阅对方的消息
        subscribe("sdd/+/mood");
        subscribe("sdd/+/heart/send");
        subscribe("sdd/+/heart/ack");
        subscribe("sdd/weather/resp");

        return true;
    }

    Serial.printf("[MQTT] Failed, rc=%d\n", _mqtt.state());
    return false;
}

bool NetworkMQTT::isConnected() { return _mqtt.connected(); }

// ===== 发布 =====
bool NetworkMQTT::publish(const char* topic, const String& payload, bool retained) {
    if (!_mqtt.connected()) return false;
    return _mqtt.publish(topic, payload.c_str(), retained);
}

bool NetworkMQTT::publishStatus(bool online) {
    JsonDocument doc;
    doc["device"] = _deviceId;
    doc["online"]  = online;
    String payload;
    serializeJson(doc, payload);
    return publish(_topicStatus().c_str(), payload, true);
}

bool NetworkMQTT::publishMood(uint8_t mood) {
    JsonDocument doc;
    doc["mood"] = mood;
    String payload;
    serializeJson(doc, payload);
    return publish(_topicMood().c_str(), payload, true);
}

bool NetworkMQTT::publishHeartSend(int count, const char* to) {
    JsonDocument doc;
    doc["from"]    = _deviceId;
    doc["to"]      = to;
    doc["count"]   = count;
    doc["time"]    = millis();
    String payload;
    serializeJson(doc, payload);
    return publish(_topicHeartSend().c_str(), payload, true);  // retained: 对方离线也能收到
}

bool NetworkMQTT::publishHeartAck(const char* to) {
    JsonDocument doc;
    doc["from"] = _deviceId;
    doc["to"]   = to;
    doc["time"] = millis();
    String payload;
    serializeJson(doc, payload);
    return publish(_topicHeartAck().c_str(), payload, false);
}

bool NetworkMQTT::publishWeatherReq(const char* reqId) {
    JsonDocument doc;
    doc["req_id"] = reqId;
    String payload;
    serializeJson(doc, payload);
    return publish(_topicWeatherReq().c_str(), payload, false);
}

// ===== 订阅 =====
void NetworkMQTT::subscribe(const char* topic) {
    _mqtt.subscribe(topic);
    Serial.printf("[MQTT] Subscribed: %s\n", topic);
}

void NetworkMQTT::onMessage(MqttCallback cb) {
    _userCallback = cb;
}

// ===== 消息回调 =====
void NetworkMQTT::_mqttCallback(char* topic, byte* payload, unsigned int length) {
    String t = String(topic);
    String p = "";
    for (unsigned int i = 0; i < length; i++) {
        p += (char)payload[i];
    }

    Serial.printf("[MQTT] <- %s: %s\n", t.c_str(), p.c_str());

    // 忽略自己发的消息 (读取 /sdd/{id}/ 中的 id)
    if (t.startsWith("sdd/")) {
        int sl = t.indexOf('/', 4);
        if (sl > 0) {
            String sender = t.substring(4, sl);
            if (sender == _deviceId) return;  // 忽略自己的
        }
    }

    if (_userCallback) {
        _userCallback(t, p);
    }
}

const char* NetworkMQTT::deviceId()  { return _deviceId; }
const char* NetworkMQTT::partnerId() {
    return (strcmp(_deviceId, "gugu") == 0) ? "vv" : "gugu";
}
