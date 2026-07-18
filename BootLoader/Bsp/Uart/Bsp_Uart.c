/**
*   @file Bsp_Uart.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/16
*   @version 1.0
*   @note
*/
#include "Bsp_Uart.h"
#include "Bsp_Flash.h"
#include "usart.h"
#include "string.h"

static IapSession_t session;

static uint16_t IapUart_Crc16Update(uint16_t crc, uint8_t data) {
    crc ^= (uint16_t)data << 8U;

    for (uint32_t bit = 0U; bit < 8U; bit++) {
        if ((crc & 0x8000U) != 0U) {
            crc = (uint16_t)((crc << 1U) ^ 0x1021U);
        }
        else {
            crc <<= 1U;
        }
    }

    return crc;
}

static uint16_t IapUart_CalculateRequestCrc(const IapFrame_t *frame)
{
    uint16_t crc = 0xFFFFU;

    crc = IapUart_Crc16Update(crc, frame->command);
    crc = IapUart_Crc16Update(crc, (uint8_t)(frame->length & 0xFFU));
    crc = IapUart_Crc16Update(crc, (uint8_t)(frame->length >> 8U));

    for (uint32_t index = 0U; index < frame->length; index++) {
        crc = IapUart_Crc16Update(crc, frame->payload[index]);
    }

    return crc;
}

