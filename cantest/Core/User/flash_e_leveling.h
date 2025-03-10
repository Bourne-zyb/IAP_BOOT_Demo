/******************************************************************************
 * @file    flash_e_leveling.h
 * @brief   Using Circular Logging to implement flash simple wear leveling
 * @author  Jason
 * @version V1.0.0
 * @date    2025-3
 * @copyright (c) 2025, All rights reserved.
 ******************************************************************************/

#ifndef __FLASH_E_LEVEL_H
#define __FLASH_E_LEVEL_H

/* Private Includes ----------------------------------------------------------*/
#include "iap_user.h"


/**
 * 循环日志法（Circular Logging）：
 * 适用于 少量标志位，通过 顺序写入 并 回绕，均匀分布写入位置，延长 Flash 使用寿命。
 * 核心思想
 * 选择一个 Flash 块（假设 大小 2KB）。
 * 该块划分为多个 小单元（例如 16 字节/32 字节）。
 * 依次写入新数据，不直接覆盖旧数据。
 * 读数据时，找到 最后一个有效数据 作为当前标志值。
 * 当整个块写满时，擦除整个块，然后从头开始。
 */

/* Exported constants --------------------------------------------------------*/
#define FLASH_SECTOR_ADDR   IAP_STATUS_ADDRESS       /* Flash扇区起始地址，用于写入标志数据 */
#define FLASH_START_SECTOR  IAP_STATUS_START_SECTOR  /* 使用于IAP状态空间 */
#define FLASH_END_SECTOR    IAP_STATUS_END_SECTOR    /* 使用于IAP状态空间 */
#define FLASH_SECTOR_SIZE   IAP_STATUS_SIZE          /* Flash扇区大小为16 KB */ 
/* Exported types ------------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#define FLASH_ENTRY_SIZE    (sizeof(iap_status_t))   /* 每次要存储的字节数（最好要跟flash写入的单位对应 */
#define FLAG_COUNT          (FLASH_SECTOR_SIZE / FLASH_ENTRY_SIZE) /* 扇区中循环存储的次数 */ 
/* Exported variables --------------------------------------------------------*/

/* Exported function prototypes ----------------------------------------------*/

#endif /* __FLASH_E_LEVEL_H */


