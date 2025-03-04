/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_if.c
  * @version        : v1.0_Cube
  * @brief          : Usb device for Virtual Com Port.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"

/* USER CODE BEGIN INCLUDE */
#include "menu.h"
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device library.
  * @{
  */

/** @addtogroup USBD_CDC_IF
  * @{
  */

/** @defgroup USBD_CDC_IF_Private_TypesDefinitions USBD_CDC_IF_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */
USBD_CDC_LineCodingTypeDef USBD_CDC_LineCoding =
{
    115200,      // 默认波特率
    0x00,        // 1位停止位
    0x00,        // 无奇偶校验
    0x08,        // 数据位，8位数据位
};

 
/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Defines USBD_CDC_IF_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */
/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Macros USBD_CDC_IF_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Variables USBD_CDC_IF_Private_Variables
  * @brief Private variables.
  * @{
  */
/* Create buffer for reception and transmission           */
/* It's up to user to redefine and/or remove those define */
/** Received data over USB are stored in this buffer      */
uint8_t UserRxBufferFS[APP_RX_DATA_SIZE];

/** Data to send over USB CDC are stored in this buffer   */
uint8_t UserTxBufferFS[APP_TX_DATA_SIZE];

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Exported_Variables USBD_CDC_IF_Exported_Variables
  * @brief Public variables.
  * @{
  */

extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_FunctionPrototypes USBD_CDC_IF_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len);

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */

#include <string.h> // 用于字符串比较

// 定义命令处理函数类型
typedef void (*CommandHandler)(void);

void handle_restart(void);
void handle_show(void);
// 命令表结构体
typedef struct {
    const char *command; // 命令字符串
    CommandHandler handler; // 命令处理函数
} CommandEntry;

CommandEntry commandTable[] = {
  {"restart", handle_restart},
  {"show", handle_show},
//  {"stop", handle_stop},
  // 继续添加更多命令
};

// 示例命令处理函数
extern uint32_t canrecv_cnt;
extern uint8_t expectedData[8]; // 用于存储预期的数据
void handle_restart(void) {
    // 将 cnt 变量清零
    canrecv_cnt = 0;
		memset(expectedData, 0, sizeof(expectedData));
		
    uint8_t usbBuf[50]; // 用于存储要发送的字符串
    uint16_t len = 0;

    // 格式化字符串到 usbBuf
    len += sprintf((char *)&usbBuf[len], "cnt has been reset to 0.\r\n");

    // 使用 CDC_Transmit_FS 发送数据   
    CDC_Transmit_FS(usbBuf, len);
}

void handle_show(void) {
    uint8_t usbBuf[50]; // 用于存储要发送的字符串
    uint16_t len = 0;

    // 格式化字符串到 usbBuf
    len += sprintf((char *)&usbBuf[len], "canrecv_cnt: %lu\r\n", canrecv_cnt);

    // 使用 CDC_Transmit_FS 发送数据
    CDC_Transmit_FS(usbBuf, len);
}



void handle_unknown(void) {
    uint8_t usbBuf[50]; // 用于存储要发送的字符串
    uint16_t len = 0;

    // 格式化字符串到 usbBuf
    len += sprintf((char *)&usbBuf[len], "Unknown command.\r\n");

    // 使用 CDC_Transmit_FS 发送数据
    CDC_Transmit_FS(usbBuf, len);
}

// 查找并执行命令
void execute_command(const char *command) {
    for (int i = 0; i < sizeof(commandTable) / sizeof(CommandEntry); i++) {
        if (strcmp(command, commandTable[i].command) == 0) {
            commandTable[i].handler();
            return;
        }
    }
    handle_unknown(); // 如果找不到命令，执行未知命令处理
}



/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

USBD_CDC_ItfTypeDef USBD_Interface_fops_FS =
{
  CDC_Init_FS,
  CDC_DeInit_FS,
  CDC_Control_FS,
  CDC_Receive_FS
};

/* Private functions ---------------------------------------------------------*/
/**
  * @brief  Initializes the CDC media low layer over the FS USB IP
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_FS(void)
{
  /* USER CODE BEGIN 3 */
  /* Set Application Buffers */
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, UserTxBufferFS, 0);
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, UserRxBufferFS);
  return (USBD_OK);
  /* USER CODE END 3 */
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_FS(void)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  Manage the CDC class requests
  * @param  cmd: Command code
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
  /* USER CODE BEGIN 5 */
  switch(cmd)
  {
    case CDC_SEND_ENCAPSULATED_COMMAND:

    break;

    case CDC_GET_ENCAPSULATED_RESPONSE:

    break;

    case CDC_SET_COMM_FEATURE:

    break;

    case CDC_GET_COMM_FEATURE:

    break;

    case CDC_CLEAR_COMM_FEATURE:

    break;

  /*******************************************************************************/
  /* Line Coding Structure                                                       */
  /*-----------------------------------------------------------------------------*/
  /* Offset | Field       | Size | Value  | Description                          */
  /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
  /* 4      | bCharFormat |   1  | Number | Stop bits                            */
  /*                                        0 - 1 Stop bit                       */
  /*                                        1 - 1.5 Stop bits                    */
  /*                                        2 - 2 Stop bits                      */
  /* 5      | bParityType |  1   | Number | Parity                               */
  /*                                        0 - None                             */
  /*                                        1 - Odd                              */
  /*                                        2 - Even                             */
  /*                                        3 - Mark                             */
  /*                                        4 - Space                            */
  /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
  /*******************************************************************************/
    case CDC_SET_LINE_CODING:
    	USBD_CDC_LineCoding.bitrate = (pbuf[3] << 24) | (pbuf[2] << 16) | (pbuf[1] << 8) | pbuf[0];
    	USBD_CDC_LineCoding.format = pbuf[4];
    	USBD_CDC_LineCoding.paritytype = pbuf[5];
    	USBD_CDC_LineCoding.datatype = pbuf[6];
    break;
 
    case CDC_GET_LINE_CODING:
    	pbuf[0] = (uint8_t)(USBD_CDC_LineCoding.bitrate);
    	pbuf[1] = (uint8_t)(USBD_CDC_LineCoding.bitrate >> 8);
    	pbuf[2] = (uint8_t)(USBD_CDC_LineCoding.bitrate >> 16);
    	pbuf[3] = (uint8_t)(USBD_CDC_LineCoding.bitrate >> 24);
    	pbuf[4] = USBD_CDC_LineCoding.format;
    	pbuf[5] = USBD_CDC_LineCoding.paritytype;
    	pbuf[6] = USBD_CDC_LineCoding.datatype;
    break;

    case CDC_SET_CONTROL_LINE_STATE:

    break;

    case CDC_SEND_BREAK:

    break;

  default:
    break;
  }

  return (USBD_OK);
  /* USER CODE END 5 */
}

