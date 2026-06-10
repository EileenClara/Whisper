/**
 * @file mqtt_handler.cpp
 * @brief Whisper MQTT TLS 通讯模块 — 实现
 */

#include "mqtt_handler.h"
#include "config.h"

// 静态实例指针
MQTTHandler* MQTTHandler::_instance = nullptr;

// ISRG Root X1 根证书（Let's Encrypt 根证书）
// 这是 Let's Encrypt 签发的证书的信任根
static const char* ISRG_ROOT_X1 = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfFsDHtz
dNcaQBOHU61VyO6/IcMJ2ASC8J2GKcBAaR1Iex6BCCMAAIhHyFQeOyEqj0+AqJQm
4HkUR1NHM4Hs5g2X0T1RR86DdQIBA6NCMEAwDgYDVR0PAQH/BAQDAgEGMA8GA1Ud
EwEB/wQFMAMBAf8wHQYDVR0OBBYEFOhKqR6lOoR6KeC+R2e+QHnYil5wMA0GCSqG
SIb3DQEBCwUAA4ICAQCoOGk8c4j0geJfLXPQJKgZF0RqDqQvFj+ZT+1lWfNkBCWQ
nFC+PRBGl6OyZ6V/YFhqAkd9HbO14P6+1TuNnEGvsyH7y/JWPhC4unW+K0SxB4Em
XRFCPv/qzX6yeOAu5qV0ZnICzZ+QJyL4wSIZlwYjIq9ZJ0kJNyTbG7PbBHqvKjXY
2VXXOZYfY7X7zjIV6Nw6vEnIVUBxC0U3tBkwDHXx0L0lQJ7+Bh+Q/fZ+6a3bGEbG
XqR3+F+XhU3n7Zq8UvhP7ZqDpnxElGJP3dANj2jT3rFqJjM9/6qVJrqRmMjhpGNs
dzCm1bG0XqYQUQ4ETZQG6X3/l/emLQ6TkpvFMgR3y3JDh4nOq8vTmLVMsfzHRlTQ
6l3vr/BdFkZZLqyz0M/6fXHiVMFU7tS6G4pHGx9LB/qflxPHx4Y8LLZ7eL0f4vLQ
5SzXqoxsLqFv1mCNg2g3Q7oH6vjGwQUuYflLQHqHXyYgXHfjy8qZ/KG2fYfHqV8I
E3vJkZ/GhWJxNqLHRn9jB/Mdzs4q2aM21uT3CR4FqNQhDmKthVjNqLJY9Y/BKglc
C4uKXoVE0jMGgHfp+PLsQI6lZGRCcY8wLQXoLxjGzthRq+jNnZ1/JFLsYmIFRnEl
1+GM2szBllMTHNGHNJbI1qpcP/+Fiyz+8tQ6oD+Kb2+4F2PJQ==
-----END CERTIFICATE-----
)EOF";

MQTTHandler::MQTTHandler() {
    _instance = this;
}

MQTTHandler::~MQTTHandler() {
    disconnect();
    _instance = nullptr;
}

bool MQTTHandler::begin(const char* brokerHost, uint16_t brokerPort,
                         const char* username, const char* password,
                         const char* deviceId) {
    _brokerHost = brokerHost;
    _brokerPort = brokerPort;
    _username = username;
    _password = password;
    _deviceId = deviceId;

    // 设置 TLS 证书
    _wifiClient.setCACert(ISRG_ROOT_X1);

    // 设置 MQTT 客户端
    _mqtt.setClient(_wifiClient);
    _mqtt.setServer(_brokerHost.c_str(), _brokerPort);
    _mqtt.setCallback(_mqttCallback);
    // 设置较大的缓冲区以容纳 JSON
    _mqtt.setBufferSize(1024);

    Serial.println("[MQTT] 配置完成");
    return true;
}

void MQTTHandler::onMessage(MQTTCallback callback) {
    _callback = callback;
}

bool MQTTHandler::connect(unsigned long timeoutMs) {
    if (_brokerHost.length() == 0) {
        Serial.println("[MQTT] Broker地址未配置");
        return false;
    }

    Serial.printf("[MQTT] 连接中: %s:%d (TLS) ...\n", _brokerHost.c_str(), _brokerPort);

    // 设置 LWT (Last Will Testament) — 设备离线时自动发布
    String lwtTopic = topicPresence();
    String lwtPayload = "{\"online\":false}";
    _mqtt.connect(
        _deviceId.c_str(),
        _username.c_str(),
        _password.c_str(),
        lwtTopic.c_str(), 0, true, lwtPayload.c_str()
    );

    unsigned long start = millis();
    while (!_mqtt.connected() && (millis() - start) < timeoutMs) {
        delay(100);
    }

    if (_mqtt.connected()) {
        _connected = true;
        _reconnectDelay = 2000;
        Serial.println("[MQTT] ✅ 已连接");

        // 订阅相关 topic
        _subscribeTopics();

        // 发布上线消息
        publishPresence(true);

        return true;
    } else {
        _connected = false;
        int state = _mqtt.state();
        Serial.printf("[MQTT] ❌ 连接失败 (state=%d)\n", state);
        return false;
    }
}

void MQTTHandler::disconnect() {
    if (_mqtt.connected()) {
        publishPresence(false);  // 发送离线通知
        _mqtt.disconnect();
    }
    _connected = false;
    Serial.println("[MQTT] 已断开");
}

