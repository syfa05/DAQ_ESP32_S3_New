#ifndef HAL_H
#define HAL_H

#include <Arduino.h>

// Initialisation des GPIO selon le mapping validé
void initHAL();

// Lecture des entrées et mise à jour des sorties
void updateHAL();

// Fonctions utilitaires pour le Dashboard
float readAnalogChannel(int ch);
bool readDigitalInput(int ch);
void writeRelay(int ch, bool state);

#endif