#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <SoftwareSerial.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <FS.h>

const char* ssid = "UwU";
const char* password = "123hmd321";

const char* fwUrl = "https://fw.ziplast.ir/?getinfo";
const char* filename = "/downloaded_file.bin";

uint16_t crMajor = 1;
uint32_t crMinor = 0;

ESP8266WiFiMulti WiFiMulti;


// Add soft serial Port for OTA
#define OTAPORT_TX 12
#define OTAPORT_RX 13
#define MAX_FW_INFO 500

EspSoftwareSerial::UART otaPort;

char fw_buf[MAX_FW_INFO];


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

bool download_and_save(const char* url, const char* dest)
{
  bool ret = false;
    // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    BearSSL::WiFiClientSecure client;
    HTTPClient https;

    client.setInsecure();

    bool mfln = client.probeMaxFragmentLength("raw.githubusercontent.com", 443, 1024);
    Serial.printf("\nConnecting to https://raw.githubusercontent.com\n");
    Serial.printf("Maximum fragment Length negotiation supported: %s\n", mfln ? "yes" : "no");
    if (mfln) { client.setBufferSizes(1024, 1024); }

    Serial.print("[HTTPS] begin...\n");

    File file = SPIFFS.open(dest, "w");

    if (https.begin(client, url) && file) {

      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK) {

          // get length of document (is -1 when Server sends no Content-Length header)
          int len = https.getSize();
          Serial.printf("file size: %d\r\n", len);


          // create buffer for read
          static uint8_t buff[128] = { 0 };

          // read all data from server
          while (https.connected() && (len > 0 || len == -1)) {
            // get available data size
            size_t size = client.available();

            if (size) {
              // read up to 128 byte
              int c = client.readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

              // write it to Serial
              // Serial.write(buff, c);

              file.write(buff, c);

              if (len > 0)
              { 
                len -= c;
              }
            }
            delay(1);
          }
          
          Serial.println();
          Serial.print("[HTTPS] connection closed or file end.\n");
          ret = true;
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      file.close();
      https.end();
    } else {
      Serial.printf("Unable to connect\n");
    }
  }
  return ret;
}

bool get_fw_info(const char* url, char* fw_buf)
{
  bool ret = false;

  if ((WiFiMulti.run() == WL_CONNECTED))
  {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setInsecure();

    HTTPClient https;

    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, url)) {

      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          strcpy(fw_buf, payload.c_str()); 
          // Serial.println(payload);
          ret = true;
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }
  return ret;
}


void loop() {

  memset(fw_buf, 0, sizeof(fw_buf));
  if(get_fw_info(fwUrl, fw_buf))
  {
    // Serial.printf("%s\r\n",fw_buf);

    StaticJsonDocument<96> doc;

    DeserializationError error = deserializeJson(doc, fw_buf, MAX_FW_INFO);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    int fw_version_minor = doc["fw_version"]["minor"]; // 0
    int fw_version_major = doc["fw_version"]["major"]; // 5

    const char* fw_crc = doc["fw_crc"]; //
    long fw_size = doc["fw_size"];
    const char*
 fw_link = doc["fw_link"];


    do
    {
      if ( !(crMajor <= fw_version_major) )
      {
        break;
      }
      
      if ( (crMajor == fw_version_major))
      {
        if( !(crMinor < fw_version_minor) )
        {
          // Nothing for update
          break;
        }
      }

      Serial.println("NEW FIRMWARE!!!");
      if(download_and_save(fw_link, filename))
      {
        readAndWriteFileToSerial(filename);

        crMajor = fw_version_major;
        crMinor = fw_version_minor;
      }
      else 
      {
        Serial.println("Download Failed");
      }

    }while(false);

  }

  Serial.println("Wait 10s before the next round...");
  delay(10000);
}
