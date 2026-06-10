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
| 📩 **快捷消息** | 6条预设（想你了💕/晚安🌙/早安☀️/在忙/等我一下/我爱你❤️） |
| ❤️ **摇一摇心跳** | 摇一摇进入心跳模式，再摇发送❤️，对方全屏❤️动画 |
| 📊 **状态同步** | 出去玩🎮/学习中📚/休闲🛋️/睡觉😴/想你💕/吃饭🍽️ |
| 🌤️ **双城天气** | 北京 + 新加坡实时天气温度 |
| 🕐 **时钟** | NTP 自动同步 |
| 💾 **离线消息** | 对方不在线时缓存，上线自动推送 |
| 🔇 **静音模式** | 屏幕朝下放3秒切换，左上角图标显示状态 |
| 🔐 **TLS 加密** | MQTT over TLS 8883，自签名证书 |

---

## 交互方式

设备**没有物理按钮**，全部通过手势操作：

| 手势 | 怎么操作 | 效果 |
|------|----------|------|
| 🔀 **摇一摇** | 晃动设备 | 进入心跳模式，"摇一摇发送❤️" |
| 🔀 **再摇** | 3秒内再次摇动 | 发送❤️给对方 |
| 🕐 **不摇了** | 3秒无操作 | 自动取消，回主屏 |
| 👆👆👆 **敲三下** | 2秒内手指敲设备背面3次 | 打开快捷消息菜单 |
| 👆 **敲一下** | 菜单中敲击 | 切换预设消息 |
| 🕐 **不动了** | 菜单中静置3秒 | 发送当前选中消息 |
| 📱 **屏幕朝下** | 扣在桌上3秒 | 切换静音 🔊 ↔ 🔇 |

### 主屏幕布局

```
┌──────────────────────────────┐
│  🔊          📶░░░      🔋   │  静音 / WiFi / 电量
│                              │
│          14:30               │  单行大字时钟
│       6月10日 周三            │  日期
│                              │
│  北京 ☀️ 32°    新加坡 🌤 28° │  双城天气
│                              │
│  💕 vv 正在想你              │  对方实时状态
│──────────────────────────────│
│  💬 想你了💕      2分钟前     │  最新消息预览
│──────────────────────────────│
│  摇一摇 ❤️     敲三下 消息    │  操作提示
└──────────────────────────────┘
```

---

## 硬件

