/**
 * @file wifi_manager.h
 * @brief Whisper WiFi 连接管理
 *
 * 首次启动或无已保存凭据时，开启 AP 配网模式（手机连热点配置WiFi）
 * 网络断开自动重连，指数退避
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiManager {
public:
    WiFiManager();
    ~WiFiManager();

    /** 连接WiFi（优先用已保存凭据，无则进入配网模式） */
    bool connect(const char* ssid = nullptr, const char* password = nullptr,
                 unsigned long timeoutMs = 15000);

    /** 断开WiFi */
    void disconnect();

    /** 检查是否已连接 */
    bool isConnected();

    /** 获取当前信号强度 (RSSI dBm) */
    int getRSSI();

    /** 获取本机 IP */
    String getLocalIP();

    /** 启动配网模式（AP模式，手机连接热点配网） */
    bool startConfigPortal(const char* apName = "Whisper-Setup",
                           unsigned long timeoutSec = 180);

    /** 处理WiFi状态变化（在主循环中调用，用于断线重连） */
    void loop();

    /** 获取WiFi状态文本描述 */
    static String getStatusText(wl_status_t status);

private:
    bool _connected = false;
    unsigned long _lastReconnectAttempt = 0;
    unsigned int  _reconnectDelay = 2000;   // 初始2秒
    const unsigned int _reconnectBase = 2000;
    const unsigned int _reconnectMax = 300000;  // 最大5分钟
    static const int _maxReconnectAttempts = 10;
    int _reconnectCount = 0;

    /** 真正的连接逻辑 */
    bool _doConnect(const char* ssid, const char* password, unsigned long timeoutMs);

    /** 尝试重连 */
    bool _tryReconnect();
};

#endif // WIFI_MANAGER_H
