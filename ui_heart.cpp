/*
 * ui_heart.cpp — 爱心实现
 */

#include "ui_heart.h"
#include "ui_status.h"    // drawMainScreen()
#include <ArduinoJson.h>
#include "display_screen.h"
#include "network_mqtt.h"
#include "app_identity.h"
#include "storage_prefs.h"
#include "img/love.h"

HeartRole UIHeart::_role = HR_NONE;
int UIHeart::_count = 0;
unsigned long UIHeart::_lastSendTime = 0;
UIHeart::HeartDrop UIHeart::_hearts[100];
int UIHeart::_animFrame = 0;

void UIHeart::begin() {
    int r = StoragePrefs::getHeartRole();
    _role = (HeartRole)r;
    _count = StoragePrefs::getHeartCount();
    _lastSendTime = StoragePrefs::getLastHeartTime();
    _animFrame = 0;
    memset(_hearts, 0, sizeof(_hearts));
    if (_role != HR_NONE) {
        Serial.printf("[Heart] Restored: role=%d count=%d\n", r, _count);
    }
}

void UIHeart::loop() {
    _animFrame++;

    if (_role == HR_RECEIVER) {
        bool anyMoving = false;
        for (int i = 0; i < _count && i < 100; i++) {
            if (!_hearts[i].landed && _hearts[i].y < _hearts[i].targetY) {
                _hearts[i].y += 3;
                if (_hearts[i].y >= _hearts[i].targetY) {
                    _hearts[i].y = _hearts[i].targetY;
                    _hearts[i].landed = true;
                }
                anyMoving = true;
            }
        }
        if (anyMoving) DisplayScreen::markDirty(ZONE_HEART);
    }
}

// ===== 交互 =====
void UIHeart::onTap() {
    switch (_role) {
        case HR_NONE:
            _becomeSender();
            _count = 1;
            _lastSendTime = millis();
            _saveState();
            NetworkMQTT::publishHeartSend(1, AppIdentity::partnerName());
            Serial.println("[Heart] Sent #1 — now SENDER");
            break;

        case HR_SENDER:
            if (_count >= HEART_MAX_COUNT) {
                Serial.println("[Heart] Max 99 reached");
                return;
            }
            _count++;
            _lastSendTime = millis();
            _saveState();
            NetworkMQTT::publishHeartSend(_count, AppIdentity::partnerName());
            Serial.printf("[Heart] Sent #%d\n", _count);
            break;

        case HR_RECEIVER:
            // 接收方 → 接受所有爱心
            NetworkMQTT::publishHeartAck(AppIdentity::partnerName());
            _clear();
            Serial.println("[Heart] Accepted — cleared");
            break;
    }
    DisplayScreen::markDirty(ZONE_HEART);
}

// ===== MQTT 事件 =====
void UIHeart::onHeartReceived(const String& json) {
    // 解析 count
    int incomingCount = 1;
    JsonDocument doc;
    if (deserializeJson(doc, json) == DeserializationError::Ok) {
        incomingCount = doc["count"] | 1;
    }

    // 如果我是发送方 → 对方也在发, 切换为接收方
    if (_role == HR_SENDER) {
        Serial.println("[Heart] Partner also sending — switching to receiver");
        _becomeReceiver();
        _count = 0;
    }

    // 已经收过同样的数量 → 跳过
    if (_role == HR_RECEIVER && incomingCount <= _count) {
        Serial.printf("[Heart] Already have %d >= %d, skip\n", _count, incomingCount);
        return;
    }

    // 新增爱心数
    int newHearts = (_role == HR_NONE) ? incomingCount : (incomingCount - _count);
    if (_role == HR_NONE) _becomeReceiver();
    _count = incomingCount;
    _saveState();

    // 新增爱心掉落动画
    for (int n = 0; n < newHearts && _count <= HEART_MAX_COUNT; n++) {
        int idx = _count - newHearts + n;
        if (idx < 100) {
            _hearts[idx].x = random(5, 220);
            _hearts[idx].y = 0;
            _hearts[idx].targetY = random(20, 215);
            _hearts[idx].angle = random(0, 30);
            _hearts[idx].landed = false;
        }
    }

    Serial.printf("[Heart] Received total=%d (+%d new)\n", _count, newHearts);
    DisplayScreen::markDirty(ZONE_HEART);
}

void UIHeart::onHeartAck(const String& json) {
    // 对方接受了 → 清空本地 + 清除服务器 retained
    if (_role == HR_SENDER) {
        String topic = "sdd/" + String(AppIdentity::name()) + "/heart/send";
        NetworkMQTT::publish(topic.c_str(), "", true);
        _clear();
        Serial.println("[Heart] Partner accepted — cleared (retained wiped)");
        DisplayScreen::markDirty(ZONE_HEART);
    }
}

