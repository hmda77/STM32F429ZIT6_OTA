/*
 * serial_com.h
 *
 *  Created on: Oct 29, 2023
 *      Author: Hamid
 */

#ifndef INC_SERIAL_COM_H_
#define INC_SERIAL_COM_H_

#include "main.h"


#define MAX_SERIAL_DATA_LENGTH					( 1024 )	// Max Data frame can have
#define MAX_SERIAL_OVERHEAD						( 9 )		// frame overhead like SOF, data length, ...
#define MAX_SERIAL_SIZE 						1024 + 9	// Maximum serial frame
#define EOF_SERIAL								0xBB		// end of serial frame indicator

#endif /* INC_SERIAL_COM_H_ */
