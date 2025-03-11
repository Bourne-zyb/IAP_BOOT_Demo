/******************************************************************************
 * @file    flash_e_leveling.c
 * @brief   Using Circular Logging to implement flash simple wear leveling
 * @author  Jason
 * @version V1.0.0
 * @date    2025-3
 * @copyright (c) 2025, All rights reserved.
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/ 
#include "flash_e_level.h"
#include "flash_if.h"
/* Private typedef -----------------------------------------------------------*/ 

/* Private define ------------------------------------------------------------*/ 
#define FLASH_E_LEVEL_DEBUG 0
/* Private macro -------------------------------------------------------------*/ 

/* Private variables ---------------------------------------------------------*/ 
static int32_t  areason_index, data_index;
static char     bit_index;
/* Private function prototypes -----------------------------------------------*/ 

/* Private functions ---------------------------------------------------------*/ 

/**
 * @brief 擦除对应 Flash 区域
 */
static HAL_StatusTypeDef el_erase_area() 
{
    /* Unlock the Flash to enable the flash control register access *************/ 
    HAL_FLASH_Unlock();
  
    if (HAL_OK != FLASH_WaitForLastOperation(FlASH_WAIT_TIMEMS))
    {
        return HAL_TIMEOUT;
    }
    for (char i = IAP_STATUS_START_SECTOR; i <= IAP_STATUS_END_SECTOR; i++)
    {
        /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
        be done by word */ 

        FLASH_Erase_Sector(i, FLASH_VOLTAGE_RANGE_3);
        if (HAL_OK != FLASH_WaitForLastOperation(FlASH_WAIT_TIMEMS))
        {
            return HAL_TIMEOUT;
        }
    }
    /* Lock the Flash to disable the flash control register access (recommended
       to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock();
}

/**
 * @brief 查找Flash扇区中最新的标志数据地址
 * @note  1. 存放的话，正序存放，查找的话，要倒着查，这样才可以方便找到最新的地址，
 *           不然查到一个使用过的地址，还要进而查看
             下一个是不是也使用过了进而才能确定最新的map使用地址 
          2. 整个区写满了之后，并不擦立刻，而是在写新的数据的时候才擦除
          3. map 中某个bit是0表示 正在使用 或者 全部使用完
 * @return 返回最新的标志数据地址
 */
static efind_status_t find_latest_data_address(flash_address_t *address_p) 
{
    map_size_t      map;
    save_data_t     data;
    map_size_t      usemap;

    /* 1. 查找可用的子区，进而确定最新区 */
    for (areason_index = (EL_AREA_SON_SUM - 1); areason_index >= 0; areason_index--)
    { 
        map = EL_GET_MAP(areason_index);
        if (EL_SON_IS_USEING(map))      
        {   // still have area can use, so find in this son area. 
            /* 2. map中查找最新的地方 */
            for (bit_index = (EL_MAP_BITS_SUM - 1); bit_index >= 0; bit_index--)
            {  
                if ((map & (1 << bit_index)) == 0) 
                {   // The i-th bit of map is 0, this area is useing or all used.
                    for (data_index = ((bit_index + 1) * EL_MAP_BIT2DATA_SUM); \
                        data_index >= (bit_index * EL_MAP_BIT2DATA_SUM);  \
                        data_index--)
                    {
                        data = EL_GET_DATA(areason_index, data_index);
                        if (EL_CHECK_DATE(data))
                        {
                            *address_p = EL_GET_DATA_ADDR(areason_index, data_index);
                            return EL_FIND_SUCCESS;
                        }
                    }   
                }
            } 
        }
    } 
    return EL_NOT_FOUND;
}



/**
 * @brief 读取最新的数据
 * @param buffer_p 用于存储读取数据的指针，指向 save_data_t 类型的变量
 * @return efind_status_t 返回查找状态，EL_FIND_SUCCESS 表示成功，其他值表示失败
 */
efind_status_t el_read_data(save_data_t* buffer_p) 
{
    flash_address_t latest_addr;
    efind_status_t status;

    status = find_latest_data_address(&latest_addr);    
    if(EL_FIND_SUCCESS == status)
    {
        *buffer_p =  *(save_data_t*)latest_addr;
        // memcpy(buffer_p, latest_addr, sizeof(save_data_t)); // 使用 memcpy 复制数据
    }
    return status;
}

static void reset_map_bit(char bit_id)
{
    map_size_t map;
    flash_address_t map_addr;

    map = EL_GET_MAP(areason_index);
    map &= (~(0x01 << (bit_id)));
    map_addr = EL_GET_MAP_ADDR(areason_index);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, map_addr, map); 
}

static flash_address_t get_nextwrite_address(flash_address_t latest_addr)
{
    flash_address_t next_write_addr;


    if((data_index == (EL_SON2DATA_SUM - 1)) &&  \
        (areason_index == (EL_AREA_SON_SUM - 1)))
    {   /* 整个空间都满了，重新写 */
        el_erase_area();
        areason_index = 0;
        bit_index = 0;
        data_index = 0;
        reset_map_bit(bit_index);
        return EL_GET_DATA_ADDR(areason_index, data_index);
    }else{
        if(data_index == (EL_SON2DATA_SUM - 1))
        {/* 找下一个 son 空间，这个写满了 */

            return EL_GET_DATA_ADDR(++areason_index, 0);
        }
        else
        {/* 在这个son空间找下一个地址 */
            ++data_index;
            if(1 == (data_index % EL_MAP_BIT2DATA_SUM))
            {
                reset_map_bit(++bit_index);
            }     
            return EL_GET_DATA_ADDR(areason_index, data_index);
        }
    }
}

/**
 * @brief 向 Flash 中写入新的 IAP 状态数据
 * @param data 指向要写入的 iap_status_t 结构体数据
 */
void el_write_data(save_data_t *save_data_p) 
{
    flash_address_t latest_addr, next_write_addr;
    efind_status_t status;

    status = find_latest_data_address(&latest_addr);    
    if (EL_FIND_SUCCESS == status)
    {
        next_write_addr = get_nextwrite_address(latest_addr);
        FLASH_If_Write(next_write_addr, (uint32_t*)save_data_p, EL_DATA_SIZE);
    }
}

void el_test(void)
{
#if FLASH_E_LEVEL_DEBUG
    uint32_t writedate = 0xffffffff;

    erase_flash_block();

    HAL_FLASH_Unlock();
    
    for (size_t i = 0; i < 32; i++)
    {
        writedate &= (~(0x01 << i));
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_SECTOR_ADDR, writedate);  
    }
    HAL_FLASH_Lock();
#endif
}


/************************ (C) COPYRIGHT Jason *****END OF FILE****/