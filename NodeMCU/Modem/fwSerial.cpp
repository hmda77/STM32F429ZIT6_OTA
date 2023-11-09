#include "fwSerial.h"


/* -------------- Defined  variables ------------- */

// serial receive state
static SER_STATE_ ser_state = SER_STATE_START;

// Serial Variables
volatile byte receivedData;
byte buffer[64];
int bufferIndex = 0;

/* Buffer to hold the received data */
static uint8_t Rx_Buffer[100];
uint8_t Tx_Buffer[MAX_SERIAL_SIZE];

/* chunk handler */
static CHUNK_HANDL_ hchunk =
{
	.chunk_state  = CUN_STATE_SOF,
	.chunk_ready  = CUN_EMPTY,
	.index		  	= 0u,
	.data_len	 		= 0u,
	.crc_check    = 0u,
};

static ser_fw_info ser_fw_data;
static uint32_t fw_crc;
static uint32_t fw_size;
static uint8_t fw_link[200];

static meta_info ser_info;

const char * filename = FW_FILE_NAME;

/*------------------- Defined Functions -------------- */
static void ser_receive_chunk(uint8_t rx_byte);
static void ser_send_resp(uint8_t rsp);
static SER_EX_ ser_process_rsp( uint8_t *buf, uint16_t len);
void send_ser_start();
void send_ser_header(meta_info *meta_data);
void send_ser_info(ser_fw_info * ota_info);
void send_ser_end();
SER_EX_ download_save_fw(const char* dest);
SER_EX_ get_info(ser_fw_info * ota_info);
uint32_t ser_calcCRC(uint8_t * pData, uint32_t DataLength);


void serial_app(){
	do
  {
    if(bufferIndex > 0) {
      for(int i = 0; i < bufferIndex; i++)
      {
        // DEBUG.write(buffer[i]);
        ser_receive_chunk(buffer[i]);
      }

      memset(buffer, 0, sizeof(buffer));
      bufferIndex = 0;

    }

		// no Byte received or chunk reception in progress
		if( (hchunk.chunk_ready == CUN_EMPTY) ||
			(hchunk.chunk_ready == CUN_BUSY)){
			break;
		}

		SER_EX_ ret = SER_EX_OK;
		// An Error occur in during receive chunk
		if(hchunk.chunk_ready == CUN_ERROR)
		{
			DEBUG.println("Receive Chunk Error");
			ret = SER_EX_ERROR;
		}
		else
		{
			DEBUG.println("Chunk Received!!!");
      ret = ser_process_rsp(Rx_Buffer, hchunk.data_len);
    }

    // Send ACK or NACK
		if( ret != SER_EX_OK ){
			DEBUG.printf("NACK\r\n");
      ser_state = SER_STATE_START;
			// ser_send_resp(SER_NACK);
		}
    else
    {
      DEBUG.printf("ACK\r\n");
      // ser_send_resp(SER_ACK);
    }

    hchunk.chunk_ready = CUN_EMPTY;
	}while(false);
}




/// @brief Serial Event Handler
/// @return None
void serialEvent() {
  while (Serial.available() > 0) {
    receivedData = Serial.read();
    if( (hchunk.chunk_ready == CUN_EMPTY) ||
			(hchunk.chunk_ready == CUN_BUSY)){
      buffer[bufferIndex] = receivedData;
      bufferIndex++;
    }
  }
}


