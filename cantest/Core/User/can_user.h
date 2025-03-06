/******************************************************************************
 * @file    can_user.h
 * @brief   This file is the header file for the CAN module, defining the CAN-related interfaces and functions.
 * @author  Jason
 * @version V1.0.0
 * @date    2025-03
 * @copyright (c) 2025, All rights reserved.
 ******************************************************************************/

#ifndef __CAN_USER_H
#define __CAN_USER_H

/* Private Includes ----------------------------------------------------------------*/
#include "main.h"

/* Exported constants ------------------------------------------------------------*/

/* Exported types -----------------------------------------------------------------*/
typedef void (*CommandHandler)(void);
typedef struct {
    const char *command; // 命令字符串
    CommandHandler handler; // 命令处理函数
} CommandEntry;

/* Exported macro ----------------------------------------------------------------*/

/* Exported variables ------------------------------------------------------------*/

/* Exported function prototypes --------------------------------------------------*/
void handle_restart(void);
void handle_show(void);

void can_board_init(void);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
void can_send(uint32_t id, uint8_t* data, uint8_t dlc);

#endif /* __CAN_USER_H */

