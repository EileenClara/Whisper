# Whisper / LoveBeacon — 异地情侣通讯终端

基于开源项目 [BeaconOps](https://github.com/Micro-Mood/BeaconOps) 硬件的异地情侣通讯设备。

```
┌─────────────┐     MQTT/TLS      ┌──────────────────┐     MQTT/TLS      ┌─────────────┐
│  设备A (中国) │ ◄──────────────► │  东京 Vultr VPS   │ ◄──────────────► │ 设备B (新加坡)│
│  ESP32-C3    │                  │  Mosquitto+FastAPI│                  │  ESP32-C3   │
└─────────────┘                   └──────────────────┘                   └─────────────┘
```

## 功能

- 📩 **快捷消息**：预设 6 条情侣消息（想你了💕 / 晚安🌙 / 早安☀️ / 在忙 / 等我一下 / 我爱你❤️）
- ❤️ **摇一摇心跳**：摇动设备发送❤️，对方屏幕弹动画+提示音
- 📊 **实时状态**：6 种状态同步（出去玩/学习中/休闲/睡觉/想你/吃饭）
- 🌤️ **双城天气**：本地和对方城市的天气+温度实时显示
- 🕐 **双时区时钟**：同时显示两地时间
- 💾 **离线消息**：对方不在线时消息缓存，上线后自动推送
- 🔐 **TLS 加密**：MQTT over TLS，Let's Encrypt 证书

## 硬件

复用 [BeaconOps PCB](https://oshwhub.com/httppp/project_owrzrbgf) 设计：

| 组件 | 型号 |
|------|------|
| 主控 | ESP32-C3 (QFN-32) |
| 屏幕 | 1.47" IPS ST7789 172×320 SPI |
| 陀螺仪 | ST LSM6DS3TR-C (I2C) |
| 音频 | MAX98357A I2S 功放 |
| 电量计 | CW2017 (I2C) |
| 电源 | USB-C 充电 + 锂电池 |
| PCB | 36.28×19.39mm 双层板 |

> ⚠️ **PCB 焊接难度**：LGA-14 (陀螺仪) 和 WLP-9 (功放) 是无引脚封装，需要回流焊设备。新手建议先买成品板调试固件，再考虑自己打板。

## 目录结构

```
Whisper/
├── README.md
├── server/                     # 服务端（部署到 VPS）
│   ├── install.sh              # 一键安装脚本
│   ├── requirements.txt        # Python 依赖
│   ├── config.json             # 配置文件（需修改）
│   ├── mosquitto.conf          # MQTT Broker 配置
│   ├── main.py                 # FastAPI 入口
│   ├── models.py               # 数据模型
│   ├── database.py             # SQLite 操作
│   ├── mqtt_bridge.py          # MQTT 消息路由
│   └── weather_service.py      # 天气拉取推送
│
└── firmware/                   # ESP32 固件
    ├── platformio.ini          # PlatformIO 配置
    └── src/
        ├── main.cpp            # 主程序
        ├── config.h            # 设备配置（需修改）
        ├── storage.h/cpp       # NVS 存储
        ├── wifi_manager.h/cpp  # WiFi 管理
        ├── mqtt_handler.h/cpp  # MQTT 通讯
        ├── display.h/cpp       # 屏幕驱动+UI
        ├── gyro.h/cpp          # 陀螺仪+摇动检测
        └── audio.h/cpp         # I2S 音频
```

---

## 部署步骤

### 第一步：VPS 准备（东京 Vultr，Ubuntu 22.04）

1. SSH 登录 VPS
2. 安装 git 并克隆项目：
   ```bash
   git clone <你的仓库> Whisper
   cd Whisper/server
   ```

3. **修改 `config.json`**：
   ```json
   {
     "weather": {
       "api_key": "你的OpenWeatherMap API Key"
     },
     "devices": {
       "deviceA": {
         "name": "你的名字",
         "partner_name": "对方的名字",
         "city": "Shanghai",
         "weather_city_id": "Shanghai,CN"
       },
       "deviceB": {
         "name": "对方的名字",
         "partner_name": "你的名字",
         "city": "Singapore",
         "weather_city_id": "Singapore,SG"
       }
     }
   }
   ```

4. **修改 `install.sh`** 中的配置变量：
   ```bash
   DOMAIN="你的域名.com"       # 如果有域名
   EMAIL="你的邮箱"
   MQTT_PASS="强密码"
   ```
   > 如果没有域名，用 VPS IP 也可以，但需要改用自签名证书。

5. **运行安装脚本**：
   ```bash
   chmod +x install.sh
   sudo ./install.sh
   ```

6. **检查服务状态**：
   ```bash
   sudo systemctl status mosquitto
   sudo systemctl status lovebeacon
   curl http://localhost:8000/health
   ```

### 第二步：固件编译

1. 安装 [VSCode](https://code.visualstudio.com/) + [PlatformIO 插件](https://platformio.org/install/ide?install=vscode)

2. 打开 `Whisper/firmware/` 文件夹

3. **修改 `src/config.h`** 中的配置：
   ```cpp
   // 设备身份
   #define DEVICE_ID "deviceA"  // 编译另一台时改为 "deviceB"

   // MQTT 服务器地址
   #define MQTT_BROKER_HOST "你的VPS_IP或域名"
   #define MQTT_PASSWORD "你的MQTT密码"

   // 个人化信息（已经在 config.h 中按 deviceA/deviceB 分别配置好）
   ```

4. **TFT_eSPI 引脚配置**：在 PlatformIO 项目中，需要编辑 TFT_eSPI 库的 `User_Setup.h` 或在 `platformio.ini` 中添加 build_flags。简便方法是：
   - 找到 `.pio/libdeps/esp32-c3-devkitm-1/TFT_eSPI/User_Setup_Select.h`
   - 注释掉所有 include，在最后添加：
   ```cpp
   #define ST7789_DRIVER
   #define TFT_WIDTH  172
   #define TFT_HEIGHT 320
   #define TFT_MOSI 7
   #define TFT_SCLK 6
   #define TFT_CS   5
   #define TFT_DC   4
   #define TFT_RST -1
   #define TFT_BL   9
   #define SPI_FREQUENCY 63500000
   #define SPI_READ_FREQUENCY 20000000
   #define SPI_TOUCH_FREQUENCY 2500000
   ```

5. **编译并烧录 deviceA**：
   ```
   pio run -e esp32-c3-devkitm-1 -t upload
   ```

6. **编译 deviceB**：修改 `config.h` 中 `#define DEVICE_ID "deviceB"`，重复步骤 5

### 第三步：设备配对

1. 两台设备上电
2. 首次使用：WiFi 凭据存储在 NVS 中，若有已保存凭据会自动连接
3. 如果 WiFi 未配置，设备会启动 AP 热点 `Whisper-Setup`（后续版本支持 Web 配网）
4. 连接成功后，设备自动同步时间 → 连接 MQTT → 显示主屏幕

---

## 交互说明

设备**没有物理按钮**，所有交互通过摇动完成：

| 动作 | 效果 |
|------|------|
| **单次摇动** | 进入心跳模式，屏幕显示"摇一摇发送❤️" |
| **心跳模式内再摇** | 发送❤️给对方，对方屏幕弹❤️动画 |
| **心跳模式超时3秒** | 自动取消，回到主屏幕 |
| **快速双摇（0.8秒内）** | 打开快捷消息菜单 |
| **菜单中摇动** | 切换预设消息（循环） |
| **菜单中静置3秒** | 发送当前选中的消息 |

---

## 故障排查

| 问题 | 检查项 |
|------|--------|
| 屏幕不亮 | 背光引脚 GPIO9 是否焊好；`TFT_ROTATION` 是否正确 |
| 连不上WiFi | 串口查看 WiFi 状态；确认路由器 2.4GHz |
| MQTT连不上 | VPS 防火墙是否放行 8883；Mosquitto 是否运行；TLS 证书是否有效 |
| 摇动不灵 | 调整 `config.h` 中 `SHAKE_THRESHOLD`（越小越灵敏） |
| 天气不更新 | OpenWeatherMap API Key 是否激活（新 Key 需等1-2小时） |
| 中国设备连不上 | 检查是否被墙；可能需要用非标准端口或套 CDN |

---

## 安全提醒

- ❌ **不要将 `config.json` 和 `config.h` 提交到公开仓库**（含 API Key 和密码）
- ✅ 生产环境请使用强密码
- ✅ 定期轮换 MQTT 密码和 API Key
- ✅ VPS 防火墙仅开放必要端口（22/8000/8883）

---

## 许可证

本项目基于 BeaconOps（AGPL-3.0 / CERN-OHL-S-2.0 / CC BY-SA 4.0），沿用相同开源协议。

- 软件代码：AGPL-3.0-only
- 硬件设计：CERN-OHL-S-2.0
- 文档：CC BY-SA 4.0

© 2024 基于 [BeaconOps](https://github.com/Micro-Mood/BeaconOps) by CoCandy 修改
