/*
 * serial_com.c
 *
 *  Created on: Oct 29, 2023
 *      Author: Hamid
 */


#include "serial_com.h"


/* -------------- Defined static variables ------------- */

/* Serial process state */
static SER_STATE_ ser_state = SER_STATE_START;

/* Buffer to hold the received data */
static uint8_t Rx_Buffer[MAX_SERIAL_SIZE];

/* chunk handler */
static CHUNK_HANDL_ hchunk =
{
	.chunk_ready  = CUN_EMPTY,
	.chunk_state  = CUN_STATE_SOF,
	.index		  	= 0u,
	.data_len	 		= 0u,
	.rec_data_crc = 0u,
};

/* Incoming data meta information */
static meta_info data_info;

/* data received size */
static uint16_t data_received_size = 0u;

/* data calculated CRC*/
static uint32_t data_calc_crc	= 0u;

/* Serial OTA Information */
extern ota_info ota_data;

/* UART Handler */
extern UART_HandleTypeDef huart5;

/* Buffer to hold the received BYTE */
extern uint8_t Rx_Byte[2];

/*Configuration */
OTA_GNRL_CFG_ *cfg_flash	=	(OTA_GNRL_CFG_*) (OTA_CFG_FLASH_ADDR);


/*------------------- Defined Functions -------------- */
static void ser_receive_chunk(uint8_t rx_byte);
static SER_EX_ ser_proccess_data( uint8_t *buf, uint16_t len);
static void ser_send_resp(UART_HandleTypeDef *huart, uint8_t rsp);
static HAL_StatusTypeDef write_cfg_to_flash( OTA_GNRL_CFG_ *cfg );
void ser_ota_requsted();
uint32_t ser_calcCRC(uint8_t * pData, uint32_t DataLength);


void serial_app(){
	do{
		// no Byte received or chunk reception in progress
		if( (hchunk.chunk_ready == CUN_EMPTY) ||
			(hchunk.chunk_ready == CUN_BUSY)){
			break;
		}

		SER_EX_ ret = SER_EX_OK;

		// An Error occur in during receive chunk
		if(hchunk.chunk_ready == CUN_ERROR)
		{
			printf("Receive Chunk Error\r\n");
			ret = SER_EX_ERROR;
		}
		else
		{
			printf("Chunk Received!!!\r\n");
			ret = ser_proccess_data(Rx_Buffer, hchunk.data_len);
		}

		// Send ACK or NACK
		if( ret != SER_EX_OK){
			ser_state = SER_STATE_START;
			printf("Sending NACK\r\n");
			ser_send_resp(&huart5, SER_NACK);
		}
		else
		{
			ser_send_resp(&huart5, SER_ACK);
		}

		/* check ota request */
		if(ota_data.ota_available & ota_data.ota_download & ota_data.ota_valid)
		{
			ser_ota_requsted();
		}

		hchunk.chunk_ready = CUN_EMPTY;
	}while(false);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	// check serial interruption
	if(huart==&huart5){
		if((hchunk.chunk_ready == CUN_EMPTY) || (hchunk.chunk_ready == CUN_BUSY))
		{
			ser_receive_chunk(Rx_Byte[0]);
		}
		memset(Rx_Byte, 0, sizeof(Rx_Byte));
		HAL_UART_Receive_IT(&huart5, Rx_Byte, 1);
	}
}

