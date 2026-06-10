/**
 * @file audio.h
 * @brief Whisper I2S 音频输出 (MAX98357A)
 *
 * 播放简单提示音（蜂鸣声 / 旋律），无震动
 * 注意：不使用 MP3 解码，纯 PWM/方波合成
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>

class Audio {
public:
    Audio();
    ~Audio();

    /** 初始化 I2S 音频输出 */
    bool begin();

    /** 播放收到消息提示音（"叮咚"） */
    void playMessageTone();

    /** 播放心跳提示音（柔和"叮"） */
    void playHeartbeatTone();

    /** 播放发送成功提示音（短"滴"） */
    void playSendTone();

    /** 播放错误提示音（低沉"嘟"） */
    void playErrorTone();

    /** 播放开机提示音 */
    void playBootTone();

    /** 停止当前播放 */
    void stop();

    /** 是否正在播放 */
    bool isPlaying();

private:
    bool _initialized = false;
    bool _playing = false;

    /** 播放指定频率和时长的音调（内部方法 */
    void _tone(int frequency, int durationMs);

    /** 启用功放使能引脚 */
    void _ampEnable(bool en);
};

#endif // AUDIO_H
