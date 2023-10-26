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


ETX_OTA_EX_ etx_ota_download_and_flash(UART_HandleTypeDef *huart);


void go_to_ota_app(UART_HandleTypeDef *huart)
{
  /*Start the Firmware or Application update */
    printf("Starting Firmware Download!!!\r\n");
    if( etx_ota_download_and_flash(huart) != ETX_OTA_EX_OK )
    {
      /* Error. Don't process. */
      printf("OTA Update : ERROR!!! HALT!!!\r\n");
      HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    HAL_Delay(10000);
//      HAL_NVIC_SystemReset();
    }
    else
    {
      /* Reset to load the new application */
      printf("Firmware update is done!!! Rebooting...\r\n");
      HAL_NVIC_SystemReset();
    }
}

ETX_OTA_EX_ etx_ota_download_and_flash(UART_HandleTypeDef *huart)
{
	// TODO:
	return ETX_OTA_EX_OK;
}
