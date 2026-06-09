#include <driver/i2s.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#define I2S_WS  48
#define I2S_SCK 14
#define I2S_SD  3
#define I2S_PORT I2S_NUM_0
#define BUFFER_LEN 512

const char* ssid     = "Tecnologia";
const char* password = "11213141";
const char* pc_ip    = "10.247.234.22";  // run ipconfig to find this
const int udp_port   = 5005;

WiFiUDP udp;

void i2s_init() {
    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = BUFFER_LEN,
    };
    i2s_pin_config_t pins = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };
    i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
    i2s_set_pin(I2S_PORT, &pins);
    i2s_start(I2S_PORT);
}

void setup() {
    Serial.begin(115200);
    i2s_init();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nConnected. IP: " + WiFi.localIP().toString());
    udp.begin(udp_port);
}

void loop() {
    int32_t raw[BUFFER_LEN];
    int16_t pcm[BUFFER_LEN];
    size_t bytes_read = 0;

    i2s_read(I2S_PORT, raw, sizeof(raw), &bytes_read, portMAX_DELAY);

    int count = bytes_read / sizeof(int32_t);
    for (int i = 0; i < count; i++) {
        pcm[i] = raw[i] >> 14;
    }

    udp.beginPacket(pc_ip, udp_port);
    udp.write((uint8_t*)pcm, count * sizeof(int16_t));
    udp.endPacket();
}