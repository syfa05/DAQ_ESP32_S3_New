#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "pins.h"
#include "web_index.h"
#include "daq.h"
#include "hal.h"
#include "tinyexpr.h"

// --- CONFIGURATION ---
DAQ daq;

AsyncWebServer server(80);

// --- SIMULATION DES DONNÉES ---
void updateSensors() {
    for(int i=0; i<10; i++) {
        daq.analog[i] = 12.0 + 10.0 * sin(millis() / (2000.0 + i*500));
    }
    for(int i=0; i<5; i++) {
        daq.inputs[i] = (sin(millis() / 5000.0 + i) > 0);
    }
}

// --- MOTEUR DE PROGRAMMATION C++ (TinyExpr) ---
void runLogic() {
    if (daq.script.length() == 0) return;
    
    double vars_val[10]; 
    for(int i=0; i<10; i++) vars_val[i] = daq.analog[i];

    te_variable vars[] = { 
        {"AN1", &vars_val[0]}, {"AN2", &vars_val[1]}, {"AN3", &vars_val[2]},
        {"AN4", &vars_val[3]}, {"AN5", &vars_val[4]}, {"AN6", &vars_val[5]},
        {"AN7", &vars_val[6]}, {"AN8", &vars_val[7]}, {"AN9", &vars_val[8]},
        {"AN10", &vars_val[9]}
    };

    // Parsing du script ligne par ligne
    int start = 0;
    int end = daq.script.indexOf('\n');
    while (start < daq.script.length()) {
        String line = (end == -1) ? daq.script.substring(start) : daq.script.substring(start, end);
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
                if(rIdx >= 0 && rIdx < 8) daq.relays[rIdx] = (res > 0.5);
                te_free(e);
            }
        }
        if (end == -1) break;
        start = end + 1;
        end = daq.script.indexOf('\n', start);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Initialize HAL
    initHAL();
    
    // Initialize DAQ settings
    daq.script = "R1 = AN1 > 15.0\nR2 = AN2 < 11.0";
    for(int i=0; i<10; i++) sprintf(daq.inputNames[i], "Analog-%d", i+1);
    for(int i=0; i<8; i++) daq.relays[i] = false;
    
    WiFi.softAP("DAQ-PRO-ESP32", "12345678");
    Serial.println("\n==================================");
    Serial.println("   DAQ Professional - Running");
    Serial.print("   IP: "); Serial.println(WiFi.softAPIP());
    Serial.println("==================================\n");

    // ROUTES SERVEUR
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){ r->send(200, "text/html", index_html); });
    
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *r){
        StaticJsonDocument<1200> doc;
        JsonArray a = doc.createNestedArray("analog");
        JsonArray rs = doc.createNestedArray("relays");
        JsonArray in = doc.createNestedArray("inputs");
        for(int i=0; i<10; i++) a.add(daq.analog[i]);
        for(int i=0; i<8; i++) rs.add(daq.relays[i]);
        for(int i=0; i<5; i++) in.add(daq.inputs[i]);
        doc["cpu"] = temperatureRead();
        String b; serializeJson(doc, b); r->send(200, "application/json", b);
    });

    server.on("/script.txt", HTTP_GET, [](AsyncWebServerRequest *r){
        r->send(200, "text/plain", daq.script);
    });

    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *r){ r->send(200); }, 
        [](AsyncWebServerRequest *r, String fn, size_t i, uint8_t *d, size_t len, bool f){
            if(!i) daq.script = ""; 
            for(size_t j=0; j<len; j++) daq.script += (char)d[j];
    });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Rebooting...");
        delay(500);
        ESP.restart();
    });

    server.begin();
    Serial.println("Web Server started.");
}

void loop() {
    static unsigned long lastTick = 0;
    if(millis() - lastTick > 500) {
        lastTick = millis();
        updateSensors();
        updateHAL();
        runLogic();
    }
}