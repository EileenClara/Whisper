/**
 * @file audio.cpp
 * @brief Whisper I2S 音频输出 — 实现
 *
 * MAX98357A 是 I2S DAC，我们通过 I2S 发送简单的方波数据
 * 产生不同频率的提示音。不依赖音频文件，纯合成。
 *
 * 简化方案：使用 ESP32 LEDC (PWM) 直接输出到 MAX98357A 的 I2S DIN
 * 因为 MAX98357A 在单声道模式下可以接收简单的 PWM 信号。
 *
 * 更简单：直接用 ledc 输出蜂鸣音到 AUDIO_ENABLE_PIN（如果功放也能接受PWM）
 *
 * 实际采用方案：通过 I2S 发送 16-bit 方波采样
 */

#include "audio.h"
#include "config.h"
#include <driver/i2s.h>
#include <driver/gpio.h>

// 采样率 16kHz，单声道 16-bit
#define I2S_SAMPLE_RATE     16000
#define I2S_BUFFER_SIZE     512  // 采样点

static int16_t _i2s_buffer[I2S_BUFFER_SIZE];

Audio::Audio() {}

Audio::~Audio() {
    stop();
}

bool Audio::begin() {
    // 功放使能引脚
    pinMode(AUDIO_ENABLE_PIN, OUTPUT);
    _ampEnable(false);

    // 配置 I2S
    i2s_config_t i2s_cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,  // MAX98357A 用右声道
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0,
    };

    i2s_pin_config_t pin_cfg = {
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_LRCK_PIN,
        .data_out_num = I2S_DIN_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE,
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_cfg, 0, nullptr);
    if (err != ESP_OK) {
        Serial.printf("[Audio] ❌ I2S驱动安装失败: %d\n", err);
        _initialized = false;
        return false;
    }

    err = i2s_set_pin(I2S_NUM_0, &pin_cfg);
    if (err != ESP_OK) {
        Serial.printf("[Audio] ❌ I2S引脚配置失败: %d\n", err);
        i2s_driver_uninstall(I2S_NUM_0);
        _initialized = false;
        return false;
    }

    _initialized = true;
    Serial.println("[Audio] ✅ MAX98357A I2S 就绪");
    return true;
}

void Audio::playMessageTone() {
    // "叮~咚" — 先高音后低音
    _ampEnable(true);
    _tone(1200, 150);
    delay(50);
    _tone(800, 300);
    _ampEnable(false);
    Serial.println("[Audio] 消息提示音");
}

void Audio::playHeartbeatTone() {
    // 柔和"叮~" — 单音渐弱
    _ampEnable(true);
    _tone(1000, 100);
    delay(50);
    _tone(1000, 100);
    _ampEnable(false);
    Serial.println("[Audio] 心跳提示音");
}

void Audio::playSendTone() {
    // 短"滴"
    _ampEnable(true);
    _tone(1500, 80);
    _ampEnable(false);
    Serial.println("[Audio] 发送提示音");
}

void Audio::playErrorTone() {
    // 低沉"嘟"
    _ampEnable(true);
    _tone(300, 400);
    _ampEnable(false);
    Serial.println("[Audio] 错误提示音");
}

void Audio::playBootTone() {
    // 开机三连音 "滴滴滴"（渐高）
    _ampEnable(true);
    _tone(800, 100);
    delay(80);
    _tone(1000, 100);
    delay(80);
    _tone(1200, 150);
    _ampEnable(false);
    Serial.println("[Audio] 开机提示音");
}

void Audio::stop() {
    _ampEnable(false);
    i2s_zero_dma_buffer(I2S_NUM_0);
    _playing = false;
}

bool Audio::isPlaying() {
    return _playing;
}

// ---- 私有方法 ----

void Audio::_tone(int frequency, int durationMs) {
    if (!_initialized) return;

    int samples_per_cycle = I2S_SAMPLE_RATE / frequency;
    int total_samples = (I2S_SAMPLE_RATE * durationMs) / 1000;
    int cycles = total_samples / samples_per_cycle;

    _playing = true;

    for (int c = 0; c < cycles; c++) {
        // 生成一个周期的方波（正半周 + 负半周）
        int half = samples_per_cycle / 2;
        int idx = 0;

        // 正半周
        for (int i = 0; i < half && idx < I2S_BUFFER_SIZE; i++, idx++) {
            _i2s_buffer[idx] = 8000;  // 50% 幅度
        }
        // 负半周
        for (int i = 0; i < half && idx < I2S_BUFFER_SIZE; i++, idx++) {
            _i2s_buffer[idx] = -8000;
        }

        size_t bytes_written;
        i2s_write(I2S_NUM_0, _i2s_buffer, idx * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    }

    // 发送一段静音以清除残留
    memset(_i2s_buffer, 0, I2S_BUFFER_SIZE * sizeof(int16_t));
    size_t bytes_written;
    i2s_write(I2S_NUM_0, _i2s_buffer, I2S_BUFFER_SIZE * sizeof(int16_t), &bytes_written, 0);

    _playing = false;
}

void Audio::_ampEnable(bool en) {
    digitalWrite(AUDIO_ENABLE_PIN, en ? HIGH : LOW);
    delayMicroseconds(100);
}
