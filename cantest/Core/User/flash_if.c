/**
  ******************************************************************************
  * @file    IAP_Main/Src/flash_if.c 
  * @author  MCD Application Team
  * @version 1.0.0
  * @date    8-April-2015
  * @brief   This file provides all the memory related operation functions.
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
#include "flash_if.h"
#include "iap_user.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define DEBUG_FLASH 1
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Unlocks Flash for write access
  * @param  None
  * @retval None
  */
void FLASH_If_Init(void)
{
#if DEBUG_FLASH
  /* Unlock the Program memory */
  HAL_FLASH_Unlock();

  /* Clear all FLASH flags */
  __HAL_FLASH_CLEAR_FLAG(	FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
													FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
  /* Unlock the Program memory */
  HAL_FLASH_Lock();
#endif
}

/**
 * @brief  Erases the application space in the FLASH memory.
 * @return HAL_StatusTypeDef
 *         - HAL_OK: if the erase operation is successful.
 *         - HAL_TIMEOUT: if any FLASH operation times out.
 */
HAL_StatusTypeDef FLASH_If_Erase_App_Space(void)
{
#if DEBUG_FLASH
  /* Unlock the Flash to enable the flash control register access *************/ 
  HAL_FLASH_Unlock();

	if(HAL_OK != FLASH_WaitForLastOperation(FlASH_WAIT_TIMEMS))
	{
		return HAL_TIMEOUT;
	}
  for(char i = APP_START_SECTOR; i <= APP_END_SECTOR; i++)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */ 

		FLASH_Erase_Sector(i, FLASH_VOLTAGE_RANGE_3);
		if(HAL_OK != FLASH_WaitForLastOperation(FlASH_WAIT_TIMEMS))
		{
			return HAL_TIMEOUT;
		}
  }
  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();
	return HAL_OK;
#endif
}




/* Public functions ---------------------------------------------------------*/
/**
  * @brief  This function writes a data buffer in flash (data are 32-bit aligned).
  * @note   After writing data buffer, the flash content is checked.
  * @param  destination: start address for target location
  * @param  p_source: pointer on buffer with data to write
  * @param  length: length of data buffer (unit is 32-bit word)
  * @retval uint32_t 0: Data successfully written to Flash memory
  *         1: Error occurred while writing data in Flash memory
  *         2: Written Data in flash memory is different from expected one
  */
uint32_t FLASH_If_Write(uint32_t destination, uint32_t *p_source, uint32_t length)
{
  uint32_t i = 0;
#if DEBUG_FLASH
  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();

  for (i = 0; (i < length) && (destination <= (USER_FLASH_END_ADDRESS-4)); i++)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */ 
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, destination, *(uint32_t*)(p_source+i)) == HAL_OK)      
    {
     /* Check the written value */
      if (*(uint32_t*)destination != *(uint32_t*)(p_source+i))
      {
        /* Flash content doesn't match SRAM content */
        return(FLASHIF_WRITINGCTRL_ERROR);
      }
      /* Increment FLASH destination address */
      destination += 4;
    }
    else
    {
      /* Error occurred while writing data in Flash memory */
      return (FLASHIF_WRITING_ERROR);
    }
  }

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();
#endif
  return (FLASHIF_OK);
}

/**
  * @brief  Returns the write protection status of application flash area.
  * @param  None
  * @retval If a sector in application area is write-protected returned value is a combinaison
            of the possible values : FLASHIF_PROTECTION_WRPENABLED, FLASHIF_PROTECTION_PCROPENABLED, ...
  *         If no sector is write-protected FLASHIF_PROTECTION_NONE is returned.
  */
uint32_t FLASH_If_GetWriteProtectionStatus(void)
{
  // uint32_t ProtectedPAGE = FLASHIF_PROTECTION_NONE;
  // FLASH_OBProgramInitTypeDef OptionsBytesStruct;

  // /* Unlock the Flash to enable the flash control register access *************/
  // HAL_FLASH_Unlock();

  // /* Check if there are write protected sectors inside the user flash area ****/
  // HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);

  // /* Lock the Flash to disable the flash control register access (recommended
  //    to protect the FLASH memory against possible unwanted operation) *********/
  // HAL_FLASH_Lock();

  // /* Get pages already write protected ****************************************/
  // ProtectedPAGE = ~(OptionsBytesStruct.WRPSector) & FLASH_PAGE_TO_BE_PROTECTED;

  // /* Check if desired pages are already write protected ***********************/
  // if(ProtectedPAGE != 0)
  // {
  //   /* Some sectors inside the user flash area are write protected */
  //   return FLASHIF_PROTECTION_WRPENABLED;
  // }
  // else
  // { 
  //   /* No write protected sectors inside the user flash area */
  //   return FLASHIF_PROTECTION_NONE;
  // }
  return FLASHIF_PROTECTION_NONE;
}

/**
  * @brief  Configure the write protection status of user flash area.
  * @param  protectionstate : FLASHIF_WRP_DISABLE or FLASHIF_WRP_ENABLE the protection
  * @retval uint32_t FLASHIF_OK if change is applied.
  */
uint32_t FLASH_If_WriteProtectionConfig(uint32_t protectionstate)
{
//  uint32_t ProtectedPAGE = 0x0;
//  FLASH_OBProgramInitTypeDef config_new, config_old;
//  HAL_StatusTypeDef result = HAL_OK;
//  

//  /* Get pages write protection status ****************************************/
//  HAL_FLASHEx_OBGetConfig(&config_old);

//  /* The parameter says whether we turn the protection on or off */
//  config_new.WRPState = (protectionstate == FLASHIF_WRP_ENABLE ? OB_WRPSTATE_ENABLE : OB_WRPSTATE_DISABLE);

//  /* We want to modify only the Write protection */
//  config_new.OptionType = OPTIONBYTE_WRP;
//  
//  /* No read protection, keep BOR and reset settings */
//  config_new.RDPLevel = OB_RDP_LEVEL_0;
//  config_new.USERConfig = config_old.USERConfig;  
//  /* Get pages already write protected ****************************************/
//  ProtectedPAGE = config_old.WRPPage | FLASH_PAGE_TO_BE_PROTECTED;

//  /* Unlock the Flash to enable the flash control register access *************/ 
//  HAL_FLASH_Unlock();

//  /* Unlock the Options Bytes *************************************************/
//  HAL_FLASH_OB_Unlock();
//  
//  /* Erase all the option Bytes ***********************************************/
//  result = HAL_FLASHEx_OBErase();
//    
//  if (result == HAL_OK)
//  {
//    config_new.WRPPage    = ProtectedPAGE;
//    result = HAL_FLASHEx_OBProgram(&config_new);
//  }
//  
//  return (result == HAL_OK ? FLASHIF_OK: FLASHIF_PROTECTION_ERRROR);
return FLASHIF_OK;
}
/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
