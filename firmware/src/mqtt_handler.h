/**
 * @file mqtt_handler.h
 * @brief Whisper MQTT TLS 通讯模块
 *
 * 使用 PubSubClient + WiFiClientSecure 连接 Mosquitto TLS 8883 端口
 * 功能:
 *  - 自动连接 + 指数退避重连
 *  - 发布消息 / 状态 / 心跳
 *  - 订阅对方消息 / 状态 / 心跳 / 天气 / 在线通知
 *  - JSON 序列化/反序列化
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/** MQTT 收到消息的回调函数类型 */
typedef std::function<void(const char* topic, const JsonDocument& doc)> MQTTCallback;

class MQTTHandler {
public:
    MQTTHandler();
    ~MQTTHandler();

    /** 设置 TLS 连接 + MQTT 客户端 */
    bool begin(const char* brokerHost, uint16_t brokerPort,
               const char* username, const char* password,
               const char* deviceId);

    /** 设置收到消息的回调 */
    void onMessage(MQTTCallback callback);

    /** 连接MQTT Broker（阻塞，带重试） */
    bool connect(unsigned long timeoutMs = 10000);

    /** 断开连接 */
    void disconnect();

    /** 检查是否已连接 */
    bool isConnected();

    /** 主循环处理（必须频繁调用以维持 MQTT 连接并处理 incoming 消息） */
    void loop();

    // ---- 发布方法 ----

    /** 发送消息给服务器 */
    void publishMessage(const char* content, const char* msgId,
                        const char* toDevice, unsigned long timestamp);

    /** 发布状态 */
    void publishStatus(const char* status, unsigned long timestamp);

    /** 发布心跳❤️ */
    void publishHeartbeat(unsigned long timestamp);

    /** 发布在线/离线 */
    void publishPresence(bool online);

    // ---- 订阅路径构造 ----

    String topicMessageSend();
    String topicMessageReceive();
    String topicStatus();
    String topicStatusNotify();
    String topicHeartbeat();
    String topicHeartbeatNotify();
    String topicWeather();
    String topicPresence();
    String topicPresenceNotify();

private:
    WiFiClientSecure _wifiClient;
    PubSubClient _mqtt;
    MQTTCallback _callback = nullptr;

    String _brokerHost;
    uint16_t _brokerPort = 8883;
    String _username;
    String _password;
    String _deviceId;

    bool _connected = false;
    unsigned long _lastReconnectAttempt = 0;
    unsigned int  _reconnectDelay = 2000;

    /** 执行订阅 */
    void _subscribeTopics();

    /** 内部回调转发 */
    static void _mqttCallback(char* topic, byte* payload, unsigned int length);

    /** 静态实例指针（用于静态回调转发） */
    static MQTTHandler* _instance;
};

#endif // MQTT_HANDLER_H
