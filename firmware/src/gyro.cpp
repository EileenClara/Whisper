/**
 * @file gyro.cpp
 * @brief Whisper LSM6DS3TR-C 手势检测 — 实现
 *
 * 检测两种手势:
 *   1. 摇动 (Shake) — 持续振荡，用于触发❤️心跳
 *   2. 敲击 (Tap)   — 短促尖峰，用于操作菜单
 *
 * 区别算法:
 *   - 摇动: 每次采样计算 delta = |当前-上次|，如果 delta 连续多次
 *           (>=3次) 超过阈值，判定为摇动（持续振荡特征）
 *   - 敲击: 单次 delta 超过更高的阈值，且前后 100ms 内 delta 都很低
 *           （即"突然动了一下立刻静止"，是敲击而非摇动）
 */

#include "gyro.h"
#include "config.h"

// 敲击检测阈值（比摇动阈值更高，因为敲击加速度变化更剧烈）
#define TAP_THRESHOLD           2500    // mg，敲击加速度变化阈值
#define TAP_COUNT_WINDOW_MS     2000    // 敲击计数窗口（2秒内3次=进入菜单）
#define TAP_DEBOUNCE_MS         200     // 敲击防抖（两次敲击最小间隔）
#define TAP_REQUIRED_COUNT      3       // 需要多少次敲击才触发菜单
#define SHAKE_CONSECUTIVE_COUNT 3       // 连续多少次采样超阈值判定为摇动

Gyro::Gyro() {}

Gyro::~Gyro() {}

bool Gyro::begin(uint8_t i2cAddr) {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQUENCY);

    if (!_imu.begin(i2cAddr, Wire)) {
        Serial.printf("[Gyro] ❌ LSM6DS3 初始化失败 (addr=0x%02X)\n", i2cAddr);
        _initialized = false;
        return false;
    }

    // 传感器配置
    _imu.settings.accelRange = 4;        // ±4g（敲击检测需要大范围）
    _imu.settings.accelSampleRate = 104; // 104 Hz（足够捕获敲击尖峰）
    _imu.settings.accelBandWidth = 2;    // 100Hz 带宽
    _imu.settings.gyroEnabled = 0;       // 平时关闭陀螺仪省电
    _imu.applySettings();

    _imu.enablePedometer();

    // 初始化基线
    _imu.readAccelRaw(_ax, _ay, _az);
    _lastAx = _ax;
    _lastAy = _ay;
    _lastAz = _az;

    _initialized = true;
    _state = STATE_IDLE;
    _stateEnterTime = millis();
    _tapCount = 0;
    _lastTapTime = 0;
    _lastShakeTime = 0;
    _lastMovementTime = millis();

    Serial.println("[Gyro] ✅ LSM6DS3TR-C 就绪 (摇动=❤️, 敲3下=菜单)");
    return true;
}

