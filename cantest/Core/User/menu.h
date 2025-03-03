/**
  ******************************************************************************
  * @file    IAP_Main/Inc/menu.h 
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    8-April-2015
  * @brief   This file provides all the headers of the menu functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MENU_H
#define __MENU_H

/* Includes ------------------------------------------------------------------*/
#include "flash_if.h"
#include "ymodem.h"

/* Imported variables --------------------------------------------------------*/
extern uint8_t aFileName[FILE_NAME_LENGTH];

/* Private variables ---------------------------------------------------------*/
typedef  void (*pFunction)(void);

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void Main_Menu(void);

// 定义传输方式的枚举类型
typedef enum {
    TRANSMIT_METHOD_USB,  // 使用 USB 传输
		// TODO: 添加其他传输方式
} TransmitMethod;

typedef enum {
    PKG_NOT_DONE = 0,     // 未接收到完整的一包数据
    PKG_COMPLETE = 1,      // 接收到完整的一包数据
    PKG_HANDLE_ING = 2      // 正在处理数据包中（意味着还有没解析处理完的数据）
} eReceiveStatus;

typedef struct {
  char  recivebuf[1200];
  uint16_t length;        // 数据总长度       
  uint16_t handle_cnt;    // 处理解析了多少个数据了（为了适配stm32官方ymodem协议中
                          //                     通过开头第一个字节来判断后续的逻辑）
  uint8_t pack64_cnt;    // 分成了多少个 64 字节的包   
  eReceiveStatus iap_pkgtatus;    // iap当前的状态
} IAP_Receive_Struct;   

extern IAP_Receive_Struct iap_recive;

typedef struct {
    uint8_t (*TransmitFunction)(uint8_t *data, uint16_t length, uint32_t timeout);  // 发送函数指针
    uint8_t (*ReceiveFunction)(uint8_t *data, uint16_t length, uint32_t timeout);   // 接收函数指针
    void (*DelayTimeMs)(uint32_t delaytime);                          // 延时函数指针
} IAP_Interface;

// 定义IAP接口

extern IAP_Interface iapInterface;
void IAP_Init(void);
#endif  /* __MENU_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
