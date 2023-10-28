/*
 * ota_app_boot.c
 *
 *  Created on: 28 Oct 2023
 *      Author: Hamid
 */

#include "ota_app_boot.h"


/*configuration */
OTA_GNRL_CFG_ *cfg_flash = (OTA_GNRL_CFG_ *) (OTA_CFG_FLASH_ADDR);


static HAL_StatusTypeDef write_cfg_to_flash( OTA_GNRL_CFG_ *cfg );
uint32_t ota_calcCRC(uint8_t * pData, uint32_t DataLength);

/**
  * @brief Validate application
  * @retval none
  */
void validate_app(){

	// Read Configuration
	OTA_GNRL_CFG_ cfg;
	memcpy(&cfg, cfg_flash, sizeof(OTA_GNRL_CFG_));

	FLASH_WaitForLastOperation( HAL_MAX_DELAY );

	uint32_t cal_data_crc = ota_calcCRC((uint8_t *) OTA_APP_FLASH_ADDR, cfg.slot_table.fw_size);


	FLASH_WaitForLastOperation( HAL_MAX_DELAY );

	// Verify the CRC
	if( cal_data_crc != cfg.slot_table.fw_crc)
	{
		printf("Error, CRC_calc = [0x%08lx], Config_CRC = [0x%08lx]\r\n HALT!", cal_data_crc, cfg.slot_table.fw_crc);
		while(1);
	}

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

