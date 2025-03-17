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
#define FLASH_E_LEVEL_DEBUG 1

/* Private macro -------------------------------------------------------------*/ 

/* Private variables ---------------------------------------------------------*/ 
/* 都是从0开始，0 表示第一个数据*/
static int32_t  areason_id, data_id;   
static char     bit_id;
/* Private function prototypes -----------------------------------------------*/ 
/* Private function prototypes -----------------------------------------------*/ 
static HAL_StatusTypeDef el_erase_flash_area(void);
static void el_write_flash_data(el_flash_address_t write_addr, void* save_data_p, uint32_t Len);
static void el_reset_map_bit(uint32_t areason_id, uint32_t data_id);
static eFIND_Status_Def el_find_latest_data_address(el_flash_address_t *address_p);
static el_flash_address_t el_get_nextwrite_address(eFIND_Status_Def status);
/* Private functions ---------------------------------------------------------*/ 
/**
 * @brief 擦除对应 Flash 区域
 */
static HAL_StatusTypeDef el_erase_flash_area() 
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
		
	return HAL_OK;
}

static void el_write_flash_data(el_flash_address_t write_addr, void* save_data_p, uint32_t Len)
{
    el_reset_map_bit(areason_id, data_id);
    FLASH_If_Write(write_addr, (uint32_t*)save_data_p, Len / FLASH_PROGRAM_SIZE); 
                                // len/4 是因为写入的是uint32_t类型的数据
}

