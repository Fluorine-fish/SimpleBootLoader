/**
*   @file Encoder_Mt6701.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/19
*   @version 1.0
 *   @note
*/
#include "Encoder_Mt6701.h"
#include "gpio.h"
#include "spi.h"

/* CRC table for MT6701 crc-6*/
static uint8_t CrcTable[64] = {
    0x00, 0x03, 0x06, 0x05, 0x0C, 0x0F, 0x0A, 0x09,
    0x18, 0x1B, 0x1E, 0x1D, 0x14, 0x17, 0x12, 0x11,
    0x30, 0x33, 0x36, 0x35, 0x3C, 0x3F, 0x3A, 0x39,
    0x28, 0x2B, 0x2E, 0x2D, 0x24, 0x27, 0x22, 0x21,
    0x23, 0x20, 0x25, 0x26, 0x2F, 0x2C, 0x29, 0x2A,
    0x3B, 0x38, 0x3D, 0x3E, 0x37, 0x34, 0x31, 0x32,
    0x13, 0x10, 0x15, 0x16, 0x1F, 0x1C, 0x19, 0x1A,
    0x0B, 0x08, 0x0D, 0x0E, 0x07, 0x04, 0x01, 0x02
   };

/*32-bit input data, right alignment, Calculation over 18 bits (mult. of 6) */
static uint8_t Encoder_Mt6701_Crc (uint32_t w_InputData)
{
    uint8_t u8Index = 0;
    uint8_t u8CRC = 0;

    u8Index = (uint8_t )(((uint32_t)w_InputData >> 12u) & 0x0000003Fu);

    u8CRC = (uint8_t )(((uint32_t)w_InputData >> 6u) & 0x0000003Fu);
    u8Index = u8CRC ^ CrcTable[u8Index];

    u8CRC = (uint8_t )((uint32_t)w_InputData & 0x0000003Fu);
    u8Index = u8CRC ^ CrcTable[u8Index];

    u8CRC = CrcTable[u8Index];

    return u8CRC;
}

bool Encoder_GetRaw(uint16_t *raw_angle) {
    uint8_t rx_buf[3] = {};
    uint16_t angle = 0, crc = 0, status = 0;
    uint32_t crc_data = 0; // 18bit data for crc-6

    // Pulldown CS
    HAL_GPIO_WritePin(SPI3_CS_GPIO_Port, SPI3_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Receive(&hspi3, (uint8_t*)&rx_buf, 3, 1);
    // PullUp CS
    HAL_GPIO_WritePin(SPI3_CS_GPIO_Port, SPI3_CS_Pin, GPIO_PIN_SET);

    angle  = (rx_buf[0] << 6) | (rx_buf[1] >> 2);
    status = ((rx_buf[1] & 0x03) << 2) | (rx_buf[2] >> 6);
    crc    = rx_buf[2] & 0x3F;
    crc_data = ((rx_buf[0] << 16) | (rx_buf[1] << 8) | rx_buf[2]) >> 6;

    if (Encoder_Mt6701_Crc(crc_data) != crc) return false;
    else *raw_angle = angle << (16 - 14); // 14位转换到16位

    return true;
}
