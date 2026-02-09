#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
// Using native I2C helpers for AT24C32 instead of ExtEEPROM library
#include "pins.h"
#include "web_index.h"

// --- STRUCTURE DE CONFIGURATION (Stockée en EEPROM) ---
// Taille totale approx : 8 (bool) + 200 (chars) + 40 (floats) = 248 octets
struct Settings {
    bool relayStates[8];
    char inputNames[10][20];
    float alertThresholds[10];
} settings;

// --- AT24C32 EEPROM I2C helper ---
// Device: AT24C32 (32kbit = 4096 bytes), 2-byte memory address, 32-byte page
const uint16_t EEPROM_PAGE_SIZE = 32;
// Use EEPROM address defined in pins.h (EEPROM_ADDR)

// Write `len` bytes from `data` to EEPROM starting at `mem_addr` (0..4095)
bool eeprom_write_bytes(uint16_t mem_addr, const uint8_t* data, size_t len) {
    size_t offset = 0;
    while (offset < len) {
        uint16_t addr = mem_addr + offset;
        uint8_t page_offset = addr % EEPROM_PAGE_SIZE;
        uint8_t space = EEPROM_PAGE_SIZE - page_offset;
        uint8_t chunk = (uint8_t)min((size_t)space, len - offset);

        Wire.beginTransmission(EEPROM_ADDR);
        Wire.write((uint8_t)(addr >> 8));
        Wire.write((uint8_t)(addr & 0xFF));
        Wire.write(data + offset, chunk);
        if (Wire.endTransmission() != 0) return false;

        // Wait for write cycle (typical 5ms)
        delay(5);
        offset += chunk;
    }
    return true;
}

// Read `len` bytes into `buffer` from EEPROM starting at `mem_addr`
bool eeprom_read_bytes(uint16_t mem_addr, uint8_t* buffer, size_t len) {
    size_t offset = 0;
    while (offset < len) {
        uint16_t addr = mem_addr + offset;
        uint8_t chunk = (uint8_t)min((size_t)32, len - offset);

        Wire.beginTransmission(EEPROM_ADDR);
        Wire.write((uint8_t)(addr >> 8));
        Wire.write((uint8_t)(addr & 0xFF));
        if (Wire.endTransmission(false) != 0) return false; // repeated start

        uint8_t read = Wire.requestFrom((int)EEPROM_ADDR, (int)chunk);
        if (read != chunk) return false;
        for (uint8_t i = 0; i < chunk; i++) buffer[offset + i] = Wire.read();
        offset += chunk;
    }
    return true;
}

AsyncWebServer server(80);

// --- GESTION EEPROM ---
void saveSettings() {
    // Écriture de la structure complète à l'adresse 0
    if (eeprom_write_bytes(0, (uint8_t*)&settings, sizeof(settings))) {
        Serial.println("Sauvegarde EEPROM reussie.");
    } else {
        Serial.println("Erreur ecriture EEPROM");
    }
}

void loadSettings() {
    // Lecture de la structure complète
    if (!eeprom_read_bytes(0, (uint8_t*)&settings, sizeof(settings))) {
        Serial.println("EEPROM lecture echouee ou vide.");
        // initialisation par defaut si lecture échoue
        for(int i=0; i<8; i++) settings.relayStates[i] = false;
        for(int i=0; i<10; i++) {
            sprintf(settings.inputNames[i], "Capteur %d", i+1);
            settings.alertThresholds[i] = 26.0;
        }
        saveSettings();
        return;
    }

    // Initialisation si l'EEPROM est vierge (détection via le premier nom ou octet 255)
    if (settings.inputNames[0][0] == 0 || (uint8_t)settings.inputNames[0][0] == 255) {
        Serial.println("EEPROM vierge ou corrompue. Initialisation par defaut...");
        for(int i=0; i<8; i++) settings.relayStates[i] = false;
        for(int i=0; i<10; i++) {
            sprintf(settings.inputNames[i], "Capteur %d", i+1);
            settings.alertThresholds[i] = 26.0; // Seuil d'alerte par défaut 26V
        }
        saveSettings();
    } else {
        Serial.println("Parametres charges depuis l'EEPROM.");
    }
}

