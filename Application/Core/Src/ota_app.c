/*
 * ota_app.c
 *
 *  Created on: 26 Oct 2023
 *      Author: Hamid
 */

#include "ota_app.h"

/*
 * Exception codes
 */
/* -------------- Defined static variables ------------- */
/* OTA State */
static OTA_STATE_ ota_state = OTA_STATE_IDLE;
/* Firmware Total Size that we are going to receive */
static uint32_t ota_fw_total_size;
/* Firmware image's CRC32 */
static uint32_t ota_fw_crc;
/* Firmware Size that we have received */
static uint32_t ota_fw_received_size;
/* Buffer to hold the received data */
static uint8_t Rx_Buffer[ OTA_PACKET_MAX_SIZE ];


/*------------------- Defined Functions -------------- */
OTA_EX_ ota_download_and_flash(UART_HandleTypeDef *huart);
static uint16_t ota_receive_chunk(UART_HandleTypeDef *huart, uint8_t *buf, uint16_t max_len );
static OTA_EX_ ota_process_data( uint8_t *buf, uint16_t len );
static void ota_send_resp(UART_HandleTypeDef *huart, uint8_t rsp);
static HAL_StatusTypeDef write_data_to_slot(uint8_t *data, uint8_t data_len, bool is_first_block);
uint32_t ota_calcCRC(uint8_t * pData, uint32_t DataLength);

/* -------------------- Functions ------------------- */

/**
 * @brief run ota application
 * @param hurat uart handler receive ota
 * @retval None
 */
void go_to_ota_app(UART_HandleTypeDef *huart)
{
  /*Start the Firmware or Application update */
    printf("Starting Firmware Download!!!\r\n");
    if( ota_download_and_flash(huart) != OTA_EX_OK )
    {
      /* Error. Don't process. */
      printf("OTA Update : ERROR!!! HALT!!!\r\n");
      HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    }
    else
    {
      /* Reset to load the new application */
      printf("Firmware update is done!!! Rebooting...\r\n");
      HAL_Delay(10000);
//      HAL_NVIC_SystemReset();
    }
}


/**
  * @brief Download the application from UART and flash it.
  * @param huart uart handler
  * @retval ETX_OTA_EX_
  */

OTA_EX_ ota_download_and_flash(UART_HandleTypeDef *huart)
{
	OTA_EX_ ret  = OTA_EX_OK;
	uint16_t    len;

	printf("Waiting for the OTA data\r\n");

	/* Reset the variables */
	ota_fw_total_size		= 0u;
	ota_fw_received_size	= 0u;
	ota_fw_crc				= 0u;
	ota_state				= OTA_STATE_START;

	do
	{
		// clear the buffer
		memset(Rx_Buffer, 0, OTA_PACKET_MAX_SIZE);
		len = ota_receive_chunk(huart, Rx_Buffer, OTA_PACKET_MAX_SIZE);

		if (len != 0)
		{
			ret = ota_process_data(Rx_Buffer, len);
		}
		else
		{
			// didn't received data or received more than expected. break
			ret = OTA_EX_ERR;
		}

		// Send ACK or NACK
		if( ret != OTA_EX_OK)
		{
			printf("Sending NACK\r\n");
			ota_send_resp(huart, OTA_NACK);
			break;
		}
		else
		{
			ota_send_resp(huart, OTA_ACK);
		}


	}while( ota_state != OTA_STATE_IDLE);

	return ret;
}

/**
  * @brief Receive a one chunk of data.
  * @paran huart uart handler
  * @param buf buffer to store the received data
  * @param max_len maximum length to receive
  * @retval OTA_EX_
  */