static void ser_receive_chunk(uint8_t rx_byte)
{
	uint32_t	cal_data_crc = 0u;

	switch(hchunk.chunk_state){

		// receive SOF byte (1byte)
		case CUN_STATE_SOF:
		{
			/* initial variable again */
			memset(Rx_Buffer, 0, sizeof(Rx_Buffer));
			hchunk.index 		 = 0u;
			hchunk.data_len 	 = 0u;
			hchunk.rec_data_crc  = 0u;


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
				hchunk.rec_data_crc  = 0u;
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
			Rx_Buffer[hchunk.index++] = rx_byte;
			if( hchunk.index >= 8+hchunk.data_len)
			{
				hchunk.rec_data_crc = *(uint32_t *) &Rx_Buffer[4+hchunk.data_len];
				hchunk.chunk_state = CUN_STATE_EOF;
			}
		break;


		case CUN_STATE_EOF:
		{
			do
			{
				Rx_Buffer[hchunk.index] = rx_byte;
				hchunk.chunk_ready = CUN_ERROR;
				hchunk.chunk_state = CUN_STATE_SOF;

				if(Rx_Buffer[hchunk.index] != SER_EOF)
				{
					break;
				}

				cal_data_crc = ser_calcCRC(&Rx_Buffer[4], hchunk.data_len);
				if(cal_data_crc != hchunk.rec_data_crc)
				{
					printf("CHUNK CRC MISMATCH!!! [Calc CRC = 0x%08lX] [Rec CRC = 0x%08lX]\r\n",
												                   cal_data_crc,
																   hchunk.rec_data_crc );
					break;
				}

				hchunk.chunk_ready = CUN_READY;

			}while(false);
		}
		break;
	}
}


static SER_EX_ ser_proccess_data( uint8_t *buf, uint16_t len)
{
	SER_EX_ ret = SER_EX_ERROR;

	do
	{
		if( (buf==NULL) || (len == 0u) )
		{
			break;
		}

		// Check Serial Abort Command
		SER_COMMAND_ *cmd = (SER_COMMAND_ *)buf;
		if(cmd->packet_type == SER_PACKET_TYPE_CMD)
		{
			if(cmd->cmd == SER_CMD_ABORT)
			{
				// Receive Serial Abort Command. stop process;
				ser_state = SER_STATE_START;
				break;
			}
		}

		switch(ser_state)
		{

			case SER_STATE_START:
			{

				data_info.data_crc 	= 0u;
				data_info.data_size	= 0u;
				data_info.data_type	= 0u;
				data_received_size	= 0u;
				data_calc_crc				= 0u;

				SER_COMMAND_ *cmd = (SER_COMMAND_ *)buf;
				if( cmd->packet_type == SER_PACKET_TYPE_CMD )
				{
					if( cmd->cmd == SER_CMD_START )
					{
						printf("Received Serial Start Command \r\n");
						ser_state = SER_STATE_HEADER;
						ret = SER_EX_OK;
					}
				}
			}
			break;


			case SER_STATE_HEADER:
			{
				SER_HEADER_ *header = (SER_HEADER_ *)buf;

				if( header->packet_type == SER_PACKET_TYPE_HEADER )
				{
					data_info.data_type = header->meta_data.data_type;
					data_info.data_size = header->meta_data.data_size;
					data_info.data_crc	 = header->meta_data.data_crc;

					printf("Received Data Header. type=[%d], size=[%d], crc=[0x%08lX]\r\n",
																									data_info.data_type,
																									data_info.data_size,
																									data_info.data_crc);
					ser_state = SER_STATE_DATA;
					ret = SER_EX_OK;
				}
			}
			break;


			case SER_STATE_DATA:
			{
				SER_DATA_				*data			= (SER_DATA_ *)buf;
				uint16_t				data_len	=	data->data_len;

				if( data->packet_type == SER_PACKET_TYPE_DATA )
				{
					switch(data_info.data_type)
					{
						case NORMAL_DATA:
						{
							// RESERVED: for incomming data
						}
						break;

						case STATUS_DATA:
						{
							// RESERVED: for modem status
						}
						break;

						case OTA_INFO_DATA:
						{
								ota_data = *(ota_info *)&buf[4];
								ota_data.ota_valid = 0;
								data_received_size 			= data_len;
								data_calc_crc			 	= ser_calcCRC(&buf[4], data_len);
								ret = SER_EX_OK;

						}
						break;

						default:
						{
							// shouldn't be here
							ret = SER_EX_ERROR;
						}
						break;
					}

					if( data_received_size >= data_info.data_size )
					{
						//Received All data, move to end
						ser_state = SER_STATE_END;
					}
				}
			}
			break;


			case SER_STATE_END:
			{
				SER_COMMAND_ *cmd = (SER_COMMAND_ *)buf;

				if( cmd->packet_type == SER_PACKET_TYPE_CMD)
				{
					if(cmd->cmd == SER_CMD_END)
					{
						printf("Receive SERIAL END COMMAND\r\nValidation...\r\n");

						//Validation the received packets
						//TODO: validation normal data

						// one packet data so:
						if( data_info.data_type == OTA_INFO_DATA ||  data_info.data_type == STATUS_DATA )
						{
							if(data_calc_crc != data_info.data_crc)
							{
								printf("ERROR: FW CRC Mismatch: calculated: [0x%08lx] received: [0x%08lx]\r\n",
												data_calc_crc, data_info.data_crc);
								break;
							}

							if(data_info.data_type == OTA_INFO_DATA)
							{
								ota_data.ota_valid = 1u;
								printf("A NEW FIRMWARE FOUND!!! VERSION = [%d,%ld]\r\n", ota_data.ota_major,
																																				 ota_data.ota_minor);
							}
						}
						printf("Validated Successfully!\r\n");

						ser_state = SER_STATE_START;
						ret = SER_EX_OK;

					}
				}
			}
			break;

			default:
			{
				ret = SER_EX_ERROR;
			}
			break;
			// state cases end
		};

	}while(false);

	return ret;
}

/**
 * @brief send response to host
 * @param huart uart handler
 * @param rsp ACK or NACK response
 * @retval none
 */
static void ser_send_resp(UART_HandleTypeDef *huart, uint8_t rsp){
	SER_RESP_ pack =
	{
		.sof 			= SER_SOF,
		.packet_type	= SER_PACKET_TYPE_RESPONSE,
		.data_len		= 1u,
		.status			= rsp,
		.eof			= SER_EOF
	};

	pack.crc = ser_calcCRC(&pack.status, 1);

	//send respond
	HAL_UART_Transmit(huart, (uint8_t *)&pack, sizeof(SER_RESP_),HAL_MAX_DELAY);

}


/*
 * @brief save boot reason to OTA update and save data in configuration
 * sector patr
 */
void ser_ota_requsted()
{
	do
	{
		HAL_StatusTypeDef ret = HAL_ERROR;

		/* read configuration */
		OTA_GNRL_CFG_ cfg;
		memcpy(&cfg, cfg_flash, sizeof(OTA_GNRL_CFG_));

		/* set reboot cause to ota request */
		cfg.reboot_cause = OTA_UPDATE_APP;

		/* write back config */
		ret = write_cfg_to_flash(&cfg);

		if( ret != HAL_OK )
		{
			printf("Write configuratin Error!!!\r\n");
			break;
		}

		printf("Reboot for Update...\r\n");

		HAL_NVIC_SystemReset();

	}while(false);
}


/**
  * @brief Write the configuration to flash
  * @param cfg config structure
  * @retval none
  */
static HAL_StatusTypeDef write_cfg_to_flash( OTA_GNRL_CFG_ *cfg )
{
	HAL_StatusTypeDef ret = HAL_ERROR;

	do
	{
		if( cfg == NULL )
		{
			break;
		}

		ret = HAL_FLASH_Unlock();
		if( ret != HAL_OK )
		{
			break;
		}

		// Check if the FLASH_FLAG_BSY
		FLASH_WaitForLastOperation(HAL_MAX_DELAY);

		// Erase the flash configuration sector
		FLASH_EraseInitTypeDef EraseInitStruct;
		uint32_t SectorError;

		EraseInitStruct.TypeErase		= FLASH_TYPEERASE_SECTORS;
		EraseInitStruct.Sector			= OTA_CFG_SECTOR;
		EraseInitStruct.NbSectors		= 1u;
		EraseInitStruct.VoltageRange	= FLASH_VOLTAGE_RANGE_3;

		// clear all flags before you write it to flash
		    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR |
		                FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR);

		ret = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
		if( ret != HAL_OK )
		{
			break;
		}

		// Write the configuration
		uint8_t *data = (uint8_t*) cfg;
		for( uint32_t i = 0u; i<sizeof(OTA_GNRL_CFG_); i++ )
		{
			ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,
									OTA_CFG_FLASH_ADDR + i,
									data[i]);
			if( ret != HAL_OK )
			{
				printf("Slot table Flash Write Error\r\n");
				break;
			}
		}

	    //Check if the FLASH_FLAG_BSY.
	    FLASH_WaitForLastOperation( HAL_MAX_DELAY );

	    if( ret != HAL_OK )
	    {
	      break;
	    }

	    ret = HAL_FLASH_Lock();
	    if( ret != HAL_OK )
	    {
	      break;
	    }
	}while(false);

	return ret;
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
