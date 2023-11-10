/*
 * main.h
 *
 *  Created on: 26-Jul-2021
 *      Author: EmbeTronicX
 */

#ifndef INC_ETX_OTA_UPDATE_MAIN_H_
#define INC_ETX_OTA_UPDATE_MAIN_H_

#define ETX_OTA_SOF  0xAA    // Start of Frame
#define ETX_OTA_EOF  0xBB    // End of Frame
#define ETX_OTA_ACK  0x00    // ACK
#define ETX_OTA_NACK 0x01    // NACK
#define OTA_REQ      0xEE		 // Command for request OTA from modem

#define ETX_APP_FLASH_ADDR 0x08040000   //Application's Flash Address

#define ETX_OTA_DATA_MAX_SIZE ( 1024 )  //Maximum data Size
#define ETX_OTA_DATA_OVERHEAD (    9 )  //data overhead
#define ETX_OTA_PACKET_MAX_SIZE ( ETX_OTA_DATA_MAX_SIZE + ETX_OTA_DATA_OVERHEAD )
#define ETX_OTA_MAX_FW_SIZE ( 1024 * 896 )

/*
 * Data types
 */
#define	NORMAL_DATA						 0x00	// NORMAL DATA
#define	STATUS_DATA 					 0x01	// data include status information
#define	OTA_INFO_DATA					 0x02	// information of OTA


/*
 * Exception codes
 */
typedef enum
{
  ETX_OTA_EX_OK       = 0,    // Success
  ETX_OTA_EX_ERR      = 1,    // Failure
}ETX_OTA_EX_;

/*
 * OTA process state
 */
typedef enum
{
  ETX_OTA_STATE_IDLE    = 0,
  ETX_OTA_STATE_START   = 1,
  ETX_OTA_STATE_HEADER  = 2,
  ETX_OTA_STATE_DATA    = 3,
  ETX_OTA_STATE_END     = 4,
}ETX_OTA_STATE_;

/*
 * Packet type
 */
typedef enum
{
  ETX_OTA_PACKET_TYPE_CMD       = 0,    // Command
  ETX_OTA_PACKET_TYPE_DATA      = 1,    // Data
  ETX_OTA_PACKET_TYPE_HEADER    = 2,    // Header
  ETX_OTA_PACKET_TYPE_RESPONSE  = 3,    // Response
}ETX_OTA_PACKET_TYPE_;

/*
 * OTA Commands
 */
typedef enum
{
  ETX_OTA_CMD_START = 0,    // OTA Start command
  ETX_OTA_CMD_END   = 1,    // OTA End command
  ETX_OTA_CMD_ABORT = 2,    // OTA Abort command
}ETX_OTA_CMD_;

/*
 * OTA meta info
 */
typedef struct
{
  uint32_t package_size;
  uint32_t package_crc;
  uint32_t reserved1;
  uint32_t reserved2;
}__attribute__((packed)) meta_info;

/*
 * OTA Command format
 *
 * ________________________________________
 * |     | Packet |     |     |     |     |
 * | SOF | Type   | Len | CMD | CRC | EOF |
 * |_____|________|_____|_____|_____|_____|
 *   1B      1B     2B    1B     4B    1B
 */
typedef struct
{
  uint8_t   sof;
  uint8_t   packet_type;
  uint16_t  data_len;
  uint8_t   cmd;
  uint32_t  crc;
  uint8_t   eof;
}__attribute__((packed)) ETX_OTA_COMMAND_;

/*
 * OTA Header format
 *
 * __________________________________________
 * |     | Packet |     | Header |     |     |
 * | SOF | Type   | Len |  Data  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B     16B     4B    1B
 */
typedef struct
{
  uint8_t     sof;
  uint8_t     packet_type;
  uint16_t    data_len;
  meta_info   meta_data;
  uint32_t    crc;
  uint8_t     eof;
}__attribute__((packed)) ETX_OTA_HEADER_;

/*
 * OTA Data format
 *
 * __________________________________________
 * |     | Packet |     |        |     |     |
 * | SOF | Type   | Len |  Data  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B    nBytes   4B    1B
 */
typedef struct
{
  uint8_t     sof;
  uint8_t     packet_type;
  uint16_t    data_len;
  uint8_t     *data;
}__attribute__((packed)) ETX_OTA_DATA_;

/*
 * OTA Response format
 *
 * __________________________________________
 * |     | Packet |     |        |     |     |
 * | SOF | Type   | Len | Status | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B      1B     4B    1B
 */
typedef struct
{
  uint8_t   sof;
  uint8_t   packet_type;
  uint16_t  data_len;
  uint8_t   status;
  uint32_t  crc;
  uint8_t   eof;
}__attribute__((packed)) ETX_OTA_RESP_;

/*
 * serial data information
 */
typedef struct
{
	uint8_t  ota_available;		// OTA availability Check
	uint8_t  ota_download;		// OTA download complete
	uint16_t ota_major;				// OTA Major version
	uint32_t ota_minor;				// OTA Minor version
	uint8_t	 ota_valid;				// OTA Valid Flag (Received byte is not affected)
	uint32_t reserved1;
	uint32_t reserved2;
}__attribute__((packed)) ser_ota_info;



/*
 * serial header meta info in header
 */
typedef struct
{
  uint8_t  data_type;		// refer to SER_DATA_TYPE
  uint32_t data_size;		// size of incoming data
  uint32_t data_crc;		// CRC of incoming data
  uint32_t reserved1;
  uint32_t reserved2;
}__attribute__((packed)) ser_meta_info;







/*
 * Serial Header format
 *
 * __________________________________________
 * |     | Packet |     | Header |     |     |
 * | SOF | Type   | Len |  Data  | CRC | EOF |
 * |_____|________|_____|________|_____|_____|
 *   1B      1B     2B     16B     4B    1B
 */
typedef struct
{
  uint8_t           sof;
  uint8_t           packet_type;
  uint16_t          data_len;
  ser_meta_info     meta_data;
  uint32_t          crc;
  uint8_t           eof;
}__attribute__((packed)) SER_HEADER_;



/*
 * Serial OTA Data format
 *
 * ___________________________________________
 * |     | Packet |     |         |     |     |
 * | SOF | Type   | Len |   OTA   | CRC | EOF |
 * |_____|________|_____|_________|_____|_____|
 *   1B      1B     2B    nBytes   4B    1B
 */
typedef struct
{
  uint8_t         sof;
  uint8_t         packet_type;
  uint16_t        data_len;
  ser_ota_info 	  ota_data;
  uint32_t        crc;
  uint8_t         eof;
}__attribute__((packed)) SER_OTA_;

#endif /* INC_ETX_OTA_UPDATE_MAIN_H_ */