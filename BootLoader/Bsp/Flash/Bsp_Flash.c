/**
*   @file Bsp_Flash.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/16
*   @version 1.0
*   @note
*/
#include "Bsp_Flash.h"
#include "string.h"

static HAL_StatusTypeDef Flash_ErasePage(uint32_t first_page, uint32_t page_count) {
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0xFFFFFFFFU;
    HAL_StatusTypeDef status;

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) return status;

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks = FLASH_BANK_1;
    erase.Page = first_page;
    erase.NbPages = page_count;

    status = HAL_FLASHEx_Erase(&erase, &page_error);
    HAL_FLASH_Lock();

    return status;
}

static HAL_StatusTypeDef Flash_Program(uint32_t addr, const uint8_t *data,
    uint32_t length, uint32_t region_start, uint32_t region_end) {
    HAL_StatusTypeDef status;
    uint32_t write_end;

    if ((data == NULL) ||
        (length == 0) ||
        ((addr  & 0x7U) != 0) ||
        ((length & 0x7U) != 0) ||
        (addr < region_start) ||
        (length > (region_end - region_start))) return HAL_ERROR;

    write_end = addr + length;
    if ((write_end < addr) || (write_end > region_end)) return HAL_ERROR;

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) return status;

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    for (uint32_t offset = 0U; offset < length; offset += 8U) {
        uint64_t value; // DOUBLEWORD 写入，一次8Bytes

        memcpy(&value, &data[offset], sizeof(value));

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, addr + offset, value);
        if (status != HAL_OK) break;
        if (*(__IO uint64_t *)(addr + offset) != value) { // 检查是否写入成功
            status = HAL_ERROR;
            break;
        }
    }

    HAL_FLASH_Lock();
    return status;
}

HAL_StatusTypeDef Flash_EraseApplication() {
    return Flash_ErasePage(APP_START_PAGE, APP_PAGE_COUNT);
}

HAL_StatusTypeDef Flash_WriteApplication(uint32_t addr, const uint8_t* data, uint32_t length) {
    return Flash_Program(addr, data, length, APP_START_ADDR, APP_END_ADDR);
}

HAL_StatusTypeDef Flash_WriteBootInfo(const BL_BootInfo_t* info) {
    HAL_StatusTypeDef status;

    if (info == NULL) return HAL_ERROR;
    status = Flash_ErasePage(BOOTINFO_START_PAGE, BOOTINFO_PAGE_COUNT);
    if (status != HAL_OK) return status;

    return Flash_Program(BOOTINFO_START_ADDR, (const uint8_t *)info, sizeof(BL_BootInfo_t), BOOTINFO_START_ADDR, BOOTINFO_START_ADDR + BOOTINFO_SIZE);
}

uint32_t Flash_CalculateCrc32(uint32_t address, uint32_t length)
{
    const uint8_t *data = (const uint8_t *)address;
    uint32_t crc = 0xFFFFFFFFU;

    for (uint32_t index = 0U; index < length; index++) {
        crc ^= data[index];

        for (uint32_t bit = 0U; bit < 8U; bit++) {
            if ((crc & 1U) != 0U) {
                crc = (crc >> 1U) ^ 0xEDB88320U;
            }
            else {
                crc >>= 1U;
            }
        }
    }

    return ~crc;
}
