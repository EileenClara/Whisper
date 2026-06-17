# SmallDesktopDisplay v2.0

ESP32-S3 情侣互动桌面显示器。两台设备通过 MQTT 通信，共享北京时间、双城天气、实时状态，支持发送爱心。

## 功能

- 🕐 **北京时间** — NTP 同步，48px 数码管显示
- 🌤️ **双城天气** — 北京 + 新加坡，OpenWeatherMap API
- 💬 **实时状态** — 8 种：Free / Play / Busy / MissU / Eat / Study / Sleep / Soccer
- 🎨 **对方状态图标** — 时钟左边显示对方当前状态图
- 💗 **爱心互动** — 摸一下发一颗，对方屏幕底部堆积，上限 99，接受后全部清空
- 📡 **MQTT 通信** — 东京 VPS 中转，离线消息 retained，重启不丢
- 📶 **WiFi 配网** — 连不上时自动开热点 AutoConnectAP，手机浏览器改密码
- 🔌 **电容触摸** — IO5 单键操作

## 硬件

| 组件 | 型号 |
|------|------|
| 主控 | ESP32-S3 |
| 屏幕 | ST7789 240×240 3线 SPI |
| 触摸 | 单键电容 (IO5) |
| 音频 | ES8311（本项目未使用） |

### 引脚

| 功能 | GPIO |
|------|:----:|
| MOSI | 1 |
| SCLK | 18 |
| DC | 17 |
| RST | 6 |
| CS | —（硬接地） |
| 背光 | 2 |
| 触摸键 | 5 |

## 快速开始

### 1. 克隆

```bash
git clone https://github.com/EileenClara/Whisper.git
```

### 2. 安装依赖库

Arduino IDE → 库管理器，搜索安装：

- **TFT_eSPI**（Bodmer）
- **TJpg_Decoder**（Bodmer）
- **PubSubClient**（Nick O'Leary）
- **ArduinoJson v7**（Benoit Blanchon）
- **Time**（Michael Margolis）

### 3. 配置 TFT_eSPI

将项目中的 `User_Setup.h` 覆盖到：
```
文档/Arduino/libraries/TFT_eSPI/User_Setup.h
```

### 4. 修 PubSubClient 包大小

编辑 `文档/Arduino/libraries/PubSubClient/src/PubSubClient.h`：
```cpp
#define MQTT_MAX_PACKET_SIZE 1024  // 原 256 → 1024
```

### 5. 配置 WiFi + VPS

编辑 `config.h`：
```cpp
#define MQTT_BROKER_HOST "你的VPS_IP"
```

编辑 `SmallDesktopDisplay.ino` 顶部：
```cpp
const char* WIFI_SSID = "你的WiFi名";
const char* WIFI_PWD  = "你的WiFi密码";
```

### 6. VPS 部署

在 VPS 上安装 Mosquitto 并上传天气代理：

```bash
# VPS 上
apt install mosquitto -y
# 复制 vps/mosquitto.conf 到 /etc/mosquitto/mosquitto.conf
systemctl restart mosquitto

pip3 install paho-mqtt requests
nohup python3 -u vps/weather_proxy.py > /var/log/sdd-weather.log 2>&1 &
```

### 7. 烧录

- 开发板：`ESP32S3 Dev Module`
- 分区方案：`Huge APP (3MB No OTA/1MB SPIFFS)`
- 端口：COM5
- 首次开机在串口输入 `1`（yg）或 `2`（vv）选择身份

## 操作

| 操作 | 主屏 | 状态选择页 |
|------|------|-----------|
| 短摸 (<0.8s) | 发送爱心 | 切下一个状态 |
| 长按 (>0.8s) | 打开状态选择 | — |
| 3秒不摸 | — | 自动确认返回 |
| 串口 `m` | 打开状态选择 | 输入数字选状态 |
| 串口 `h` | 发送爱心 | — |
| 串口 `w` | 刷新天气 | — |

## MQTT 协议

| 主题 | 方向 | 说明 |
|------|------|------|
| `sdd/{id}/status` | 发布 (retained) | 在线状态 |
| `sdd/{id}/mood` | 发布 (retained) | 0-7 状态值 |
| `sdd/{id}/heart/send` | 发布 (retained) | 发送爱心 `{count:N}` |
| `sdd/{id}/heart/ack` | 发布 | 接受爱心 |
| `sdd/+/weather/req` | 订阅 | 天气请求 |
| `sdd/weather/resp` | 订阅 | 天气响应 |

## 架构

```
ESP32-S3 (yg)  ←──MQTT/TLS──→  VPS Mosquitto  ←──MQTT/TLS──→  ESP32-S3 (vv)
                                    │
                            weather_proxy.py
                                    │
                            OpenWeatherMap API
```

## License

Based on SmallDesktopDisplay by Misaka / 微车游 (v1.4).  
v2.0 rewritten for ESP32-S3.
