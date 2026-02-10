#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include "pins.h"
#include "web_index.h"
#include "tinyexpr.h"

// ============================================
// STRUCTURES & GLOBAUX
// ============================================
struct Settings {
    bool relayStates[8];
    char inputNames[10][20];
    float alertThresholds[10];
} settings;

AsyncWebServer server(80);
SPIClass spiSD(FSPI);

// Variable pour l'intervalle de log (en ms)
unsigned long logInterval = 60000; 
unsigned long lastLogTime = 0;

// ============================================
// EEPROM & PARAMÈTRES
// ============================================
void writeEEPROM(uint16_t addr, uint8_t data) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write(addr >> 8);
    Wire.write(addr & 0xFF);
    Wire.write(data);
    Wire.endTransmission();
    delay(5);
}

uint8_t readEEPROM(uint16_t addr) {
    Wire.beginTransmission(EEPROM_ADDR);
    Wire.write(addr >> 8);
    Wire.write(addr & 0xFF);
    Wire.endTransmission();
    Wire.requestFrom(EEPROM_ADDR, 1);
    return Wire.available() ? Wire.read() : 0xFF;
}

void saveSettings() {
    uint8_t* p = (uint8_t*)&settings;
    for (uint16_t i = 0; i < sizeof(settings); i++) {
        writeEEPROM(i, *p++);
    }
}

void loadSettings() {
    uint8_t* p = (uint8_t*)&settings;
    for (uint16_t i = 0; i < sizeof(settings); i++) {
        *p++ = readEEPROM(i);
    }
    // Initialiser les noms par défaut si vide
    if (settings.inputNames[0][0] == 0 || (uint8_t)settings.inputNames[0][0] == 255) {
        for (int i = 0; i < 10; i++) {
            sprintf(settings.inputNames[i], "Entree %d", i + 1);
        }
        saveSettings();
    }
}

// ============================================
// LECTURES ANALOGIQUES
// ============================================
float getV(int ch) {
    long sum = 0;
    for (int i = 0; i < 32; i++) {
        sum += analogRead(ANALOG_PINS[ch]);
    }
    return (sum / 32.0 * 3.3 / 4095.0) * 3.06; // Ratio 68k/33k
}

// ============================================
// MOTEUR LOGIQUE TINYEXPR
// ============================================
void runSDScript() {
    File f = SD.open("/script.txt");
    if (!f) return;
    
    double v[10];
    for (int i = 0; i < 10; i++) {
        v[i] = getV(i);
    }
    
    te_variable vars[] = {
        {"AN1", &v[0]}, {"AN2", &v[1]}, {"AN3", &v[2]},
        {"AN4", &v[3]}, {"AN5", &v[4]}
    };

    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        
        if (line.indexOf('=') != -1) {
            String target = line.substring(0, line.indexOf('='));
            String exprStr = line.substring(line.indexOf('=') + 1);
            
            int err;
            te_expr *e = te_compile(exprStr.c_str(), vars, 5, &err);
            
            if (e) {
                double result = te_eval(e);
                int relayIdx = target.substring(1).toInt() - 1;
                bool state = result > 0.5;
                
                digitalWrite(RELAY_PINS[relayIdx], state);
                settings.relayStates[relayIdx] = state;
                te_free(e);
            }
        }
    }
    f.close();
}

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL); loadSettings();
    spiSD.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
    SD.begin(SD_CS, spiSD);

    pinMode(BUZZER_PIN, OUTPUT);
    for(int i=0; i<8; i++) { pinMode(RELAY_PINS[i], OUTPUT); digitalWrite(RELAY_PINS[i], settings.relayStates[i]); }

    WiFi.softAP("DAQ-S3-API", "12345678");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(200, "text/html", index_html); });
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *r){
        StaticJsonDocument<1500> doc;
        JsonArray a = doc.createNestedArray("analog");
        JsonArray rs = doc.createNestedArray("relays");
        JsonArray ns = doc.createNestedArray("names");
        for(int i=0; i<10; i++) { a.add(getV(i)); ns.add(settings.inputNames[i]); }
        for(int i=0; i<8; i++) rs.add(settings.relayStates[i]);
        String b; serializeJson(doc, b); r->send(200, "application/json", b);
    });

    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *r){
        StaticJsonDocument<200> doc;
        doc["cpu_temp"] = temperatureRead();
        doc["wifi_rssi"] = WiFi.RSSI();
        doc["sd_status"] = SD.cardSize() > 0 ? "OK" : "ERR";
        String b; serializeJson(doc, b); r->send(200, "application/json", b);
    });

    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *r){ r->send(200); }, 
        [](AsyncWebServerRequest *r, String fn, size_t i, uint8_t *d, size_t len, bool f){
            static File uf; if(!i) uf = SD.open("/script.txt", FILE_WRITE);
            if(uf) uf.write(d, len); if(f) uf.close();
    });

    server.begin();
    
    // Route pour télécharger le fichier directement
    server.serveStatic("/datalog.csv", SD, "/datalog.csv");

    // Route pour effacer le fichier
    server.on("/clear-logs", HTTP_GET, [](AsyncWebServerRequest *request){
        if (SD.remove("/datalog.csv")) {
            request->send(200, "text/plain", "Logs supprimes");
        } else {
            request->send(500, "text/plain", "Erreur");
        }
    });
}

void loop() {
    unsigned long currentMillis = millis();

    // 1. Exécution de la logique API (toutes les secondes)
    static unsigned long lastLogic = 0;
    if(currentMillis - lastLogic > 1000) {
        lastLogic = currentMillis;
        runSDScript();
    }

    // 2. Enregistrement des logs (toutes les minutes)
    if(currentMillis - lastLogTime > logInterval) {
        lastLogTime = currentMillis;
        logData();
        
        // Petit clin d'oeil visuel avec la Status LED
        digitalWrite(STATUS_LED, HIGH);
        delay(50);
        digitalWrite(STATUS_LED, LOW);
    }
}

// Variable pour l'intervalle de log (en ms) - ex: 60000 pour 1 minute
unsigned long logInterval = 60000; 
unsigned long lastLogTime = 0;

void logData() {
    // On ouvre le fichier en mode "APPEND" (ajout à la fin)
    File logFile = SD.open("/datalog.csv", FILE_APPEND);
    
    if (logFile) {
        // Horodatage simplifié (Uptime en secondes si pas de RTC)
        logFile.print(millis() / 1000);
        logFile.print(";");

        // Enregistrement des 10 tensions
        for (int i = 0; i < 10; i++) {
            logFile.print(getV(i), 2);
            logFile.print(";");
        }

        // Enregistrement de l'état des relais (sous forme d'un octet 0-255)
        byte relayByte = 0;
        for (int i = 0; i < 8; i++) {
            if (settings.relayStates[i]) relayByte |= (1 << i);
        }
        logFile.println(relayByte);

        logFile.close();
        Serial.println("Données loguées sur SD.");
    } else {
        Serial.println("Erreur d'écriture sur SD !");
    }
}