static uint32_t IapUart_ReadLe32(const uint8_t *data) {
    return ((uint32_t)data[0]) |
           ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

static HAL_StatusTypeDef IapUart_ReceiveFrame(IapFrame_t *frame,
                                              IapStatus_t *frame_status) {
    uint8_t byte;
    uint8_t header[3];
    uint8_t received_crc_bytes[2];
    uint16_t received_crc;
    uint16_t calculated_crc;

    frame->command = 0U;
    frame->length = 0U;
    *frame_status = IAP_STATUS_BAD_FRAME;

    do
    {
        if (HAL_UART_Receive(&huart1, &byte, 1U, HAL_MAX_DELAY) != HAL_OK)
        {
            return HAL_ERROR;
        }
    } while (byte != IAP_REQUEST_SOF);

    if (HAL_UART_Receive(&huart1, header, sizeof(header), 1000U) != HAL_OK)
    {
        return HAL_TIMEOUT;
    }

    frame->command = header[0];
    frame->length = (uint16_t)header[1] |
                    ((uint16_t)header[2] << 8U);

    if (frame->length > IAP_MAX_PAYLOAD_SIZE)
    {
        *frame_status = IAP_STATUS_BAD_LENGTH;
        return HAL_ERROR;
    }

    if ((frame->length > 0U) &&
        (HAL_UART_Receive(&huart1,
                          frame->payload,
                          frame->length,
                          3000U) != HAL_OK))
    {
        return HAL_TIMEOUT;
    }

    if (HAL_UART_Receive(&huart1,
                         received_crc_bytes,
                         sizeof(received_crc_bytes),
                         1000U) != HAL_OK)
    {
        return HAL_TIMEOUT;
    }

    received_crc = (uint16_t)received_crc_bytes[0] |
                   ((uint16_t)received_crc_bytes[1] << 8U);
    calculated_crc = IapUart_CalculateRequestCrc(frame);

    if (received_crc != calculated_crc)
    {
        *frame_status = IAP_STATUS_CRC_ERROR;
        return HAL_ERROR;
    }

    *frame_status = IAP_STATUS_OK;
    return HAL_OK;
}

static void IapUart_SendResponse(uint8_t command,
                                 IapStatus_t status,
                                 const uint8_t *payload,
                                 uint16_t length) {
    uint8_t header[5];
    uint8_t crc_bytes[2];
    uint16_t crc = 0xFFFFU;

    header[0] = IAP_RESPONSE_SOF;
    header[1] = command;
    header[2] = (uint8_t)status;
    header[3] = (uint8_t)(length & 0xFFU);
    header[4] = (uint8_t)(length >> 8U);

    crc = IapUart_Crc16Update(crc, header[1]);
    crc = IapUart_Crc16Update(crc, header[2]);
    crc = IapUart_Crc16Update(crc, header[3]);
    crc = IapUart_Crc16Update(crc, header[4]);

    for (uint32_t index = 0U; index < length; index++)
    {
        crc = IapUart_Crc16Update(crc, payload[index]);
    }

    crc_bytes[0] = (uint8_t)(crc & 0xFFU);
    crc_bytes[1] = (uint8_t)(crc >> 8U);

    HAL_UART_Transmit(&huart1, header, sizeof(header), HAL_MAX_DELAY);

    if ((payload != NULL) && (length > 0U))
    {
        HAL_UART_Transmit(&huart1,
                          (uint8_t *)payload,
                          length,
                          HAL_MAX_DELAY);
    }

    HAL_UART_Transmit(&huart1,
                      crc_bytes,
                      sizeof(crc_bytes),
                      HAL_MAX_DELAY);
}

static IapStatus_t IapUart_HandleErase(const IapFrame_t *frame) {
    BL_BootInfo_t info = {0};

    if (frame->length != 12U)
    {
        return IAP_STATUS_BAD_LENGTH;
    }

    session.app_size = IapUart_ReadLe32(&frame->payload[0]);
    session.app_crc32 = IapUart_ReadLe32(&frame->payload[4]);
    session.app_version = IapUart_ReadLe32(&frame->payload[8]);

    if ((session.app_size == 0U) ||
        (session.app_size > APP_SIZE))
    {
        return IAP_STATUS_BAD_LENGTH;
    }

    info.magic = BOOTINFO_MAGIC;
    info.state = BOOTINFO_STATE_UPDATING;
    info.app_addr = APP_START_ADDR;
    info.app_size = session.app_size;
    info.app_crc32 = session.app_crc32;
    info.app_version = session.app_version;
    info.reserved[0] = 0xFFFFFFFFU;
    info.reserved[1] = 0xFFFFFFFFU;

    if (Flash_WriteBootInfo(&info) != HAL_OK) {
        return IAP_STATUS_FLASH_ERROR;
    }

    if (Flash_EraseApplication() != HAL_OK) {
        return IAP_STATUS_FLASH_ERROR;
    }

    session.active = true;
    return IAP_STATUS_OK;
}

static IapStatus_t IapUart_HandleWrite(const IapFrame_t *frame)
{
    uint32_t offset;
    uint32_t data_length;
    uint32_t padded_firmware_size;

    if (!session.active) {
        return IAP_STATUS_BAD_STATE;
    }

    if (frame->length < 12U) {
        return IAP_STATUS_BAD_LENGTH;
    }

    offset = IapUart_ReadLe32(&frame->payload[0]);
    data_length = (uint32_t)frame->length - 4U;
    padded_firmware_size = (session.app_size + 7U) & ~7U;

    if (((offset & 0x7U) != 0U) ||
        ((data_length & 0x7U) != 0U) ||
        (data_length > IAP_MAX_BLOCK_SIZE) ||
        (offset > padded_firmware_size) ||
        (data_length > (padded_firmware_size - offset))) {
        return IAP_STATUS_BAD_LENGTH;
    }

    if (Flash_WriteApplication(APP_START_ADDR + offset,
                                 &frame->payload[4],
                                 data_length) != HAL_OK) {
        return IAP_STATUS_FLASH_ERROR;
    }

    return IAP_STATUS_OK;
}

static IapStatus_t IapUart_HandleVerify(void)
{
    BL_BootInfo_t info = {0};
    uint32_t calculated_crc;

    if (!session.active) {
        return IAP_STATUS_BAD_STATE;
    }

    calculated_crc = Flash_CalculateCrc32(APP_START_ADDR,
                                            session.app_size);
    if (calculated_crc != session.app_crc32) {
        return IAP_STATUS_CRC_ERROR;
    }

    info.magic = BOOTINFO_MAGIC;
    info.state = BOOTINFO_STATE_VALID;
    info.app_addr = APP_START_ADDR;
    info.app_size = session.app_size;
    info.app_crc32 = session.app_crc32;
    info.app_version = session.app_version;
    info.reserved[0] = 0xFFFFFFFFU;
    info.reserved[1] = 0xFFFFFFFFU;

    if (Flash_WriteBootInfo(&info) != HAL_OK) {
        return IAP_STATUS_FLASH_ERROR;
    }

    session.active = false;
    return IAP_STATUS_OK;
}

bool Uart_WaitForTrigger(uint32_t timeout_ms) {
    uint8_t trigger;

    if (HAL_UART_Receive(&huart1, &trigger, 1U, timeout_ms) != HAL_OK) {
        return false;
    }

    return (trigger == (uint8_t)IAP_TRIGGER_CHAR) ||
           (trigger == (uint8_t)'u');
}

void Uart_Run(void) {
    IapFrame_t frame;
    BL_BootInfo_t info;
    const char ready_message[] = "IAP ready\r\n";

    memset(&session, 0, sizeof(session));
    BL_BootInfoRead(&info);

    if ((info.magic == BOOTINFO_MAGIC) &&
        (info.state == BOOTINFO_STATE_UPDATING) &&
        (info.app_addr == APP_START_ADDR) &&
        (info.app_size > 0U) &&
        (info.app_size <= APP_SIZE)) {
        session.active = true;
        session.app_size = info.app_size;
        session.app_crc32 = info.app_crc32;
        session.app_version = info.app_version;
    }

    HAL_UART_Transmit(&huart1,
                      (uint8_t *)ready_message,
                      sizeof(ready_message) - 1U,
                      HAL_MAX_DELAY);

    while (1) {
        IapStatus_t receive_status;
        IapStatus_t command_status;

        if (IapUart_ReceiveFrame(&frame, &receive_status) != HAL_OK) {
            IapUart_SendResponse(frame.command,
                                 receive_status,
                                 NULL,
                                 0U);
            continue;
        }


        switch (frame.command) {
            case IAP_CMD_GET_INFO:
                const char trigger_message_get_info[] =
                "[IAP] Start Get Info...\r\n";

                HAL_UART_Transmit(&huart1,
                                  (uint8_t *)trigger_message_get_info,
                                  sizeof(trigger_message_get_info) - 1U,
                                  10);

                if (frame.length != 0U) {
                    command_status = IAP_STATUS_BAD_LENGTH;
                    IapUart_SendResponse(frame.command,
                                         command_status,
                                         NULL,
                                         0U);
                    break;
                }

                BL_BootInfoRead(&info);
                IapUart_SendResponse(frame.command,
                                     IAP_STATUS_OK,
                                     (const uint8_t *)&info,
                                     sizeof(info));
                break;

            case IAP_CMD_ERASE_APP:
                const char trigger_message_erase_app[] = "[IAP] Start Erase App...\r\n";

                HAL_UART_Transmit(&huart1,
                                  (uint8_t *)trigger_message_erase_app,
                                  sizeof(trigger_message_erase_app) - 1U,
                                  10);

                command_status = IapUart_HandleErase(&frame);
                IapUart_SendResponse(frame.command,
                                     command_status,
                                     NULL,
                                     0U);
                break;

            case IAP_CMD_WRITE_BLOCK:
                const char trigger_message_write_block[] = "[IAP] Start Write Block..\r\n";

                HAL_UART_Transmit(&huart1,
                                  (uint8_t *)trigger_message_write_block,
                                  sizeof(trigger_message_write_block) - 1U,
                                  10);

                command_status = IapUart_HandleWrite(&frame);
                IapUart_SendResponse(frame.command,
                                     command_status,
                                     NULL,
                                     0U);
                break;

            case IAP_CMD_VERIFY:
                const char trigger_message_verify[] = "[IAP] Start Verify...\r\n";

                HAL_UART_Transmit(&huart1,
                                  (uint8_t *)trigger_message_verify,
                                  sizeof(trigger_message_verify) - 1U,
                                  10);

                command_status = (frame.length == 0U) ?
                                 IapUart_HandleVerify() :
                                 IAP_STATUS_BAD_LENGTH;
                IapUart_SendResponse(frame.command,
                                     command_status,
                                     NULL,
                                     0U);
                break;

            case IAP_CMD_JUMP_APP:
                const char trigger_message_jump[] = "[IAP] Start Jump to App...\r\n";

                HAL_UART_Transmit(&huart1,
                                  (uint8_t *)trigger_message_jump,
                                  sizeof(trigger_message_jump) - 1U,
                                  10);

                command_status = (frame.length == 0U) &&
                                 BL_IsApplicationBootable() ?
                                 IAP_STATUS_OK :
                                 IAP_STATUS_INVALID_APP;
                IapUart_SendResponse(frame.command,
                                     command_status,
                                     NULL,
                                     0U);

                if (command_status == IAP_STATUS_OK)
                {
                    HAL_Delay(20U);

                    const char trigger_message_jmp[] =
                    "[IAP] Jump to App!\r\n";

                    HAL_UART_Transmit(&huart1,
                                      (uint8_t *)trigger_message_jmp,
                                      sizeof(trigger_message_jmp) - 1U,
                                      10);

                    BL_JumpToApplication();
                }
                break;

            default:
                const char trigger_message_unknown[] =
                "[IAP] Command Unkown!\r\n";

                HAL_UART_Transmit(&huart1,
                                  (uint8_t *)trigger_message_unknown,
                                  sizeof(trigger_message_unknown) - 1U,
                                  10);

                IapUart_SendResponse(frame.command,
                                     IAP_STATUS_UNKNOWN_COMMAND,
                                     NULL,
                                     0U);
                break;
        }
    }
}