/**
  * @brief  Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  *
  *         @note
  *         This function will issue a NAK packet on any OUT packet received on
  *         USB endpoint until exiting this function. If you exit this function
  *         before transfer is complete on CDC interface (ie. using DMA controller)
  *         it will result in receiving more data while previous ones are still
  *         not sent.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
  /* USER CODE BEGIN 6 */
  USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
  USBD_CDC_ReceivePacket(&hUsbDeviceFS);

  if (64 == *Len)
  {
      // 意味接受的数据大于 64 bytes，被分包了，或者是 正好64字节
      //（在之后的地方进行超时判断，一定时间内几 ms 内没新数据到来 就判断为结束）
      memcpy(&iap_recive.recivebuf[iap_recive.pack64_cnt * 64], Buf, 64);
      iap_recive.pack64_cnt++;
      iap_recive.iap_pkgtatus = PKG_NOT_DONE;
  }
  else
  {
      memcpy(&iap_recive.recivebuf[iap_recive.pack64_cnt * 64], Buf, *Len);
      iap_recive.iap_pkgtatus = PKG_COMPLETE;
  }
  iap_recive.length += *Len;
   
  return (USBD_OK);
  /* USER CODE END 6 */
}

/**
  * @brief  CDC_Transmit_FS
  *         Data to send over USB IN endpoint are sent over CDC interface
  *         through this function.
  *         @note
  *
  *
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 7 */
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
  if (hcdc->TxState != 0){
    return USBD_BUSY;
  }
  USBD_CDC_SetTxBuffer(&hUsbDeviceFS, Buf, Len);
  result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);
  /* USER CODE END 7 */
  return result;
}

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */

/**
  * @}
  */

/**
  * @}
  */