static uint16_t ota_receive_chunk(UART_HandleTypeDef *huart, uint8_t *buf, uint16_t max_len )
{
	int16_t  ret;
	uint16_t index		  =	0u;
	uint16_t data_len;
	uint32_t cal_data_crc = 0u;
	uint32_t rec_data_crc = 0u;

	do
	{
		// Receive SOF byte(1 byte)
		ret = HAL_UART_Receive(huart, &buf[index], 1, HAL_MAX_DELAY);
		if( ret != HAL_OK)
		{
			break;
		}

		if (buf[index++] != OTA_SOF)
		{
			// Not received start of frame
			break;
		}

		// Received the packet type (1 byte)
		ret = HAL_UART_Receive(huart, &buf[index++], 1, HAL_MAX_DELAY);
		if ( ret != HAL_OK)
		{
			// doesn't received anything
			break;
		}

		// Get the data length (2 byte)
		ret = HAL_UART_Receive(huart, &buf[index], 2, HAL_MAX_DELAY);
		if ( ret != HAL_OK)
		{
			// doesn't received anything
			break;
		}
		data_len = *(uint16_t *)&buf[index];
		index += 2u;

		for(uint16_t i = 0u; i < data_len; i++){
			ret = HAL_UART_Receive(huart, &buf[index++], 1, HAL_MAX_DELAY);
			if ( ret != HAL_OK)
			{
				// doesn't received anything
				break;
			}
		}

		if ( ret != HAL_OK)
		{
			// doesn't received anything
			break;
		}

		// Get the CRC (4 byte)
		ret = HAL_UART_Receive(huart, &buf[index], 4, HAL_MAX_DELAY);
		if ( ret != HAL_OK)
		{
			// doesn't received anything
			break;
		}
		rec_data_crc = *(uint32_t *)&buf[index];
		index +=4u;

		// Receive EOF byte (1 byte)
		ret = HAL_UART_Receive(huart, &buf[index], 1, HAL_MAX_DELAY);
		if ( ret != HAL_OK)
		{
			// doesn't received anything
			break;
		}

		if(buf[index++] != OTA_EOF)
		{
			// NOT received EOF
			ret = OTA_EX_ERR;
			break;
		}

		//calculate the received data's CRC
		cal_data_crc = ota_calcCRC(&buf[OTA_DATA_STARTBYTES], data_len);

		// Verify the CRC
		if( cal_data_crc != rec_data_crc )
		{
			printf("Chunk's CRC mismatch [Calc CRC = 0x%08lX] [Rec CRC = 0x%08lX]\r\n",
			                                                   cal_data_crc, rec_data_crc );
			ret = OTA_EX_ERR;
			break;
		}

	}while(false);

	if( ret != HAL_OK )
	{
		//clear the index if error
		index = 0u;
	}

	if( index > max_len )
	{
		printf("Received more data than expected. Expected = %d, Received = %d\r\n",
															  	  max_len, index );
		index = 0u;
	}

	return index;
}


/**
  * @brief Process the received data from UART.
  * @param buf buffer to store the received data
  * @param max_len maximum length to receive
  * @retval OTA_EX_
  */
