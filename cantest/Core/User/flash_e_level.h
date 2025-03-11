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
 * 核心思想：
 * 选择一个 Flash 块（假设 大小 16KB）。
 * 该块划分为多个 小单元（例如 16 字节/32 字节）。
 * 依次写入新数据，不直接覆盖旧数据。
 * 读数据时，找到 最后一个有效数据 作为当前标志值。
 * 当整个块写满时，擦除整个块，然后从头开始。
 */
/* Exported constants --------------------------------------------------------*/
#define AREA_SON_SIZE           (0x400)                  /* 2K 大小*/
#define HEADER                  (0x55AA)
#define ENDER                   (0xAA55)
#define EL_FLASH_START_ADDRESS  IAP_STATUS_ADDRESS       /* Flash扇区起始地址，用于写入标志数据 */
#define EL_FLASH_SECTOR_START   IAP_STATUS_START_SECTOR  /* 使用于IAP状态空间 */
#define EL_FLASH_SECTOR_END     IAP_STATUS_END_SECTOR    /* 使用于IAP状态空间 */
#define EL_AREA_SIZE            IAP_STATUS_SIZE          /* 均衡磨损扇区大小为 16 KB */ 

#define _ALIGN                  2                      /* world 字 对齐 跟flash写入的保持一致*/
typedef    uint32_t              map_size_t;
typedef    uint32_t              flash_address_t;

/* Exported types ------------------------------------------------------------*/
typedef enum {
    EL_FIND_SUCCESS,    // Successfully found the latest data address
    EL_NOT_FOUND       // No valid data address found
} efind_status_t;

#pragma pack(push, _ALIGN)           /* flash 按照 FLASH_TYPEPROGRAM_WORD 写入的，因此按照2字节对齐 */ 
typedef struct  
{   
    uint16_t header;
    //TODO: add save data start.
    iap_status_t iap_status;
    //TODO: add save data end.
    uint16_t ender;
}save_data_t;
#pragma pack(pop)   

/* Exported macro ------------------------------------------------------------*/
#define MAX_UNSIGNED_TYPE(type) ((type)(~(type)0))       /*  Macro to get the maximum value of an unsigned type */
#define EL_AREA_SON_UNUSE_VALUE (MAX_UNSIGNED_TYPE(map_size_t))

#define EL_AREA_SON_SUM         (EL_AREA_SIZE / (AREA_SON_SIZE))        /* 共计有多少个子区域*/

#define EL_MAP_SIZE             (sizeof(map_size_t))
#define EL_DATA_SIZE            (sizeof(save_data_t))                   /* 每次要存储的字节数（最好要跟flash写入的单位对应 */
#define EL_SON2DATA_SUM   \
    (((AREA_SON_SIZE - EL_MAP_SIZE) / EL_DATA_SIZE) - (((AREA_SON_SIZE - EL_MAP_SIZE) % EL_DATA_SIZE)))   /* 子区域中可以存储数据的总个数 */ 

#define EL_MAP_BITS_SUM         (sizeof(map_size_t) * 8)
#define EL_MAP_BIT2DATA_SUM     (EL_SON2DATA_SUM / EL_MAP_BITS_SUM)     /* 每个bit表示多少个数据已经被写入了 */

#define EL_GET_SON_START_ADDR(index)    ((flash_address_t)(EL_FLASH_START_ADDRESS + (index * AREA_SON_SIZE)))
#define EL_GET_MAP_ADDR(index)  EL_GET_SON_START_ADDR(index)
#define EL_GET_MAP(index)       (*(map_size_t*)EL_GET_MAP_ADDR(index))

#define EL_GET_DATA_ADDR(sonindex, index)    \
    ((flash_address_t)((EL_GET_SON_START_ADDR(sonindex) + EL_MAP_SIZE + (index * EL_DATA_SIZE))))
#define EL_GET_DATA(sonindex, index)    (*(save_data_t*)EL_GET_DATA_ADDR(sonindex, index))
#define EL_CHECK_DATE(data)     ((HEADER == (data).header) && (ENDER == (data).ender) ? 1 : 0)
#define EL_SON_IS_USEING(map)   ((EL_AREA_SON_UNUSE_VALUE != map) ? 1 : 0)
#define EL_SET_MAPBIT(map)   ((EL_AREA_SON_UNUSE_VALUE != map) ? 1 : 0)
/* Exported variables --------------------------------------------------------*/


/* Exported function prototypes ----------------------------------------------*/
void el_test(void);

#endif /* __FLASH_E_LEVEL_H */


