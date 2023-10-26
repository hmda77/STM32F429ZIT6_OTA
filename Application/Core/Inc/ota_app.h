/*
 * ota_app.h
 *
 *  Created on: 26 Oct 2023
 *      Author: Hamid
 */

#ifndef INC_OTA_APP_H_
#define INC_OTA_APP_H_

#include "main.h"

typedef enum
{
  ETX_OTA_EX_OK       = 0,    // Success
  ETX_OTA_EX_ERR      = 1,    // Failure
}ETX_OTA_EX_;

void go_to_ota_app(UART_HandleTypeDef *huart);

#endif /* INC_OTA_APP_H_ */