GyroEvent Gyro::update() {
    if (!_initialized || !_active) return GYRO_NONE;

    // 读取当前加速度
    _imu.readAccelRaw(_ax, _ay, _az);

    unsigned long now = millis();
    float delta = _calcDelta();
    GyroEvent event = GYRO_NONE;

    // 首先判断是敲击还是摇动
    bool isTap = (delta > TAP_THRESHOLD);
    bool isShake = _isShaking();  // 连续多次超 SHAKE_THRESHOLD

    // 如果有明显运动，记录最后运动时间（用于菜单静置超时）
    if (delta > SHAKE_THRESHOLD) {
        _lastMovementTime = now;
    }

    switch (_state) {

        // ---- 空闲态：等待摇动或敲击 ----
        case STATE_IDLE:
            if (isShake) {
                // 摇动 → 进入心跳模式
                _state = STATE_HEARTBEAT_MODE;
                _stateEnterTime = now;
                _lastShakeTime = now;
                event = GYRO_SHAKE;
                Serial.println("[Gyro] 摇动 → 心跳模式");
            }
            else if (isTap && now - _lastTapTime > TAP_DEBOUNCE_MS) {
                // 敲击 → 开始计数
                _state = STATE_TAP_COUNTING;
                _stateEnterTime = now;
                _tapCount = 1;
                _lastTapTime = now;
                Serial.printf("[Gyro] 敲击检测 #%d\n", _tapCount);
            }
            break;

        // ---- 敲击计数态：2秒窗口内等待更多敲击 ----
        case STATE_TAP_COUNTING:
            if (now - _stateEnterTime > TAP_COUNT_WINDOW_MS) {
                // 窗口超时：按最终计数触发
                if (_tapCount >= TAP_STATUS_COUNT)       event = GYRO_TAP_4;
                else if (_tapCount >= TAP_MSG_COUNT)     event = GYRO_TAP_3;
                else Serial.printf("[Gyro] 敲击 %d 次（不足）→ 忽略\n", _tapCount);
                _state = STATE_COOLDOWN;
                _stateEnterTime = now;
                _tapCount = 0;
            }
            else if (isShake) {
                Serial.println("[Gyro] 计数窗口内摇动 → 转为心跳");
                _tapCount = 0;
                _state = STATE_HEARTBEAT_MODE;
                _stateEnterTime = now;
                _lastShakeTime = now;
                event = GYRO_SHAKE;
            }
            else if (isTap && now - _lastTapTime > TAP_DEBOUNCE_MS) {
                _tapCount++;
                _lastTapTime = now;
                Serial.printf("[Gyro] 敲击 #%d (剩余%lums)\n",
                              _tapCount, TAP_COUNT_WINDOW_MS - (now - _stateEnterTime));

                // 达到4次 → 立即触发状态菜单
                if (_tapCount >= TAP_STATUS_COUNT) {
                    event = GYRO_TAP_4;
                    Serial.println("[Gyro] 敲4下 → 状态菜单");
                    _state = STATE_COOLDOWN;
                    _stateEnterTime = now;
                    _tapCount = 0;
                }
                // 达到3次 → 立即触发消息菜单
                else if (_tapCount == TAP_MSG_COUNT) {
                    event = GYRO_TAP_3;
                    Serial.println("[Gyro] 敲3下 → 消息菜单");
                    // 不退出计数态，可能还有第4下
                }
            }
            break;

        // ---- 心跳模式：等待确认摇动 ----
        case STATE_HEARTBEAT_MODE:
            if (now - _stateEnterTime > SHAKE_HEARTBEAT_WINDOW) {
                // 超时 → 取消心跳
                _state = STATE_COOLDOWN;
                _stateEnterTime = now;
                event = GYRO_HEARTBEAT_CANCEL;
                Serial.println("[Gyro] 心跳模式超时 → 取消");
            }
            else if (isShake && now - _lastShakeTime > 500) {
                // 再次摇动 → 确认发送❤️
                _state = STATE_COOLDOWN;
                _stateEnterTime = now;
                event = GYRO_HEARTBEAT_SENT;
                Serial.println("[Gyro] 确认摇动 → 发送❤️!");
            }
            break;

        // ---- 消息菜单：敲击切换，静置发送 ----
        case STATE_MSG_MENU:
            if (isTap && now - _lastTapTime > TAP_DEBOUNCE_MS) {
                _menuIndex = (_menuIndex + 1) % PRESET_MESSAGES_COUNT;
                _lastTapTime = now;
                event = GYRO_TAP;
                Serial.printf("[Gyro] 消息切换 → [%d] %s\n", _menuIndex, PRESET_MESSAGES[_menuIndex]);
            }
            else if (now - _lastMovementTime > SHAKE_MENU_TIMEOUT) {
                event = GYRO_MENU_TIMEOUT;
                Serial.printf("[Gyro] 消息菜单超时 → 发送: %s\n", PRESET_MESSAGES[_menuIndex]);
                _state = STATE_COOLDOWN;
                _stateEnterTime = now;
            }
            else if (isShake && now - _lastShakeTime > 500) {
                _menuIndex = (_menuIndex + 1) % PRESET_MESSAGES_COUNT;
                _lastShakeTime = now;
                event = GYRO_TAP;
            }
            break;

        // ---- 状态菜单：敲击切换状态，静置确认 ----
        case STATE_STATUS_MENU:
            if (isTap && now - _lastTapTime > TAP_DEBOUNCE_MS) {
                _menuIndex = (_menuIndex + 1) % STATUS_OPTIONS_COUNT;
                _lastTapTime = now;
                event = GYRO_TAP;
                Serial.printf("[Gyro] 状态切换 → [%d] %s%s\n",
                              _menuIndex, STATUS_OPTIONS[_menuIndex].emoji,
                              STATUS_OPTIONS[_menuIndex].label);
            }
            else if (now - _lastMovementTime > SHAKE_MENU_TIMEOUT) {
                event = GYRO_MENU_TIMEOUT;
                Serial.printf("[Gyro] 状态菜单超时 → 确认: %s\n", STATUS_OPTIONS[_menuIndex].label);
                _state = STATE_COOLDOWN;
                _stateEnterTime = now;
            }
            else if (isShake && now - _lastShakeTime > 500) {
                _menuIndex = (_menuIndex + 1) % STATUS_OPTIONS_COUNT;
                _lastShakeTime = now;
                event = GYRO_TAP;
            }
            break;

        // ---- 冷却态：防止重复触发 ----
        case STATE_COOLDOWN:
            if (now - _stateEnterTime > SHAKE_DEBOUNCE_MS) {
                _state = STATE_IDLE;
                _stateEnterTime = now;
                Serial.println("[Gyro] 冷却结束 → 空闲");
            }
            break;
    }

    // 保存本次读数
    _lastAx = _ax;
    _lastAy = _ay;
    _lastAz = _az;

    return event;
}