// --- LECTURE ANALOGIQUE (0-24V) ---
float getVoltage(int ch) {
    long sum = 0;
    for(int i=0; i<64; i++) sum += analogRead(ANALOG_PINS[ch]);
    float avgRaw = (float)sum / 64.0;
    
    // Formule basée sur vos résistances 68k / 33k (Ratio ~3.06)
    // Tension Pin = (Raw * 3.3 / 4095)
    // Tension Reelle = Tension Pin * (68+33)/33
    return (avgRaw * 3.3 / 4095.0) * ((68000.0 + 33000.0) / 33000.0);
}

// --- SETUP ---
void setup() {
    Serial.begin(115200);
    delay(1000); // Temps de stabilisation
    
    // 1. Initialisation I2C & EEPROM [SDA=47, SCL=48]
    Wire.begin(I2C_SDA, I2C_SCL);
    
    // Quick I2C presence check for EEPROM device
    Wire.beginTransmission(EEPROM_ADDR);
    uint8_t err = Wire.endTransmission();
    if (err != 0) {
        Serial.println("ERREUR: EEPROM AT24C32 non detectee sur bus I2C !");
    }

    loadSettings();

    // 2. Initialisation Hardware (Relais, Buzzer, ADC)
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    analogReadResolution(12);

    for(int i=0; i<NUM_RELAYS; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        // Appliquer l'état sauvegardé au démarrage
        digitalWrite(RELAY_PINS[i], settings.relayStates[i] ? HIGH : LOW);
    }

    // 3. WiFi (Mode Point d'Accès pour test direct)
    WiFi.softAP("DAQ-ESP32-PRO", "12345678");
    Serial.println("WiFi OK : DAQ-ESP32-PRO");
    Serial.print("Adresse IP du serveur : ");
    Serial.println(WiFi.softAPIP());

    // --- ROUTES SERVEUR WEB ---

    // Page d'accueil (Sert l'index_html stocké dans web_index.h)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){
        r->send(200, "text/html", index_html);
    });

    // API de données JSON pour le Dashboard
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

    // API Contrôle Relais (Ex: /relay?id=0&state=1)
    server.on("/relay", HTTP_GET, [](AsyncWebServerRequest *r){
        if(r->hasParam("id") && r->hasParam("state")) {
            int id = r->getParam("id")->value().toInt();
            int s = r->getParam("state")->value().toInt();
            if(id >= 0 && id < 8) {
                digitalWrite(RELAY_PINS[id], s);
                settings.relayStates[id] = (s == 1);
                saveSettings();
                Serial.printf("Relais %d -> %s\n", id+1, s ? "ON":"OFF");
            }
        }
        r->send(200, "text/plain", "OK");
    });

    // API Renommer Entrée (Ex: /rename?id=0&name=Batterie)
    server.on("/rename", HTTP_GET, [](AsyncWebServerRequest *r){
        if(r->hasParam("id") && r->hasParam("name")) {
            int id = r->getParam("id")->value().toInt();
            String newName = r->getParam("name")->value();
            if(id >= 0 && id < 10) {
                memset(settings.inputNames[id], 0, sizeof(settings.inputNames[id]));
                strncpy(settings.inputNames[id], newName.c_str(), 19);
                saveSettings();
            }
        }
        r->send(200, "text/plain", "OK");
    });

    // API Modifier Seuil (Ex: /limit?id=0&val=24.5)
    server.on("/limit", HTTP_GET, [](AsyncWebServerRequest *r){
        if(r->hasParam("id") && r->hasParam("val")) {
            int id = r->getParam("id")->value().toInt();
            float val = r->getParam("val")->value().toFloat();
            if(id >= 0 && id < 10) {
                settings.alertThresholds[id] = val;
                saveSettings();
            }
        }
        r->send(200, "text/plain", "OK");
    });

    server.begin();
}

// --- LOOP (Surveillance Alarme & Heartbeat) ---
void loop() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 1500) {
        lastCheck = millis();
        bool alarmActive = false;
        
        for(int i=0; i<10; i++) {
            // On ne check que si le seuil est > 0
            if (settings.alertThresholds[i] > 0) {
                if (getVoltage(i) > settings.alertThresholds[i]) {
                    alarmActive = true;
                    Serial.printf("!!! ALERTE Tension sur %s !!!\n", settings.inputNames[i]);
                }
            }
        }
        
        if (alarmActive) {
            // Signal sonore bref sur le Buzzer (IO46)
            digitalWrite(BUZZER_PIN, HIGH);
            delay(150);
            digitalWrite(BUZZER_PIN, LOW);
        }
    }
}