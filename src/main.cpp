#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "pins.h"
#include "web_index.h"

struct Settings {
    bool relayStates[8];
    char inputNames[10][20];
    float alertThresholds[10];
} settings;

AsyncWebServer server(80);

// --- FONCTIONS I2C NATIVES POUR AT24C32 ---

void writeEEPROM(uint16_t address, uint8_t data) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(address >> 8));   // MSB
    Wire.write((uint8_t)(address & 0xFF)); // LSB
    Wire.write(data);
    Wire.endTransmission();
    delay(5); // Temps d'écriture obligatoire pour AT24C32
}

uint8_t readEEPROM(uint16_t address) {
    uint8_t data = 0xFF;
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write((uint8_t)(address >> 8));
    Wire.write((uint8_t)(address & 0xFF));
    Wire.endTransmission();
    Wire.requestFrom(EEPROM_ADDR, (uint8_t)1);
    if (Wire.available()) data = Wire.read();
    return data;
}

void saveSettings() {
    uint8_t* p = (uint8_t*)&settings;
    for (uint16_t i = 0; i < sizeof(settings); i++) {
        writeEEPROM(i, *p++);
    }
    Serial.println("Sauvegarde EEPROM OK");
}

void loadSettings() {
    uint8_t* p = (uint8_t*)&settings;
    for (uint16_t i = 0; i < sizeof(settings); i++) {
        *p++ = readEEPROM(i);
    }
    
    // Initialisation si vierge
    if (settings.inputNames[0][0] == 0 || (uint8_t)settings.inputNames[0][0] == 255) {
        Serial.println("Initialisation par defaut...");
        for(int i=0; i<8; i++) settings.relayStates[i] = false;
        for(int i=0; i<10; i++) {
            sprintf(settings.inputNames[i], "Entree %d", i+1);
            settings.alertThresholds[i] = 26.0;
        }
        saveSettings();
    }
}

// --- RESTE DU CODE ---

float getVoltage(int ch) {
    long sum = 0;
    for(int i=0; i<64; i++) sum += analogRead(ANALOG_PINS[ch]);
    return ((float)sum / 64.0 * 3.3 / 4095.0) * ((68000.0 + 33000.0) / 33000.0);
}

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL); // IO47, IO48
    
    loadSettings();

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    analogReadResolution(12);

    for(int i=0; i<NUM_RELAYS; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        digitalWrite(RELAY_PINS[i], settings.relayStates[i]);
    }

    WiFi.softAP("DAQ-ESP32-PRO", "12345678");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(200, "text/html", index_html); });

    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *r){
        StaticJsonDocument<1500> doc;
        JsonArray a = doc.createNestedArray("analog");
        JsonArray n = doc.createNestedArray("names");
        JsonArray t = doc.createNestedArray("thresholds");
        JsonArray rs = doc.createNestedArray("relays");
        for(int i=0; i<10; i++) {
            a.add(getVoltage(i));
            n.add(settings.inputNames[i]);
            t.add(settings.alertThresholds[i]);
        }
        for(int i=0; i<8; i++) rs.add(settings.relayStates[i]);
        String response; serializeJson(doc, response);
        r->send(200, "application/json", response);
    });

    server.on("/relay", HTTP_GET, [](AsyncWebServerRequest *r){
        if(r->hasParam("id") && r->hasParam("state")) {
            int id = r->getParam("id")->value().toInt();
            int s = r->getParam("state")->value().toInt();
            digitalWrite(RELAY_PINS[id], s);
            settings.relayStates[id] = (s == 1);
            saveSettings();
        }
        r->send(200, "text/plain", "OK");
    });

    server.on("/rename", HTTP_GET, [](AsyncWebServerRequest *r){
        if(r->hasParam("id") && r->hasParam("name")) {
            int id = r->getParam("id")->value().toInt();
            strncpy(settings.inputNames[id], r->getParam("name")->value().c_str(), 19);
            saveSettings();
        }
        r->send(200, "text/plain", "OK");
    });

    server.on("/limit", HTTP_GET, [](AsyncWebServerRequest *r){
        if(r->hasParam("id") && r->hasParam("val")) {
            int id = r->getParam("id")->value().toInt();
            settings.alertThresholds[id] = r->getParam("val")->value().toFloat();
            saveSettings();
        }
        r->send(200, "text/plain", "OK");
    });

    server.begin();
}

void loop() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 1500) {
        lastCheck = millis();
        bool alarm = false;
        for(int i=0; i<10; i++) {
            if(settings.alertThresholds[i] > 0 && getVoltage(i) > settings.alertThresholds[i]) alarm = true;
        }
        if(alarm) { digitalWrite(BUZZER_PIN, HIGH); delay(100); digitalWrite(BUZZER_PIN, LOW); }
    }
}