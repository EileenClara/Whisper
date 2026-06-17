/*
 * SmallDesktopDisplay v2.0
 * 情侣互动桌面显示器 — ESP32-S3
 * 北京时间 | 双城天气 | 双人状态 | 爱心互动
 */

#include <ArduinoJson.h>
#include "config.h"
#include "storage_prefs.h"
#include "backlight.h"
#include "display_screen.h"
#include "touch_key.h"
#include "network_wifi.h"
#include "network_ntp.h"
#include "network_mqtt.h"
#include "ui_clock.h"
#include "ui_weather.h"
#include "weather_bridge.h"
#include "weathernum.h"
#include "app_identity.h"
// 对方状态图标
#include "img/mood_0.h"
#include "img/mood_1.h"
#include "img/mood_2.h"
#include "img/mood_3.h"
#include "img/mood_4.h"
#include "img/mood_5.h"
#include "img/mood_6.h"
#include "img/mood_7.h"
#include "ui_status.h"
#include "ui_heart.h"

// ===== WiFi 凭证 =====
const char* WIFI_SSID = "uskj302";
const char* WIFI_PWD  = "qwertyuiop01";

// ===== 主屏绘制 (全量) =====
void drawMainScreen() {
    TFT_eSPI& tft = DisplayScreen::tft();
    tft.fillScreen(TFT_BLACK);

    // 分隔线
    tft.drawLine(0, 76, 240, 76, 0x3186);   // 天气/时钟
    tft.drawLine(0, 158, 240, 158, 0x3186); // 时钟/状态
    tft.drawLine(0, 182, 240, 182, 0x3186); // 状态/爱心

    // 天气
    UIWeather::draw();
    // 对方状态图标 (时钟左边, 48x48)
    {
        const uint16_t* icons[] = {mood_0_img,mood_1_img,mood_2_img,mood_3_img,
                                   mood_4_img,mood_5_img,mood_6_img,mood_7_img};
        int pm = UIStatus::partnerMood();
        tft.setSwapBytes(true);
        tft.pushImage(4, 88, MOOD_W, MOOD_H, icons[pm]);
        tft.setSwapBytes(false);
    }
    // 时钟
    UIClock::draw();
    // 状态
    UIStatus::drawStatusBar();
    // 爱心
    UIHeart::draw();

    DisplayScreen::clearDirty();
}

// ===== MQTT 消息处理 =====
void onMqttMessage(const String& topic, const String& payload) {
    // 对方状态更新
    if (topic.endsWith("/mood")) {
        JsonDocument doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            uint8_t mood = doc["mood"] | 0;
            UIStatus::setPartnerMood(mood);
            UIStatus::setPartnerOnline(true);
        }
    }

    // 对方上线
    if (topic.endsWith("/status")) {
        JsonDocument doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            bool online = doc["online"] | false;
            UIStatus::setPartnerOnline(online);
            if (online) {
                uint8_t mood = doc["mood"] | 0;
                UIStatus::setPartnerMood(mood);
            }
        }
    }

    // 收到爱心
    if (topic.endsWith("/heart/send")) {
        UIHeart::onHeartReceived(payload);
    }

    // 对方接受了爱心
    if (topic.endsWith("/heart/ack")) {
        UIHeart::onHeartAck(payload);
    }

    // 天气数据
    if (topic.endsWith("/weather/resp")) {
        WeatherBridge::handleResponse(payload);
        UIWeather::update();
    }
}

