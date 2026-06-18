[![zh](https://img.shields.io/badge/lang-%E4%B8%AD%E6%96%87-gray)](README_CN.md)
[![en](https://img.shields.io/badge/lang-en-red)](README.md)

# SmallDesktopDisplay v2.0

ESP32-S3 couple's interactive desktop display. Two devices communicate via MQTT, sharing Beijing time, dual-city weather, real-time status, and heart messaging.

## Features

- 🕐 **Beijing Time** — NTP synced, 48px digital clock
- 🌤️ **Dual-city Weather** — Beijing + Singapore via OpenWeatherMap API
- 💬 **Real-time Status** — 8 moods: Free / Play / Busy / MissU / Eat / Study / Sleep / Soccer
- 🎨 **Partner Mood Icon** — Shows partner's current mood icon beside the clock
- 💗 **Heart Messaging** — Tap to send. Hearts scatter randomly on partner's screen. No cooldown, max 52. Accept to clear all.
- 🎯 **Adaptive Touch** — Auto-calibrates threshold per device on boot
- 📡 **MQTT Communication** — Tokyo VPS relay with retained messages. Survives reboots.
- 📶 **WiFi Portal** — Auto-opens hotspot `AutoConnectAP` if WiFi fails. Phone browser to reconfigure.
- 🔌 **Capacitive Touch** — Single-button operation via IO5

## Hardware

| Component | Model |
|-----------|-------|
| MCU | ESP32-S3 |
| Display | ST7789 240×240 3-wire SPI |
| Touch | Single capacitive key (IO5) |
| Audio | ES8311 (unused) |

### Pinout

| Function | GPIO |
|----------|:----:|
| MOSI | 1 |
| SCLK | 18 |
| DC | 17 |
| RST | 6 |
| CS | — (GND) |
| Backlight | 2 |
| Touch Key | 5 |

## Quick Start

### 1. Clone

```bash
git clone https://github.com/EileenClara/Whisper.git
```

### 2. Install Libraries

Arduino IDE → Library Manager, install:

- **TFT_eSPI** (Bodmer)
- **TJpg_Decoder** (Bodmer)
- **PubSubClient** (Nick O'Leary)
- **ArduinoJson v7** (Benoit Blanchon)
- **Time** (Michael Margolis)

### 3. Configure TFT_eSPI

Overwrite the library's `User_Setup.h` with the one in this repo:
```
Documents/Arduino/libraries/TFT_eSPI/User_Setup.h
```

### 4. Increase PubSubClient Packet Size

Edit `Documents/Arduino/libraries/PubSubClient/src/PubSubClient.h`:
```cpp
#define MQTT_MAX_PACKET_SIZE 1024  // was 256
```

### 5. Set WiFi + VPS

In `config.h`:
```cpp
#define MQTT_BROKER_HOST "your-vps-ip"
```

In `SmallDesktopDisplay.ino`:
```cpp
const char* WIFI_SSID = "your-wifi";
const char* WIFI_PWD  = "your-password";
```

### 6. VPS Deployment

Install Mosquitto and upload the weather proxy:

```bash
# On VPS
apt install mosquitto -y
# Copy vps/mosquitto.conf to /etc/mosquitto/mosquitto.conf
systemctl restart mosquitto

pip3 install paho-mqtt requests
nohup python3 -u vps/weather_proxy.py > /var/log/sdd-weather.log 2>&1 &
```

### 7. Flash

- Board: `ESP32S3 Dev Module`
- Partition Scheme: `Huge APP (3MB No OTA/1MB SPIFFS)`
- On first boot, open Serial Monitor and type `1` (yougo) or `2` (vv) to set identity.

## Controls

| Action | Main Screen | Mood Picker |
|--------|-------------|-------------|
| Short tap (<0.8s) | Send heart | Next mood |
| Long press (>0.8s) | Open mood picker | — |
| 3s idle | — | Auto-confirm & return |
| Serial `m` | Open mood picker | Select by number |
| Serial `h` | Send heart | — |
| Serial `w` | Refresh weather | — |

## MQTT Protocol

| Topic | Direction | Description |
|-------|-----------|-------------|
| `sdd/{id}/status` | Publish (retained) | Online status |
| `sdd/{id}/mood` | Publish (retained) | Mood 0-7 |
| `sdd/{id}/heart/send` | Publish (retained) | Send heart `{count:N}` |
| `sdd/{id}/heart/ack` | Publish | Accept hearts |
| `sdd/+/weather/req` | Subscribe | Weather request |
| `sdd/weather/resp` | Subscribe | Weather response |

## Architecture

```
ESP32-S3 (yougo)  ←──MQTT──→  VPS Mosquitto  ←──MQTT──→  ESP32-S3 (vv)
                                    │
                            weather_proxy.py
                                    │
                            OpenWeatherMap API
```

## License

Based on SmallDesktopDisplay by Misaka / 微车游 (v1.4).  
v2.0 rewritten for ESP32-S3.