static OTA_EX_ ota_process_data( uint8_t *buf, uint16_t len )
{
	OTA_EX_ ret = OTA_EX_ERR;

	do
	{
		if( (buf== NULL) || (len == 0u))
		{
			break;
		}

		//Check OTA Abort Command
		OTA_COMMAND_ *cmd = (OTA_COMMAND_ *)buf;
		if (cmd->packet_type == OTA_PACKET_TYPE_CMD){
			if(cmd->cmd == OTA_CMD_ABORT){
				// received OTA Abort Command stop process
				break;
			}
		}

		switch (ota_state)
		{
			case OTA_STATE_IDLE:
			{
				printf("OTA_STATE_IDLE....\r\n");
				ret = OTA_EX_OK;
			}
			break;

			case OTA_STATE_START:
			{
				OTA_COMMAND_ *cmd = (OTA_COMMAND_*)buf;
				if( cmd->packet_type == OTA_PACKET_TYPE_CMD )
				{
					if( cmd->cmd == OTA_CMD_START)
					{
						printf("Received OTA Start command\r\n");
						ota_state = OTA_STATE_HEADER;
						ret = OTA_EX_OK;
					}
				}
			}
			break;

			case OTA_STATE_HEADER:
			{
				OTA_HEADER_ *header = (OTA_HEADER_ *)buf;

				if( header->packet_type == OTA_PACKET_TYPE_HEADER )
				{
					ota_fw_total_size = header->meta_data.package_size;
					ota_fw_crc 		  = header->meta_data.package_crc;
					printf("Received OTA Header. FW size = %ld, FW crc = [0x%08lX]\r\n",
													ota_fw_received_size, ota_fw_crc);

					ota_state = OTA_STATE_DATA;
					ret = OTA_EX_OK;

				}
			}
			break;

			case OTA_STATE_DATA:
			{
				OTA_DATA_	 		*data 	 = (OTA_DATA_ *)buf;
				uint16_t			data_len = data->data_len;
				HAL_StatusTypeDef	ex		 = HAL_ERROR;

				if( data->packet_type == OTA_PACKET_TYPE_DATA ){
					bool is_first_block = false;

					if( ota_fw_received_size == 0){
						// This is the first block
						is_first_block = true;
						//TODO: CFG if needed
					}

					/* Write the chunk to the Flash */
					ex = write_data_to_slot(buf+4, data_len, is_first_block);

					//Delete this line in future
					ota_fw_received_size += data_len;

					if( ex == HAL_OK)
					{
						printf("[%ld/%ld]\r\n", ota_fw_received_size/OTA_DATA_MAX_SIZE,
												ota_fw_total_size/OTA_DATA_MAX_SIZE);

						if( ota_fw_received_size >= ota_fw_total_size)
						{
							// receive all data, move to end
							ota_state = OTA_STATE_END;
						}
						ret = OTA_EX_OK;
					}
				}
			}
			break;

			case OTA_STATE_END:
			{
				OTA_COMMAND_ *cmd = (OTA_COMMAND_ *)buf;

				if( cmd->packet_type == OTA_PACKET_TYPE_CMD )
				{
					if( cmd->cmd )
					{
						printf("Received OTA END COMMAND\r\n");

						printf("Validating the received Binary....\r\n");

						uint32_t cal_crc = ota_calcCRC((uint8_t *) OTA_SLOT_FLASH_ADDR
																, ota_fw_total_size);

						if(cal_crc != ota_fw_crc)
						{
							printf("ERROR: FW CRC Mismatch: calculated: [0x%08lx] received: [0x%08lx]\r\n",
																			cal_crc, ota_fw_crc);
							break;
						}

						printf("Done!\r\n");

						// TODO: Update CFG

						ret = OTA_EX_OK;
					}
				}
			}
			break;

			default:
			{
				ret = OTA_EX_ERR;
			}
			break;
		};
	}while(false);

	return ret;
}


/**
 * @brief Write received data to flash slot
 * @param data data to be written
 * @param data_len data length
 * @param is_first_block true if this is first block, false not first block
 * @retval HAL_StatusTypeDef
 */
static HAL_StatusTypeDef write_data_to_slot(uint8_t *data,
											uint8_t data_len,
											bool is_first_block)
{
	HAL_StatusTypeDef ret = HAL_ERROR;

	do
	{
		// Unlock Flash
		ret = HAL_FLASH_Unlock();
		if ( ret != HAL_OK )
		{
			break;
		}

		// Erase Only on First Block
		if( is_first_block )
		{
			printf("Erasing The Slot Flash memory....\r\n");
			// Erase The Flash
			FLASH_EraseInitTypeDef EraseInitStruct;
			uint32_t SectorError;

			EraseInitStruct.TypeErase			= FLASH_TYPEERASE_SECTORS;
			EraseInitStruct.Sector				= OTA_SLOT_SECTOR;
			EraseInitStruct.NbSectors			= OTA_SLOT_NB_SECTOR;
			EraseInitStruct.VoltageRange		= FLASH_VOLTAGE_RANGE_3;

			ret = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);

			if( ret != HAL_OK ){
				printf("Flash Erase Error On Sector 0x%08lx\r\n",SectorError);
				break;
			}
		}


	}while(false);

	return ret;
}

/**
 * @brief send response to host
 * @param huart uart handler
 * @param rsp ACK or NACK response
 * @retval none
 */
static void ota_send_resp(UART_HandleTypeDef *huart, uint8_t rsp){
	OTA_RESP_ pack =
	{
	  .sof			= OTA_SOF,
	  .packet_type	= OTA_PACKET_TYPE_RESPONSE,
	  .data_len		= 1u,
	  .status		= rsp,
	  .eof			= OTA_EOF
	};

	pack.crc = ota_calcCRC(&pack.status, 1);

	//send respond
	HAL_UART_Transmit(huart, (uint8_t *)&pack, sizeof(OTA_RESP_),HAL_MAX_DELAY);

}


/**
 * @brief Calculate CRC32
 * @param pData data that should be calculated
 * @param DataLength length of data
 * @retval CRC32
 */

uint32_t ota_calcCRC(uint8_t * pData, uint32_t DataLength)
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