// ===== setup =====
void setup() {
    Serial.begin(921600);
    delay(500);

    Serial.println("\n====================================");
    Serial.println("  SmallDesktopDisplay v2.0 - ESP32S3");
    Serial.println("====================================");

    // 1. 存储
    StoragePrefs::begin();

    // 2. 背光
    Backlight::begin();
    Backlight::set(StoragePrefs::getBrightness());

    // 3. 屏幕
    DisplayScreen::begin();
    Serial.println("[Setup] Display OK");

    // 4. 触摸按键 (IO5 电容)
    TouchKey::begin();
    Serial.println("[Setup] Touch Key OK (IO5)");

    // 5. 设备身份
    AppIdentity::begin();

    // 6. 首次开机 → 身份选择页
    if (!AppIdentity::isSet()) {
        currentPage = PAGE_IDENTITY;
        AppIdentity::drawPicker();
        Serial.println("[Setup] Select: 1=yougo  2=vv");

        // 等待选择 (串口)
        while (!AppIdentity::isSet()) {
            // 串口备选
            if (Serial.available()) {
                char c = Serial.read();
                if (c == '1') { AppIdentity::set(ID_GUGU); Serial.println("-> yougo"); }
                if (c == '2') { AppIdentity::set(ID_VV);   Serial.println("-> vv"); }
            }
            delay(20);
        }

        // 选择完毕 → 画主屏
        drawMainScreen();
        Serial.printf("[Setup] Identity: %s\n", AppIdentity::name());
    } else {
        drawMainScreen();
        Serial.printf("[Setup] Identity: %s (saved)\n", AppIdentity::name());
    }

    // 7. WiFi
    NetworkWiFi::begin(WIFI_SSID, WIFI_PWD);

    // 8. NTP
    NetworkNTP::begin();

    // 9. MQTT (需要 WiFi 先连上)
    // MQTT 在 loop 里自动连接

    // 10. UI 模块
    UIClock::begin();
    UIWeather::begin();
    UIStatus::begin();
    UIHeart::begin();
    WeatherBridge::begin();

    Serial.println("[Setup] Complete!");
}

// ===== loop =====
void loop() {
    // 始终运行
    NetworkWiFi::loop();

    // WiFi 刚连上 (从 portal 退出) → 重绘主屏
    if (NetworkWiFi::justConnected()) {
        drawMainScreen();
    }

    // 每 10 秒输出状态
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 10000) {
        lastStatus = millis();
        Serial.printf("[STATUS] WiFi=%d MQTT=%d IP=%s\n",
            WiFi.status(), NetworkMQTT::isConnected(),
            WiFi.localIP().toString().c_str());
    }

    // 触摸按键
    TapType tap = TouchKey::check();

    if (currentPage == PAGE_MAIN) {
        if (tap == TAP_SHORT) {
            UIHeart::onTap();
        }
        else if (tap == TAP_LONG) {
            UIStatus::showMoodPicker();
        }
    }
    else if (currentPage == PAGE_MOOD_PICKER) {
        if (tap == TAP_SHORT) {
            UIStatus::pickerNext();
        }
        if (UIStatus::pickerShouldConfirm()) {
            UIStatus::pickerConfirm();
            drawMainScreen();
        }
    }

    // WiFi 连上后才运行这些
    if (NetworkWiFi::isConnected()) {
        // MQTT 初始化 + 连接
        static bool mqttInited = false;
        if (!mqttInited) {
            NetworkMQTT::begin(AppIdentity::name());
            NetworkMQTT::onMessage(onMqttMessage);
            mqttInited = true;
        }

        NetworkMQTT::loop();
        NetworkNTP::loop();
        WeatherBridge::loop();
    }

    // NTP 同步后更新时钟
    static bool wasSynced = false;
    if (NetworkNTP::isSynced() && !wasSynced) {
        wasSynced = true;
        UIClock::draw();
    }

    // 爱心动画
    UIHeart::loop();

    // ===== 串口命令 =====
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (currentPage == PAGE_MAIN) {
            if (cmd == "m" || cmd == "M") {
                UIStatus::showMoodPicker();
            }
            else if (cmd == "h" || cmd == "H") {
                UIHeart::onTap();
            }
            else if (cmd == "w" || cmd == "W") {
                WeatherBridge::forceRefresh();
            }
            else if (cmd == "help") {
                Serial.println("m=mood h=heart w=weather");
            }
        }
        else if (currentPage == PAGE_MOOD_PICKER) {
            // 串口输入数字也能选
            int mood = cmd.toInt();
            if (cmd.length() == 1 && mood >= 0 && mood < MOOD_COUNT) {
                UIStatus::setOwnMood(mood);
                UIStatus::publishOwnMood();
                UIStatus::hideMoodPicker();
                drawMainScreen();
            }
        }
    }

    // ===== 按需刷新 (脏区域) =====
    if (currentPage == PAGE_MAIN) {
        UIClock::update();
        UIWeather::update();

        if (DisplayScreen::dirtyZones() != 0) {
            if (DisplayScreen::isDirty(ZONE_STATUS)) {
                UIStatus::drawStatusBar();
            }
            if (DisplayScreen::isDirty(ZONE_HEART)) {
                UIHeart::draw();
            }
            DisplayScreen::clearDirty();
        }
    }

    delay(20);
}
