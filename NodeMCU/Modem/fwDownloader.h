
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <FS.h>

#define MAX_FW_INFO 500     // Maximum FW information Length (in Byte)

#define FW_SERVER     "fw.ziplast.ir"
#define FW_INFO_URL   "https://fw.ziplast.ir/?getinfo"
#define FW_FILE_NAME  "/fw.bin"


/* 
* executed return
*/
typedef enum
{
  NET_EX_OK     = 0,
  NET_EX_ERROR  = 1,
}NET_EX_; 

typedef struct
{
  uint16_t  fw_version_major;
  uint32_t  fw_version_minor;
  uint32_t  fw_crc;
  uint32_t  fw_size;
  char*     fw_link;
}__attribute__((packed))FW_INFO_;

/* function defenitions */
NET_EX_ get_fw_info(const char* url, char* fw_buf);
NET_EX_ download_and_save(const char* url, const char* dest);
NET_EX_ fw_main(uint16_t current_major, uint32_t current_minor);