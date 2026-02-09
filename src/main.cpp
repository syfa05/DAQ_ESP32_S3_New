#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "pins.h" // Assurez-vous que ce fichier existe dans le dossier 'include'

// --- CONFIGURATION ---
const char* ssid = "VOTRE_SSID";
const char* password = "VOTRE_MOT_DE_PASSE";

AsyncWebServer server(80);

// --- LOGIQUE ANALOGIQUE ---
const float R_HIGH = 68000.0; 
const float R_LOW = 33000.0;
const float VOLTAGE_DIVIDER_RATIO = (R_HIGH + R_LOW) / R_LOW;
const float ADC_VREF = 3.3; 

float getVoltage(int ch) {
    long sum = 0;
    for(int i=0; i<64; i++) sum += analogRead(ANALOG_PINS[ch]);
    float avgRaw = (float)sum / 64.0;
    return (avgRaw * ADC_VREF / 4095.0) * VOLTAGE_DIVIDER_RATIO;
}

// --- INITIALISATION (C'est ce qui manquait !) ---
void setup() {
    Serial.begin(115200);
    
    // Configurer les Relais en sortie
    for(int i=0; i<NUM_RELAYS; i++) {
        pinMode(RELAY_PINS[i], OUTPUT);
        digitalWrite(RELAY_PINS[i], LOW);
    }

    // Configurer l'ADC
    analogReadResolution(12);

    // Connexion Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnecté ! IP : " + WiFi.localIP().toString());

    // Route API pour les données JSON
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
        StaticJsonDocument<512> doc;
        JsonArray data = doc.createNestedArray("analog");
        for(int i=0; i < NUM_ANALOG_INPUTS; i++) {
            data.add(getVoltage(i));
        }
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server.begin();
}

// --- BOUCLE PRINCIPALE ---
void loop() {
    // Le serveur asynchrone gère tout en arrière-plan
    delay(1000); 
}