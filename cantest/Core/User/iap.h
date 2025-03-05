#ifndef __IAP_H
#define __IAP_H

#include "usbd_cdc_if.h"
#include "menu.h"

/* 
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
*/

/* Start user code address:  Sector 4 */
#define APPLICATION_ADDRESS     			((uint32_t)0x08010000)

/* use for iap erase the app space*/
#define APP_START_SECTOR							FLASH_SECTOR_4
#define APP_END_SECTOR								FLASH_SECTOR_5

/* application size 192 KB */
#define USER_FLASH_SIZE               ((uint32_t)0x00030000) 			

/* Notable Flash addresses */
#define USER_FLASH_END_ADDRESS        ((uint32_t)0x0807FFFF)


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

typedef struct {
  HAL_StatusTypeDef (*TransmitFunction)(void *data, uint16_t length, uint32_t timeout);  // 发送函数指针
  HAL_StatusTypeDef (*ReceiveFunction)(uint8_t *data, uint16_t length, uint32_t timeout);   // 接收函数指针
  void (*DelayTimeMsFunction)(uint32_t delaytime);                          // 延时函数指针
	void (*funtionJumpFunction)(void);
} IAP_Interface;

void IAP_Init(void);

extern IAP_Receive_Struct iap_recive;
extern IAP_Interface iapInterface;


#endif  

