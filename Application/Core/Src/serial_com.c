/*
 * serial_com.c
 *
 *  Created on: Oct 29, 2023
 *      Author: Hamid
 */


#include "serial_com.h"



/* Buffer to hold the received data */
static uint8_t Rx_Buffer[MAX_SERIAL_SIZE];

/* chunk handler */
static CHUNK_HANDL_ hchunk =
{
	.chunk_ready  = CUN_EMPTY,
	.chunk_state  = CUN_STATE_SOF,
	.index		  = 0u,
	.data_len	  = 0u,
	.rec_data_crc = 0u,
};

extern UART_HandleTypeDef huart5;
extern uint8_t Rx_Byte[2];



static void ser_receive_chunk(uint8_t rx_byte);
uint32_t ser_calcCRC(uint8_t * pData, uint32_t DataLength);


void serial_app(){

	if(hchunk.chunk_ready == CUN_READY)
	{
		printf("Chunk Received!!!\r\n");
		hchunk.chunk_ready = CUN_EMPTY;
	}
	if(hchunk.chunk_ready == CUN_ERROR)
	{
		printf("Receive Chunk Error\r\n");
		hchunk.chunk_ready = CUN_EMPTY;
	}
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