/**
 * @file main.cpp
 * @brief Whisper / LoveBeacon — ESP32-C3 固件主程序
 *
 * 硬件: BeaconOps PCB (ESP32-C3 + ST7789 + LSM6DS3TR-C + MAX98357A)
 * 交互: 纯摇一摇（无物理按钮）
 *   - 单摇 → 进入心跳模式（3秒内再摇 = 发送❤️给对方）
 *   - 双摇（0.8秒内） → 打开快捷消息菜单（摇动切换，静置3秒发送）
 *   - 收到消息/心跳 → 亮屏 + 播放提示音（无震动）
 *
 * ============================================================
 * 编译说明:
 *   deviceA: pio run -e esp32-c3-devkitm-1 -t upload
 *   deviceB: 修改 src/config.h 中 DEVICE_ID 为 "deviceB" 再编译
 *
 *   或通过命令行:
 *   pio run -t upload --build-property "build_flags=-DDEVICE_ID=\"deviceB\""
 * ============================================================
 */

#include <Arduino.h>
#include <time.h>

#include "config.h"
#include "storage.h"
#include "wifi_manager.h"
#include "mqtt_handler.h"
#include "display.h"
#include "gyro.h"
#include "audio.h"

// ============================================================
// 全局对象
// ============================================================

Storage g_storage;
WiFiManager g_wifi;
MQTTHandler g_mqtt;
Display g_display;
Gyro g_gyro;
Audio g_audio;

// ============================================================
// 运行时状态
// ============================================================

// 对方最新状态
static String g_partnerStatus = "";
static String g_partnerStatusEmoji = "";

// 最新收到的消息
static String g_lastMessage = "";
static unsigned long g_lastMessageTime = 0;

// 天气数据
static float g_localTemp = 0;
static String g_localWeatherIcon = "";
static float g_partnerTemp = 0;
static String g_partnerWeatherIcon = "";
static unsigned long g_lastWeatherUpdate = 0;

// 当前本地状态
static String g_currentStatus = "love";  // 默认 "想你"

// 系统状态
static bool g_wifiConnected = false;
static bool g_mqttConnected = false;
static unsigned long g_lastNtpSync = 0;
static unsigned long g_lastScreenRefresh = 0;
static unsigned long g_lastStatusPublish = 0;

// 屏幕模式
enum ScreenMode {
    SCREEN_HOME,
    SCREEN_HEARTBEAT,
    SCREEN_MENU,
    SCREEN_HEART_ANIM,
    SCREEN_CONNECTING,
};
static ScreenMode g_screenMode = SCREEN_CONNECTING;
static unsigned long g_screenModeEnter = 0;

// 消息接收标记（用于触发UI更新）
static bool g_newMessageReceived = false;
static bool g_newHeartbeatReceived = false;
static int g_newHeartbeatCount = 0;

// ============================================================
// 函数声明
// ============================================================

// 网络初始化
bool initWiFi();
bool initNTP();
bool initMQTT();

// 时间工具
String getLocalTimeStr();
String getLocalDateStr();
String getPartnerTimeStr();
String getPartnerDateStr();
String getRelativeTimeStr(unsigned long timestamp);

// 消息处理回调
void onMQTTMessage(const char* topic, const JsonDocument& doc);

// 摇动事件处理
void handleShakeEvent(GyroEvent event);

// 屏幕更新
void updateScreen();

