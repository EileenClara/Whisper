/*
 * ui_heart.h — 爱心状态机 + 动画
 *
 * 角色模型:
 *   NONE     — 双方都无角色
 *   SENDER   — 我是发送方, 可继续发 (30分钟冷却, 上限99)
 *   RECEIVER — 我是接收方, 爱心堆积中, 点击接受
 *
 * 发送方: 显示 "♥已发 N (冷却:mm:ss)"
 * 接收方: 爱心从顶部掉落 → 底部随机堆积
 * 接受后: 双方清空, 回到 NONE
 */

#ifndef UI_HEART_H
#define UI_HEART_H

#include "config.h"

enum HeartRole {
    HR_NONE = 0,
    HR_SENDER,      // 我是发送方
    HR_RECEIVER     // 我是接收方
};

class UIHeart {
public:
    static void begin();
    static void loop();    // 30ms 动画帧

    // 交互
    static void onTap();   // 点击爱心区

    // MQTT 事件
    static void onHeartReceived(const String& json);  // 收到 heart/send
    static void onHeartAck(const String& json);       // 收到 heart/ack

    // 状态
    static HeartRole role();
    static int count();          // 当前爱心数

    // 绘制
    static void draw();

    // 冷却倒计时
    static unsigned long cooldownRemaining();  // 剩余冷却毫秒

private:
    static void _becomeSender();
    static void _becomeReceiver();
    static void _clear();

    static void _drawSenderArea();
    static void _drawReceiverArea();
    static void _drawIdleArea();

    static void _drawHeart(int x, int y, int size, uint16_t color);

    static HeartRole _role;
    static int       _count;
    static unsigned long _lastSendTime;

    // 接收方动画: 每颗心的位置和角度
    struct HeartDrop {
        int x;       // 底部 x 位置
        int y;       // 当前 y (动画用)
        int targetY; // 目标 y
        int angle;   // 微小旋转 (0-15)
        bool landed; // 已落地
    };
    static HeartDrop _hearts[100];  // 实际最多 99
    static int       _animFrame;
};

#endif
