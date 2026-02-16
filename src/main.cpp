#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "pins.h"
#include "web_index.h"
#include "tinyexpr.h"

// --- VARIABLES DE SIMULATION ---
struct Settings {
    bool relayStates[8];
    char inputNames[10][20];
} settings;

AsyncWebServer server(80);

// Le script est stocké ici au lieu de la carte SD pour le test nu
String virtualScript = "R1 = AN1 > 15.0\nR2 = AN2 < 11.5\nR3 = AN1 + AN2 > 25.0"; 

// --- SIMULATION DES TENSIONS (Génération de courbes sinusoïdales) ---
float getV(int ch) {
    // Crée une variation fluide entre 10V et 20V pour le dashboard
    return 15.0 + 5.0 * sin(millis() / (2000.0 + (ch * 500)));
}

// --- MOTEUR LOGIQUE EN RAM ---
void runRAMScript() {
    double v[10]; 
    for(int i = 0; i < 10; i++) v[i] = getV(i);

    // Dictionnaire de variables pour TinyExpr
    te_variable vars[] = { 
        {"AN1",&v[0]}, {"AN2",&v[1]}, {"AN3",&v[2]}, {"AN4",&v[3]}, {"AN5",&v[4]},
        {"AN6",&v[5]}, {"AN7",&v[6]}, {"AN8",&v[7]}, {"AN9",&v[8]}, {"AN10",&v[9]}
    };

    // Analyse du script ligne par ligne depuis la String RAM
    int start = 0;
    int end = virtualScript.indexOf('\n');
    
    while (start < virtualScript.length()) {
        String line = (end == -1) ? virtualScript.substring(start) : virtualScript.substring(start, end);
        line.trim();

        if(line.indexOf('=') != -1 && !line.startsWith("#")){
            String target = line.substring(0, line.indexOf('='));
            String exprS = line.substring(line.indexOf('=') + 1);
            
            target.trim();
            exprS.trim();

            int err;
            te_expr *e = te_compile(exprS.c_str(), vars, 10, &err);
            
            if(e){
                double res = te_eval(e);
                // Extraction de l'index (ex: "R1" -> index 0)
                int rIdx = target.substring(1).toInt() - 1;
                if(rIdx >= 0 && rIdx < 8) {
                    settings.relayStates[rIdx] = (res > 0.5);
                }
                te_free(e);
            }
        }
        if (end == -1) break;
        start = end + 1;
        end = virtualScript.indexOf('\n', start);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Initialisation des noms fictifs pour le test
    for(int i=0; i<10; i++) sprintf(settings.inputNames[i], "Simu-Ch %d", i+1);
    for(int i=0; i<8; i++) settings.relayStates[i] = false;

    // WiFi en mode Point d'Accès
    WiFi.softAP("DAQ-S3-SIMU", "12345678");
    Serial.println("\n==================================");
    Serial.println("   DAQ S3 - MODE SIMULATION OK   ");
    Serial.print("   IP: "); Serial.println(WiFi.softAPIP());
    Serial.println("==================================\n");

    // Route : Page d'accueil
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *r){
        r->send(200, "text/html", index_html);
    });

    // Route : API de données JSON
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *r){
        StaticJsonDocument<1200> doc;
        JsonArray a = doc.createNestedArray("analog");
        JsonArray rs = doc.createNestedArray("relays");
        JsonArray ns = doc.createNestedArray("names");
        for(int i=0; i<10; i++) {
            a.add(getV(i));
            ns.add(settings.inputNames[i]);
        }
        for(int i=0; i<8; i++) rs.add(settings.relayStates[i]);
        String response;
        serializeJson(doc, response);
        r->send(200, "application/json", response);
    });

    // Route : Diagnostic simulé
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *r){
        StaticJsonDocument<200> doc;
        doc["cpu_temp"] = 42.5; // Valeur fixe pour simu
        doc["wifi_rssi"] = WiFi.RSSI();
        doc["sd_status"] = "SIMULATED";
        String b; serializeJson(doc, b);
        r->send(200, "application/json", b);
    });

    // Route : Lire le script actuel
    server.on("/script.txt", HTTP_GET, [](AsyncWebServerRequest *r){
        r->send(200, "text/plain", virtualScript);
    });

    // Route : Upload vers la RAM
    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *r){ 
        r->send(200, "text/plain", "OK"); 
    }, [](AsyncWebServerRequest *r, String fn, size_t index, uint8_t *data, size_t len, bool final){
        if(!index) virtualScript = ""; 
        for(size_t j=0; j<len; j++) virtualScript += (char)data[j];
    });

    server.begin();
}

void loop() {
    static unsigned long lastExec = 0;
    if(millis() - lastExec > 500) {
        lastExec = millis();
        runRAMScript();
    }
}