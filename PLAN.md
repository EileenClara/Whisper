# SmallDesktopDisplay v2.0 — 情侣互动桌面显示器 实施计划

## 背景

将现有的 ESP32 桌面显示器（V1.4，原作者 Misaka/微车游）完全重写为 ESP32-S3 情侣互动设备。两台设备通过东京 VPS 上的 MQTT broker 通信，共享北京时间、双城市天气、双方状态，支持发送爱心互动。

## 回答你的问题

- **需要联网吗？** 需要。ESP32 通过 WiFi 联网做三件事：NTP 授时（阿里云）、MQTT 连接东京 VPS、通过 VPS 代理获取天气。
- **需要 VPS 吗？** 需要。VPS 上跑两个东西：**Mosquitto MQTT Broker**（设备间通信）+ **Python 天气代理脚本**（用你的 Key `271834fb90974033d0a897a008687d88` 调 OpenWeatherMap）。

## 架构总览

```
┌─────────────────┐         TLS:8883          ┌─────────────────┐
│  ESP32-S3 (果果) │◄────────MQTT─────────────►│  ESP32-S3 (vv)   │
│  240x240 TFT     │                           │  240x240 TFT     │
│  XPT2046 Touch   │                           │  XPT2046 Touch   │
└────────┬─────────┘                           └────────┬─────────┘
         │                                              │
         │         东京 VPS (Mosquitto Broker)            │
         └────────────┬─────────────────────────────────┘
                      │ MQTT weather/req
                      ▼
              ┌───────────────┐
              │ weather_proxy │──► OpenWeatherMap API
              │   (Python)    │    (Beijing + Singapore)
              └───────────────┘
```

## 模块化文件结构（从 1671 行单文件拆分为模块）

```
SmallDesktopDisplay/
├── SmallDesktopDisplay.ino      # 入口：setup() / loop()
├── config.h                     # 所有常量、引脚、版本号
├── app_identity.h/cpp           # 设备身份：果果/vv（首次开机选择）
├── storage_prefs.h/cpp          # Preferences(NVS) 持久化存储
├── backlight.h/cpp              # LEDC PWM 背光控制（替代 analogWrite）
├── display_screen.h/cpp         # TFT 初始化 + 精灵管理 + 脏区域追踪
├── touch_handler.h/cpp          # XPT2046 触摸读取 + 区域检测
├── network_wifi.h/cpp           # WiFi 非阻塞状态机 + WiFiManager 配网
├── network_ntp.h/cpp            # NTP 北京时间同步
├── network_mqtt.h/cpp           # MQTT+TLS 客户端 + LWT + 消息回调
├── ui_clock.h/cpp               # 数码管时钟（复用 Number 类）
├── ui_weather.h/cpp             # 双城市天气面板渲染
├── ui_status.h/cpp              # 双人状态显示 + 状态选择菜单
├── ui_heart.h/cpp               # 爱心状态机 + 发送/接收/渐隐动画
├── weather_bridge.h/cpp         # 通过 MQTT 请求/接收天气数据
├── app_state.h/cpp              # 全局应用状态聚合
├── number.h/cpp                 # [复用] 数码管渲染类
├── weathernum.h/cpp             # [复用] 天气图标渲染类
├── font/                        # [复用] 全部 31 个字体/数字文件
├── img/                         # [复用] 全部图像资源
│   ├── tianqi/                  # 天气图标 JPEG (t0~t99)
│   ├── pangzi/                  # 太空人动画帧（可选项）
│   └── temperature/humidity.h   # 温湿度图标
├── certs/ca_cert.h              # [新] MQTT TLS CA 证书
└── vps/                         # [新] VPS 端脚本（不编译进 ESP32）
    ├── mosquitto.conf           # Mosquitto broker 配置
    ├── generate_certs.sh        # TLS 证书生成脚本
    └── weather_proxy.py         # Python MQTT→OpenWeatherMap 代理
```

## ESP32-S3 引脚分配（修复原版冲突）

| 功能 | 引脚 | 说明 |
|------|------|------|
| TFT SCK | GPIO18 | SPI 时钟 |
| TFT MOSI | GPIO1 | SPI 数据 |
| TFT MISO | -1 | SPI 读取（不接） |
| TFT DC | GPIO17 | 数据/命令 |
| TFT RST | GPIO6 | 复位 |
| TFT CS | -1 | 片选（硬接地） |
| **TFT 背光** | **GPIO2** | **LEDC PWM** |
| Touch CS | 待测 | XPT2046 片选 |
| Touch IRQ | 待测 | 触摸中断 |
| ⚠️ GPIO19/20 | **禁止使用** | ESP32-S3 原生 USB D-/D+，占用会导致 COM 断连 |

**关键兼容性修复：**
- `analogWrite()` → `ledcSetup(0,5000,8)` + `ledcWrite(0,duty)`（ESP32-S3 不支持 analogWrite）
- `EEPROM` → `Preferences`（NVS 存储，ESP32-S3 推荐）
- WiFi 阻塞等待 → 非阻塞状态机（setup 不再死等）

## MQTT 通信协议

