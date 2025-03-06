/******************************************************************************
 * @file    iap.c
 * @brief   In-Application Programming (IAP) module implementation.
 * @author  Jason
 * @version V1.0.0
 * @date    2025-3
 * @copyright (c) 2025, All rights reserved.
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "iap.h"

/* Private typedef -----------------------------------------------------------*/
typedef void (*pFunction)(void);

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
IAP_Interface iapInterface;
IAP_Receive_Struct iap_recive;
static uint32_t JumpAddress;
static pFunction JumpToApplication;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

static void funtionJump()
{
    /* Test if user code is programmed starting from address "APPLICATION_ADDRESS" */
    if (((*(__IO uint32_t*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000)
    {
        HAL_DeInit();
        __disable_irq();  /* 禁止全局中断*/
        /* Jump to user application */
        JumpAddress = *(__IO uint32_t*) (APPLICATION_ADDRESS + 4);
        JumpToApplication = (pFunction) JumpAddress;
        /* Initialize user application's Stack Pointer */
        __set_MSP(*(__IO uint32_t*) APPLICATION_ADDRESS);
        JumpToApplication();
    }
}


/**
 * @brief USB虚拟串口阻塞式发送函数，用于通过CDC接口发送数据。
 *        如果发送缓冲区繁忙，函数会等待直到数据成功发送或超时。
 * 
 * @param buffer 指向发送数据缓冲区的指针
 * @param len 需要发送的数据长度
 * @param timeout 发送超时时间（单位：毫秒）
 * @retval HAL_StatusTypeDef 返回状态，HAL_OK表示成功，HAL_TIMEOUT表示超时
 */
static HAL_StatusTypeDef TransmitAdapter(void* buffer, uint16_t len, uint32_t timeout) 
{
    while (USBD_OK != CDC_Transmit_FS((uint8_t*)buffer, len)) 
    {
        iapInterface.DelayTimeMsFunction(10);  // 每次等待10ms
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
 * @note  如果一次接收的数据过多，函数会只返回当前所需的数据，并在下次调用时继续传输剩余的数据。
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
static HAL_StatusTypeDef ReceiveAdapter(uint8_t *data, uint16_t needlength, uint32_t timeout) 
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
                iapInterface.DelayTimeMsFunction(10);   
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
static void DelayTimeAdapter(uint32_t delaytime)
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
    iapInterface.DelayTimeMsFunction = DelayTimeAdapter;
    iapInterface.funtionJumpFunction = funtionJump;
}


/************************ (C) COPYRIGHT Jason *****END OF FILE****/