#ifndef PINS_H
#define PINS_H

// Entrées Analogiques (0-24V via diviseur)
const int ANALOG_PINS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; // %IW0 - %IW9

// Entrées Digitales (Opto-isolées)
const int INPUT_PINS[] = {38, 39, 40, 41, 42};             // %IX0.0 - %IX0.4

// Sorties Digitales (Relais via ULN2803)
const int RELAY_PINS[] = {11, 12, 13, 14, 15, 16, 17, 18}; // %QX0.0 - %QX0.7

// Système
//#define STATUS_LED 46
//#define SD_CS 45
#define I2C_SDA 47
#define I2C_SCL 48


#define NUM_ANALOG_INPUTS 10

// I2C (EEPROM AT24C32)

#define EEPROM_ADDR 0x50

// SPI Carte SD
#define SD_CS   45
#define SD_MISO 37
#define SD_CLK  36
#define SD_MOSI 35

// Alerte
#define BUZZER_PIN 46
#define STATUS_LED 38 // LED D3/D4 selon routage


#endif