#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\nDAQ_ESP32_S3 - nouveau projet");
  Serial.println("Carte: ESP32-S3-DevKitC-1");
  Serial.println("Framework: Arduino");
}

void loop() {
  Serial.println("DAQ_ESP32_S3 en fonctionnement...");
  delay(1000);
}
