/*
 * app_identity.h — 设备身份管理
 * 首次开机显示选择页: "我是果果" / "我是vv"
 * 选择后存入 Preferences，之后不再询问
 */

#ifndef APP_IDENTITY_H
#define APP_IDENTITY_H

#include "config.h"

enum DeviceID {
    ID_NONE = 0,
    ID_GUGU = 1,
    ID_VV   = 2
};

class AppIdentity {
public:
    static void begin();           // 检查是否已设置
    static bool isSet();           // 已选择？
    static DeviceID get();         // 当前身份
    static const char* name();     // "gugu" / "vv"
    static const char* nameCN();   // "果果" / "vv"
    static const char* partnerName();    // 对方的 name
    static const char* partnerNameCN();  // 对方的 nameCN

    static void set(DeviceID id);  // 设置并保存

    // 首次开机选择页
    static void drawPicker();      // 绘制选择页面
    static DeviceID hitPicker(uint16_t tx, uint16_t ty);  // 点击检测

private:
    static DeviceID _id;
};

#endif
