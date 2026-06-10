# Whisper — 异地情侣通讯终端

基于 [BeaconOps](https://github.com/Micro-Mood/BeaconOps) 硬件的情侣专属通讯设备。

```
┌─────────────┐     MQTT/TLS      ┌──────────────────┐     MQTT/TLS      ┌─────────────┐
│ 设备A (北京)  │ ◄──────────────► │  东京 VPS         │ ◄──────────────► │ 设备B (新加坡)│
│  ESP32-C3    │                  │  Mosquitto+FastAPI│                  │  ESP32-C3   │
└─────────────┘                   └──────────────────┘                   └─────────────┘
```

---

## 功能

| 功能 | 说明 |
|------|------|
| ❤️ **摇一摇心跳** | 摇一摇 → 3秒内再摇 → 发送❤️，对方全屏❤️动画 |
| 📊 **状态同步** | 出去玩🎮 / 学习中📚 / 休闲🛋️ / 睡觉😴 / 想你💕 / 吃饭🍽️ |
| 🌤️ **双城天气** | 北京 + 新加坡实时温度天气 |
| 🕐 **时钟** | NTP 自动同步 |
| 🔇 **静音模式** | 屏幕朝下放3秒切换，翻回朝上才能再次切换 |
| 🔐 **TLS 加密** | MQTT over TLS 8883，自签名证书 |

---

## 交互方式

设备**没有物理按钮**，3 个手势：

| 手势 | 怎么操作 | 效果 |
|------|----------|------|
| 🔀 **摇一摇** | 晃动设备 | 进入心跳模式，"摇一摇发送❤️" |
| 🔀 **再摇** | 3秒内再次摇动 | 发送❤️给对方 |
| 🕐 **不摇了** | 3秒无操作 | 取消，回主屏 |
| 👆 ×4 **敲四下** | 2秒内敲设备背面4次 | 打开状态菜单 |
| 👆 **敲一下** | 状态菜单中敲击 | 切换状态选项 |
| 🕐 **静置** | 菜单中3秒不动 | 确认当前状态，同步给对方 |
| 📱 **屏幕朝下** | 扣桌上3秒 | 切换静音 🔇，翻回朝上解锁 |

---

## 主屏幕

```
┌──────────────────────────────┐
│  🔊               📶░░░  🔋  │
│                              │
│          14:30               │  时间
│       6月10日 周三            │
│                              │
│  北京 🌤 32°    新加坡 🌤 28°│  双城天气
│                              │
│  ┌──────────┐ ┌──────────┐  │
│  │ 💕 想你  │ │ 📚 学习中 │  │  我的状态 │ 对方状态
│  └──────────┘ └──────────┘  │
│                              │
│            ❤️                │  上次心跳
│           刚刚               │
│──────────────────────────────│
│  摇一摇 ❤️       敲四下 状态  │
└──────────────────────────────┘
```

---

## 硬件

复用 [BeaconOps PCB](https://oshwhub.com/httppp/project_owrzrbgf)，**零改动**：

| 组件 | 型号 |
|------|------|
| 主控 | ESP32-C3 (QFN-32) |
| 屏幕 | 1.47" IPS ST7789 172×320 SPI |
| 陀螺仪 | ST LSM6DS3TR-C (I2C) |
| 音频 | MAX98357A I2S 功放 |
| 电量计 | CW2017 (I2C) |
| 电源 | USB-C 充电 + 锂电池 |
| PCB | 36.28×19.39mm 双层板 |

> ⚠️ LGA-14 和 WLP-9 是无引脚封装，需要回流焊。

---

## 项目结构

```
Whisper/
├── README.md
├── server/
│   ├── install.sh              # 一键安装（自动生成自签名证书）
│   ├── requirements.txt
│   ├── config.json             # 服务端配置（API Key / 密码 / 城市）
│   ├── mosquitto.conf          # Mosquitto TLS 配置
│   ├── main.py                 # FastAPI 入口
│   ├── models.py               # 数据模型
│   ├── database.py             # SQLite
│   ├── mqtt_bridge.py          # MQTT 消息路由
│   └── weather_service.py      # OpenWeatherMap 定时拉取推送
│
└── firmware/
    ├── platformio.ini          # PlatformIO（ESP32-C3 + Arduino）
    └── src/
        ├── main.cpp            # 主程序 + 状态机
        ├── config.h            # 引脚 + 配置（编译前修改）
        ├── storage.h/cpp       # NVS 持久化
        ├── wifi_manager.h/cpp  # WiFi + WiFiManager 配网
        ├── mqtt_handler.h/cpp  # MQTT TLS + JSON
        ├── display.h/cpp       # ST7789 屏幕 + UI
        ├── gyro.h/cpp          # 手势检测（摇动+敲击）
        └── audio.h/cpp         # I2S 提示音
```

---

## 部署

### 第一步：服务端（VPS）

```bash
git clone https://github.com/EileenClara/Whisper.git
cd Whisper/server

# 编辑 install.sh:
#   USE_SELF_SIGNED=true
#   SERVER_IP="198.13.56.214"
#   MQTT_PASS="你的MQTT密码"

# 编辑 config.json:
#   weather.api_key        → OpenWeatherMap API Key
#   devices.deviceA.name   → 你的名字
#   devices.deviceB.name   → 对方的名字
#   mqtt.password          → 与 install.sh 一致

chmod +x install.sh
sudo ./install.sh

# 验证
sudo systemctl status mosquitto
sudo systemctl status lovebeacon
curl http://localhost:8000/health
```

`install.sh` 运行后会打印自签名 CA 证书，复制备用。

### 第二步：固件

```bash
# VSCode + PlatformIO 打开 firmware/ 文件夹

# 1. 修改 src/config.h:
#    MQTT_BROKER_HOST → "198.13.56.214"
#    MQTT_PASSWORD     → 与 config.json 一致

# 2. 修改 src/mqtt_handler.cpp:
#    把 CA 证书粘贴到 CA_CERT 占位符

# 3. TFT_eSPI 引脚配置:
#    编辑 .pio/libdeps/.../TFT_eSPI/User_Setup_Select.h
#    注释所有 include，末尾加:
#      #define ST7789_DRIVER
#      #define TFT_WIDTH  172  /  #define TFT_HEIGHT 320
#      #define TFT_MOSI 7  /  #define TFT_SCLK 6
#      #define TFT_CS 5    /  #define TFT_DC 4
#      #define TFT_RST -1  /  #define TFT_BL 9
#      #define SPI_FREQUENCY 63500000

# 4. 编译 deviceA:
pio run -e esp32-c3-devkitm-1 -t upload

# 5. 改 DEVICE_ID="deviceB"，编译 deviceB
```

### 第三步：WiFi 配网

```
首次开机 → 无保存的WiFi
  └─ 自动开启热点 "Whisper-Setup"
    └─ 手机连上 → 自动弹出配网页面
      └─ 选WiFi输密码 → 设备保存 → 自动重启
```

之后自动连接。

---

## MQTT Topic

```
lovebeacon/{device_id}/heartbeat           # 设备上行：发❤️
lovebeacon/{device_id}/heartbeat/notify    # 设备下行：收到❤️
lovebeacon/{device_id}/status              # 设备上行：状态更新
lovebeacon/{device_id}/status/notify       # 设备下行：对方状态
lovebeacon/{device_id}/weather             # 服务端下行：天气推送
lovebeacon/{device_id}/presence            # LWT：上下线
lovebeacon/{device_id}/presence/notify     # 服务端下行：对方上下线
```

---

## 故障排查

| 问题 | 检查 |
|------|------|
| 屏幕不亮 | 背光 GPIO9 焊接；`TFT_ROTATION` |
| 连不上WiFi | 2.4GHz；串口日志 |
| 配网页面不弹 | 随机MAC；手动 192.168.4.1 |
| MQTT连不上 | 防火墙 8883；`systemctl status mosquitto` |
| TLS 报错 | CA 证书是否包含 BEGIN/END 行 |
| 摇动不灵 | 调低 `config.h` 中 `SHAKE_THRESHOLD` |
| 敲击不认 | 调低 `TAP_THRESHOLD`；敲背面更灵敏 |
| 天气不更新 | API Key 是否激活 |
| VSCode 红线 | `Ctrl+Shift+P` → Rebuild C/C++ Project Index |

---

## 与 BeaconOps 的区别

| | BeaconOps | Whisper |
|---|---|---|
| 定位 | 企业通知系统 | 情侣 1v1 |
| 设备数 | N 台 | 固定 2 台 |
| 管理界面 | Web 控制台 | 无 |
| 通讯方式 | 控制台→设备 | 状态 + ❤️（设备↔设备） |
| 交互 | 摇一摇确认 | 摇一摇=❤️ / 敲四下=状态 / 朝下=静音 |
| 天气 | ❌ | ✅ 双城 |
| 硬件 | 同 PCB | 零改动 |

---

## 许可证

基于 BeaconOps 修改，沿用分层授权：

- 软件：[AGPL-3.0-only](LICENSE)
- 硬件：[CERN-OHL-S-2.0](LICENSE)
- 文档：[CC BY-SA 4.0](LICENSE)

© 2024 基于 [BeaconOps](https://github.com/Micro-Mood/BeaconOps) by CoCandy
