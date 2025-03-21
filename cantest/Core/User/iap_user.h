/******************************************************************************
 * @file    iap_user.h
 * @brief   In-Application Programming (IAP) module header file.
 * @author  Jason
 * @version V1.0.0
 * @date    2025-3
 * @copyright (c) 2025, All rights reserved.
 ******************************************************************************/

#ifndef __IAP_USER_H
#define __IAP_USER_H

/* Private Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"
#include "can_user.h"
#include "flash_if.h"
#include "menu.h"

/* Flash Memory Layout -------------------------------------------------------
    Define the address from where user application will be loaded.
    Note: this area is reserved for the IAP code   
+--------------+------------+----------------------------+-----------+----------------------------+
| Flash Region | Name       | Block Base Addresses       | Size      | Comment                    |
+--------------+------------+----------------------------+-----------+----------------------------+
| Main Memory  | Sector 0   | 0x0800 0000 - 0x0800 3FFF  | 16 Kbyte  | Bootloader    48 KB        |
|              | Sector 1   | 0x0800 4000 - 0x0800 7FFF  | 16 Kbyte  |                            |
|              | Sector 2   | 0x0800 8000 - 0x0800 BFFF  | 16 Kbyte  |                            |
|              +------------+----------------------------+-----------+----------------------------+
|              | Sector 3   | 0x0800 C000 - 0x0800 FFFF  | 16 Kbyte  | Upgrade flag storage 16 KB |
|              +------------+----------------------------+-----------+----------------------------+
|              | Sector 4   | 0x0801 0000 - 0x0801 FFFF  | 64 Kbyte  | App1          192 KB       |
|              | Sector 5   | 0x0802 0000 - 0x0803 FFFF  | 128 Kbyte |                            |
|              +------------+----------------------------+-----------+----------------------------+
|              | Sector 6   | 0x0804 0000 - 0x0805 FFFF  | 128 Kbyte | Backup        256 KB       |
|              | Sector 7   | 0x0806 0000 - 0x0807 FFFF  | 128 Kbyte |                            |
+--------------+------------+----------------------------+-----------+----------------------------+
------------------------------------------------------------------------------*/

/* Exported constants ------------------------------------------------------------*/
#define IAP_STATUS_ADDRESS                  ((uint32_t)0x0800C000)  /* Start user code address: Sector 3 */
#define IAP_STATUS_START_SECTOR             FLASH_SECTOR_3          /* Use for IAP status space */
#define IAP_STATUS_END_SECTOR               FLASH_SECTOR_3          /* Use for IAP status space */
#define IAP_STATUS_SIZE                     ((uint32_t)0x00004000)  /* 16 KB */

#define HEADER                              (0x55AA)
#define ENDER                               (0xAA55)
#define APPLICATION_ADDRESS                 ((uint32_t)0x08010000)  /* Start user code address: Sector 4 */
#define APP_START_SECTOR                    FLASH_SECTOR_4          /* Use for IAP erase the app space */
#define APP_END_SECTOR                      FLASH_SECTOR_5          /* Use for IAP erase the app space */
#define USER_FLASH_SIZE                     ((uint32_t)0x00030000)  /* Application size 192 KB */

#define USER_FLASH_END_ADDRESS              ((uint32_t)0x0807FFFF)  /* Notable Flash addresses */

/* Exported types -----------------------------------------------------------*/
typedef enum {
  TRANSMIT_METHOD_USB,  /* Use USB transmission */
	TRANSMIT_METHOD_CAN,  /* Use CAN transmission */
  // TODO: Add other transmission methods
} eIAP_TransmitMethod_Def;

typedef enum
{
  EL_FIND_SUCCESS, // Successfully found the latest data address
  EL_FIND_ERR,     // Have data but not right.
  EL_NOT_FOUND     // No valid data address found
} eFIND_Status_Def;

typedef enum
{
  IAP_NO_APP,
  IAP_DOWNING_BIN,
  IAP_APP_DONE,
} eIAP_Status_Def;

typedef enum
{
  NEWAPP_VILIBLE,
  NEWAPP_NOT_VILIBLE,
}eNEWAPP_Status_Def;

typedef struct  
{   
  eIAP_TransmitMethod_Def status;
  eIAP_Status_Def 				transmitMethod;
  uint16_t 								version;
	uint32_t								size;
}iap_msg_t;

#define FLASH_PROGRAM_SIZE       			4           /* world 字 对齐 跟flash写入的保持一致*/
#pragma pack(push, FLASH_PROGRAM_SIZE)         		/* flash 按照 FLASH_TYPEPROGRAM_WORD 写入的，因此按照4字节对齐 */ 
typedef struct  
{   
    uint16_t header;
    //TODO: add save data start.
    iap_msg_t iap_msg;
    //TODO: add save data end.
    uint16_t ender;
}save_data_t;
#pragma pack(pop)   

typedef enum {
  PKG_NOT_DONE = 0,     /* Not received a complete package */
  PKG_COMPLETE = 1,     /* Received a complete package */
  PKG_HANDLE_ING = 2    /* Processing the package (means there is still unprocessed data) */
} eReceiveStatus;

typedef struct {
  char recivebuf[1200];         /* Data buffer */
  uint16_t length;              /* Total data length */
  uint16_t handle_cnt;          /* Number of processed data (for STM32 official Ymodem protocol) */
  uint8_t pack64_cnt;           /* Number of 64-byte packages */
  eReceiveStatus iap_pkgtatus;  /* Current IAP status */
} IAP_Receive_Struct;

typedef struct {
  HAL_StatusTypeDef (*TransmitFunction)(void *data, uint16_t length, uint32_t timeout);  /* Transmit function pointer */
  HAL_StatusTypeDef (*ReceiveFunction)(uint8_t *data, uint16_t length, uint32_t timeout); /* Receive function pointer */
  void (*DelayTimeMsFunction)(uint32_t delaytime);                                      /* Delay function pointer */
  eNEWAPP_Status_Def (*funtionCheckFunction)(void);                                     /* Function check pointer */
  void (*funtionJumpFunction)(void);                                                    /* Function jump pointer */
} IAP_Interface;

/* Exported macro -------------------------------------------------------------*/

/* Exported variables ---------------------------------------------------------*/
extern IAP_Receive_Struct iap_recive;
extern IAP_Interface iapInterface;

/* Exported function prototypes -----------------------------------------------*/
void IAP_Init(void);
eFIND_Status_Def read_iap_status(save_data_t *read_data);
void write_iap_status(save_data_t *write_data);
#endif /* __IAP_USER_H */

