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
#define DEBUG_TX 12
#define DEBUG_RX 13

EspSoftwareSerial::UART DEBUG;

// Serial Variables
volatile byte receivedData;
byte buffer[128];
int bufferIndex = 0;


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
    delay(1000);
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

void serialEvent() {
  while (Serial.available() > 0) {
    receivedData = Serial.read();
    buffer[bufferIndex] = receivedData;
    bufferIndex++;
  }
}


void loop() {

  // if ((WiFiMulti.run() == WL_CONNECTED)) {
  //   fw_main(crMajor, crMinor);
  // }

  if (bufferIndex > 0) {
    // Handle or process the data
    for (int i = 0; i < bufferIndex; i++)
    {
      DEBUG.write(buffer[i]);
    }
    // Reset the buffer
    memset(buffer, 0, sizeof(buffer));
    bufferIndex = 0;
  }
  

  // DEBUG.println("Wait 10s before the next round...");
  // delay(10000);
}
