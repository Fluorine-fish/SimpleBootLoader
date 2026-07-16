/**
*   @file BootLoader.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/16
*   @version 1.0
*   @note
*/
#ifndef BOOTLOADER_BOOTLOADER_H
#define BOOTLOADER_BOOTLOADER_H

#include "stdbool.h"
#include "stdint.h"

/* Flash Address */
#define BOOTLOADER_START_ADDR 0x08000000U
#define BOOTLOADER_SIZE (24U * 1024U)  // 24KB
#define APP_START_ADDR 0x08006000U
#define APP_SIZE (96U * 1024U) // 96KB
#define APP_END_ADDR (APP_START_ADDR + APP_SIZE)
#define DATA_START_ADDR 0x0801E000U
#define DATA_SIZE (4U * 1024U) // 4KB
#define BOOTINFO_START_ADDR 0x0801F000U
#define BOOTINFO_SIZE (4U * 1024U) // 4KB

/* Flash Size */
#define FLASH_PAGE_SIZE_BYTES 0x800U
#define APP_START_PAGE 12U
#define APP_PAGE_COUNT 48U
#define BOOTINFO_START_PAGE 62U
#define BOOTINFO_PAGE_COUNT 2U

/* SRAM1 + SRAM2 */
#define MAIN_SRAM_START_ADDR 0x20000000U
#define MAIN_SRAM_END_ADDR 0x20005800U

/* BootLoader */
#define BOOTINFO_MAGIC 0x434C4931U // "BLT1"
#define BOOTINFO_START_UPDATING 0x55504454 // "UPDT"
#define BOOTINFO_STATE_VALID 0x56414C44U // "VALD"

// 32Bytes, 8Bytes Algin
typedef struct {
    uint32_t magic;
    uint32_t state;
    uint32_t app_addr;
    uint32_t app_size;
    uint32_t app_crc32;
    uint32_t app_version;
    uint32_t reserved[2];
} BL_BootInfo_t;

void BL_BootInfoRead(BL_BootInfo_t *info);
bool BL_BootInfoIsUpdating();
bool BL_IsVectorTableValid();
bool BL_IsApplicationBootable();
void BL_JumpToApplication();

#endif //BOOTLOADER_BOOTLOADER_H