static void el_reset_map_bit(uint32_t sonid, uint32_t dataid)
{
    el_table_t table;
    el_flash_address_t table_addr;
    char bit_id;

    if (CNT_FIRST == (dataid % EL_MAP_BIT2DATA_SUM))
    {
        bit_id = ((--dataid) / EL_MAP_BIT2DATA_SUM);
        table = EL_GET_TABLE(sonid);
        table.map &= (~(0x01 << (bit_id)));
        table.dataSize = EL_DATA_SIZE;
        table_addr = EL_GET_TABLE_ADDR(sonid);
        FLASH_If_Write(table_addr, (uint32_t *)&table, EL_TABLE_SIZE / FLASH_PROGRAM_SIZE);
    }
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
static eFIND_Status_Def el_find_latest_data_address(el_flash_address_t *address_p) 
{
    static el_savedata_t   data;
    static el_table_t      table;
    char havedata_flg = 0;
	
    /* 1. 查找可用的子区，进而确定最新区 */
    for (areason_id = EL_AREA_SON_SUM; areason_id > 0; areason_id--)
    {
        table = EL_GET_TABLE(areason_id);
        if (EL_SON_IS_USEING(table.map))
        {
            havedata_flg = 1;
        }
        if (EL_SON_IS_USEING(table.map) && (EL_CHECK_TABLE(table)))      
        {   // still have area can use, so find in this son area. 
            /* 2. map中查找最新的地方 */
            for (bit_id = EL_MAP_BITS_SUM ; bit_id > 0; bit_id--)
            {  
                if ((table.map & (1 << EL_ID_2_ADDRID(bit_id))) == 0) 
                {   // The i-th bit of map is 0, this area is useing or all used.
                    if(bit_id == EL_MAP_BITS_SUM)
                    {   // 这个son空间中最后的一个部分
                        data_id = EL_SON2DATA_SUM;
                    }			
                    else
                    {
                        data_id = ((EL_ID_2_ADDRID(bit_id) + 1) * EL_MAP_BIT2DATA_SUM);  
                    }
                    
                    for (; (data_id > 0); data_id--)
                    {
                        data = EL_GET_DATA(areason_id, data_id);
                        if (EL_CHECK_DATE(data))
                        {
                            *address_p = EL_GET_DATA_ADDR(areason_id, data_id);
                            return EL_FIND_SUCCESS;
                        }
                    }   
                }
            } 
        }
    }
    if (1 == havedata_flg)
    {
        return EL_FIND_ERR;
    }
    else
    {
        return EL_NOT_FOUND;
    }
}

static el_flash_address_t el_get_nextwrite_address(eFIND_Status_Def status)
{
    switch (status)
    {
        case EL_FIND_SUCCESS:
            if ((data_id == EL_SON2DATA_SUM) && \
                (areason_id == EL_AREA_SON_SUM))
            {   /* 整个空间都满了，重新写 */
                el_erase_flash_area();
                areason_id = CNT_FIRST;
                bit_id = CNT_FIRST;
                data_id = CNT_FIRST;
                return EL_GET_DATA_ADDR(areason_id, data_id);
            }
            else
            {
                if (data_id == EL_SON2DATA_SUM)
                {   /* 找下一个 son 空间，这个写满了 */
                    data_id = CNT_FIRST;
                    return EL_GET_DATA_ADDR(++areason_id, data_id);
                }
                else
                {   /* 在这个son空间找下一个地址 */
                    return EL_GET_DATA_ADDR(areason_id, ++data_id);
                }
            }
            break;
        case EL_FIND_ERR:
            el_erase_flash_area();
        case EL_NOT_FOUND:
            areason_id = CNT_FIRST;
            bit_id = CNT_FIRST;
            data_id = CNT_FIRST;
            return EL_GET_DATA_ADDR(areason_id, data_id);
            break;
        default:
            return NULL;
            break;
    }
}

/**
 * @brief 将数据写入Flash存储器
 * 该函数首先查找最新的数据地址，然后计算下一个写入地址，并将数据写入Flash存储器。
 * @param save_data_p 指向要保存的数据的指针
 * @return 无
 */
void el_flash_write(el_savedata_t* save_date_p) 
{
    el_flash_address_t next_write_addr;
    el_flash_address_t addr;
  
    next_write_addr = el_get_nextwrite_address(el_find_latest_data_address(&addr));
    el_write_flash_data(next_write_addr, (uint32_t*)save_date_p, EL_DATA_SIZE);
}

/**
 * @brief 读取最新的数据
 * @param buffer_p 用于存储读取数据的指针，指向 el_savedata_t 类型的变量
 * @return efind_status_t 返回查找状态，EL_FIND_SUCCESS 表示成功，其他值表示失败
 */
eFIND_Status_Def el_flash_read(el_savedata_t* recv_date_p) 
{
    el_flash_address_t latest_addr;
    eFIND_Status_Def status;

    status = el_find_latest_data_address(&latest_addr);    
    if(EL_FIND_SUCCESS == status)
    {
        *recv_date_p =  *(el_savedata_t*)latest_addr;
        // memcpy(recv_date_p, latest_addr, sizeof(el_savedata_t)); // 使用 memcpy 复制数据
    }
    if (EL_FIND_ERR == status)
    {
        el_erase_flash_area();
    }
    if(EL_NOT_FOUND == status)
    {
        //*recv_date_p = (el_save_data_t*)0;
    }
    return status;
}

void el_test(void)
{
#if FLASH_E_LEVEL_DEBUG

    el_savedata_t save_data;
    el_savedata_t read_data;
    eFIND_Status_Def status;
    uint16_t version = 0;

    el_erase_flash_area();
    while(1)
    {
    // 测试写入数据并读取
        save_data.iap_msg.status = IAP_NO_APP;
        save_data.iap_msg.version = version++;
        save_data.header = HEADER;
        save_data.ender = ENDER;
                
                
        el_flash_write(&save_data);
    
        // 读取数据
        status = el_flash_read(&read_data);
        if (status == EL_FIND_SUCCESS) {
           // printf("Latest data found with status: %d\n", read_data.iap_msg.status);
        } else {
           // printf("Error reading latest data after writing IAP_NO_APP\n");
            return;
        }

        // 测试写入数据并读取
        save_data.iap_msg.status = IAP_DOWNING_BIN;
        save_data.iap_msg.version = version++;
        el_flash_write(&save_data);;
    

        // 读取数据
        status = el_flash_read(&read_data);
        if (status == EL_FIND_SUCCESS) {
           // printf("Latest data found with status: %d\n", read_data.iap_msg.status);
        } else {
           // printf("Error reading latest data after writing IAP_DOWNING_BIN\n");
            return;
        }

        // 测试写入数据并读取
        save_data.iap_msg.status = IAP_APP_DONE;
        save_data.iap_msg.version = version++;
        el_flash_write(&save_data);


        // 读取数据
        status = el_flash_read(&read_data);
        if (status == EL_FIND_SUCCESS) {
           // printf("Latest data found with status: %d\n", read_data.iap_msg.status);
        } else {
           // printf("Error reading latest data after writing IAP_APP_DONE\n");
            return;
        }
    }
#endif
}


/************************ (C) COPYRIGHT Jason *****END OF FILE****/
