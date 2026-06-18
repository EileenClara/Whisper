/*
 * config.h — 项目全局配置
 * 所有编译时常量、引脚定义、默认值集中在这里
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================
// 版本
// ============================
#define VERSION_STRING  "SDD v2.0"
#define VERSION_DATE    "2025-06"

// ============================
// 屏幕
// ============================
#define SCREEN_WIDTH    240
#define SCREEN_HEIGHT   240

// ============================
// 引脚定义 (已验证可用)
// ============================
#define PIN_TFT_MOSI    1
#define PIN_TFT_SCLK    18
#define PIN_TFT_DC      17
#define PIN_TFT_RST     6
#define PIN_TFT_BL      2

// 触摸 (XPT2046, 老板配置暗示 CS=21)
#define PIN_TOUCH_CS    21
#define PIN_TOUCH_IRQ   -1   // 不用中断

// ============================
// 背光 LEDC
// ============================
#define LEDC_CH_BL      0
#define LEDC_FREQ_BL    5000
#define LEDC_RES_BL     8
#define BL_DEFAULT_PCT  80   // 默认亮度 80%

// ============================
// WiFi
// ============================
#define WIFI_CONNECT_TIMEOUT_MS   20000   // 连接超时 20 秒
#define WIFI_RECONNECT_INTERVAL_MS 10000  // 重连间隔 10 秒
#define WIFI_PORTAL_TIMEOUT_SEC   180     // 配网超时 3 分钟

// ============================
// MQTT
// ============================
#define MQTT_BROKER_HOST    "198.13.56.214"
#define MQTT_BROKER_PORT    1883   // 先不用 TLS
#define MQTT_KEEPALIVE      30
#define MQTT_USERNAME        ""                   // 设备 ID 作为用户名
#define MQTT_PASSWORD        ""                   // MQTT 密码

// ============================
// NTP
// ============================
#define NTP_SERVER          "ntp6.aliyun.com"
#define NTP_SYNC_INTERVAL   300    // 5 分钟同步一次
#define NTP_TIMEZONE        8      // UTC+8 北京时间

// ============================
// 天气
// ============================
#define WEATHER_INTERVAL_MS     (10 * 60 * 1000)  // 10 分钟刷新
#define WEATHER_STALE_MS        (30 * 60 * 1000)  // 超过 30 分钟标记为旧数据

// ============================
// 爱心
// ============================
#define HEART_COOLDOWN_MS       (30 * 60 * 1000)  // 发送冷却 30 分钟
#define HEART_MAX_COUNT         52  // 最大爱心数
#define HEART_TIMEOUT_MS        (120 * 1000)      // 单颗发送后无回应 2 分钟 (目前不自动过期)

// ============================
// 状态
// ============================
// 8 种心情状态
enum Mood : uint8_t {
    MOOD_FREE     = 0,  // 空闲
    MOOD_GAMING   = 1,  // 游戏
    MOOD_BUSY     = 2,  // 忙碌
    MOOD_MISS_YOU = 3,  // 想你
    MOOD_EATING   = 4,  // 干饭
    MOOD_STUDYING = 5,  // 学习
    MOOD_SLEEPING = 6,  // 睡觉
    MOOD_SOCCER   = 7   // 踢球
};

// 状态中文名 (索引 = Mood 值) — 不能用 PROGMEM!
static const char* const MOOD_NAMES[] = {
    "空闲", "游戏", "忙碌", "想你",
    "干饭", "学习", "睡觉", "踢球"
};

// 状态颜色 (RGB565) — 左暖右冷配色
static const uint16_t MOOD_COLORS[] = {
    0xA79C,  // 空闲 Free    #A5F3E4 薄荷绿
    0x87FB,  // 游戏 Game    #80FFDB 亮青
    0xFF96,  // 忙碌 Busy    #FFF3B0 暖黄
    0xDD37,  // 想你 MissU   #D8A7B8 淡裸粉
    0xCF39,  // 干饭 Eat     #C8E6C9 柔和绿
    0xD63D,  // 学习 Study   #D1C4E9 薰衣草
    0xB5DC,  // 睡觉 Sleep   #B0B9E6 灰蓝
    0xB75E   // 踢球 Soccer  #B2EBF2 青绿
};

#define MOOD_COUNT  8

// ============================
// 触摸去抖
// ============================
#define TOUCH_DEBOUNCE_MS   200

// ============================
// 界面刷新
// ============================
#define UI_REFRESH_MS       50    // 20fps

#endif // CONFIG_H