static SER_EX_ ser_process_rsp( uint8_t *buf, uint16_t len) {
  SER_EX_ ret = SER_EX_ERROR;
  do
  {
    if( (buf==NULL) || len == 0u)
    {
      break;
    }

    // Check Serial Abort Command
    SER_COMMAND_ *cmd = (SER_COMMAND_ *)buf;
		if(cmd->packet_type == SER_PACKET_TYPE_CMD)
		{
      if( cmd->cmd ==  SER_CMD_ABORT)
      {
        // Receive Serial Abort Command
        ser_state = SER_STATE_START;
        break;
      }
		}

    switch(ser_state)
    {
      case SER_STATE_START:
      {
        SER_COMMAND_ *cmd = (SER_COMMAND_ *)buf;
        if(cmd->packet_type == SER_PACKET_TYPE_CMD)
		    {
          switch(cmd->cmd)
          {
            //request for firmware information
            case SER_CMD_FW_STATUS:
            {
              memset(&ser_fw_data, 0, sizeof(ser_fw_info));
              memset(&ser_info, 0, sizeof(meta_info));

              if(get_info(&ser_fw_data) != SER_EX_OK)
              {
                break;
              }

              // set header meta info
              ser_info.data_type  = OTA_INFO_DATA;
              ser_info.data_size  = sizeof(ser_fw_info);
              ser_info.data_crc   = ser_calcCRC((uint8_t *)&ser_fw_data, sizeof(ser_fw_info));
              send_ser_start();
              ser_state   = SER_STATE_HEADER;
              ret = SER_EX_OK;
            }
            break;

            case SER_CMD_FW_DL:
            {
              // Download Firmware
              if(download_save_fw(filename) != SER_EX_OK)
              {
                break;
              }

              // update fw info
              ser_fw_data.ota_download = 1u;

              // set header meta info
              ser_info.data_type  = OTA_INFO_DATA;
              ser_info.data_size  = sizeof(ser_fw_info);
              ser_info.data_crc   = ser_calcCRC((uint8_t *)&ser_fw_data, sizeof(ser_fw_info));
              send_ser_start();
              ser_state = SER_STATE_HEADER;
              ret = SER_EX_OK;
            }
            break;

            case SER_CMD_FW_GET:
            {
              DEBUG.println("SER_CMD_FW_GET!");
              send_ser_start();
              ser_state   = SER_STATE_HEADER;
              ret = SER_EX_OK;
            }
            break;


            default:
            {
              DEBUG.println("invalid command!");
            }
            break;
          }
        }
      }
      break;

      case SER_STATE_HEADER:
      {
        SER_RESP_ *rsp = (SER_RESP_ *)buf;
        if(rsp->packet_type == SER_PACKET_TYPE_RESPONSE)
        {
          if(rsp->status == SER_ACK)
          {
            send_ser_header(&ser_info);
            ser_state   = SER_STATE_DATA;
            ret = SER_EX_OK;
          }
        }
      }
      break;

      case SER_STATE_DATA:
      {
        SER_RESP_ *rsp = (SER_RESP_ *)buf;
        if(rsp->packet_type == SER_PACKET_TYPE_RESPONSE)
        {
          if(rsp->status == SER_ACK)
          {
            switch(ser_info.data_type){
              case OTA_INFO_DATA:
              {
                send_ser_info(&ser_fw_data);
                ser_state   = SER_STATE_END;
                ret = SER_EX_OK;
              }break;

              default:
              {
                ser_state = SER_STATE_START;
              }
              break;
            }
          }
        }
      }
      break;

      case SER_STATE_END:
      {
        SER_RESP_ *rsp = (SER_RESP_ *)buf;
        if(rsp->packet_type == SER_PACKET_TYPE_RESPONSE)
        {
          if(rsp->status == SER_ACK)
          {
            send_ser_end();
            ser_state = SER_STATE_START;
            ret = SER_EX_OK;
          }
        }
      }
      break;

      default:
      {
        // shouldn't be here
        ser_state = SER_STATE_START;
      }
      break;
    }


  }while(false);

  return ret;
}

void send_ser_start()
{
  uint16_t len;
  SER_COMMAND_ *ser_start = (SER_COMMAND_ *)Tx_Buffer;

  memset(Tx_Buffer, 0, sizeof(Tx_Buffer));

  ser_start->sof          = SER_SOF;
  ser_start->packet_type  = SER_PACKET_TYPE_CMD;
  ser_start->data_len     = 1u;
  ser_start->cmd          = SER_CMD_START;
  ser_start->crc          = ser_calcCRC(&ser_start->cmd, 1);
  ser_start->eof          = SER_EOF;

  len = sizeof(SER_COMMAND_);

  for(int i=0; i < len; i++)
  {
    delay(1);
    Serial.write(Tx_Buffer[i]);
  }

}

void send_ser_header(meta_info *meta_data)
{
  uint16_t len;
  SER_HEADER_  * ser_header = (SER_HEADER_ *)Tx_Buffer;
  
  memset(Tx_Buffer, 0, sizeof(Tx_Buffer));

  ser_header->sof         = SER_SOF;
  ser_header->packet_type = SER_PACKET_TYPE_HEADER;
  ser_header->data_len    = sizeof(meta_info);
  ser_header->crc         = ser_calcCRC( (uint8_t *)meta_data, sizeof(meta_info));
  ser_header->eof         = SER_EOF;

  memcpy(&ser_header->meta_data, meta_data, sizeof(meta_info));

  len = sizeof(SER_HEADER_);

  for(int i=0; i < len; i++)
  {
    delay(1);
    Serial.write(Tx_Buffer[i]);
  }
}

void send_ser_info(ser_fw_info * ota_info)
{
  uint16_t len;
  SER_OTA_ *ser_req = (SER_OTA_ *)Tx_Buffer;

  memset(Tx_Buffer, 0, sizeof(Tx_Buffer));

  ser_req->sof             =  SER_SOF;
  ser_req->packet_type     =  SER_PACKET_TYPE_DATA;
  ser_req->data_len        =  sizeof(ser_fw_info);
  ser_req->crc             =  ser_calcCRC( (uint8_t *)ota_info, sizeof(ser_fw_info));
  ser_req->eof             =  SER_EOF;

  memcpy( &ser_req->ota_data, ota_info, sizeof(ser_fw_info) );

  len = sizeof(SER_OTA_);
  
  for(int i=0; i < len; i++)
  {
    delay(1);
    Serial.write(Tx_Buffer[i]);
  }
}