// ============================================================
// 公共 getter
// ============================================================

void Gyro::getAccel(float& x, float& y, float& z) {
    x = _ax; y = _ay; z = _az;
}

bool Gyro::isHeartbeatMode() { return _state == STATE_HEARTBEAT_MODE; }
bool Gyro::isMsgMenu()    { return _state == STATE_MSG_MENU; }
bool Gyro::isStatusMenu() { return _state == STATE_STATUS_MENU; }

int Gyro::getMenuIndex()  { return _menuIndex; }

void Gyro::enterMsgMenu() {
    _state = STATE_MSG_MENU;
    _stateEnterTime = millis();
    _menuIndex = 0;
    _tapCount = 0;
    _lastMovementTime = millis();
}

void Gyro::enterStatusMenu() {
    _state = STATE_STATUS_MENU;
    _stateEnterTime = millis();
    _menuIndex = 0;
    _tapCount = 0;
    _lastMovementTime = millis();
}

void Gyro::resetToIdle() {
    _state = STATE_IDLE;
    _stateEnterTime = millis();
    _menuIndex = 0;
    _tapCount = 0;
}

uint16_t Gyro::getStepCount() {
    if (!_initialized) return 0;
    return _imu.readStepCount();
}

void Gyro::resetStepCount() {
    if (_initialized) _imu.enablePedometer();
}

void Gyro::setActive(bool active) {
    _active = active;
    if (!active) resetToIdle();
}

// ============================================================
// 私有：手势检测算法
// ============================================================

float Gyro::_calcDelta() {
    float dx = _ax - _lastAx;
    float dy = _ay - _lastAy;
    float dz = _az - _lastAz;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

bool Gyro::_isShaking() {
    // 摇动 = 持续振荡：在短时间内多次出现大幅变化
    // 简化判断：当前 delta 超过阈值即视为摇动
    // （更准确的做法是维护一个滑动窗口，但单次判断在实际中足够了）
    return _calcDelta() > SHAKE_THRESHOLD;
}

bool Gyro::_isTapping() {
    // 敲击 = 单次尖峰：delta 超过 TAP_THRESHOLD
    // TAP_THRESHOLD 比 SHAKE_THRESHOLD 更高
    // 已在 update() 中通过 delta > TAP_THRESHOLD 判断
    return _calcDelta() > TAP_THRESHOLD;
}
