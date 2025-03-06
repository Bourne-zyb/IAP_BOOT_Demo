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


/* Flash Memory Layout -------------------------------------------------------
    Define the address from where user application will be loaded.
    Note: this area is reserved for the IAP code   
+--------------+------------+----------------------------+-----------+----------------------------+
| Flash Region | Name       | Block Base Addresses       | Size      | Comment                    |
+--------------+------------+----------------------------+-----------+----------------------------+
| Main Memory  | Sector 0   | 0x0800 0000 - 0x0800 3FFF  | 16 Kbyte  | Bootloader    58 KB        |
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
#define APPLICATION_ADDRESS                 ((uint32_t)0x08010000)  /* Start user code address: Sector 4 */
#define APP_START_SECTOR                    FLASH_SECTOR_4         /* Use for IAP erase the app space */
#define APP_END_SECTOR                      FLASH_SECTOR_5         /* Use for IAP erase the app space */
#define USER_FLASH_SIZE                     ((uint32_t)0x00030000) /* Application size 192 KB */
#define USER_FLASH_END_ADDRESS              ((uint32_t)0x0807FFFF) /* Notable Flash addresses */

/* Exported types -----------------------------------------------------------*/
typedef enum {
  TRANSMIT_METHOD_USB,  /* Use USB transmission */
	TRANSMIT_METHOD_CAN,  /* Use CAN transmission */
  // TODO: Add other transmission methods
} TransmitMethod;

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
  void (*funtionJumpFunction)(void);                                                    /* Function jump pointer */
} IAP_Interface;


/* Exported macro -------------------------------------------------------------*/

/* Exported variables ---------------------------------------------------------*/
extern IAP_Receive_Struct iap_recive;
extern IAP_Interface iapInterface;

/* Exported function prototypes -----------------------------------------------*/
void IAP_Init(void);

#endif /* __IAP_USER_H */

