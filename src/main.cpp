#include <Arduino.h>

#include <Arduino.h>
#include "pins.h"

// Configuration des résistances du pont diviseur (à ajuster selon votre BOM)
// Exemple : R_haut = 68k, R_bas = 33k (Ratio ~3.06)
const float R_HIGH = 68000.0; 
const float R_LOW = 33000.0;
const float VOLTAGE_DIVIDER_RATIO = (R_HIGH + R_LOW) / R_LOW;

// Référence de tension de l'ESP32-S3 (généralement 3.1V à 3.3V)
const float ADC_VREF = 3.3; 

/**
 * Lit une tension réelle sur une entrée spécifique
 * @param channel Index de l'entrée (0 à 9)
 * @return Tension réelle en Volts
 */
float readAnalogVoltage(int channel) {
    if (channel < 0 || channel >= NUM_ANALOG_INPUTS) return 0.0;

    long sum = 0;
    const int SAMPLES = 64; // On prend 64 mesures pour lisser le bruit

    for (int i = 0; i < SAMPLES; i++) {
        sum += analogRead(ANALOG_PINS[channel]);
        delayMicroseconds(10);
    }

    float avgRaw = (float)sum / SAMPLES;
    
    // Conversion Raw -> Tension aux bornes de l'ESP32 (0-3.3V)
    float vPin = (avgRaw * ADC_VREF) / 4095.0;

    // Conversion Tension Pin -> Tension Réelle (0-24V+)
    return vPin * VOLTAGE_DIVIDER_RATIO;
}

void setupAnalog() {
    // Configuration de la résolution ADC à 12 bits (0-4095)
    analogReadResolution(12);
    
    // Optionnel : ajuster l'atténuation pour lire jusqu'à 3.1V max sur la pin
    for(int i=0; i < NUM_ANALOG_INPUTS; i++) {
        pinMode(ANALOG_PINS[i], INPUT);
    }
}

/** 
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
*/