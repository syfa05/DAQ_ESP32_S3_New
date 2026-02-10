/* * Définition des broches pour le projet DAQ-ESP32-S3
 * Basé sur le schéma : 07012026-PCB-DAQ-FS
 */

#ifndef PINS_H
#define PINS_H
#define EEPROM_ADDR 0x50 // Adresse I2C pour l'AT24C32

// --- SORTIES RELAIS (Via ULN2803) ---
const int RELAY_PINS[] = {11, 12, 13, 14, 15, 16, 17, 18};
#define NUM_RELAYS 8

// --- ENTRÉES ANALOGIQUES ---
const int ANALOG_PINS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
#define NUM_ANALOG_INPUTS 10

// --- BUS I2C (EEPROM AT24C32) ---
#define I2C_SDA 47
#define I2C_SCL 48
#define EEPROM_I2C_ADDRESS 0x50 // Adresse standard AT24C32

// --- AUTRES PÉRIPHÉRIQUES ---
#define BUZZER_PIN 46 // Selon schéma Q1/LS1
#define STATUS_LED 38 // LED D3/D4 selon routage

// --- CARTE SD (SPI) ---
#define SD_CS   45
#define SD_MISO 37
#define SD_CLK  36
#define SD_MOSI 35

#endif