所有主题前缀 `sdd/`，设备 ID 为 `gugu` 或 `vv`。

| 主题 | 方向 | 用途 |
|------|------|------|
| `sdd/{id}/status` | 发布 (LWT) | 在线状态（retained），断线自动设 offline |
| `sdd/{id}/mood` | 发布 | 当前状态（0=空闲 1=游戏 2=忙碌 3=想你 4=干饭 5=学习 6=睡觉 7=踢球）|
| `sdd/{id}/heart/send` | 发布 | 发送爱心 {from, to, heart_id, send_time, expires_at} |
| `sdd/{id}/heart/ack` | 发布 | 接受爱心 {from, to, heart_id, ack_time} |
| `sdd/+/weather/req` | 订阅 | VPS 代理监听，ESP32 发布请求 |
| `sdd/weather/resp` | 订阅 | VPS 广播双城市天气 JSON |

**TLS 方案：** 服务器单向 TLS（ESP32 内嵌 CA 证书验证 broker），客户端通过 MQTT username 区分身份。简单且安全。

## 240×240 屏幕布局（紧凑单页）

```
┌──────────────────────────────┐ y=0
│ 北京 ☀ 25°  │ 新加坡 🌧 30°  │ 天气区 (0-80)
│ 湿度 65%   │ 湿度 80%       │
├──────────────────────────────┤ y=80
│                              │
│   14 : 30 : 25              │ 时钟区 (80-150)
│   (36×60 大数码管)          │
│   6月14日  周六              │
├──────────────────────────────┤ y=150
│ 果果 💗想你  │  vv 🟣学习   │ 状态区 (150-175) 并排
├──────────────────────────────┤ y=175
│                              │
│  ♥♥♥ 爱心堆积区             │ 爱心区 (175-240)
│  无角色: 点我发爱心         │
│  发送方: ♥已发3 (冷却中)    │
│  接收方: 爱心堆底部         │
└──────────────────────────────┘ y=240
```

### 触摸区域映射（极简设计，无独立按钮）

- **(0,0)-(75,80)** → 北京天气区（点击强制刷新天气）
- **(75,0)-(170,80)** → 新加坡天气区（点击强制刷新天气）
- **(0,150)-(120,175)** → 果果状态区（并排左半边，点击自己 → 跳转全屏状态选择页）
- **(120,150)-(239,175)** → vv状态区（并排右半边，点击自己 → 跳转全屏状态选择页；对方只读）
- **(0,175)-(239,240)** → **爱心区**：
  - IDLE 时：点击 → 成为发送方，发第 1 颗爱心
  - SENDER 时（冷却已过）：点击 → 再发 1 颗
  - RECEIVER 时（有爱心堆积）：点击 → 接受全部爱心，清空

### 状态选择页（全屏 240×240，点击自己状态后跳转）

整屏 8 个状态选项，每行 2 个，4 行排列。每个选项带颜色圆点 + 状态名。点击选中 → 保存 + 发布 MQTT → 返回主屏。

### 8 种状态及颜色（全屏选择页，2 列 × 4 行）
点击自己状态 → 跳转整屏。每格 100×50，带颜色圆点 + 状态名。点击选中 → 保存 + 发布 MQTT → 返回主屏。

| 状态 | 颜色值 | 色块 | 位置 |
|------|--------|------|------|
| 空闲 | 0x07E0 | 🟢 | 第1行左 |
| 游戏 | 0x001F | 🔵 | 第1行右 |
| 忙碌 | 0xFD20 | 🟠 | 第2行左 |
| 想你 | 0xF81F | 💗 | 第2行右 |
| 干饭 | 0xFFE0 | 🟡 | 第3行左 |
| 学习 | 0x8010 | 🟣 | 第3行右 |
| 睡觉 | 0x18C4 | 🔷 | 第4行左 |
| 踢球 | 0x07FF | 🟦 | 第4行右 |

## 核心状态机

### 爱心交互流程（单向累积模型，不自动消失）

**核心规则：**
- 同一时间只有**一个人是发送方**，另一个人是接收方
- 发送方每 30 分钟可发 1 颗爱心，上限 **99 颗**
- 爱心**不会自动消失**——只有接收方点击接受后，双方爱心区才清空
- 清空后，**任意一方**都可以重新发起（谁先点谁就是发送方）

```
初始状态：双方爱心区为空，任意一方可发起

A 点击爱心区（空）→ A 成为发送方
    │
    ├─ 发送第 1 颗 → MQTT publish heart/send (count=1)
    │   A 爱心区显示："♥ 已发 1"
    │   B 爱心区：爱心从屏幕顶端掉落，滚到底部堆积
    │
    ├─ 30分钟后，A 再点击 → 发送第 2 颗 (count=2)
    │   A 爱心区显示："♥ 已发 2"
    │   B 爱心区：第 2 颗爱心掉落，堆在第 1 颗旁边
    │
    ├─ A 继续发... count=3, 4, 5 ... 上限 99
    │
    └─ B 点击爱心区（有爱心堆积）→ 接受！
        │
        ├─ MQTT publish heart/ack
        ├─ 双方爱心区清空
        └─ 回到初始状态，任意一方可重新发起
```