// ============================================================
// Setup
// ============================================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println();
    Serial.println("╔══════════════════════════════════╗");
    Serial.println("║       Whisper / LoveBeacon      ║");
    Serial.println("║   ESP32-C3 情侣通讯终端         ║");
    Serial.println("╚══════════════════════════════════╝");
    Serial.printf("设备ID: %s\n", DEVICE_ID);
    Serial.printf("名字: %s\n", MY_NAME);

    // 1. 存储初始化
    if (!g_storage.begin()) {
        Serial.println("[Main] ❌ NVS 初始化失败！");
    }

    // 2. 显示初始化
    g_display.begin();
    g_display.drawConnectingScreen("正在连接 WiFi...");

    // 3. 音频初始化
    g_audio.begin();
    g_audio.playBootTone();

    // 4. 陀螺仪初始化
    if (!g_gyro.begin(LSM6DS3_I2C_ADDR)) {
        Serial.println("[Main] ⚠️ 陀螺仪初始化失败，摇动功能不可用");
    }

    // 5. WiFi连接
    if (!initWiFi()) {
        Serial.println("[Main] ⚠️ WiFi连接失败，进入配网模式");
        g_display.drawWiFiSetupPrompt("Whisper-Setup");
        g_wifi.startConfigPortal("Whisper-Setup", 180);
        // 配网模式中等待连接
        unsigned long portalStart = millis();
        while (!g_wifi.isConnected() && (millis() - portalStart) < 180000) {
            g_wifi.loop();
            delay(500);
        }
        if (g_wifi.isConnected()) {
            g_wifiConnected = true;
        }
    } else {
        g_wifiConnected = true;
    }

    if (!g_wifiConnected) {
        Serial.println("[Main] ❌ 无WiFi连接，重启中...");
        delay(3000);
        ESP.restart();
    }

    // 6. NTP 时间同步
    g_display.drawConnectingScreen("正在同步时间...");
    if (initNTP()) {
        Serial.println("[Main] ✅ NTP时间已同步");
    }

    // 7. MQTT 连接
    g_display.drawConnectingScreen("正在连接服务器...");
    if (initMQTT()) {
        g_mqttConnected = true;
        Serial.println("[Main] ✅ MQTT已连接");
    }

    // 8. 发布初始状态
    g_lastStatusPublish = millis();
    g_mqtt.publishStatus(g_currentStatus.c_str(), time(nullptr));
    g_mqtt.publishPresence(true);

    // 进入主屏幕
    g_screenMode = SCREEN_HOME;
    g_screenModeEnter = millis();
    updateScreen();

    Serial.println("[Main] ✅ 初始化完成，进入主循环");
}

// ============================================================
// Loop
// ============================================================