bool MQTTHandler::isConnected() {
    return _mqtt.connected() && _connected;
}

void MQTTHandler::loop() {
    if (!_mqtt.connected()) {
        // 断线重连
        unsigned long now = millis();
        if (now - _lastReconnectAttempt >= _reconnectDelay) {
            _lastReconnectAttempt = now;
            if (connect(5000)) {
                Serial.println("[MQTT] 重连成功");
            } else {
                // 指数退避
                _reconnectDelay = min(_reconnectDelay * 2, (unsigned int)MQTT_RECONNECT_MAX_MS);
                Serial.printf("[MQTT] 重连失败，%u秒后重试\n", _reconnectDelay / 1000);
            }
        }
        _connected = false;
        return;
    }

    _connected = true;
    _mqtt.loop();  // 处理 incoming 消息 + keepalive
}

// ============================================================
// 发布方法
// ============================================================

void MQTTHandler::publishMessage(const char* content, const char* msgId,
                                  const char* toDevice, unsigned long timestamp) {
    JsonDocument doc;
    doc["msg_id"] = msgId;
    doc["to"] = toDevice;
    doc["content"] = content;
    doc["type"] = "preset";
    doc["timestamp"] = timestamp;

    String payload;
    serializeJson(doc, payload);

    String topic = topicMessageSend();
    _mqtt.publish(topic.c_str(), payload.c_str());
    Serial.printf("[MQTT] 消息已发送: %s\n", content);
}

void MQTTHandler::publishStatus(const char* status, unsigned long timestamp) {
    JsonDocument doc;
    doc["status"] = status;
    doc["timestamp"] = timestamp;

    String payload;
    serializeJson(doc, payload);

    String topic = topicStatus();
    _mqtt.publish(topic.c_str(), payload.c_str());
    Serial.printf("[MQTT] 状态已发布: %s\n", status);
}

void MQTTHandler::publishHeartbeat(unsigned long timestamp) {
    JsonDocument doc;
    doc["timestamp"] = timestamp;

    String payload;
    serializeJson(doc, payload);

    String topic = topicHeartbeat();
    _mqtt.publish(topic.c_str(), payload.c_str());
    Serial.println("[MQTT] ❤️ 心跳已发送");
}

void MQTTHandler::publishPresence(bool online) {
    JsonDocument doc;
    doc["online"] = online;

    String payload;
    serializeJson(doc, payload);

    String topic = topicPresence();
    _mqtt.publish(topic.c_str(), payload.c_str(), true);  // retain
    Serial.printf("[MQTT] 在线状态: %s\n", online ? "上线" : "离线");
}

// ============================================================
// Topic 路径
// ============================================================

String MQTTHandler::topicMessageSend() {
    return String(MQTT_TOPIC_PREFIX) + "/" + _deviceId + "/message/send";
}

String MQTTHandler::topicMessageReceive() {
    return String(MQTT_TOPIC_PREFIX) + "/" + _deviceId + "/message/receive";
}

String MQTTHandler::topicStatus() {
    return String(MQTT_TOPIC_PREFIX) + "/" + _deviceId + "/status";
}

String MQTTHandler::topicStatusNotify() {
    return String(MQTT_TOPIC_PREFIX) + "/" + _deviceId + "/status/notify";
}

String MQTTHandler::topicHeartbeat() {
    return String(MQTT_TOPIC_PREFIX) + "/" + _deviceId + "/heartbeat";
}

String MQTTHandler::topicHeartbeatNotify() {
    return String(MQTT_TOPIC_PREFIX) + "/" + _deviceId + "/heartbeat/notify";
}

String MQTTHandler::topicWeather() {
    return String(MQTT_TOPIC_PREFIX) + "/" + _deviceId + "/weather";
}

String MQTTHandler::topicPresence() {
    return String(MQTT_TOPIC_PREFIX) + "/" + _deviceId + "/presence";
}

String MQTTHandler::topicPresenceNotify() {
    return String(MQTT_TOPIC_PREFIX) + "/" + _deviceId + "/presence/notify";
}

// ============================================================
// 内部方法
// ============================================================

void MQTTHandler::_subscribeTopics() {
    // 订阅所有需要接收的 topic
    _mqtt.subscribe(topicMessageReceive().c_str(), 1);
    _mqtt.subscribe(topicStatusNotify().c_str(), 1);
    _mqtt.subscribe(topicHeartbeatNotify().c_str(), 1);
    _mqtt.subscribe(topicWeather().c_str(), 1);
    _mqtt.subscribe(topicPresenceNotify().c_str(), 1);

    Serial.println("[MQTT] 已订阅所有下行 topic");
}

void MQTTHandler::_mqttCallback(char* topic, byte* payload, unsigned int length) {
    // 将 payload 转为字符串
    char buf[512];
    unsigned int copyLen = min(length, (unsigned int)(sizeof(buf) - 1));
    memcpy(buf, payload, copyLen);
    buf[copyLen] = '\0';

    Serial.printf("[MQTT] 收到: %s <- %s\n", topic, buf);

    // 解析 JSON
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, buf);
    if (err) {
        Serial.printf("[MQTT] JSON 解析失败: %s\n", err.c_str());
        return;
    }

    // 调用用户回调
    if (_instance && _instance->_callback) {
        _instance->_callback(topic, doc);
    }
}
