/**
 * @file gyro.h
 * @brief Whisper LSM6DS3TR-C 陀螺仪驱动 + 摇动检测
 *
 * 使用 SparkFun LSM6DS3 Arduino 库
 * 功能:
 *  - 单摇（1次摇动）→ 进入心跳模式（3秒内再摇 = 发送❤️）
 *  - 双摇（0.8秒内2次摇动）→ 进入快捷消息菜单
 *  - 菜单中摇动 = 切换选项
 *  - 平时陀螺仪低功耗轮询，节省电量
 */

#ifndef GYRO_H
#define GYRO_H

#include <Arduino.h>
#include <Wire.h>
#include "SparkFunLSM6DS3.h"

/** 陀螺仪事件类型 */
enum GyroEvent {
    GYRO_NONE = 0,          // 无事件
    GYRO_SHAKE,             // 检测到单次摇动
    GYRO_DOUBLE_SHAKE,      // 双摇（快捷消息菜单）
};

/** 摇动检测器状态机 */
enum ShakeState {
    SHAKE_IDLE = 0,         // 空闲，等待摇动
    SHAKE_FIRST_DETECTED,   // 检测到第一次摇动，等待双摇窗口
    SHAKE_HEARTBEAT_MODE,   // 心跳模式激活，等待确认摇动
    SHAKE_MENU_MODE,        // 快捷消息菜单模式
    SHAKE_COOLDOWN,         // 冷却中（防重复触发）
};

class Gyro {
public:
    Gyro();
    ~Gyro();

    /** 初始化传感器 */
    bool begin(uint8_t i2cAddr = 0x6A);

    /** 在主循环中调用，处理摇动检测状态机
     *  @return GyroEvent 检测到的事件类型
     */
    GyroEvent update();

    /** 获取加速度值 (mg) */
    void getAccel(float& x, float& y, float& z);

    /** 是否正在心跳模式（等待确认摇动） */
    bool isHeartbeatMode();

    /** 是否在菜单模式 */
    bool isMenuMode();

    /** 获取菜单中当前选中的消息索引 */
    int getMenuIndex();

    /** 重置状态机到空闲 */
    void resetToIdle();

    /** 获取步数（如果启用了 pedometer） */
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
    ShakeState _state = SHAKE_IDLE;
    unsigned long _shakeTimestamp = 0;   // 上次摇动时间
    unsigned long _stateEnterTime = 0;   // 进入当前状态的时间
    int _menuIndex = 0;                  // 菜单当前选项

    // 加速度数据
    float _ax = 0, _ay = 0, _az = 0;
    float _lastAx = 0, _lastAy = 0, _lastAz = 0;

    /** 计算加速度变化幅度 */
    float _calcDelta();

    /** 检测是否超过阈值 */
    bool _isOverThreshold();
};

#endif // GYRO_H
