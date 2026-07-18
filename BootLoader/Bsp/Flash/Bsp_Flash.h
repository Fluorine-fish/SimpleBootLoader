/**
*   @file Bsp_Flash.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/16
*   @version 1.0
*   @note
*/
#ifndef BOOTLOADER_BSP_FLASH_H
#define BOOTLOADER_BSP_FLASH_H

#include "BootLoader.h"
#include "stm32g4xx_hal.h"

HAL_StatusTypeDef Flash_EraseApplication();
HAL_StatusTypeDef Flash_WriteApplication(uint32_t addr, const uint8_t* data, uint32_t length);
HAL_StatusTypeDef Flash_WriteBootInfo(const BL_BootInfo_t* info);
uint32_t Flash_CalculateCrc32(uint32_t addr, uint32_t length);

#endif //BOOTLOADER_BSP_FLASH_H