#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "pins.h"
#include "web_index.h"
#include "tinyexpr.h"

// --- CONFIGURATION ---
struct SystemState {
    bool relayStates[8];
    float voltages[10];
    String currentScript;
} state;

AsyncWebServer server(80);

// --- SIMULATION DES DONNÉES ---
void updateSimulatedData() {
    for(int i=0; i<10; i++) {
        state.voltages[i] = 12.0 + 8.0 * sin(millis() / (3000.0 + (i*400)));
    }
}

// --- MOTEUR DE PROGRAMMATION C++ (TinyExpr) ---
void runRuntimeEngine() {
    double v[10]; 
    for(int i=0; i<10; i++) v[i] = state.voltages[i];

    te_variable vars[] = { 
        {"AN1",&v[0]}, {"AN2",&v[1]}, {"AN3",&v[2]}, {"AN4",&v[3]}, {"AN5",&v[4]},
        {"AN6",&v[5]}, {"AN7",&v[6]}, {"AN8",&v[7]}, {"AN9",&v[8]}, {"AN10",&v[9]}
    };

    // Parsing du script ligne par ligne
    int start = 0;
    int end = state.currentScript.indexOf('\n');
    while (start < state.currentScript.length()) {
        String line = (end == -1) ? state.currentScript.substring(start) : state.currentScript.substring(start, end);
        line.trim();

        if(line.indexOf('=') != -1 && !line.startsWith("#")){
            String target = line.substring(0, line.indexOf('='));
            String expr = line.substring(line.indexOf('=')+1);
            target.trim(); expr.trim();

            int err;
            te_expr *e = te_compile(expr.c_str(), vars, 10, &err);
            if(e){
                double res = te_eval(e);
                int rIdx = target.substring(1).toInt() - 1;
                if(rIdx >= 0 && rIdx < 8) state.relayStates[rIdx] = (res > 0.5);
                te_free(e);
            }
        }
        if (end == -1) break;
        start = end + 1;
        end = state.currentScript.indexOf('\n', start);
    }
}

void setup() {
    Serial.begin(115200);
    state.currentScript = "R1 = AN1 > 15.0\nR2 = AN2 < 11.0"; // Script par défaut
    
    WiFi.softAP("DAQ-PRO-S3", "12345678");

    // ROUTES SERVEUR
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(200, "text/html", index_html); });
    
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *r){
        StaticJsonDocument<1200> doc;
        JsonArray a = doc.createNestedArray("analog");
        JsonArray rs = doc.createNestedArray("relays");
        for(int i=0; i<10; i++) a.add(state.voltages[i]);
        for(int i=0; i<8; i++) rs.add(state.relayStates[i]);
        doc["cpu"] = temperatureRead();
        String b; serializeJson(doc, b); r->send(200, "application/json", b);
    });

    server.on("/script.txt", HTTP_GET, [](AsyncWebServerRequest *r){
        r->send(200, "text/plain", state.currentScript);
    });

    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *r){ r->send(200); }, 
        [](AsyncWebServerRequest *r, String fn, size_t i, uint8_t *d, size_t len, bool f){
            if(!i) state.currentScript = ""; 
            for(size_t j=0; j<len; j++) state.currentScript += (char)d[j];
    });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Rebooting...");
        delay(500);
        ESP.restart();
    });

    server.begin();
    Serial.println("Serveur Web Professionnel lancé.");
}

void loop() {
    static unsigned long lastTick = 0;
    if(millis() - lastTick > 500) {
        lastTick = millis();
        updateSimulatedData();
        runRuntimeEngine();
    }
}