#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "pins.h"
#include "web_index.h"

// --- CONFIGURATION WI-FI ---
const char* ssid_sta = "VOTRE_BOX_WIFI";      // Nom de votre box
const char* pass_sta = "VOTRE_MOT_DE_PASSE";  // Mot de passe box
const char* ssid_ap  = "DAQ-ESP32-S3";        // Nom du réseau créé par la carte
const char* pass_ap  = "12345678";            // Mot de passe du réseau carte

AsyncWebServer server(80);

// --- PARAMÈTRES ANALOGIQUES ---
const float R_HIGH = 68000.0; 
const float R_LOW = 33000.0;
const float VOLTAGE_DIVIDER_RATIO = (R_HIGH + R_LOW) / R_LOW;
const float ADC_VREF = 3.3; 

// Fonction de lecture lissée
float readVoltage(int ch) {
    long sum = 0;
    const int samples = 64;
    for(int i = 0; i < samples; i++) {
        sum += analogRead(ANALOG_PINS[ch]);
        delayMicroseconds(10);
    }
    float avgRaw = (float)sum / samples;
    float vPin = (avgRaw * ADC_VREF) / 4095.0;
    return vPin * VOLTAGE_DIVIDER_RATIO;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- Système DAQ ESP32-S3 Initialisation ---");

    // 1. Configuration des Relais (Sorties)
    for(int i = 0; i < NUM_RELAYS; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        digitalWrite(RELAY_PINS[i], LOW); // Relais éteints au démarrage
    }

    // 2. Configuration Analogique
    analogReadResolution(12); // Précision maximale (0-4095)

    // 3. Gestion Wi-Fi (Mode Hybride)
    WiFi.mode(WIFI_AP_STA);
    WiFi.begin(ssid_sta, pass_sta);
    
    Serial.print("Tentative de connexion au Wi-Fi");
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 15) {
        delay(500);
        Serial.print(".");
        retry++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnecté à la box ! IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\nEchec connexion box. Création du Point d'Accès...");
        WiFi.softAP(ssid_ap, pass_ap);
        Serial.println("Réseau créé: DAQ-ESP32-S3 | IP: 192.168.4.1");
    }

    // --- ROUTES DU SERVEUR WEB ---

    // Page d'accueil (Interface Web)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });

    // API : Envoi des tensions en JSON
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
        StaticJsonDocument<512> doc;
        JsonArray data = doc.createNestedArray("analog");
        for(int i = 0; i < NUM_ANALOG_INPUTS; i++) {
            data.add(readVoltage(i));
        }
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // API : Contrôle des Relais
    server.on("/relay", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("id") && request->hasParam("state")) {
            int id = request->getParam("id")->value().toInt();
            int state = request->getParam("state")->value().toInt();
            if(id >= 0 && id < NUM_RELAYS) {
                digitalWrite(RELAY_PINS[id], state ? HIGH : LOW);
                Serial.printf("Relais %d mis à %s\n", id + 1, state ? "ON" : "OFF");
            }
        }
        request->send(200, "text/plain", "OK");
    });

    server.begin();
    Serial.println("Serveur Web démarré.");
}

void loop() {
    // Le serveur est asynchrone, loop() reste libre pour d'autres tâches.
    // On peut ajouter ici une surveillance de sécurité si besoin.
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 5000) {
        // Petit "Heartbeat" dans le moniteur série toutes les 5 secondes
        lastCheck = millis();
    }
}