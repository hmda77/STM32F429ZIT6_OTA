#include "fwSerial.h"

// Serial Variables
volatile byte receivedData;
byte buffer[128];
int bufferIndex = 0;


void serialEvent() {
  while (Serial.available() > 0) {
    receivedData = Serial.read();
    buffer[bufferIndex] = receivedData;
    bufferIndex++;
  }
}
