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


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "common.h"
#include "flash_if.h"
#include "menu.h"
#include "ymodem.h"
#include "iap_user.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

uint32_t FlashProtection = 0;
uint8_t aFileName[FILE_NAME_LENGTH];

/* Private function prototypes -----------------------------------------------*/
static void SerialDownload(void);
static void SerialUpload(void);

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Download a file via serial port
  * @param  None
  * @retval None
  */
static void SerialDownload(void)
{
  uint8_t number[11] = {0};
  uint32_t size = 0;
  COM_StatusTypeDef result;

  Serial_PutString("Waiting for the file to be sent ... (press 'a' to abort)\n\r");
  result = Ymodem_Receive( &size );
  if (result == COM_OK)
  {
		iapInterface.DelayTimeMsFunction(100);
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
 static void SerialUpload(void)
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

int fputc(int ch, FILE *f)
{
	while(CDC_Transmit_FS((uint8_t *)&ch, 1) == USBD_BUSY);
	return ch;
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
  Serial_PutString("\r\n=              (C) COPYRIGHT 2025 Jason Bourne                       =");
  Serial_PutString("\r\n=                                                                    =");
  Serial_PutString("\r\n=  STM32F2xx In-Application Programming Application  (Version 1.0.0) =");
  Serial_PutString("\r\n=                                                                    =");
  Serial_PutString("\r\n=                                                     By Jason       =");
  Serial_PutString("\r\n======================================================================");
  Serial_PutString("\r\n\r\n");

  /* Test if any sector of Flash memory where user application will be loaded is write protected */
  FlashProtection = FLASH_If_GetWriteProtectionStatus();

  while (1)
  {
    Serial_PutString("\r\n==========================================================\r\n\n");
    Serial_PutString(" * Please press '1' to upgrade new app within 5 seconds,   \r\n\n");
    Serial_PutString(" * or it will run the old app.                           \r\n\n");
    Serial_PutString("=================== Main Menu ============================\r\n\n");
    Serial_PutString("  Download image to the internal Flash ----------------- 1\r\n\n");
  //  Serial_PutString("  Upload image from the internal Flash ----------------- 2\r\n\n");
    Serial_PutString("  Execute the loaded application now---------------------3\r\n\n");

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
    key = '3';        
    iapInterface.ReceiveFunction( &key, 1, BL_TIMEOUT);
    switch (key)
    {
    case '1' :
      /* Download user application in the Flash */
      SerialDownload();
      break;
    // case '2' :
    //   /* Upload user application from the Flash */
    //   SerialUpload();
    //   break;
    case '3' :
      Serial_PutString("Start program execution......\r\n\n");
			iapInterface.funtionJumpFunction();
       break;
    // case '4' :
    //   if (FlashProtection != FLASHIF_PROTECTION_NONE)
    //   {
    //     /* Disable the write protection */
    //     if (FLASH_If_WriteProtectionConfig(FLASHIF_WRP_DISABLE) == FLASHIF_OK)
    //     {
    //       Serial_PutString("Write Protection disabled...\r\n");
    //       Serial_PutString("System will now restart...\r\n");
    //       /* Launch the option byte loading */
    //       HAL_FLASH_OB_Launch();
    //     }
    //     else
    //     {
    //       Serial_PutString("Error: Flash write un-protection failed...\r\n");
    //     }
    //   }
    //   else
    //   {
    //     if (FLASH_If_WriteProtectionConfig(FLASHIF_WRP_ENABLE) == FLASHIF_OK)
    //     {
    //       Serial_PutString("Write Protection enabled...\r\n");
    //       Serial_PutString("System will now restart...\r\n");
    //       /* Launch the option byte loading */
    //       HAL_FLASH_OB_Launch();
    //     }
    //     else
    //     {
    //       Serial_PutString("Error: Flash write protection failed...\r\n");
    //     }
    //   }
    //   break;
	default:
	Serial_PutString("Invalid Number ! ==> The number should be either 1, 3\r");
	break;
    }
  }
}

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
