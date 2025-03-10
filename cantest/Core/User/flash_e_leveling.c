/******************************************************************************
 * @file    flash_e_leveling.c
 * @brief   Using Circular Logging to implement flash simple wear leveling
 * @author  Jason
 * @version V1.0.0
 * @date    2025-3
 * @copyright (c) 2025, All rights reserved.
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/ 
#include "flash_e_leveling.h"

/* Private typedef -----------------------------------------------------------*/ 

/* Private define ------------------------------------------------------------*/ 

/* Private macro -------------------------------------------------------------*/ 

/* Private variables ---------------------------------------------------------*/ 

/* Private function prototypes -----------------------------------------------*/ 

/* Private functions ---------------------------------------------------------*/ 


/******************************************************************************
 * @file    flash_e_leveling.c
 * @brief   使用循环日志法（Circular Logging）实现简单的Flash均衡磨损
 * @author  Jason
 * @date    2025-03-05
 ******************************************************************************/

/**
 * @brief 查找Flash扇区中最新的标志数据地址
 * @return 返回最新的标志数据地址
 */
uint32_t find_latest_flag_address() {
    uint32_t addr = FLASH_SECTOR_ADDR;
    uint8_t flagByte;

    for (int i = 0; i < FLAG_COUNT; i++) {
        flagByte = *(uint8_t*)addr;
        if (flagByte == 0xFF) {  // 找到未写入的位置
            return (i == 0) ? FLASH_SECTOR_ADDR : addr - FLASH_ENTRY_SIZE;
        }
        addr += FLASH_ENTRY_SIZE;
    }
    return FLASH_SECTOR_ADDR + FLASH_SECTOR_SIZE - FLASH_ENTRY_SIZE;
}

/**
 * @brief 擦除Flash扇区
 */
void erase_flash_block() 
{
    /* Unlock the Flash to enable the flash control register access *************/ 
    HAL_FLASH_Unlock();
  
    for(char i = FLASH_START_SECTOR; i <= FLASH_END_SECTOR; i++)
    {
      /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
         be done by word */ 
      FLASH_Erase_Sector(i, FLASH_VOLTAGE_RANGE_3);
    }
  
    /* Lock the Flash to disable the flash control register access (recommended
       to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock();
}

/**
 * @brief 向Flash中写入最新的标志数据
 * @param data 指向要写入的16字节数据的指针
 */
void write_flag(uint8_t* data) {
//    uint32_t latest_addr = find_latest_flag_address();

//    if (latest_addr + FLASH_ENTRY_SIZE >= FLASH_SECTOR_ADDR + FLASH_SECTOR_SIZE) {
//        erase_flash_block();  // 扇区已满，擦除整个扇区
//        latest_addr = FLASH_SECTOR_ADDR;
//    }

//    HAL_FLASH_Unlock();
//    HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, latest_addr, 0x5AA5); // 写入标志位置 0x00
//    for (int i = 0; i < FLAG_SIZE; i++) {
//        HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, latest_addr + 1 + i, data[i]);
//    }
//    HAL_FLASH_Lock();
}

/**
 * @brief 读取最新的标志数据
 * @param buffer 指向存储读取数据的缓冲区，应至少为FLAG_SIZE字节大小
 */
void read_latest_flag(uint8_t* buffer) {
//    uint32_t latest_addr = find_latest_flag_address();
//    for (int i = 0; i < FLAG_SIZE; i++) {
//        buffer[i] = *(uint8_t*)(latest_addr + 1 + i);
//    }
}



/************************ (C) COPYRIGHT Jason *****END OF FILE****/