void send_ser_end()
{
  uint16_t len;
  SER_COMMAND_ *ser_end = (SER_COMMAND_ *)Tx_Buffer;

  memset(Tx_Buffer, 0, sizeof(Tx_Buffer));

  ser_end->sof          = SER_SOF;
  ser_end->packet_type  = SER_PACKET_TYPE_CMD;
  ser_end->data_len     = 1u;
  ser_end->cmd          = SER_CMD_END;
  ser_end->crc          = ser_calcCRC(&ser_end->cmd, 1);
  ser_end->eof          = SER_EOF;

  len = sizeof(SER_COMMAND_);

  for(int i=0; i < len; i++)
  {
    delay(1);
    Serial.write(Tx_Buffer[i]);
  }

}

SER_EX_ download_save_fw(const char* dest)
{
  SER_EX_ ret = SER_EX_ERROR;

  do
  {
    const char * url = (char *)fw_link;
    if(download_fw(url, dest) != NET_EX_OK )
    {
      DEBUG.println("Download Failed");
      break;
    }

    
    DEBUG.println("New Firmware Downloaded!");
    

    // open doenloaded file
    File file = SPIFFS.open(filename, "r");

    if(!file)
    {
      DEBUG.println("There was ann error opening file");
      break;
    }

    // check size
    if( file.size() != fw_size )
    {
      DEBUG.printf("Size Mismatch!!! rec_file_size = [%d], fw_real_size = [%d]\r\n", 
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
      DEBUG.printf("CRC MISMATCH!!! Calc_crc = [0x%08lx], fw_crc = [0x%08lx]\r\n", crc, fw_crc);
      break;
    }

    DEBUG.println("crc_check_OK");

    ret = SER_EX_OK;
    
  }while(false);
  
  return ret;
}

SER_EX_ get_info(ser_fw_info * ota_info)
{
  SER_EX_ ret = SER_EX_ERROR;
  /* ------MODEM VARIABLE----- */
  char fw_buf[MAX_FW_INFO];

  /* server configurations */
  const char* fwServer    = FW_SERVER;
  const char* fwUrl       = FW_INFO_URL;
  const char* filename    = FW_FILE_NAME;
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    do
    {
      /* clear buffer */
      memset(fw_buf, 0, sizeof(fw_buf));

      if(get_fw_info(fwUrl, fw_buf) != NET_EX_OK)
      {
        DEBUG.println("get_fw_info failed");
        break;
      }

          // create JSON object
    StaticJsonDocument<96> doc;

    DeserializationError error = deserializeJson(doc, fw_buf, MAX_FW_INFO);

    if (error) {
      DEBUG.print(F("deserializeJson() failed: "));
      DEBUG.println(error.f_str());
      break;
    }
    
    ota_info->ota_available = 1;
    ota_info->ota_download  = 0;
    ota_info->ota_major     = doc["fw_version"]["major"];
    ota_info->ota_minor     = doc["fw_version"]["minor"];
    ota_info->ota_valid     = 0;
    ota_info->reserved1     = 0;
    ota_info->reserved2     = 0;
    
    // store values after JSON Decoding

    const char* fw_crc_s = doc["fw_crc"];
    fw_crc = (uint32_t)strtol(fw_crc_s, NULL, 16);
    // const char* fw_size_s = doc["fw_size"];
    // sprintf(fw_size_s,"%s\0",);
    fw_size = doc["fw_size"];
    memset(fw_link, 0, sizeof(fw_link));
    const char* fw_link_s = doc["fw_link"];
    memcpy(fw_link, fw_link_s, strlen(fw_link_s));

    DEBUG.printf("FW Version: [%d.%d]\r\nFW Size: [%d B]\r\nFW CRC = [0x%08x]\r\nFW Link = [%s]",
                ota_info->ota_major,
                ota_info->ota_minor,
                fw_size,
                fw_crc,
                fw_link);
      ret = SER_EX_OK;
    }while(false);
  }
  else
  {
    DEBUG.println("WiFi is Not Connected");
  }
  return ret;
}


