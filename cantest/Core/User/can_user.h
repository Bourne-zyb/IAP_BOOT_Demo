
#ifndef __CAN_USER_H
#define __CAN_USER_H

/* Includes ------------------------------------------------------------------*/

#include "main.h"


// 定义命令处理函数类型
typedef void (*CommandHandler)(void);

void handle_restart(void);
void handle_show(void);
// 命令表结构体
typedef struct {
    const char *command; // 命令字符串
    CommandHandler handler; // 命令处理函数
} CommandEntry;


/* 函数声明 */
void can_board_init(void);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
void can_send(uint32_t id, uint8_t* data, uint8_t dlc);

#endif  

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
