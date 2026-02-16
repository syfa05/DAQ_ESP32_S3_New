#include <SD.h>
#include <SPI.h>

SPIClass spiSD(FSPI); // Utilisation du bus SPI spécifique

void initSD() {
    spiSD.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, spiSD)) {
        Serial.println("Carte SD : Erreur ou absente.");
    } else {
        Serial.println("Carte SD : Initialisee avec succes.");
        // Exemple de lecture du fichier de config
        File file = SD.open("/config.txt");
        if(file) {
            Serial.println("Configuration chargee depuis SD.");
            file.close();
        }
    }
}