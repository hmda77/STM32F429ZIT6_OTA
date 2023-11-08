#include "fwSerial.h"


/* -------------- Defined  variables ------------- */

// serial receive state
static SER_STATE_ ser_state = SER_STATE_START;
static SER_STATE_ last_state = SER_STATE_START;

// Serial Variables
volatile byte receivedData;
byte buffer[64];
int bufferIndex = 0;

/* Buffer to hold the received data */
static uint8_t Rx_Buffer[MAX_SERIAL_SIZE];
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



/*------------------- Defined Functions -------------- */
static void ser_receive_chunk(uint8_t rx_byte);
static void ser_send_resp(uint8_t rsp);
static SER_EX_ ser_process_rsp( uint8_t *buf, uint16_t len);
SER_EX_ send_info();
void send_ser_start();
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
            case SER_CMD_FW_STATUS:
            {
              DEBUG.println("SER_CMD_FW_STATUS!");

              send_ser_start();
              last_state  = ser_state;
              ser_state   = SER_STATE_WRSP;
              ret = SER_EX_OK;
            }
            break;

            case SER_CMD_FW_GET:
            {
              DEBUG.println("SER_CMD_FW_GET!");
              send_ser_start();
              last_state  = ser_state;
              ser_state   = SER_STATE_WRSP;
              ret = SER_EX_OK;
            }


            default:
            {
              DEBUG.println("invalid command!");
            }
            break;
          }
        }
      }
      break;

      case SER_STATE_WRSP:
      {
        SER_RESP_ *rsp = (SER_RESP_ *)buf;
        if(rsp->packet_type == SER_PACKET_TYPE_RESPONSE)
        {
          if(rsp->status == SER_ACK)
          {
            ret = SER_EX_OK;
            switch(last_state)
            {
              case SER_STATE_START:
              {
                ser_state = SER_STATE_HEADER;
              }
              break;

              case SER_STATE_HEADER:
              {
                ser_state = SER_STATE_DATA;
              }
              break;

              case SER_STATE_DATA:
              {
                ser_state = SER_STATE_END;
              }

              case SER_STATE_END:
              {
                ser_state = SER_STATE_START;
              }
              break;
              default:
              {
                ret = SER_EX_ERROR;
                ser_state = SER_STATE_START;
              }
              break;
            }
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
  DEBUG.println("D1");
  SER_COMMAND_ *ser_start = (SER_COMMAND_ *)Tx_Buffer;
  DEBUG.println("D2");
  int ret = 0;

  // memset(Tx_Buffer, 0, MAX_SERIAL_SIZE);
  // DEBUG.println("D3");

  // ser_start->sof          = SER_SOF;
  // ser_start->packet_type  = SER_PACKET_TYPE_CMD;
  // ser_start->data_len     = 1u;
  // ser_start->cmd          = SER_CMD_START;
  // // ser_start->crc          = ser_calcCRC(&ser_start->cmd, 1);
  // ser_start->crc          = 0u;
  // ser_start->eof          = SER_EOF;
  // DEBUG.println("D4");

  // len = sizeof(SER_COMMAND_);

  // DEBUG.println("D5");
  // for(int i=0; i < len; i++)
  // {
  //   delay(1);
  //   Serial.write(Tx_Buffer[i]);
  // }

}


SER_EX_ send_info()
{
  SER_EX_ ret = SER_EX_ERROR;
  // /* ------MODEM VARIABLE----- */
  // char fw_buf[MAX_FW_INFO];

  // /* server configurations */
  // const char* fwServer    = FW_SERVER;
  // const char* fwUrl       = FW_INFO_URL;
  // const char* filename    = FW_FILE_NAME;
  // if ((WiFiMulti.run() == WL_CONNECTED)) {
  //   do
  //   {
  //     /* clear buffer */
  //     memset(fw_buf, 0, sizeof(fw_buf));

  //     if(get_fw_info(fwUrl, fw_buf) != NET_EX_OK)
  //     {
  //       DEBUG.println("get_fw_info failed");
  //       break;
  //     }

  //         // create JSON object
  //   StaticJsonDocument<96> doc;

  //   DeserializationError error = deserializeJson(doc, fw_buf, MAX_FW_INFO);

  //   if (error) {
  //     DEBUG.print(F("deserializeJson() failed: "));
  //     DEBUG.println(error.f_str());
  //     break;
  //   }

  //   // store values after JSON Decoding
  //   uint32_t fw_version_minor = doc["fw_version"]["minor"];
  //   uint16_t fw_version_major = doc["fw_version"]["major"]; 

  //   const char* fw_crc_s = doc["fw_crc"];
  //   uint32_t fw_crc = (uint32_t)strtol(fw_crc_s, NULL, 16);
  //   uint32_t fw_size = doc["fw_size"];
  //   const char* fw_link = doc["fw_link"];

  //   DEBUG.printf("FW Version: [%d.%d]\r\nFW Size: [%d B]\r\nFW CRC = [0x%08x]\r\nFW Link = [%s]",
  //               fw_version_major,
  //               fw_version_minor,
  //               fw_size,
  //               fw_crc,
  //               fw_link);
  //     ret = SER_EX_OK;
  //   }while(false);
  // }
  // else
  // {
  //   DEBUG.println("WiFi is Not Connected");
  // }
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