#define WEBSOCKETS_MAX_DATA_SIZE 2048

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebSocketsServer.h>
#include "driver/i2s.h"

#define I2S_MIC_WS   48
#define I2S_MIC_SCK  14
#define I2S_MIC_SD   3
#define I2S_MIC_PORT I2S_NUM_1

#define I2S_SPK_BCLK  41
#define I2S_SPK_LRCLK 42
#define I2S_SPK_DOUT  2
#define I2S_SPK_PORT  I2S_NUM_0

#define MIC_BUFFER 512
#define UDP_PORT   5005

const char* ssid     = "Tecnologia";
const char* password = "11213141";
const char* pc_ip    = "10.247.234.22";

WiFiUDP udp;
WebSocketsServer webSocket(8765);

int32_t* raw_buf = nullptr;
int16_t* pcm_buf = nullptr;

void i2s_mic_init() {
    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = MIC_BUFFER,
    };
    i2s_pin_config_t pins = {
        .bck_io_num   = I2S_MIC_SCK,
        .ws_io_num    = I2S_MIC_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num  = I2S_MIC_SD
    };
    i2s_driver_install(I2S_MIC_PORT, &cfg, 0, NULL);
    i2s_set_pin(I2S_MIC_PORT, &pins);
    i2s_start(I2S_MIC_PORT);
}

void i2s_spk_init() {
    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 24000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512
    };
    i2s_pin_config_t pins = {
        .bck_io_num   = I2S_SPK_BCLK,
        .ws_io_num    = I2S_SPK_LRCLK,
        .data_out_num = I2S_SPK_DOUT,
        .data_in_num  = I2S_PIN_NO_CHANGE
    };
    i2s_driver_install(I2S_SPK_PORT, &cfg, 0, NULL);
    i2s_set_pin(I2S_SPK_PORT, &pins);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_BIN) {
        size_t written;
        i2s_write(I2S_SPK_PORT, payload, length, &written, portMAX_DELAY);
    } else if (type == WStype_TEXT) {
        String msg = String((char*)payload);
        if (msg.indexOf("end") >= 0) {
            Serial.println("Playback done.");
        }
    } else if (type == WStype_CONNECTED) {
        Serial.println("Python connected.");
    } else if (type == WStype_DISCONNECTED) {
        Serial.println("Python disconnected.");
    }
}

void mic_task(void* param) {
    while (true) {
        size_t bytes_read = 0;
        i2s_read(I2S_MIC_PORT, raw_buf, MIC_BUFFER * sizeof(int32_t), &bytes_read, portMAX_DELAY);
        int count = bytes_read / sizeof(int32_t);
        for (int i = 0; i < count; i++) {
            pcm_buf[i] = raw_buf[i] >> 14;
        }
        udp.beginPacket(pc_ip, UDP_PORT);
        udp.write((uint8_t*)pcm_buf, count * sizeof(int16_t));
        udp.endPacket();
    }
}

void setup() {
    Serial.begin(115200);

    raw_buf = (int32_t*)ps_malloc(MIC_BUFFER * sizeof(int32_t));
    pcm_buf = (int16_t*)ps_malloc(MIC_BUFFER * sizeof(int16_t));
    if (!raw_buf || !pcm_buf) {
        Serial.println("PSRAM alloc failed!");
        while (1);
    }
    Serial.println("PSRAM allocated.");

    i2s_mic_init();
    i2s_spk_init();

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nIP: " + WiFi.localIP().toString());

    udp.begin(UDP_PORT);
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    Serial.println("Ready.");

    xTaskCreatePinnedToCore(mic_task, "mic", 8192, NULL, 1, NULL, 1);
}

void loop() {
    webSocket.loop();
}