/// @brief serial receive chunk
/// @param rx_byte 1 byte received from serial
/// @return None
static void ser_receive_chunk(uint8_t rx_byte)
{
  static uint32_t rec_data_crc;

  switch(hchunk.chunk_state){
		// receive SOF byte (1byte)
		case CUN_STATE_SOF:
		{
			/* initial variable again */
      memset(Rx_Buffer, 0, sizeof(Rx_Buffer));
			hchunk.index 		    = 0u;
			hchunk.data_len 	  = 0u;
			hchunk.crc_check = 0u;

      if(rx_byte == SER_SOF)
			{
				Rx_Buffer[hchunk.index++] = rx_byte;
				hchunk.chunk_state = CUN_STATE_PKT_TYPE;
				hchunk.chunk_ready = CUN_BUSY;
			}
		}
		break;
    		// receive the packet type (1byte)
		case CUN_STATE_PKT_TYPE:
		{
			if( rx_byte == SER_SOF ){
				/* initial variable again */
				memset(Rx_Buffer, 0, sizeof(Rx_Buffer));
				hchunk.index 		 = 0u;
				hchunk.data_len 	 = 0u;
				hchunk.crc_check  = 0u;
			}else
			{
				Rx_Buffer[hchunk.index++] = rx_byte;
				hchunk.chunk_state = CUN_STATE_LENGTH;
			}
		}
		break;

		// Get the data length
		case CUN_STATE_LENGTH:
		{
			Rx_Buffer[hchunk.index++] = rx_byte;

			if( hchunk.index >=4 ){
				hchunk.data_len = *(uint16_t *) &Rx_Buffer[2];
				if(hchunk.data_len <= MAX_SERIAL_DATA_LENGTH){
					hchunk.chunk_state = CUN_STATE_DATA;
				}
				else {
					hchunk.chunk_state = CUN_STATE_SOF;
				}
			}
		}
		break;

		// Receive data
		case CUN_STATE_DATA:
		{
			Rx_Buffer[hchunk.index++] = rx_byte;
			if( hchunk.index >= 4+hchunk.data_len )
			{
				hchunk.chunk_state = CUN_STATE_CRC;
			}
		}
		break;

		// Get the CRC
		case CUN_STATE_CRC:
    {
			Rx_Buffer[hchunk.index++] = rx_byte;
			if( hchunk.index >= 8+hchunk.data_len)
			{
        uint16_t inx = 4+hchunk.data_len;
        uint32_t rec_data_crc;
        uint16_t crc1 = *(uint16_t *) &Rx_Buffer[inx];
        uint16_t crc2 = *(uint16_t *) &Rx_Buffer[inx+2];
        rec_data_crc = (uint32_t) crc2<<16 | crc1;
        hchunk.chunk_state = CUN_STATE_EOF;
        
        uint32_t	cal_data_crc = 0u;
        cal_data_crc = ser_calcCRC(&Rx_Buffer[4], hchunk.data_len);    
        if(cal_data_crc != rec_data_crc)
        {
          DEBUG.printf("CHUNK CRC MISMATCH!!! received = [0x%08x] expect = [0x%08x]!!!\r\n",
                                                                rec_data_crc,
                                                                cal_data_crc);
          hchunk.crc_check = 1u;
        }
      }
    }
		break;


		case CUN_STATE_EOF:
		{
      do {
        Rx_Buffer[hchunk.index] = rx_byte;
        hchunk.chunk_ready = CUN_ERROR;
        hchunk.chunk_state = CUN_STATE_SOF;
        if(Rx_Buffer[hchunk.index] != SER_EOF)
        {
          break;
        }
        if(hchunk.crc_check)
        {
          break;
        }
        hchunk.chunk_ready = CUN_READY;
      }while(false);
		}
		break;
  }

}


/**
 * @brief send response to host
 * @param huart uart handler
 * @param rsp ACK or NACK response
 * @retval none
 */
static void ser_send_resp(uint8_t rsp){
	SER_RESP_ pack =
	{
		.sof 			    = SER_SOF,
		.packet_type	= SER_PACKET_TYPE_RESPONSE,
		.data_len		  = 1u,
		.status			  = rsp,
	};

	pack.crc = ser_calcCRC(&pack.status, 1);
  pack.eof = SER_EOF;

	//send respond
	serial_write((uint8_t *)&pack, sizeof(SER_RESP_));

}

void serial_write(uint8_t * buf, uint16_t len)
{
  for(int i=0; i < len; i++){
    Serial.write(buf[i]);
  }
}

/**
 * @brief Calculate CRC32
 * @param pData data that should be calculated
 * @param DataLength length of data
 * @retval CRC32
 */
uint32_t ser_calcCRC(uint8_t * pData, uint32_t DataLength)
{
    uint32_t Checksum = 0xFFFFFFFF;
    for(unsigned int i=0; i < DataLength; i++)
    {
        uint8_t top = (uint8_t)(Checksum >> 24);
        top ^= pData[i];
        Checksum = (Checksum << 8) ^ crc_table[top];
    }
    return Checksum;
}