void loop() {
    unsigned long now = millis();

    // ---- WiFi 维护 ----
    g_wifi.loop();
    bool wifiOk = g_wifi.isConnected();
    if (wifiOk != g_wifiConnected) {
        g_wifiConnected = wifiOk;
        if (wifiOk) {
            Serial.println("[Main] WiFi 已恢复");
        } else {
            Serial.println("[Main] WiFi 已断开");
        }
    }

    // ---- MQTT 维护 ----
    g_mqtt.loop();
    g_mqttConnected = g_mqtt.isConnected();

    // ---- NTP 定期同步 ----
    if (now - g_lastNtpSync > NTP_UPDATE_INTERVAL_MS) {
        initNTP();  // 内部会调用 configTime，安全
        g_lastNtpSync = now;
    }

    // ---- 陀螺仪事件处理（非菜单/心跳模式时） ----
    if (g_screenMode == SCREEN_HOME) {
        GyroEvent event = g_gyro.update();
        if (event != GYRO_NONE) {
            handleShakeEvent(event);
        }
    }

    // ---- 菜单模式处理 ----
    if (g_screenMode == SCREEN_MENU) {
        GyroEvent event = g_gyro.update();
        if (event == GYRO_DOUBLE_SHAKE) {
            // 菜单超时/确认 → 发送选中的消息
            int idx = g_gyro.getMenuIndex();
            if (idx >= 0 && idx < PRESET_MESSAGES_COUNT) {
                String msgId = String(millis(), HEX);
                g_mqtt.publishMessage(
                    PRESET_MESSAGES[idx],
                    msgId.c_str(),
                    (strcmp(DEVICE_ID, "deviceA") == 0) ? "deviceB" : "deviceA",
                    time(nullptr)
                );
                g_audio.playSendTone();
            }
            g_screenMode = SCREEN_HOME;
            g_screenModeEnter = now;
            updateScreen();
        }
        // 实时刷新菜单显示
        static int lastMenuIdx = -1;
        int curIdx = g_gyro.getMenuIndex();
        if (curIdx != lastMenuIdx) {
            lastMenuIdx = curIdx;
            g_display.drawMessageMenu(curIdx, PRESET_MESSAGES, PRESET_MESSAGES_COUNT);
        }
    }

    // ---- 心跳模式处理 ----
    if (g_screenMode == SCREEN_HEARTBEAT) {
        GyroEvent event = g_gyro.update();
        if (event == GYRO_SHAKE) {
            // 确认发送 ❤️
            g_mqtt.publishHeartbeat(time(nullptr));
            g_audio.playHeartbeatTone();
            Serial.println("[Main] ❤️ 心跳已发送!");

            g_screenMode = SCREEN_HOME;
            g_screenModeEnter = now;
            updateScreen();
        } else if (!g_gyro.isHeartbeatMode()) {
            // 超时取消
            g_screenMode = SCREEN_HOME;
            g_screenModeEnter = now;
            updateScreen();
        }
    }

    // ---- ❤️动画播放 ----
    if (g_screenMode == SCREEN_HEART_ANIM) {
        // 动画由 display.drawHeartAnimation() 内部阻塞完成
        // 倒计时显示
        static unsigned long heartAnimStart = 0;
        if (heartAnimStart == 0) {
            heartAnimStart = now;
        }
        if (now - heartAnimStart > 3500) {
            heartAnimStart = 0;
            g_screenMode = SCREEN_HOME;
            g_screenModeEnter = now;
            updateScreen();
        }
    }

    // ---- 新消息处理 ----
    if (g_newMessageReceived) {
        g_newMessageReceived = false;
        g_audio.playMessageTone();
        g_display.setBrightness(255);  // 亮屏
        g_screenMode = SCREEN_HOME;
        updateScreen();
    }

    // ---- 新心跳处理 ----
    if (g_newHeartbeatReceived) {
        g_newHeartbeatReceived = false;
        g_audio.playHeartbeatTone();
        g_display.setBrightness(255);
        g_screenMode = SCREEN_HEART_ANIM;
        g_display.drawHeartAnimation();
        g_screenMode = SCREEN_HOME;
        updateScreen();
    }

    // ---- 定期刷新屏幕 ----
    if (g_screenMode == SCREEN_HOME && (now - g_lastScreenRefresh > 10000)) {
        updateScreen();
        g_lastScreenRefresh = now;
    }

    // ---- 定期发布状态 ----
    if (now - g_lastStatusPublish > 60000) {
        g_mqtt.publishStatus(g_currentStatus.c_str(), time(nullptr));
        g_lastStatusPublish = now;
    }

    // ---- 屏幕超时变暗 ----
    if (g_screenMode == SCREEN_HOME) {
        unsigned long elapsed = now - g_screenModeEnter;
        if (elapsed > SCREEN_TIMEOUT_MS) {
            g_display.setBrightness(SCREEN_DIM_BRIGHTNESS);
        }
    }

    delay(50);  // 主循环节流
}

// ============================================================
// 网络初始化
// ============================================================

bool initWiFi() {
    Serial.println("[Main] 正在连接WiFi...");
    g_display.drawConnectingScreen("正在连接 WiFi...");

    // 尝试用已保存凭据连接
    String savedSSID = g_storage.getWiFiSSID();
    if (savedSSID.length() > 0) {
        Serial.printf("[Main] 尝试已保存网络: %s\n", savedSSID.c_str());
        if (g_wifi.connect(savedSSID.c_str(), g_storage.getWiFiPassword().c_str())) {
            return true;
        }
    }

    // 尝试默认凭据
    if (strlen(WIFI_SSID_DEFAULT) > 0) {
        Serial.printf("[Main] 尝试默认网络: %s\n", WIFI_SSID_DEFAULT);
        if (g_wifi.connect(WIFI_SSID_DEFAULT, WIFI_PASSWORD_DEFAULT)) {
            return true;
        }
    }

    return false;
}

