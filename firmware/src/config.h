/**
 * @file config.h
 * @brief Whisper / LoveBeacon — 设备配置文件
 *
 * 硬件引脚兼容 BeaconOps PCB (ESP32-C3 + ST7789 + LSM6DS3TR-C + MAX98357A)
 * 编译前请修改下方 <<用户配置>> 区域
 *
 * 编译 deviceA:  #define DEVICE_ID "deviceA"
 * 编译 deviceB:  #define DEVICE_ID "deviceB"
 *
 * 或通过 PlatformIO build_flags 传入:
 *   pio run -t upload --build-property "build_flags=-DDEVICE_ID=\"deviceA\""
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// << 用户配置 — 编译前必须修改 >>
// ============================================================

// 设备身份（deviceA 或 deviceB）
#ifndef DEVICE_ID
#define DEVICE_ID               "deviceA"   // 编译另一台时改为 "deviceB"
#endif

// ---- WiFi 配置 ----
// 设备首次使用时通过 WiFiManager 配网（手机连设备热点）
// 但如果想硬编码 WiFi，可以填在这里（不填则自动进入配网模式）
#define WIFI_SSID_DEFAULT       ""          // 留空 = 首次启动进入配网模式
#define WIFI_PASSWORD_DEFAULT   ""

// ---- MQTT Broker 地址 ----
// 替换为你的 VPS 域名或 IP
#define MQTT_BROKER_HOST        "198.13.56.214"
#define MQTT_BROKER_PORT        8883        // TLS 端口
#define MQTT_USERNAME           "lovebeacon"  // 与 mosquitto.conf 对应
#define MQTT_PASSWORD           "Tyg.2005315"

// ---- MQTT Topic 基础前缀 ----
#define MQTT_TOPIC_PREFIX       "lovebeacon"

// ---- TLS 证书 ----
// Let's Encrypt 根证书（ISRG Root X1），内置在固件中
// 如果使用自签名证书，需要替换为你的 CA 证书
// 见 firmware/src/certs.h

// ---- 设备个人信息 ----
#if DEVICE_ID == "deviceA"
    #define MY_NAME             "果果"
    #define PARTNER_NAME        "vv"
    #define MY_CITY             "Beijing"
    #define MY_CITY_ID          "Beijing,CN"
    #define MY_TZ_STRING        "CST-8"         // 中国标准时间 UTC+8
    #define PARTNER_CITY        "Singapore"
    #define PARTNER_CITY_ID     "Singapore,SG"
    #define PARTNER_TZ_STRING   "<+08>-8"       // 新加坡 UTC+8
#else  // deviceB
    #define MY_NAME             "vv"
    #define PARTNER_NAME        "果果"
    #define MY_CITY             "Singapore"
    #define MY_CITY_ID          "Singapore,SG"
    #define MY_TZ_STRING        "<+08>-8"       // 新加坡 UTC+8
    #define PARTNER_CITY        "Beijing"
    #define PARTNER_CITY_ID     "Beijing,CN"
    #define PARTNER_TZ_STRING   "CST-8"         // 中国标准时间 UTC+8
#endif

// ---- 天气配置 ----
#define WEATHER_UPDATE_INTERVAL_MS   (30 * 60 * 1000)  // 30分钟

// ---- NTP 时间同步 ----
#define NTP_SERVER_1            "ntp.aliyun.com"
#define NTP_SERVER_2            "pool.ntp.org"
#define NTP_UPDATE_INTERVAL_MS  (60 * 60 * 1000)       // 每小时同步一次

// ---- 屏幕设置 ----
#define SCREEN_TIMEOUT_MS       (30 * 1000)  // 30秒无操作后屏幕变暗（不关，调低背光）
#define SCREEN_DIM_BRIGHTNESS   10           // 暗屏时背光亮度 (0-255)

// ---- 摇动检测阈值 ----
#define SHAKE_THRESHOLD         1500        // 摇动加速度阈值 (mg)，越低越灵敏
#define SHAKE_DEBOUNCE_MS       2500        // 摇动冷却时间（防重复触发）
#define SHAKE_HEARTBEAT_WINDOW  3000        // 心跳模式窗口（摇一下后3秒内再摇=发❤️）
#define SHAKE_DOUBLE_WINDOW     800         // 双摇检测窗口（ms内两次摇动=进入菜单）
#define SHAKE_MENU_TIMEOUT      3000        // 菜单模式中静置超时(ms)，自动发送当前选中消息

// ---- 预设快捷消息 ----
#define PRESET_MESSAGES_COUNT   6
static const char* PRESET_MESSAGES[] = {
    "想你了💕",
    "晚安🌙",
    "早安☀️",
    "在忙",
    "等我一下",
    "我爱你❤️",
};

// ---- 状态选项 ----
#define STATUS_OPTIONS_COUNT    6
struct StatusOption {
    const char* key;
    const char* emoji;
    const char* label;
};
static const StatusOption STATUS_OPTIONS[] = {
    {"play",   "🎮", "出去玩"},
    {"study",  "📚", "学习中"},
    {"relax",  "🛋️", "休闲"},
    {"sleep",  "😴", "睡觉"},
    {"love",   "💕", "想你"},
    {"eat",    "🍽️", "吃饭"},
};

// ============================================================
// 硬件引脚定义（与 BeaconOps PCB 一致，请勿修改）
// ============================================================

// ---- I2C 总线 ----
#define I2C_SCL_PIN             GPIO_NUM_2
#define I2C_SDA_PIN             GPIO_NUM_8
#define I2C_FREQUENCY           100000

// ---- SPI 屏幕 (ST7789 172×320) ----
#define TFT_MOSI                GPIO_NUM_7
#define TFT_SCLK                GPIO_NUM_6
#define TFT_CS                  GPIO_NUM_5
#define TFT_DC                  GPIO_NUM_4
#define TFT_RST                 -1          // GPIO_NUM_NC（无复位引脚）
#define TFT_BL                  GPIO_NUM_9  // 背光PWM
#define TFT_WIDTH               172
#define TFT_HEIGHT              320
#define TFT_ROTATION            0           // USB口朝下 = 0；朝上 = 2
#define TFT_SPI_FREQ            63500000

// ---- I2S 音频 (MAX98357A) ----
#define I2S_BCK_PIN             GPIO_NUM_1
#define I2S_LRCK_PIN            GPIO_NUM_0
#define I2S_DIN_PIN             GPIO_NUM_3
#define AUDIO_ENABLE_PIN        GPIO_NUM_10  // 音频功放使能

// ---- 充电检测 ----
#define CHARGER_CHRG_PIN        GPIO_NUM_20  // 充电中（低有效）
#define CHARGER_STDBY_PIN       GPIO_NUM_21  // 充电完成（低有效）

// ---- LSM6DS3TR-C I2C地址 ----
#define LSM6DS3_I2C_ADDR        0x6A        // SA0=GND 时为 0x6A; SA0=VDD 时为 0x6B

// ============================================================
// 软件配置常量
// ============================================================

// ---- WiFi 重连 ----
#define WIFI_CONNECT_TIMEOUT_MS     15000
#define WIFI_RETRY_INTERVAL_MS      10000
#define WIFI_MAX_RETRY              5

// ---- MQTT 重连 ----
#define MQTT_RECONNECT_BASE_MS      2000    // 基础重连延迟
#define MQTT_RECONNECT_MAX_MS       300000  // 最大重连延迟（5分钟）
#define MQTT_KEEPALIVE_S            60

// ---- NVS 命名空间 ----
#define NVS_NAMESPACE               "whisper"
#define NVS_KEY_DEVICE_NAME         "dev_name"
#define NVS_KEY_PARTNER_NAME        "partner"
#define NVS_KEY_CITY                "city"
#define NVS_KEY_TZ                  "timezone"
#define NVS_KEY_WIFI_SSID           "wifi_ssid"
#define NVS_KEY_WIFI_PASS           "wifi_pass"

// ---- 默认值 ----
#define DEFAULT_HOSTNAME            "Whisper"

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
