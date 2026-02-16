#ifndef DAQ_H
#define DAQ_H

#include <Arduino.h>

// DAQ Data Structure
struct DAQ {
    float analog[10];
    bool inputs[5];
    bool relays[8];
    String script;
    char inputNames[10][32];
};

// Global DAQ instance
extern DAQ daq;

#endif