bool initNTP() {
    // 设置时区
    setenv("TZ", MY_TZ_STRING, 1);
    tzset();

    // 使用 NTP 同步
    configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2, "time.google.com");

    // 等待同步（最多5秒）
    struct tm timeinfo;
    int retry = 0;
    while (!getLocalTime(&timeinfo) && retry < 50) {
        delay(100);
        retry++;
    }

    if (retry >= 50) {
        Serial.println("[NTP] ❌ 时间同步失败");
        return false;
    }

    time_t now;
    time(&now);
    Serial.printf("[NTP] ✅ 时间已同步: %s", ctime(&now));
    g_lastNtpSync = millis();
    return true;
}

bool initMQTT() {
    // 配置 MQTT
    g_mqtt.begin(
        MQTT_BROKER_HOST,
        MQTT_BROKER_PORT,
        MQTT_USERNAME,
        MQTT_PASSWORD,
        DEVICE_ID
    );

    // 设置消息回调
    g_mqtt.onMessage(onMQTTMessage);

    // 连接（最多重试3次）
    for (int i = 0; i < 3; i++) {
        if (g_mqtt.connect(10000)) {
            return true;
        }
        Serial.printf("[Main] MQTT 连接失败 (%d/3)，5秒后重试...\n", i + 1);
        delay(5000);
    }

    return false;
}

// ============================================================
// MQTT 消息处理回调
// ============================================================

void onMQTTMessage(const char* topic, const JsonDocument& doc) {
    String topicStr = String(topic);
    Serial.printf("[Main] 处理消息: %s\n", topic);

    // ---- 收到消息 ----
    if (topicStr.endsWith("/message/receive")) {
        const char* content = doc["content"] | "";
        const char* fromName = doc["from_name"] | "对方";
        unsigned long ts = doc["timestamp"] | 0;

        g_lastMessage = String(fromName) + ": " + String(content);
        g_lastMessageTime = ts;
        g_newMessageReceived = true;

        Serial.printf("[Main] 📩 新消息: %s\n", g_lastMessage.c_str());
    }

    // ---- 收到心跳 ----
    else if (topicStr.endsWith("/heartbeat/notify")) {
        g_newHeartbeatReceived = true;
        g_newHeartbeatCount++;
        Serial.printf("[Main] ❤️ 收到心跳! (总计 %d)\n", g_newHeartbeatCount);
    }

    // ---- 收到状态更新 ----
    else if (topicStr.endsWith("/status/notify")) {
        const char* emoji = doc["emoji"] | "❓";
        const char* label = doc["label"] | "未知";

        g_partnerStatusEmoji = String(emoji);
        g_partnerStatus = g_partnerStatusEmoji + " " + String(label);

        Serial.printf("[Main] 📊 对方状态更新: %s\n", g_partnerStatus.c_str());
    }

    // ---- 收到天气 ----
    else if (topicStr.endsWith("/weather")) {
        const char* city = doc["city"] | "";
        float temp = doc["temperature"] | 0.0f;
        const char* iconCode = doc["icon_code"] | "01d";

        // 判断是本地还是对方的天气
        if (strstr(city, MY_CITY_ID)) {
            g_localTemp = temp;
            g_localWeatherIcon = String(iconCode);
        } else {
            g_partnerTemp = temp;
            g_partnerWeatherIcon = String(iconCode);
        }

        g_lastWeatherUpdate = millis();
        Serial.printf("[Main] 🌤️ 天气更新: %s %.0f°C\n", city, temp);
    }

    // ---- 收到上下线通知 ----
    else if (topicStr.endsWith("/presence/notify")) {
        bool online = doc["online"] | false;
        const char* name = doc["device_name"] | "对方";
        Serial.printf("[Main] 📡 %s %s\n", name, online ? "上线了" : "离线了");
    }
}

// ============================================================
// 摇动事件处理
// ============================================================

