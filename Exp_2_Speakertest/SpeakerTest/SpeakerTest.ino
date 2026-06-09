#include <Arduino.h>
#include "driver/i2s.h"

#define I2S_BCLK 41
#define I2S_LRCLK 42
#define I2S_DOUT 2

#define SAMPLE_RATE 16000

// I2S config
i2s_config_t i2s_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
  .sample_rate = SAMPLE_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = I2S_COMM_FORMAT_I2S,
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 4,
  .dma_buf_len = 512
};

i2s_pin_config_t pin_config = {
  .bck_io_num = I2S_BCLK,
  .ws_io_num = I2S_LRCLK,
  .data_out_num = I2S_DOUT,
  .data_in_num = I2S_PIN_NO_CHANGE
};

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing I2S speaker...");

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  delay(500);

  Serial.println("Playing 1 kHz beep...");
  playTone(1000, 1000);  // 1 kHz for 1 second

  delay(1000);

  Serial.println("Playing sweep test...");
  for (int freq = 200; freq <= 4000; freq += 50) {
    playTone(freq, 50);
  }

  Serial.println("Test complete!");
}

void loop() {
  // Nothing here
}

void playTone(int freq, int duration_ms) {
  const int samples = SAMPLE_RATE * duration_ms / 1000;
  const float increment = 2.0f * PI * freq / SAMPLE_RATE;

  int16_t sample;
  float angle = 0;

  for (int i = 0; i < samples; i++) {
    sample = (int16_t)(15000 * sin(angle));
    angle += increment;

    size_t written;
    i2s_write(I2S_NUM_0, &sample, sizeof(sample), &written, portMAX_DELAY);
  }
}
