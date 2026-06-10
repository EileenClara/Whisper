/**
 * @file gyro.h
 * @brief Whisper LSM6DS3TR-C 陀螺仪驱动 + 手势检测
 *
 * 使用 SparkFun LSM6DS3 Arduino 库
 * 交互逻辑（防误触设计）:
 *  - 摇一摇（持续振荡）→ 进入心跳模式（3秒内再摇 = 发送❤️）
 *  - 敲三下（短促尖峰脉冲×3，2秒内）→ 进入快捷消息菜单
 *  - 菜单中：敲击切换选项，静置3秒自动发送选中消息
 *
 * 摇动 vs 敲击 的区别:
 *  - 摇动 = 持续高频振荡（加速度反复过零）
 *  - 敲击 = 短促单次尖峰（<50ms 内加速度急升急降，然后恢复静止）
 */

#ifndef GYRO_H
#define GYRO_H

#include <Arduino.h>
#include <Wire.h>
#include "SparkFunLSM6DS3.h"

/** 手势事件类型 */
enum GyroEvent {
    GYRO_NONE = 0,          // 无事件
    GYRO_SHAKE,             // 检测到摇动 → 进入/确认心跳
    GYRO_TAP,               // 检测到敲击 → 菜单中切换选项
    GYRO_TAP_3,             // 连续3次敲击 → 进入消息菜单
    GYRO_TAP_4,             // 连续4次敲击 → 进入状态菜单
    GYRO_HEARTBEAT_SENT,    // 心跳模式中确认摇动 → 发送❤️
    GYRO_MENU_TIMEOUT,      // 菜单中静置超时 → 发送/确认
    GYRO_HEARTBEAT_CANCEL,  // 心跳模式超时 → 取消
};

/** 手势检测状态机 */
enum GestureState {
    STATE_IDLE = 0,           // 空闲
    STATE_TAP_COUNTING,       // 连续敲击计数（2秒窗口）
    STATE_HEARTBEAT_MODE,     // 心跳模式
    STATE_MSG_MENU,           // 快捷消息菜单
    STATE_STATUS_MENU,        // 状态选择菜单
    STATE_COOLDOWN,           // 冷却中
};

class Gyro {
public:
    Gyro();
    ~Gyro();

    /** 初始化传感器 */
    bool begin(uint8_t i2cAddr = 0x6A);

    /** 在主循环中调用，处理手势检测状态机
     *  @return GyroEvent 检测到的事件类型
     */
    GyroEvent update();

    /** 获取加速度值 (原始值，供调试) */
    void getAccel(float& x, float& y, float& z);

    /** 是否正在心跳模式 */
    bool isHeartbeatMode();

    /** 是否在消息菜单模式 */
    bool isMsgMenu();

    /** 是否在状态菜单模式 */
    bool isStatusMenu();

    /** 获取当前选中索引（消息菜单或状态菜单通用） */
    int getMenuIndex();

    /** 进入消息菜单模式 */
    void enterMsgMenu();

    /** 进入状态菜单模式 */
    void enterStatusMenu();

    /** 重置到空闲 */
    void resetToIdle();

    /** 获取步数 */
    uint16_t getStepCount();

    /** 复位步数计数器 */
    void resetStepCount();

    /** 启用/禁用持续运动检测（省电） */
    void setActive(bool active);

private:
    LSM6DS3 _imu;
    bool _initialized = false;
    bool _active = true;

    // 状态机
    GestureState _state = STATE_IDLE;
    unsigned long _stateEnterTime = 0;
    int _menuIndex = 0;

    // 加速度数据（mg 单位）
    float _ax = 0, _ay = 0, _az = 0;
    float _lastAx = 0, _lastAy = 0, _lastAz = 0;

    // 敲击计数
    int _tapCount = 0;
    unsigned long _lastTapTime = 0;
    unsigned long _lastShakeTime = 0;

    // 静止检测（菜单模式用）
    unsigned long _lastMovementTime = 0;

    /** 计算加速度变化幅度 */
    float _calcDelta();

    /** 检测是否超过摇动阈值（持续振荡） */
    bool _isShaking();

    /** 检测是否发生敲击（短促尖峰） */
    bool _isTapping();
};

#endif // GYRO_H
