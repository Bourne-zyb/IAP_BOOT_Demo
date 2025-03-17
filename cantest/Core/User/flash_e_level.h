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
 * 
 * 
 *  3. map 中某个bit是0表示 正在使用 或者 全部使用完
 *  都是从0开始，0 表示第一个数据
 * TODO: 添加整个flash的验证计算对不对，如果被窜给就删除全部数据，重新写入默认值
 * TODO: 添加对数据不足32的支持，不然bit不满
 */
/* Exported constants --------------------------------------------------------*/
#define CNT_FIRST               (1)    
#define AREA_SON_SIZE           (0x400)                  /* 2K 大小*/

#define EL_FLASH_START_ADDRESS  IAP_STATUS_ADDRESS       /* Flash扇区起始地址，用于写入标志数据 */
#define EL_FLASH_SECTOR_START   IAP_STATUS_START_SECTOR  /* 使用于IAP状态空间 */
#define EL_FLASH_SECTOR_END     IAP_STATUS_END_SECTOR    /* 使用于IAP状态空间 */
#define EL_AREA_SIZE            IAP_STATUS_SIZE          /* 均衡磨损扇区大小为 16 KB */ 

typedef    uint32_t             el_map_size_t;
typedef    uint32_t             el_flash_address_t;
typedef    save_data_t          el_savedata_t;
/* Exported types ------------------------------------------------------------*/
#pragma pack(push, FLASH_PROGRAM_SIZE)           /* flash 按照 FLASH_TYPEPROGRAM_WORD 写入的，因此按照4字节对齐 */ 
typedef struct  
{   
    el_map_size_t map;
    uint16_t dataSize;
}el_table_t;
#pragma pack(pop)  

/* Exported macro ------------------------------------------------------------*/
#define MAX_UNSIGNED_TYPE(type)        ((type)(~(type)0))       /*  Macro to get the maximum value of an unsigned type */
#define EL_AREA_SON_UNUSE_VALUE        (MAX_UNSIGNED_TYPE(el_map_size_t))

#define EL_AREA_SON_SUM                (EL_AREA_SIZE / (AREA_SON_SIZE))        /* 共计有多少个子区域 */
#define EL_TABLE_SIZE                  (sizeof(el_table_t))
#define EL_DATA_SIZE                   (sizeof(el_savedata_t))                   /* 每次要存储的字节数（最好要跟flash写入的单位对应） */
#define EL_SON2DATA_SUM                ((AREA_SON_SIZE - EL_TABLE_SIZE) / EL_DATA_SIZE)   /* 子区域中可以存储数据的总个数 */
#define EL_MAP_BITS_SUM                (sizeof(el_map_size_t) * 8)

#define EL_MAP_BIT2DATA_SUM            \
    ((0 != (EL_SON2DATA_SUM % EL_MAP_BITS_SUM)) ? \
     ((EL_SON2DATA_SUM / EL_MAP_BITS_SUM) + 1) : \
     (EL_SON2DATA_SUM / EL_MAP_BITS_SUM))
//#define EL_LAST_BIT2DATA_SUM           \
//    ((0 != (EL_SON2DATA_SUM % EL_MAP_BITS_SUM)) ? \
    (EL_SON2DATA_SUM % EL_MAP_BITS_SUM) :\
    EL_MAP_BIT2DATA_SUM)

#define EL_GET_SON_START_ADDR(sonid)   \
    ((el_flash_address_t)(EL_FLASH_START_ADDRESS + ((sonid - 1) * AREA_SON_SIZE)))
#define EL_GET_TABLE_ADDR(sonid)       EL_GET_SON_START_ADDR(sonid)
#define EL_GET_TABLE(sonid)            (*(el_table_t*)EL_GET_TABLE_ADDR(sonid))

#define EL_GET_DATA_ADDR(sonid, dataid) \
    ((el_flash_address_t)((EL_GET_SON_START_ADDR(sonid) + EL_TABLE_SIZE + ((dataid - 1) * EL_DATA_SIZE))))
#define EL_GET_DATA(sonid, dataid)   (*(el_savedata_t*)EL_GET_DATA_ADDR(sonid, dataid))

#define EL_ID_2_ADDRID(id2addrIDX)      (id2addrIDX - 1)		
#define EL_CHECK_DATE(data)             ((HEADER == (data).header) && (ENDER == (data).ender) ? 1 : 0)
#define EL_CHECK_TABLE(table)           ((EL_DATA_SIZE == table.dataSize) ? 1 : 0)
#define EL_SON_IS_USEING(map)           ((EL_AREA_SON_UNUSE_VALUE != map) ? 1 : 0)

/* Exported variables --------------------------------------------------------*/


/* Exported function prototypes ----------------------------------------------*/
void el_flash_write(el_savedata_t* save_date_p);
eFIND_Status_Def el_flash_read(el_savedata_t* recv_date_p);
void el_test(void);

#endif /* __FLASH_E_LEVEL_H */


