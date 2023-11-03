#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <SoftwareSerial.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "UwU";
const char* password = "123hmd321";
const char* fileUrl = "https://raw.githubusercontent.com/hmda77/STM32F429ZIT6_OTA/ModemCode/Application/Debug/Application.bin";


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

}


void loop() {

  Serial.println("Wait 10s before the next round...");
  delay(10000);
}