### 爱心显示动画

**接收方（B 设备）爱心堆积效果：**
- 爱心从屏幕**顶端掉落**（带简单动画），滚入底部爱心区
- 爱心在底部**随机位置堆积**，带微小旋转角度，不完全重叠
- 爱心大小约 14×14 像素，底部 65px 空间可容纳多行
- 第 N 颗爱心坐标随机微调，模拟自然散落

**发送方（A 设备）爱心区显示：**
- 纯文字显示 "♥ 已发 N 颗"
- 30 分钟冷却中显示 "♥ 已发 N (冷却 mm:ss)"

### 状态机（HeartSession）

```
enum HeartRole { NONE, SENDER, RECEIVER };

状态：
IDLE       → 双方都无角色，谁先点爱心区谁变 SENDER
SENDING    → 发送方刚发出，等待 MQTT 确认
SENT        → 发送方已确认，显示计数，30分钟冷却计时中
RECEIVING  → 接收方，爱心堆积中，等待用户点击接受
ACCEPTING  → 接收方点击接受，播放清空动画
```

### 天气数据流
```
ESP32 → MQTT weather/req → VPS weather_proxy.py 
     → OpenWeatherMap API (Beijing + Singapore)
     → MQTT weather/resp → 两台 ESP32 同时收到
```

## VPS 部署清单

1. **Mosquitto Broker**（Docker 或 apt 安装），配置 TLS 8883 端口
2. **TLS 证书**：自签 CA → 签发服务器证书，ESP32 嵌入 CA 公钥验证
3. **Python 天气代理**：监听 MQTT 天气请求 → 调 OpenWeatherMap → 发布结果
4. **ACL 权限**：只允许 `sdd/#` 主题，设备用户密码认证

## 实施顺序（5 个阶段）

### 阶段 1：基础框架
- 创建所有 `.h/.cpp` 文件骨架
- 实现 `config.h`、`storage_prefs`、`backlight`（LEDC）、`display_screen`
- 配置 TFT_eSPI 库的 `User_Setup.h` 匹配 ESP32-S3 引脚
- 验证：屏幕点亮、背光 PWM 正常

### 阶段 2：网络连接
- `network_wifi`（非阻塞 WiFi + WiFiManager 配网）
- `network_ntp`（北京时间同步）
- `network_mqtt`（TLS 连接 + LWT + 自动重连）
- VPS 部署 Mosquitto + 生成证书
- 验证：WiFi 连接、NTP 同步、MQTT 连接成功

### 阶段 3：核心 UI
- `ui_clock`（复用 Number 类 + 数字 JPEG）
- `ui_weather`（双城市天气面板布局）
- `weather_bridge`（MQTT 天气请求/响应）
- VPS 部署 Python 天气代理
- 验证：时钟显示、双城市天气正常

### 阶段 4：交互功能
- `touch_handler`（XPT2046 驱动 + 区域检测 + 去抖）
- `app_identity`（首次开机身份选择界面）
- `ui_status`（状态选择菜单 + MQTT 同步）
- `ui_heart`（爱心完整状态机 + 几何绘制爱心形状）
- 验证：触摸可用、状态同步、爱心互通

### 阶段 5：打磨
- 爱心动画优化（弹跳、渐隐、粒子效果）
- WiFi/MQTT 断线重连健壮性
- 伙伴在线/离线状态指示
- 堆内存优化（减少 String 碎片）
- 恢复出厂设置（双键长按）

## 关键技术风险及对策

| 风险 | 对策 |
|------|------|
| TJpg_Decoder + WiFi + MQTT 并发 | JPEG 解码同步但 S3 够快；MQTT 低优先级 |
| XPT2046 触摸校准不一致 | 校准值存 Preferences，预留四角校准模式 |
| VPS 天气代理宕机 | ESP32 缓存上次数据，超 30 分钟显示"(旧)"标记 |
| 双方同时发爱心导致状态混乱 | heart_id 含设备前缀+时间戳，独立管理各自生命周期 |
| 堆碎片化（String 操作频繁） | 热路径用固定 char 数组，持久 String 用 reserve() |

## 需要保留的现有资源

- `font/ZdyLwFont_20.h` — 中文矢量字体
- `font/W_3660_i*.h` / `O_3660_i*.h` / `W_1830_i*.h` — 30 个数字图片
- `img/tianqi/t*.h` — 22 个天气图标
- `img/temperature.h` / `img/humidity.h` — 温湿度图标
- `number.h/cpp` — Number 类（直接复用）
- `weathernum.h/cpp` — WeatherNum 类（直接复用）

## 需要的 Arduino 库 (platformio.ini)

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
lib_deps =
    bodmer/TFT_eSPI @ ^2.5.0
    bodmer/TJpg_Decoder @ ^1.1.0
    knolleary/PubSubClient @ ^2.8
    bblanchon/ArduinoJson @ ^6.21.0
    tzapu/WiFiManager @ ^2.0.0
    paulstoffregen/Time @ ^1.6.1
```
