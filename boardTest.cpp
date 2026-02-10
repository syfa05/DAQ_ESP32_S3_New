#include <Arduino.h>
#include <Wire.h>
#include "pins.h"

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n========================================");
    Serial.println("   PROCEDURE DE TEST MATERIEL DAQ V2   ");
    Serial.println("========================================\n");

    // 1. Initialisation des Pins
    pinMode(BUZZER_PIN, OUTPUT);
    for (int i = 0; i < NUM_RELAYS; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        digitalWrite(RELAY_PINS[i], LOW);
    }
    analogReadResolution(12);

    // 2. Test I2C (EEPROM AT24C32)
    Serial.println("[1/4] Scan I2C (Pins 47/48)...");
    Wire.begin(I2C_SDA, I2C_SCL);
    byte nDevices = 0;
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  -> Trouvé dispositif à 0x%02X", address);
            if (address == 0x50) Serial.println(" [EEPROM OK]");
            else Serial.println(" [Inconnu]");
            nDevices++;
        }
    }
    if (nDevices == 0) Serial.println("  !! ERREUR: Aucun composant I2C détecté.");

    // 3. Test Buzzer (Transistor Q1 / IO46)
    Serial.println("\n[2/4] Test Buzzer (IO46)...");
    for(int i=0; i<3; i++) {
        digitalWrite(BUZZER_PIN, HIGH); delay(100);
        digitalWrite(BUZZER_PIN, LOW);  delay(100);
    }
    Serial.println("  -> Buzzer terminé.");

    // 4. Test Relais (Chenillard ULN2803 / IO11-18)
    Serial.println("\n[3/4] Test Relais (Sequence 1 a 8)...");
    for (int i = 0; i < NUM_RELAYS; i++) {
        Serial.printf("  -> Relais %d ON\n", i + 1);
        digitalWrite(RELAY_PINS[i], HIGH);
        delay(300);
        digitalWrite(RELAY_PINS[i], LOW);
    }
    Serial.println("  -> Relais terminés.");

    Serial.println("\n[4/4] Lecture des 10 Entrees Analogiques...");
}

void loop() {
    Serial.print("\rTensions: ");
    for (int i = 0; i < NUM_ANALOG_INPUTS; i++) {
        float raw = analogRead(ANALOG_PINS[i]);
        float voltage = (raw * 3.3 / 4095.0) * ((68000.0 + 33000.0) / 33000.0);
        Serial.printf("[%d:%.2fV] ", i + 1, voltage);
    }
    delay(500); // Mise à jour toutes les 0.5s
}