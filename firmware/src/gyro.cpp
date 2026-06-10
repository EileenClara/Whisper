/**
 * @file gyro.cpp
 * @brief Whisper LSM6DS3TR-C 陀螺仪驱动 — 实现
 *
 * 摇动检测逻辑:
 *  1. 持续采样加速度（100Hz），计算与前一次的差值
 *  2. 差值超过阈值 → 记录为一次"摇动"
 *  3. 状态机根据摇动间隔判断：单摇/双摇/心跳确认
 */

#include "gyro.h"
#include "config.h"

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

    // 基础设置
    _imu.settings.accelRange = 4;        // ±4g
    _imu.settings.accelSampleRate = 104; // 104 Hz
    _imu.settings.accelBandWidth = 2;    // 100Hz 带宽
    _imu.settings.gyroEnabled = 0;       // 平时关闭陀螺仪省电
    _imu.applySettings();

    // 启用 pedometer（内置计步器）
    _imu.enablePedometer();

    // 初始读数
    _imu.readAccelRaw(_ax, _ay, _az);
    _lastAx = _ax;
    _lastAy = _ay;
    _lastAz = _az;

    _initialized = true;
    _state = SHAKE_IDLE;
    _stateEnterTime = millis();

    Serial.println("[Gyro] ✅ LSM6DS3TR-C 就绪");
    return true;
}

GyroEvent Gyro::update() {
    if (!_initialized || !_active) return GYRO_NONE;

    // 读取当前加速度（原始值需转换为 mg，库内部已处理）
    _imu.readAccelRaw(_ax, _ay, _az);

    unsigned long now = millis();
    GyroEvent event = GYRO_NONE;

    switch (_state) {

        case SHAKE_IDLE:
            // 检测摇动
            if (_isOverThreshold()) {
                _shakeTimestamp = now;
                _state = SHAKE_FIRST_DETECTED;
                _stateEnterTime = now;
                Serial.println("[Gyro] 检测到第一次摇动");
            }
            break;

        case SHAKE_FIRST_DETECTED:
            // 等待双摇窗口
            if (now - _shakeTimestamp > SHAKE_DOUBLE_WINDOW) {
                // 超时 → 单摇 = 进入心跳模式
                _state = SHAKE_HEARTBEAT_MODE;
                _stateEnterTime = now;
                _menuIndex = 0;
                event = GYRO_SHAKE;
                Serial.println("[Gyro] 单摇 → 心跳模式");
            } else if (_isOverThreshold()) {
                // 窗口内再次摇动 → 双摇 = 进入菜单
                _state = SHAKE_MENU_MODE;
                _stateEnterTime = now;
                _menuIndex = 0;
                event = GYRO_DOUBLE_SHAKE;
                Serial.println("[Gyro] 双摇 → 快捷消息菜单");
            }
            break;

        case SHAKE_HEARTBEAT_MODE:
            // 心跳模式：等待确认摇动
            if (now - _stateEnterTime > SHAKE_HEARTBEAT_WINDOW) {
                // 超时 → 取消心跳模式
                _state = SHAKE_COOLDOWN;
                _stateEnterTime = now;
                Serial.println("[Gyro] 心跳模式超时 → 取消");
                event = GYRO_NONE;  // 信号：取消
            } else if (_isOverThreshold()) {
                // 确认摇动！发送❤️
                _state = SHAKE_COOLDOWN;
                _stateEnterTime = now;
                event = GYRO_SHAKE;  // 信号：确认发送
                Serial.println("[Gyro] 心跳确认！❤️");
            }
            break;

        case SHAKE_MENU_MODE:
            // 菜单模式：摇动=切换选项，超时=发送
            if (_isOverThreshold() && now - _shakeTimestamp > 400) {
                // 每次摇动切换到下一条消息
                _menuIndex = (_menuIndex + 1) % PRESET_MESSAGES_COUNT;
                _shakeTimestamp = now;
                Serial.printf("[Gyro] 菜单切换 → [%d] %s\n", _menuIndex, PRESET_MESSAGES[_menuIndex]);
            }
            if (now - _shakeTimestamp > SHAKE_MENU_TIMEOUT && now - _stateEnterTime > SHAKE_MENU_TIMEOUT) {
                // 长时间无摇动 → 发送当前选中消息
                _state = SHAKE_COOLDOWN;
                _stateEnterTime = now;
                event = GYRO_DOUBLE_SHAKE;  // 携带已选菜单索引
                Serial.printf("[Gyro] 菜单超时 → 发送: %s\n", PRESET_MESSAGES[_menuIndex]);
            }
            break;

        case SHAKE_COOLDOWN:
            // 冷却：防重复触发
            if (now - _stateEnterTime > SHAKE_DEBOUNCE_MS) {
                _state = SHAKE_IDLE;
                Serial.println("[Gyro] 冷却结束 → 空闲");
            }
            break;
    }

    // 保存本次读数供下次差分
    _lastAx = _ax;
    _lastAy = _ay;
    _lastAz = _az;

    return event;
}

void Gyro::getAccel(float& x, float& y, float& z) {
    x = _ax;
    y = _ay;
    z = _az;
}

bool Gyro::isHeartbeatMode() {
    return _state == SHAKE_HEARTBEAT_MODE;
}

bool Gyro::isMenuMode() {
    return _state == SHAKE_MENU_MODE;
}

int Gyro::getMenuIndex() {
    return _menuIndex;
}

void Gyro::resetToIdle() {
    _state = SHAKE_IDLE;
    _stateEnterTime = millis();
    _menuIndex = 0;
}

uint16_t Gyro::getStepCount() {
    if (!_initialized) return 0;
    return _imu.readStepCount();
}

void Gyro::resetStepCount() {
    if (_initialized) {
        _imu.enablePedometer();  // 重新启用即复位
    }
}

void Gyro::setActive(bool active) {
    _active = active;
    if (!active) {
        resetToIdle();
    }
}

// ---- 私有方法 ----

float Gyro::_calcDelta() {
    float dx = _ax - _lastAx;
    float dy = _ay - _lastAy;
    float dz = _az - _lastAz;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

bool Gyro::_isOverThreshold() {
    return _calcDelta() > SHAKE_THRESHOLD;
}
