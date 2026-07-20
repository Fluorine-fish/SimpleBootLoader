/**
*   @file BootLoader.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/16
*   @version 1.0
*   @note
*/
#include "BootLoader.h"

#include "Bsp_Flash.h"
#include "string.h"
#include "core_cm4.h"
#include "stm32g4xx_hal_uart.h"
#include "usart.h"

// 一个空输入空返回的函数指针，方便直接跳转到App的入口执行程序
typedef void (*ApplicationEntry_t)(void);

static bool BL_IsStackPointerValid(uint32_t stack_pointer)
{
    if ((stack_pointer & 0x3U) != 0U) { // 判断SP栈指针末两位是否0
        return false;
    }

    // SP要落在22KB的SRAM区域内
    return (stack_pointer >= MAIN_SRAM_START_ADDR) &&
           (stack_pointer <= MAIN_SRAM_END_ADDR);
}

void BL_BootInfoRead(BL_BootInfo_t *info) {
    if (info != NULL) {
        memcpy(info, (const void *)BOOTINFO_START_ADDR, sizeof(BL_BootInfo_t));
    }
}

// 判断魔术字和升级状态确定是否在进行升级
bool BL_BootInfoIsUpdating() {
    BL_BootInfo_t tmp_info;
    BL_BootInfoRead(&tmp_info);

    return (tmp_info.magic == BOOTINFO_MAGIC) &&
           (tmp_info.state == BOOTINFO_STATE_UPDATING);
}

bool BL_IsVectorTableValid() {
    /*
     *Application 向量表的前两个32位数据是 Application初始的MSP 和 Reset_Handler地址
     *Cortex-M 中 实际执行的地址需要包含Thumb位，而用于地址判断的需要去掉Thumb（2字节对齐）
     */
    uint32_t app_stack = *(__IO uint32_t*)APP_START_ADDR;
    uint32_t app_reset = *(__IO uint32_t*)(APP_START_ADDR + 4U);
    uint32_t reset_addr = app_reset & ~1U; // & ~1U相当于去掉最低位，～为取反

    if (!BL_IsStackPointerValid(app_stack)) return false;
    if ((app_reset & 1U) == 0U) return false;

    return (reset_addr >= APP_START_ADDR) &&
           (reset_addr < APP_END_ADDR);
}

bool BL_IsApplicationBootable() {
    BL_BootInfo_t tmp_info;

    if (!BL_IsVectorTableValid()) return false;
    BL_BootInfoRead(&tmp_info);

    /* 方便开发阶段使用J-Link直接烧录Application。 */
    if ((tmp_info.magic == 0xFFFFFFFFU) && (tmp_info.state == 0xFFFFFFFFU)) {
        return true;
    }

    if ((tmp_info.magic != BOOTINFO_MAGIC) ||
     (tmp_info.state != BOOTINFO_STATE_VALID) ||
     (tmp_info.app_addr != APP_START_ADDR) ||
     (tmp_info.app_size == 0U) ||
     (tmp_info.app_size > APP_SIZE)) {
        return false;
    }

    return Flash_CalculateCrc32(APP_START_ADDR, tmp_info.app_size) == tmp_info.app_crc32;
}

void BL_JumpToApplication() {
    uint32_t app_stack;
    uint32_t app_reset;
    ApplicationEntry_t app_entry;

    if (!BL_IsVectorTableValid()) return;

    app_stack = *(__IO uint32_t*)APP_START_ADDR;
    app_reset = *(__IO uint32_t*)(APP_START_ADDR + 4U);
    app_entry = (ApplicationEntry_t)(uintptr_t)app_reset;

    __disable_irq();

    HAL_UART_DeInit(&huart1);

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk |
            SCB_ICSR_PENDSVCLR_Msk;

    for (uint32_t index = 0U; index < 8U; index++) {
        NVIC->ICER[index] = 0xFFFFFFFFU;
        NVIC->ICPR[index] = 0xFFFFFFFFU;
    }

    HAL_RCC_DeInit();
    HAL_DeInit();

    SCB->VTOR = APP_START_ADDR;

    __set_BASEPRI(0U);
    __set_FAULTMASK(0U);
    __set_CONTROL(0U);

    __DSB();
    __ISB();

    __set_MSP(app_stack);
    __DSB();
    __ISB();

    __enable_irq();
    app_entry();

    while (1) ;
}
