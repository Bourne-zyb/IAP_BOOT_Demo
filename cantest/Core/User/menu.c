/**
  ******************************************************************************
  * @file    IAP_Main/Src/menu.c 
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    8-April-2015
  * @brief   This file provides the software which contains the main menu routine.
  *          The main menu gives the options of:
  *             - downloading a new binary file, 
  *             - uploading internal flash memory,
  *             - executing the binary file already loaded 
  *             - configuring the write protection of the Flash sectors where the 
  *               user loads his binary file.
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

/** @addtogroup STM32F1xx_IAP
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "common.h"
#include "flash_if.h"
#include "menu.h"
#include "ymodem.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
pFunction JumpToApplication;
uint32_t JumpAddress;
uint32_t FlashProtection = 0;
uint8_t aFileName[FILE_NAME_LENGTH];

/* Private function prototypes -----------------------------------------------*/
void SerialDownload(void);
void SerialUpload(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Download a file via serial port
  * @param  None
  * @retval None
  */
void SerialDownload(void)
{
  uint8_t number[11] = {0};
  uint32_t size = 0;
  COM_StatusTypeDef result;

  Serial_PutString("Waiting for the file to be sent ... (press 'a' to abort)\n\r");
  result = Ymodem_Receive( &size );
  if (result == COM_OK)
  {
		iapInterface.DelayTimeMs(100);
    Serial_PutString("\n\n\r Programming Completed Successfully!\n\r--------------------------------\r\n Name: ");
    Serial_PutString(aFileName);
    Int2Str(number, size);
    Serial_PutString("\n\r Size: ");
    Serial_PutString(number);
    Serial_PutString(" Bytes\r\n");
    Serial_PutString("-------------------\n");
  }
  else if (result == COM_LIMIT)
  {
    Serial_PutString("\n\n\rThe image size is higher than the allowed space memory!\n\r");
  }
  else if (result == COM_DATA)
  {
    Serial_PutString("\n\n\rVerification failed!\n\r");
  }
  else if (result == COM_ABORT)
  {
    Serial_PutString("\r\n\nAborted by user.\n\r");
  }
  else
  {
    Serial_PutString("\n\rFailed to receive the file!\n\r");
  }
}

/**
  * @brief  Upload a file via serial port.
  * @param  None
  * @retval None
  */
void SerialUpload(void)
{
  uint8_t status = 0;

  Serial_PutString("\n\n\rSelect Receive File\n\r");
	
	iapInterface.ReceiveFunction(&status, 1, RX_TIMEOUT);

  if ( status == CRC16)
  {
    /* Transmit the flash image through ymodem protocol */
    status = Ymodem_Transmit((uint8_t*)APPLICATION_ADDRESS, (const uint8_t*)"UploadedFlashImage.bin", USER_FLASH_SIZE);

    if (status != 0)
    {
      Serial_PutString("\n\rError Occurred while Transmitting File\n\r");
    }
    else
    {
      Serial_PutString("\n\rFile uploaded successfully \n\r");
    }
  }
}

/**
  * @brief  Display the Main Menu on HyperTerminal
  * @param  None
  * @retval None
  */
void Main_Menu(void)
{
  uint8_t key = 0, key2;

  Serial_PutString("\r\n======================================================================");
  Serial_PutString("\r\n=              (C) COPYRIGHT 2015 STMicroelectronics                 =");
  Serial_PutString("\r\n=                                                                    =");
  Serial_PutString("\r\n=  STM32F2xx In-Application Programming Application  (Version 1.0.0) =");
  Serial_PutString("\r\n=                                                                    =");
  Serial_PutString("\r\n=                                   By MCD Application Team          =");
  Serial_PutString("\r\n======================================================================");
  Serial_PutString("\r\n\r\n");

  /* Test if any sector of Flash memory where user application will be loaded is write protected */
  FlashProtection = FLASH_If_GetWriteProtectionStatus();

  while (1)
  {

    Serial_PutString("\r\n=================== Main Menu ============================\r\n\n");
    Serial_PutString("  Download image to the internal Flash ----------------- 1\r\n\n");
    Serial_PutString("  Upload image from the internal Flash ----------------- 2\r\n\n");
    Serial_PutString("  Execute the loaded application ----------------------- 3\r\n\n");


//    if(FlashProtection != FLASHIF_PROTECTION_NONE)
//    {
//      Serial_PutString("  Disable the write protection ------------------------- 4\r\n\n");
//    }
//    else
//    {
//      Serial_PutString("  Enable the write protection -------------------------- 4\r\n\n");
//    }
    Serial_PutString("==========================================================\r\n\n");

    /* Clean the input path */
    //__HAL_UART_FLUSH_DRREGISTER(&UartHandle);
	
    /* Receive key */
    iapInterface.ReceiveFunction( &key, 1, RX_TIMEOUT);
		

    switch (key)
    {
    case '1' :
      /* Download user application in the Flash */
      SerialDownload();
      break;
    case '2' :
      /* Upload user application from the Flash */
      SerialUpload();
      break;
    case '3' :
      Serial_PutString("Start program execution......\r\n\n");
			HAL_DeInit();
			__disable_irq();  /* 禁止全局中断*/
      /* execute the new program */
      JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
      /* Jump to user application */
      JumpToApplication = (pFunction) JumpAddress;
      /* Initialize user application's Stack Pointer */
      __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
      JumpToApplication();
      break;
    case '4' :
      if (FlashProtection != FLASHIF_PROTECTION_NONE)
      {
        /* Disable the write protection */
        if (FLASH_If_WriteProtectionConfig(FLASHIF_WRP_DISABLE) == FLASHIF_OK)
        {
          Serial_PutString("Write Protection disabled...\r\n");
          Serial_PutString("System will now restart...\r\n");
          /* Launch the option byte loading */
          HAL_FLASH_OB_Launch();
        }
        else
        {
          Serial_PutString("Error: Flash write un-protection failed...\r\n");
        }
      }
      else
      {
        if (FLASH_If_WriteProtectionConfig(FLASHIF_WRP_ENABLE) == FLASHIF_OK)
        {
          Serial_PutString("Write Protection enabled...\r\n");
          Serial_PutString("System will now restart...\r\n");
          /* Launch the option byte loading */
          HAL_FLASH_OB_Launch();
        }
        else
        {
          Serial_PutString("Error: Flash write protection failed...\r\n");
        }
      }
      break;
	default:
	Serial_PutString("Invalid Number ! ==> The number should be either 1, 2, 3\r");
	break;
    }
  }
}


/************************ (C) IAP Init ******************************************/

#include "usbd_def.h"


IAP_Interface iapInterface;
IAP_Receive_Struct iap_recive;


/**
 * @brief USB虚拟串口阻塞式发送函数，用于通过CDC接口发送数据。
 *        如果发送缓冲区繁忙，函数会等待直到数据成功发送或超时。
 * 
 * @param buffer 指向发送数据缓冲区的指针
 * @param len 需要发送的数据长度
 * @param timeout 发送超时时间（单位：毫秒）
 * @retval HAL_StatusTypeDef 返回状态，HAL_OK表示成功，HAL_TIMEOUT表示超时
 */
HAL_StatusTypeDef TransmitAdapter(void* buffer, uint16_t len, uint32_t timeout) 
{
    while (USBD_OK != CDC_Transmit_FS((uint8_t*)buffer, len)) 
    {
        iapInterface.DelayTimeMs(10);  // 每次等待10ms
        timeout -= 10;
        if (timeout <= 10)  // 超时
        {
            return HAL_TIMEOUT;  
        }
    }
    return HAL_OK;
}

/**
 * @brief 通过USB虚拟串口进行阻塞式数据接收。
 * 
 * 该函数用于从USB虚拟串口接收数据。如果未收到所需的字节数，函数会一直等待直到超时。
 * 如果一次接收的数据过多，函数会只返回当前所需的数据，并在下次调用时继续传输剩余的数据。
 * 在这种二次获取数据的情况下，如果剩余数据不足，函数会只传输当前剩余的字节数，并返回错误状态。
 * 
 * @param data 指向接收数据缓冲区的指针，用于存储接收到的数据。
 * @param needlength 需要接收的数据长度（单位：字节）。
 * @param timeout 接收超时时间（单位：毫秒）。
 * 
 * @retval HAL_StatusTypeDef 返回状态：
 *         - HAL_OK: 接收成功。
 *         - HAL_TIMEOUT: 接收超时。
 *         - HAL_ERROR: 接收过程中发生错误。
 * （这里的错误返回一定要按照 HAL_StatusTypeDef 里面带来写，因为 Ymodem 是stm32官方的，
 *  官方的协议栈就是按照这样的返回值做处理的，不然会出错！！）
 */
HAL_StatusTypeDef ReceiveAdapter(uint8_t *data, uint16_t needlength, uint32_t timeout) 
{ 
    uint16_t recive_cnt = 0, remainingLength;
    HAL_StatusTypeDef status = HAL_OK;  

    switch (iap_recive.iap_pkgtatus) 
    {
        case PKG_NOT_DONE:
            // 当有数据后，等到10ms内没有新数据更新才认为是一包结束
            while ((0 == iap_recive.length) || (recive_cnt < iap_recive.length)) 
            {
                recive_cnt = iap_recive.length; 
                iapInterface.DelayTimeMs(10);   
                timeout -= 10;
                if (timeout <= 10)  
                {
                    return  HAL_TIMEOUT;  
                }
            }
            iap_recive.iap_pkgtatus = PKG_COMPLETE;
        case PKG_COMPLETE:
            iap_recive.handle_cnt = 0;
            iap_recive.iap_pkgtatus = PKG_HANDLE_ING;
        case PKG_HANDLE_ING:
            remainingLength = iap_recive.length - iap_recive.handle_cnt;  
            if (needlength <= remainingLength)
            {
                memcpy(data, &iap_recive.recivebuf[iap_recive.handle_cnt], needlength);
                iap_recive.handle_cnt += needlength;
            }
            else
            {
                memcpy(data, &iap_recive.recivebuf[iap_recive.handle_cnt], remainingLength);
                iap_recive.handle_cnt += remainingLength;
                status = HAL_TIMEOUT;  
            }
            // 如果全部处理完，就重新开始接受
            if (iap_recive.length == iap_recive.handle_cnt)
            {
                memset(&iap_recive, 0, sizeof(IAP_Receive_Struct)); // 将结构体的所有字节设置为零
            }
            break;

        default:
            status = HAL_ERROR;  // HAL_ERROR
            break;
    }
    return status; 
}



/**
  * @brief  用户自动补充 延时函数 单位 ms
  * @param  delaytime 延时的毫秒数。
  * @retval None
  */
void DelayTimeAdapter(uint16_t delaytime)
{
    for (uint32_t i = 0; i < delaytime; i++) {
        for (volatile uint32_t j = 0; j < 12000; j++) {
            // 空循环，消耗时间
        }
    }
}

/**
  * @brief  Initialize the IAP: Configure communication
  * @param  None
  * @retval None
  */
void IAP_Init(void)
{
	iapInterface.TransmitFunction = TransmitAdapter;
  iapInterface.ReceiveFunction = ReceiveAdapter;
  iapInterface.DelayTimeMs = DelayTimeAdapter;
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
