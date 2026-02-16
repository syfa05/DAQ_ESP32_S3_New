#include <Arduino.h>

// --- MAPPING DES SORTIES (%QX0.0 à %QX0.7) ---
// GPIO 11 à 18
const int pin_outputs[] = {11, 12, 13, 14, 15, 16, 17, 18};

// --- MAPPING DES ENTRÉES DIGITALES (%IX0.0 à %IX0.4) ---
// GPIO 38 à 42
const int pin_inputs[] = {38, 39, 40, 41, 42};

// --- MAPPING DES ENTRÉES ANALOGIQUES (%IW0 à %IW9) ---
// GPIO 1 à 10
const int pin_analog[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

void initHAL() {
    // Initialisation des Sorties (Relais)
    for (int i = 0; i < 8; i++) {
        pinMode(pin_outputs[i], OUTPUT);
        digitalWrite(pin_outputs[i], LOW);
    }

    // Initialisation des Entrées Digitales (Optos)
    for (int i = 0; i < 5; i++) {
        pinMode(pin_inputs[i], INPUT); 
    }

    // Initialisation des Entrées Analogiques
    for (int i = 0; i < 10; i++) {
        pinMode(pin_analog[i], INPUT);
    }
}

void updateHAL() {
    // 1. Lire les entrées physiques et les mettre dans les variables OpenPLC
    for (int i = 0; i < 5; i++) {
        bool val = digitalRead(pin_inputs[i]);
        // OpenPLC utilise un tableau interne bool_input[]
        set_bool_input(i, val); 
    }

    // 2. Lire les entrées analogiques (ADC 12 bits -> 0-4095)
    for (int i = 0; i < 10; i++) {
        uint16_t val = analogRead(pin_analog[i]);
        set_int_input(i, val);
    }

    // 3. Écrire les sorties physiques depuis les variables OpenPLC
    for (int i = 0; i < 8; i++) {
        bool val = get_bool_output(i);
        digitalWrite(pin_outputs[i], val);
    }
}