#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <SoftwareSerial.h>
#include "fwDownloader.h"

/* WiFi Configuration */
const char* ssid = "UwU";
const char* password = "123hmd321";

/* Current version of firmware */
uint16_t crMajor = 1;
uint32_t crMinor = 0;

ESP8266WiFiMulti WiFiMulti;


// Add soft serial Port for OTA
#define OTAPORT_TX 12
#define OTAPORT_RX 13

EspSoftwareSerial::UART otaPort;


void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  otaPort.begin(115200, SWSERIAL_8N1, OTAPORT_RX, OTAPORT_TX, false);
  if (!otaPort) { // If the object did not initialize, then its configuration is invalid
  Serial.println("Invalid EspSoftwareSerial pin configuration, check config"); 
    while (1) { // Don't continue with invalid configuration
      delay (1000);
    }
  } 

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid , password);

  // Initialize file system
  SPIFFS.begin();

}


void readAndWriteFileToSerial(const char* filename) {
  File file = SPIFFS.open(filename, "r");
  Serial.println("write down file");
  if (file) {
    while (file.available()) {
      otaPort.write(file.read());
    }
    file.close();
  }
}




void loop() {

  if ((WiFiMulti.run() == WL_CONNECTED)) {
    fw_main(crMajor, crMinor);
  }

  Serial.println("Wait 10s before the next round...");
  delay(10000);
}
