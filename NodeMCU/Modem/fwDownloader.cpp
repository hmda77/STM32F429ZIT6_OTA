#include "HardwareSerial.h"

#include "fwDownloader.h"

char fw_buf[MAX_FW_INFO];

/* server configurations */
const char* fwServer    = FW_SERVER;
const char* fwUrl       = FW_INFO_URL;
const char* filename    = FW_FILE_NAME;




NET_EX_ fw_main(uint16_t current_major, uint32_t current_minor)
{
  NET_EX_ ret = NET_EX_ERROR;
  do{
    /* clear buffer */
    memset(fw_buf, 0, sizeof(fw_buf));

    if(get_fw_info(fwUrl, fw_buf) != NET_EX_OK)
    {
      Serial.println("get_fw_info failed");
      break;
    }
    
    // create JSON object
    StaticJsonDocument<96> doc;

    DeserializationError error = deserializeJson(doc, fw_buf, MAX_FW_INFO);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      break;
    }

    // store values after JSON Decoding
    uint32_t fw_version_minor = doc["fw_version"]["minor"];
    uint16_t fw_version_major = doc["fw_version"]["major"]; 

    const char* fw_crc_s = doc["fw_crc"];
    uint32_t fw_crc = (uint32_t)strtol(fw_crc_s, NULL, 16);
    uint32_t fw_size = doc["fw_size"];
    const char* fw_link = doc["fw_link"];

    // check if download needed or not
    if ( !(current_major <= fw_version_major) )
    {
      break;
    }

    if ( (current_major == fw_version_major))
    {
      if( !(current_minor < fw_version_minor) )
      {
        // There is Nothing for update
        break;
      }
    }


    // download and save new firmware
    Serial.println("NEW FIRMWARE!!!");
    Serial.printf("FW Version: [%d.%d]\r\nFW Size: [%d B]\r\nFW CRC = [0x%08x]\r\nFW Link = [%s]",
                    fw_version_major,
                    fw_version_minor,
                    fw_size,
                    fw_crc,
                    fw_link);
    if(download_and_save(fw_link, filename) != NET_EX_OK )
    {
      Serial.println("Download Failed");
      break;

    }

    Serial.println("New Firmware Downloaded!");

      NET_EX_ ret = NET_EX_ERROR;

    // open doenloaded file
    File file = SPIFFS.open(filename, "r");

    if(!file)
    {
      Serial.println("There was ann error opening file");
      break;
    }

    // check size
    if( file.size() != fw_size )
    {
      Serial.printf("Size Mismatch!!! rec_file_size = [%d], fw_real_size = [%d]\r\n", 
                                                                file.size(),
                                                                fw_size);
      file.close();
      break;
    }

    //check CRC
    uint32_t crc = 0xFFFFFFFF;

    while (file.available()) {
        uint8_t byte = file.read();
        crc = (crc >> 8) ^ crc32b_table[(crc ^ byte) & 0xFF];
    }

    crc ^= 0xFFFFFFFF;

    file.close();
    
    if(crc != fw_crc)
    {
      Serial.printf("CRC MISMATCH!!! Calc_crc = [0x%08lx], fw_crc = [0x%08lx]\r\n", crc, fw_crc);
      break;
    }

    Serial.println("crc_check_OK");

    ret = NET_EX_OK;
    // readAndWriteFileToSerial(filename);

    // crMajor = fw_version_major;
    // crMinor = fw_version_minor;

  }while(false);

  return ret;
}


/*
* @brief get the last firmware information from server
* @param url fw information url
* @param fw_buff firmware information buffer
* @retval NET_EX_
*/
NET_EX_ get_fw_info(const char* url, char* fw_buf)
{
  NET_EX_ ret = NET_EX_ERROR;

  // create a secure client object
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

  // set connection as insecure
  client->setInsecure();

  // creat https client object
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
        ret = NET_EX_OK;
      }
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();
  } else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  return ret;
}

/*
* @breif download and save firmware
* @param url firmware url
* @parm dest destination for saving received file
* @retval NET_EX_
*/
NET_EX_ download_and_save(const char* url, const char* dest)
{
  NET_EX_ ret = NET_EX_ERROR;

  // create a secure client object
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

  // set connection as insecure
  client->setInsecure();

  
  // create http client
  HTTPClient https;
  

  bool mfln = client->probeMaxFragmentLength("raw.githubusercontent.com", 443, 1024);
  Serial.printf("\nConnecting to https://raw.githubusercontent.com\n");
  Serial.printf("Maximum fragment Length negotiation supported: %s\n", mfln ? "yes" : "no");
  if (mfln) { client->setBufferSizes(1024, 1024); }

  Serial.print("[HTTPS] begin...\n");

  File file = SPIFFS.open(dest, "w");

  if (https.begin(*client, url)) {

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
          size_t size = client->available();

          if (size) {
            // read up to 128 byte
            int c = client->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

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
        ret = NET_EX_OK;
      }
    } else {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }
    file.close();
    https.end();
  } else {
    Serial.printf("Unable to connect\n");
  }
  return ret;
}


/**
 * @brief Calculate CRC32
 * @param pData data that should be calculated
 * @param DataLength length of data
 * @retval CRC32
 */

uint32_t calculate_crc32b(uint8_t * data, int length) {
    uint32_t crc = 0xFFFFFFFF;

    for (int i = 0; i < length; i++) {
        uint8_t byte = data[i];
        crc = (crc >> 8) ^ crc32b_table[(crc ^ byte) & 0xFF];
    }

    crc ^= 0xFFFFFFFF;

    return crc;
}
