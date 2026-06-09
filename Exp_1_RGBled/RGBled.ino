#include <Adafruit_NeoPixel.h>

#define LED_PIN   48  // Built-in RGB LED on ESP32-S3 DevKitM
#define NUM_LEDS  1

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(50);  // 0-255
  strip.show();
}

void loop() {
  // Red
  strip.setPixelColor(0, strip.Color(255, 0, 0));
  strip.show();
  delay(2000);

  // Green
  strip.setPixelColor(0, strip.Color(0, 255, 0));
  strip.show();
  delay(2000);

  // Blue
  strip.setPixelColor(0, strip.Color(0, 0, 255));
  strip.show();
  delay(2000);

  // OFF
  strip.setPixelColor(0, strip.Color(0, 0, 0));
  strip.show();
  delay(2000);
}