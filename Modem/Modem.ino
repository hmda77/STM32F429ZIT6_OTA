#include <SoftwareSerial.h>


unsigned long previousMillis = 0;  // will store last time LED was updated

const long interval = 1000;  // interval at which to blink (milliseconds)

// Add soft serial Port for OTA
#define OTAPORT_TX 12
#define OTAPORT_RX 13

EspSoftwareSerial::UART otaPort;

void setup() {

  Serial.begin(115200);

  otaPort.begin(115200, SWSERIAL_8N1, OTAPORT_RX, OTAPORT_TX, false);
  if (!otaPort) { // If the object did not initialize, then its configuration is invalid
  Serial.println("Invalid EspSoftwareSerial pin configuration, check config"); 
    while (1) { // Don't continue with invalid configuration
      delay (1000);
    }
  } 
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;


    otaPort.println("HI");
  }
}