// ===== 内部 =====
void UIHeart::_becomeSender()   { _role = HR_SENDER; _saveState(); }
void UIHeart::_becomeReceiver() { _role = HR_RECEIVER; _saveState(); }
void UIHeart::_clear() {
    _role = HR_NONE;
    _count = 0;
    _lastSendTime = 0;
    StoragePrefs::clearHeartState();
    memset(_hearts, 0, sizeof(_hearts));
}
void UIHeart::_saveState() {
    StoragePrefs::setHeartRole((int)_role);
    StoragePrefs::setHeartCount(_count);
    StoragePrefs::setLastHeartTime(_lastSendTime);
}

HeartRole UIHeart::role() { return _role; }
int UIHeart::count() { return _count; }

unsigned long UIHeart::cooldownRemaining() {
    if (_role != HR_SENDER) return 0;
    if (_lastSendTime == 0) return 0;
    unsigned long elapsed = millis() - _lastSendTime;
    if (elapsed >= HEART_COOLDOWN_MS) return 0;
    return HEART_COOLDOWN_MS - elapsed;
}

// ===== 绘制 =====
void UIHeart::draw() {
    // 从底部 175 到 240
    switch (_role) {
        case HR_NONE:    _drawIdleArea();    break;
        case HR_SENDER:  _drawSenderArea();  break;
        case HR_RECEIVER:_drawReceiverArea();break;
    }
}

void UIHeart::_drawIdleArea() {
    TFT_eSPI& tft = DisplayScreen::tft();
    tft.fillRect(0, 184, 240, 56, TFT_BLACK);
    tft.setSwapBytes(true);
    tft.pushImage(96, 188, LOVE_W, LOVE_H, love_img);
    tft.setSwapBytes(false);
    tft.setTextColor(0xFE78, TFT_BLACK);  // 柔光粉
    tft.setTextSize(1);
    tft.setCursor(80, 220);
    tft.print("Tap to send");
}

void UIHeart::_drawSenderArea() {
    TFT_eSPI& tft = DisplayScreen::tft();
    tft.fillRect(0, 184, 240, 56, TFT_BLACK);

    tft.setSwapBytes(true);
    tft.pushImage(20, 188, LOVE_W, LOVE_H, love_img);
    tft.setSwapBytes(false);
    tft.setTextColor(0xFE78, TFT_BLACK);  // 柔光粉
    tft.setTextSize(2);
    tft.setCursor(75, 196);
    tft.print("Sent: ");
    tft.print(_count);

    tft.setTextSize(1);
    if (_count < HEART_MAX_COUNT) {
        tft.setTextColor(0x07E0, TFT_BLACK);
        tft.setCursor(70, 224);
        tft.print("Tap to send more");
    } else {
        tft.setTextColor(0x7BEF, TFT_BLACK);
        tft.setCursor(70, 224);
        tft.print("Max 99 reached");
    }
}

void UIHeart::_drawReceiverArea() {
    TFT_eSPI& tft = DisplayScreen::tft();
    // 全屏爱心覆盖 (散落)
    tft.fillScreen(TFT_BLACK);

    for (int i = 0; i < _count && i < 100; i++) {
        int hx = _hearts[i].x;
        int hy = _hearts[i].y;
        if (hy <= 1) continue;  // 还没开始下落
        int size = 10 + (i % 5);
        _drawHeart(hx, hy, size, TFT_RED);
    }

    // 底部操作提示
    tft.fillRect(0, 210, 240, 30, 0x2104);
    if (_count > 0) {
        tft.setTextColor(TFT_WHITE, 0x2104);
        tft.setTextSize(2);
        tft.setCursor(30, 214);
        tft.printf("Tap to accept (%d)", _count);
    } else {
        tft.setTextColor(0x7BEF, 0x2104);
        tft.setTextSize(2);
        tft.setCursor(65, 214);
        tft.print("Waiting...");
    }
}

// ===== 爱心几何绘制 (三角形+两个圆, 简单版) =====
void UIHeart::_drawHeart(int x, int y, int size, uint16_t color) {
    TFT_eSPI& tft = DisplayScreen::tft();
    // 简化爱心: 两个圆 + 一个三角形
    int r = size / 4;
    // 左上圆
    tft.fillCircle(x - r, y - r, r, color);
    // 右上圆
    tft.fillCircle(x + r, y - r, r, color);
    // 底部三角
    tft.fillTriangle(x - size / 2, y,
                     x + size / 2, y,
                     x, y + size / 2 + r,
                     color);
}
