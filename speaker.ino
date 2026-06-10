#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include "driver/i2s.h"

#define I2S_BCLK  41
#define I2S_LRCLK 42
#define I2S_DOUT  2
#define SAMPLE_RATE 24000

const char* ssid     = "Tecnologia";
const char* password = "11213141";

WebSocketsServer webSocket(8765);

void i2s_init() {
    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512
    };
    i2s_pin_config_t pins = {
        .bck_io_num   = I2S_BCLK,
        .ws_io_num    = I2S_LRCLK,
        .data_out_num = I2S_DOUT,
        .data_in_num  = I2S_PIN_NO_CHANGE
    };
    i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pins);
}

// skip WAV header (44 bytes)
bool headerSkipped = false;
int headerBytes = 0;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type == WStype_BIN) {
        uint8_t* data = payload;
        size_t len = length;

        if (!headerSkipped) {
            if (headerBytes + len >= 44) {
                int skip = 44 - headerBytes;
                data += skip;
                len  -= skip;
                headerSkipped = true;
            } else {
                headerBytes += len;
                return;
            }
        }

        size_t written;
        i2s_write(I2S_NUM_0, data, len, &written, portMAX_DELAY);

    } else if (type == WStype_TEXT) {
        String msg = String((char*)payload);
        if (msg.indexOf("end") >= 0) {
            Serial.println("Playback done.");
            headerSkipped = false;
            headerBytes = 0;
        }
    } else if (type == WStype_CONNECTED) {
        Serial.println("Python connected.");
    } else if (type == WStype_DISCONNECTED) {
        Serial.println("Python disconnected.");
    }
}

void setup() {
    Serial.begin(115200);
    i2s_init();

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nIP: " + WiFi.localIP().toString());

    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    webSocket.loop();
}