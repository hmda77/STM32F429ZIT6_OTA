#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <SoftwareSerial.h>
// #include "fwDownloader.h"
#include "fwSerial.h"

/* WiFi Configuration */
const char* ssid = "UwU";
const char* password = "123hmd321";

ESP8266WiFiMulti WiFiMulti;


// Add soft serial Port for OTA
#define DEBUG_TX 12
#define DEBUG_RX 13

EspSoftwareSerial::UART DEBUG;


void setup() {

  Serial.begin(9600);
  // Serial.setDebugOutput(true);

  DEBUG.begin(115200, SWSERIAL_8N1, DEBUG_RX, DEBUG_TX, false);
  if (!DEBUG) { // If the object did not initialize, then its configuration is invalid
    while (1) { // Don't continue with invalid configuration
      delay (1000);
    }
  }
  

  DEBUG.println();
  DEBUG.println();
  DEBUG.println();

  for (uint8_t t = 4; t > 0; t--) {
    DEBUG.printf("[SETUP] WAIT %d...\n", t);
    DEBUG.flush();
    delay(100);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid , password);

  // Initialize file system
  SPIFFS.begin();

}


void readAndWriteFileToSerial(const char* filename) {
  File file = SPIFFS.open(filename, "r");
  DEBUG.println("write down file");
  if (file) {
    while (file.available()) {
      Serial.write(file.read());
    }
    file.close();
  }
}

void loop() {

  serial_app();
  

  // DEBUG.println("Wait 10s before the next round...");
  // delay(10000);
}
