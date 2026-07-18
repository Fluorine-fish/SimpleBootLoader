/**
*   @file Bsp_Uart.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/16
*   @version 1.0
*   @note
*/
#ifndef BOOTLOADER_BSP_UART_H
#define BOOTLOADER_BSP_UART_H

#include "bootloader.h"

#define IAP_TRIGGER_CHAR            'U'
#define IAP_REQUEST_SOF             0x5AU
#define IAP_RESPONSE_SOF            0xA5U
#define IAP_MAX_BLOCK_SIZE          256U
#define IAP_MAX_PAYLOAD_SIZE        (4U + IAP_MAX_BLOCK_SIZE)

#define IAP_CMD_GET_INFO            0xA1U
#define IAP_CMD_ERASE_APP           0xA2U
#define IAP_CMD_WRITE_BLOCK         0xA3U
#define IAP_CMD_VERIFY              0xA4U
#define IAP_CMD_JUMP_APP            0xA5U

typedef enum {
    IAP_STATUS_OK = 0x00U,
    IAP_STATUS_BAD_FRAME = 0x01U,
    IAP_STATUS_BAD_LENGTH = 0x02U,
    IAP_STATUS_BAD_STATE = 0x03U,
    IAP_STATUS_FLASH_ERROR = 0x04U,
    IAP_STATUS_CRC_ERROR = 0x05U,
    IAP_STATUS_INVALID_APP = 0x06U,
    IAP_STATUS_UNKNOWN_COMMAND = 0x07U
} IapStatus_t;

typedef struct {
    uint8_t command;
    uint16_t length;
    uint8_t payload[IAP_MAX_PAYLOAD_SIZE];
} IapFrame_t;

typedef struct {
    bool active;
    uint32_t app_size;
    uint32_t app_crc32;
    uint32_t app_version;
} IapSession_t;

bool Uart_WaitForTrigger(uint32_t timeout_ms);
void Uart_Run(void);

#endif //BOOTLOADER_BSP_UART_H