void handleShakeEvent(GyroEvent event) {
    if (event == GYRO_SHAKE) {
        // 单摇 → 进入心跳模式
        Serial.println("[Main] 进入心跳模式");
        g_screenMode = SCREEN_HEARTBEAT;
        g_screenModeEnter = millis();
        g_display.drawHeartbeatPrompt();
        g_display.setBrightness(255);
    }
    else if (event == GYRO_DOUBLE_SHAKE) {
        // 双摇 → 进入快捷消息菜单
        Serial.println("[Main] 进入快捷消息菜单");
        g_screenMode = SCREEN_MENU;
        g_screenModeEnter = millis();
        g_display.drawMessageMenu(0, PRESET_MESSAGES, PRESET_MESSAGES_COUNT);
        g_display.setBrightness(255);
    }
}

// ============================================================
// 屏幕更新
// ============================================================

void updateScreen() {
    if (g_screenMode != SCREEN_HOME) return;

    String localTime = getLocalTimeStr();
    String localDate = getLocalDateStr();
    String partnerTime = getPartnerTimeStr();
    String partnerDate = getPartnerDateStr();
    String msgPreview = g_lastMessage;
    String msgTime = "";
    if (g_lastMessageTime > 0) {
        msgTime = getRelativeTimeStr(g_lastMessageTime);
    }

    g_display.drawHomeScreen(
        localTime.c_str(),
        localDate.c_str(),
        MY_CITY,
        g_localTemp,
        g_localWeatherIcon.c_str(),
        partnerTime.c_str(),
        partnerDate.c_str(),
        PARTNER_CITY,
        g_partnerTemp,
        g_partnerWeatherIcon.c_str(),
        g_partnerStatus.c_str(),
        msgPreview.c_str(),
        msgTime.c_str(),
        g_wifi.getRSSI()
    );

    g_lastScreenRefresh = millis();
}

// ============================================================
// 时间工具函数
// ============================================================

struct tm _getTimeInfo(const char* tzString) {
    time_t now;
    time(&now);

    // 临时切换时区
    char oldTZ[64] = {0};
    const char* currentTZ = getenv("TZ");
    if (currentTZ) strncpy(oldTZ, currentTZ, sizeof(oldTZ) - 1);

    setenv("TZ", tzString, 1);
    tzset();

    struct tm tm_info;
    localtime_r(&now, &tm_info);

    // 恢复时区
    if (oldTZ[0]) {
        setenv("TZ", oldTZ, 1);
    } else {
        setenv("TZ", MY_TZ_STRING, 1);
    }
    tzset();

    return tm_info;
}

String getLocalTimeStr() {
    struct tm ti = _getTimeInfo(MY_TZ_STRING);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", ti.tm_hour, ti.tm_min);
    return String(buf);
}

String getLocalDateStr() {
    struct tm ti = _getTimeInfo(MY_TZ_STRING);
    const char* weekdays[] = {"周日","周一","周二","周三","周四","周五","周六"};
    char buf[32];
    snprintf(buf, sizeof(buf), "%d月%d日 %s", ti.tm_mon + 1, ti.tm_mday, weekdays[ti.tm_wday]);
    return String(buf);
}

String getPartnerTimeStr() {
    struct tm ti = _getTimeInfo(PARTNER_TZ_STRING);
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", ti.tm_hour, ti.tm_min);
    return String(buf);
}

String getPartnerDateStr() {
    struct tm ti = _getTimeInfo(PARTNER_TZ_STRING);
    const char* weekdays[] = {"周日","周一","周二","周三","周四","周五","周六"};
    char buf[32];
    snprintf(buf, sizeof(buf), "%d月%d日 %s", ti.tm_mon + 1, ti.tm_mday, weekdays[ti.tm_wday]);
    return String(buf);
}

String getRelativeTimeStr(unsigned long timestamp) {
    time_t now;
    time(&now);
    long diff = now - (long)timestamp;

    if (diff < 60) return "刚刚";
    if (diff < 3600) return String(diff / 60) + "分钟前";
    if (diff < 86400) return String(diff / 3600) + "小时前";
    return String(diff / 86400) + "天前";
}