复用 [BeaconOps PCB](https://oshwhub.com/httppp/project_owrzrbgf)：

| 组件 | 型号 |
|------|------|
| 主控 | ESP32-C3 (QFN-32) |
| 屏幕 | 1.47" IPS ST7789 172×320 SPI |
| 陀螺仪 | ST LSM6DS3TR-C (I2C) |
| 音频 | MAX98357A I2S 功放 |
| 电量计 | CW2017 (I2C) |
| 电源 | USB-C 充电 + 锂电池 |
| PCB | 36.28×19.39mm 双层板 |

> ⚠️ LGA-14 和 WLP-9 是无引脚封装，需要回流焊。新手建议先调试固件再打板。

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
│   ├── database.py             # SQLite（离线消息+天气缓存）
│   ├── mqtt_bridge.py          # MQTT 消息路由转发
│   └── weather_service.py      # OpenWeatherMap 定时拉取推送
│
└── firmware/
    ├── platformio.ini          # PlatformIO 配置（ESP32-C3 + Arduino）
    └── src/
        ├── main.cpp            # 主程序 + 状态机
        ├── config.h            # 硬件引脚 + 用户配置（编译前修改）
        ├── storage.h/cpp       # NVS 持久化存储
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
# SSH 登录 VPS
git clone https://github.com/EileenClara/Whisper.git
cd Whisper/server

# 编辑 install.sh，确认:
#   USE_SELF_SIGNED=true
#   SERVER_IP="198.13.56.214"
#   MQTT_PASS="你的MQTT密码"

# 编辑 config.json，填入:
#   weather.api_key        → OpenWeatherMap API Key
#   devices.deviceA.name   → "果果"
#   devices.deviceB.name   → "vv"
#   mqtt.password          → 与 install.sh 一致

chmod +x install.sh
sudo ./install.sh

# 验证
sudo systemctl status mosquitto
sudo systemctl status lovebeacon
curl http://localhost:8000/health
```

运行 `install.sh` 后，终端会打印自签名 **CA 证书**，复制它。

### 第二步：固件

```bash
# 用 VSCode + PlatformIO 打开 firmware/ 文件夹

# 1. 修改 src/config.h:
#    MQTT_BROKER_HOST → "198.13.56.214"
#    MQTT_PASSWORD     → 与 config.json 一致

# 2. 修改 src/mqtt_handler.cpp:
#    把 install.sh 输出的 CA 证书粘贴到 CA_CERT 占位符

# 3. TFT_eSPI 引脚配置:
#    编辑 .pio/libdeps/esp32-c3-devkitm-1/TFT_eSPI/User_Setup_Select.h
#    注释所有 include，末尾加:
#      #define ST7789_DRIVER
#      #define TFT_WIDTH  172
#      #define TFT_HEIGHT 320
#      #define TFT_MOSI 7
#      ...
#    （完整定义见 README 旧版）

# 4. 编译 deviceA:
pio run -e esp32-c3-devkitm-1 -t upload

# 5. 改 config.h 中 DEVICE_ID="deviceB"，编译 deviceB:
pio run -e esp32-c3-devkitm-1 -t upload
```

### 第三步：WiFi 配网

```
设备首次开机 → 无已保存WiFi
  │
  └─ 自动开启热点 "Whisper-Setup"
        │
        └─ 手机连上热点 → 自动弹出配网页面
              │
              └─ 选择家里WiFi → 输入密码 → 设备保存 → 自动重启
```

之后每次开机自动连接，不需要重复配网。

---

## MQTT Topic 设计

```
lovebeacon/{device_id}/message/send        # 设备上行：发消息
lovebeacon/{device_id}/message/receive     # 设备下行：收消息
lovebeacon/{device_id}/status              # 设备上行：状态更新
lovebeacon/{device_id}/status/notify       # 设备下行：对方状态
lovebeacon/{device_id}/heartbeat           # 设备上行：发❤️
lovebeacon/{device_id}/heartbeat/notify    # 设备下行：收到❤️
lovebeacon/{device_id}/weather             # 服务端下行：天气推送
lovebeacon/{device_id}/presence            # LWT：上下线
lovebeacon/{device_id}/presence/notify     # 服务端下行：对方上下线
```

---

## 故障排查

| 问题 | 检查 |
|------|------|
| 屏幕不亮 | 背光 GPIO9 焊接；`TFT_ROTATION` 方向 |
| 连不上WiFi | 2.4GHz；串口查看日志 |
| 配网页面不弹 | 手机是否开了随机MAC；手动访问 192.168.4.1 |
| MQTT连不上 | VPS 防火墙 8883；`systemctl status mosquitto` |
| TLS 报错 | CA 证书是否粘贴正确；是否包含 BEGIN/END 行 |
| 摇动不灵敏 | 调低 `config.h` 中 `SHAKE_THRESHOLD` |
| 天气不更新 | API Key 是否激活（新 Key 等1-2小时） |
| VSCode 红线 | `Ctrl+Shift+P` → `PlatformIO: Rebuild C/C++ Project Index` |

---

## 与 BeaconOps 的区别

| | BeaconOps 原版 | Whisper |
|---|---|---|
| 定位 | 企业通知系统（多设备批量管理） | 情侣 1v1 通讯 |
| 设备数 | N 台（批次管理） | 固定 2 台 |
| 管理界面 | Web 控制台（Vue 3） | 无（纯设备交互） |
| 认证 | HMAC-SHA256 动态密码 | 用户名+密码 |
| 交互 | 摇一摇确认消息 | 摇一摇=❤️ / 敲三下=消息 / 朝下=静音 |
| 消息方向 | 控制台→设备（单向下发） | 设备↔设备（双向） |
| 快捷消息 | ❌ | ✅ 6条情侣预设 |
| 天气 | ❌ | ✅ 双城实时天气 |
| 离线消息 | NVS 持久化队列 | SQLite 服务端缓存 |

---

## 许可证

基于 BeaconOps 修改，沿用分层授权：

- 软件代码：[AGPL-3.0-only](LICENSE)
- 硬件设计：[CERN-OHL-S-2.0](LICENSE)
- 文档：[CC BY-SA 4.0](LICENSE)

© 2024 基于 [BeaconOps](https://github.com/Micro-Mood/BeaconOps) by CoCandy
