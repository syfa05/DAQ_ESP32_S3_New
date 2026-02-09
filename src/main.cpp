#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
// #include <extEEPROM.h> // TODO: Add correct library 
#include "pins.h"
#include "web_index.h"

// --- STRUCTURE DE CONFIGURATION (Stockée en EEPROM) ---
struct Settings {
    bool relayStates[8];
    char inputNames[10][20];
    float alertThresholds[10];
} settings;

// Déclaration de l'EEPROM AT24C32 (32kbits, 1 device, 32 bytes page size)
// TODO: Configure correct EEPROM library
// extEEPROM myEEPROM(kbits_32, 1, 32, EEPROM_ADDR);

AsyncWebServer server(80);

// --- GESTION EEPROM ---
void saveSettings() {
    // TODO: Implement EEPROM write with correct library
    // uint8_t err = myEEPROM.write(0, (uint8_t*)&settings, sizeof(settings));
    // if (err != 0) Serial.printf("Erreur ecriture EEPROM: %d\n", err);
}

void loadSettings() {
    // TODO: Implement EEPROM read with correct library
    // myEEPROM.read(0, (uint8_t*)&settings, sizeof(settings));
    
    // Initialisation si l'EEPROM est vierge (détection via le premier nom)
    if (settings.inputNames[0][0] == 0 || (uint8_t)settings.inputNames[0][0] == 255) {
        Serial.println("EEPROM vierge. Initialisation par defaut...");
        for(int i=0; i<8; i++) settings.relayStates[i] = false;
        for(int i=0; i<10; i++) {
            sprintf(settings.inputNames[i], "Capteur %d", i+1);
            settings.alertThresholds[i] = 26.0; // Seuil d'alerte par défaut 26V
        }
        saveSettings();
    }
}

// --- LECTURE ANALOGIQUE (0-24V) ---
float getVoltage(int ch) {
    long sum = 0;
    for(int i=0; i<64; i++) sum += analogRead(ANALOG_PINS[ch]);
    float avgRaw = (float)sum / 64.0;
    // Formule basée sur vos résistances 68k / 33k
    return (avgRaw * 3.3 / 4095.0) * ((68000.0 + 33000.0) / 33000.0);
}

// --- SETUP ---
void setup() {
    Serial.begin(115200);
    
    // 1. Initialisation I2C & EEPROM
    Wire.begin(I2C_SDA, I2C_SCL);
    // TODO: Initialize EEPROM with correct library
    // uint8_t eepromStatus = myEEPROM.begin(myEEPROM.twiClock400kHz);
    // if (eepromStatus != 0) Serial.println("ERREUR: EEPROM non detectee!");
    
    loadSettings();

    // 2. Initialisation Hardware (Relais, Buzzer, ADC)
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    analogReadResolution(12);

    for(int i=0; i<NUM_RELAYS; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        digitalWrite(RELAY_PINS[i], settings.relayStates[i]);
    }

    // 3. WiFi (Mode Point d'Accès)
    WiFi.softAP("DAQ-ESP32-PRO", "12345678");
    Serial.println("Serveur pret sur http://192.168.4.1");

    // --- ROUTES SERVEUR WEB ---

    // Page d'accueil
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){
        r->send(200, "text/html", index_html);
    });

    // API de données
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
        String response;
        serializeJson(doc, response);
        r->send(200, "application/json", response);
    });

    // API Contrôle Relais
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

    // API Renommer Entrée
    server.on("/rename", HTTP_GET, [](AsyncWebServerRequest *r){
        if(r->hasParam("id") && r->hasParam("name")) {
            int id = r->getParam("id")->value().toInt();
            strncpy(settings.inputNames[id], r->getParam("name")->value().c_str(), 19);
            saveSettings();
        }
        r->send(200, "text/plain", "OK");
    });

    // API Modifier Seuil
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

// --- LOOP (Surveillance Alarme) ---
void loop() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 1500) {
        lastCheck = millis();
        bool alarm = false;
        for(int i=0; i<10; i++) {
            if(settings.alertThresholds[i] > 0 && getVoltage(i) > settings.alertThresholds[i]) {
                alarm = true;
            }
        }
        if(alarm) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(100);
            digitalWrite(BUZZER_PIN, LOW);
        }
    }
}