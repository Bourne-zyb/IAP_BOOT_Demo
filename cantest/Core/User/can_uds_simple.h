/******************************************************************************
 * @file    can_uds_simple.h
 * @brief   CAN User Data Service (UDS) Simple Example header file.
 * @author  Jason
 * @version V1.0.0
 * @date    2025-3
 * @copyright (c) 2025, All rights reserved.
 ******************************************************************************/

#ifndef __CAN_UDS_SIMPLE_H
#define __CAN_UDS_SIMPLE_H

/* Private Includes ----------------------------------------------------------*/
#include "can_user.h"

/* Exported constants --------------------------------------------------------*/
#define APPLICATION_ADDRESS 0x8001800
#define FLASH_PAGE_SIZE ((uint16_t)0x800)
#define FLASH_PAGES_COUNT 61

#define PROG_START_ADDR 0x8001800
#define PROG_END_ADDR 0x8020000

#define CANID_RX 0x707
#define CANID_TX 0x70F

#define FC_FS_CTS 0 // ContinueToSend
#define FC_FS_WT 1 // Wait
#define FC_FS_OVFLW 2 // Overflow

#define bool uint8_t
#define true 1
#define false 0

/* Exported types ------------------------------------------------------------*/
typedef struct {
    uint32_t CanPort;
    uint16_t Id;
    uint8_t DLC;
    uint8_t Data[8];
    uint8_t Delay;
} canMsg;
/* Exported macro ------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/
void can_uds_handle(uint32_t id, uint8_t dlc, uint8_t *data);

#endif /* __CMD_USER_H */
 
 