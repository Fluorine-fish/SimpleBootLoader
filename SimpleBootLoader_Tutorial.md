---
title: "基于STM32G431实现一个最简单的BootLoader"
description: "基于现有SimpleBootLoader工程，从Flash分区、链接脚本和向量表重定位开始，实现应用跳转、USART1基础IAP、Flash编程、BootInfo记录与固件校验"
slug: "stm32g431-simple-bootloader"

# 分类信息
domain: "驱动与BSP层"
module: "Bootloader开发"
content_type: "tutorial"
skill_level: "beginner"

# 标签和关联
tags:
  - STM32G431
  - Bootloader开发
  - IAP升级
  - Flash编程
  - USART通信
prerequisites:
  - "Cortex-M启动流程"
  - "STM32CubeMX基础"
related_contents:
  - "固件分区与内存布局"
  - "Bootloader与Application跳转"

# 学习信息
estimated_time: 180
difficulty_score: 4

# 作者和版本
author: "SimpleBootLoader项目教程"
created_at: "2026-07-16"
updated_at: "2026-07-16"
version: "1.0"

# 语言和本地化
language: "zh-CN"

# 状态和可见性
status: "published"
is_featured: false
is_premium: false

# SEO和元数据
keywords:
  - STM32G431 Bootloader
  - STM32G4 IAP
  - USART Bootloader
  - Flash双字编程
  - 向量表重定位
cover_image: ""
---

# 基于STM32G431实现一个最简单的BootLoader

## 概述

本教程以当前`SimpleBootLoader`目录中的真实工程为基础，实现一个适用于STM32G431VBT6的基础BootLoader。工程不增加备份固件区，也不实现失败回滚，而是直接通过USART1将固件写入Application分区，目标与README中“验证BootLoader、实现IAP基础功能”的描述保持一致。

当前工程已经由STM32CubeMX生成了两个独立项目：

- `BootLoader`：负责启动判断、串口升级、Flash编程和应用跳转
- `Application`：实际业务程序，当前包含FreeRTOS、ADC、I2C、OPAMP、SPI、TIM等组件

完成本教程后，你将能够：

- 理解STM32G431上电后的向量表加载过程
- 按README规划BootLoader、Application、Data和BootInfo分区
- 为两个工程设置不同的Flash链接地址
- 修正STM32G431不连续RAM所带来的链接和栈校验问题
- 判断Application向量表是否有效
- 安全地从BootLoader跳转到Application
- 使用STM32G4 HAL库擦除Flash页并按64位双字写入数据
- 通过USART1接收固件并完成最基础的IAP升级
- 使用BootInfo记录固件大小、CRC32、版本和升级状态
- 使用Python上位机发送Application二进制文件

!!! warning "本教程的升级策略"
    本项目没有备份分区。开始升级后，BootLoader会直接擦除Application。此时如果掉电或传输中断，原Application将无法继续运行，但BootLoader仍然保留，可以重新进入IAP并再次下载固件。这种设计适合功能验证，不适合要求升级失败自动恢复的产品。

## 准备工作

### 硬件要求

本教程使用当前工程指定的硬件平台：

- **微控制器**：STM32G431VBT6，128KB内部Flash
- **调试器**：ST-Link V2、ST-Link V3或J-Link
- **串口工具**：3.3V USB转TTL模块
- **连接线**：连接USART1和公共地

当前`BootLoader.ioc`已经配置USART1：

```text
STM32G431                 USB转TTL
PA9  (USART1_TX)  ------> RX
PA10 (USART1_RX)  <------ TX
GND               ------> GND

串口参数：115200，8位数据位，1位停止位，无校验，无流控
```

!!! warning "电平要求"
    STM32G431的GPIO使用3.3V逻辑电平，不要将5V TTL串口直接连接到PA9或PA10。

### 软件要求

- **配置工具**：STM32CubeMX
- **构建系统**：CMake 3.22及以上版本、Ninja
- **编译器**：GNU Arm Embedded Toolchain，命令前缀为`arm-none-eabi-`
- **下载工具**：STM32CubeProgrammer、OpenOCD或IDE内置下载器
- **串口工具**：任意串口助手，或本教程提供的Python脚本
- **Python依赖**：Python 3、pyserial

当前工程的CMake预设已经提供`Debug`和`Release`两种构建方式，不需要重新创建工程。

### 当前项目结构

本教程基于以下已有目录：

```text
SimpleBootLoader/
├── README.md
├── BootLoader/
│   ├── BootLoader.ioc
│   ├── CMakeLists.txt
│   ├── CMakePresets.json
│   ├── STM32G431XX_FLASH.ld
│   ├── startup_stm32g431xx.s
│   ├── Core/
│   │   ├── Inc/
│   │   │   ├── main.h
│   │   │   ├── gpio.h
│   │   │   └── usart.h
│   │   └── Src/
│   │       ├── main.c
│   │       ├── gpio.c
│   │       ├── usart.c
│   │       └── system_stm32g4xx.c
│   └── Drivers/
│
└── Application/
    ├── Application.ioc
    ├── CMakeLists.txt
    ├── CMakePresets.json
    ├── STM32G431XX_FLASH.ld
    ├── startup_stm32g431xx.s
    ├── Core/
    │   ├── Inc/
    │   └── Src/
    │       ├── main.c
    │       ├── app_freertos.c
    │       └── system_stm32g4xx.c
    ├── Middlewares/Third_Party/FreeRTOS/
    └── Drivers/
```

为了保持代码职责清晰，教程会在BootLoader工程中建议增加以下用户文件：

```text
BootLoader/Core/
├── Inc/
│   ├── bootloader.h
│   ├── flash_if.h
│   └── iap_uart.h
└── Src/
    ├── bootloader.c
    ├── flash_if.c
    └── iap_uart.c
```

这些文件是教程中的目标结构。本教程本身不会替你修改当前工程。

## 核心内容

### 步骤1：规划Flash内存布局

STM32G431VBT6包含128KB内部Flash，地址范围为`0x08000000 ~ 0x0801FFFF`。STM32G431的Flash页大小为2KB，也就是`0x800`字节，共64页。

按照README给出的设计，分区如下：

| 分区 | 地址范围 | 大小 | 页号 | 作用 |
|---|---|---:|---:|---|
| BootLoader | `0x08000000 ~ 0x08005FFF` | 24KB | Page0 ~ Page11 | BootLoader程序 |
| Application | `0x08006000 ~ 0x0801DFFF` | 96KB | Page12 ~ Page59 | Application运行区 |
| Data | `0x0801E000 ~ 0x0801EFFF` | 4KB | Page60 ~ Page61 | 驱动校准数据等 |
| BootInfo | `0x0801F000 ~ 0x0801FFFF` | 4KB | Page62 ~ Page63 | 固件元数据和升级状态 |

```text
STM32G431 Flash布局（128KB，2KB/Page）：

0x08000000  ┌──────────────────────────────┐
            │ BootLoader                  │ 24KB
            │ Page0 ~ Page11              │
0x08006000  ├──────────────────────────────┤
            │ Application                 │ 96KB
            │ Page12 ~ Page59             │
0x0801E000  ├──────────────────────────────┤
            │ Data                        │ 4KB
            │ Page60 ~ Page61             │
0x0801F000  ├──────────────────────────────┤
            │ BootInfo                    │ 4KB
            │ Page62 ~ Page63             │
0x08020000  └──────────────────────────────┘
```

!!! note "README中的地址冲突"
    README把Data和BootInfo都写成了`0x0801F000 ~ 0x0801FFFF`。但Page60~Page61实际对应`0x0801E000 ~ 0x0801EFFF`，Page62~Page63才对应`0x0801F000 ~ 0x0801FFFF`。本教程按页号推导后的地址使用，不修改原README。

创建统一的地址定义：

```c
// BootLoader/Core/Inc/bootloader.h
#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

#define BOOTLOADER_START_ADDR       0x08000000U
#define BOOTLOADER_SIZE             (24U * 1024U)

#define APP_START_ADDR              0x08006000U
#define APP_SIZE                    (96U * 1024U)
#define APP_END_ADDR                (APP_START_ADDR + APP_SIZE)

#define DATA_START_ADDR             0x0801E000U
#define DATA_SIZE                   (4U * 1024U)

#define BOOTINFO_START_ADDR         0x0801F000U
#define BOOTINFO_SIZE               (4U * 1024U)

#define FLASH_PAGE_SIZE_BYTES       0x800U
#define APP_FIRST_PAGE              12U
#define APP_PAGE_COUNT              48U
#define BOOTINFO_FIRST_PAGE         62U
#define BOOTINFO_PAGE_COUNT         2U

/* SRAM1 16KB与SRAM2 6KB在0x20000000处连续，共22KB。 */
#define MAIN_SRAM_START_ADDR        0x20000000U
#define MAIN_SRAM_END_ADDR          0x20005800U

#define BOOTINFO_MAGIC              0x424C4931U  /* "BLI1" */
#define BOOTINFO_STATE_UPDATING     0x55504454U  /* "UPDT" */
#define BOOTINFO_STATE_VALID        0x56414C44U  /* "VALD" */

typedef struct
{
    uint32_t magic;
    uint32_t state;
    uint32_t app_addr;
    uint32_t app_size;
    uint32_t app_crc32;
    uint32_t app_version;
    uint32_t reserved0;
    uint32_t reserved1;
} BootInfo_t;

void BootInfo_Read(BootInfo_t *info);
bool BootInfo_IsUpdating(void);
bool BootLoader_IsVectorTableValid(void);
bool BootLoader_IsApplicationBootable(void);
void BootLoader_JumpToApplication(void);

#endif
```

`BootInfo_t`正好为32字节，是8字节的整数倍，可以使用STM32G4要求的64位双字编程方式写入Flash。

### 步骤2：确认两个独立工程

BootLoader和Application必须分别编译、分别链接：

- BootLoader的向量表位于`0x08000000`
- Application的向量表位于`0x08006000`
- 两个工程都包含自己的`Reset_Handler`、`SystemInit()`和`main()`
- CPU复位后只会自动从`0x08000000`读取初始MSP和复位向量
- BootLoader需要手动读取Application向量表，再把控制权交给Application

当前BootLoader工程已经完成以下基础初始化：

```c
HAL_Init();
SystemClock_Config();       // HSI 16MHz，无PLL
MX_GPIO_Init();
MX_USART1_UART_Init();      // PA9/PA10，115200，8N1
```

当前Application工程使用HSE和PLL，并初始化FreeRTOS以及多个外设。BootLoader跳转时不能假设两个工程使用相同的系统时钟，所以跳转前需要恢复时钟和外设状态，让Application按照自己的`SystemClock_Config()`重新初始化。

如果新增`bootloader.c`、`flash_if.c`和`iap_uart.c`，应在`BootLoader/CMakeLists.txt`的用户源文件区域加入：

```cmake
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Src/bootloader.c
    Core/Src/flash_if.c
    Core/Src/iap_uart.c
)
```

`Core/Inc`已经由CubeMX生成的CMake目标加入包含路径，不需要重复配置头文件目录。

### 步骤3：修改两个工程的链接脚本

当前两个链接脚本都仍然使用默认配置：

```ld
FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 128K
```

这会导致Application仍被链接到Flash起始地址，并与BootLoader重叠。需要分别修改。

**BootLoader链接脚本**：`BootLoader/STM32G431XX_FLASH.ld`

```ld
/* BootLoader：Flash Page0 ~ Page11，共24KB */
MEMORY
{
  RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 22K
  CCMRAM (xrw)   : ORIGIN = 0x10000000, LENGTH = 10K
  FLASH (rx)     : ORIGIN = 0x08000000, LENGTH = 24K
}
```

**Application链接脚本**：`Application/STM32G431XX_FLASH.ld`

```ld
/* Application：Flash Page12 ~ Page59，共96KB */
MEMORY
{
  RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 22K
  CCMRAM (xrw)   : ORIGIN = 0x10000000, LENGTH = 10K
  FLASH (rx)     : ORIGIN = 0x08006000, LENGTH = 96K
}
```

其余段定义保持原样。这里仅声明了CCMRAM区域，现有`.data`、`.bss`、堆和栈仍然放在`RAM`中。如果后续希望使用CCMRAM，需要单独增加输出段并显式放置变量。

### 步骤4：修正STM32G431的RAM配置

当前链接脚本把RAM写成从`0x20000000`开始的连续32KB：

```ld
RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 32K
```

但STM32G431VBT6的32KB RAM并不是一个连续地址区：

| RAM区域 | 起始地址 | 大小 | 说明 |
|---|---:|---:|---|
| SRAM1 | `0x20000000` | 16KB | 普通系统SRAM |
| SRAM2 | `0x20004000` | 6KB | 与SRAM1连续 |
| CCMSRAM | `0x10000000` | 10KB | 独立的CCM RAM地址空间 |

SRAM1和SRAM2连续形成`0x20000000 ~ 0x200057FF`的22KB主RAM，合法的栈顶可以等于`0x20005800`。如果仍使用32KB连续RAM配置，链接器会把初始栈顶设置为`0x20008000`，这个地址不属于主RAM。

因此两个工程都应使用：

```ld
RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 22K
```

BootLoader判断Application初始栈指针时，也应使用明确范围：

```c
static bool BootLoader_IsStackPointerValid(uint32_t stack_pointer)
{
    if ((stack_pointer & 0x3U) != 0U)
    {
        return false;
    }

    return (stack_pointer >= MAIN_SRAM_START_ADDR) &&
           (stack_pointer <= MAIN_SRAM_END_ADDR);
}
```

这比常见的`(stack & 0x2FFE0000) == 0x20000000`更适合当前芯片，因为它不会把不存在的RAM地址误判为有效。

### 步骤5：重定位Application中断向量表

Application的链接地址改为`0x08006000`后，它的中断向量表也位于这个地址。Application启动时必须设置`SCB->VTOR`，否则发生中断后，CPU仍可能从BootLoader向量表中查找处理函数。

在`Application/Core/Src/system_stm32g4xx.c`中找到：

```c
/* #define USER_VECT_TAB_ADDRESS */
```

改为：

```c
#define USER_VECT_TAB_ADDRESS
```

然后把默认偏移改为：

```c
#define VECT_TAB_OFFSET         0x00006000U
```

最终`SystemInit()`中的现有代码会执行：

```c
SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
```

等价于：

```c
SCB->VTOR = 0x08006000U;
```

!!! note "为什么在BootLoader和Application中都设置VTOR"
    BootLoader跳转前设置VTOR，可以保证控制权切换时立即使用Application向量表；Application在`SystemInit()`中再次设置，可以保证它从调试器直接启动时也使用正确向量表。两处设置并不冲突。

`0x08006000`是`0x200`的整数倍，满足当前STM32G4系统文件对向量表地址对齐的要求。

### 步骤6：检查Application是否有效

Application向量表的前两个32位数据分别是：

```text
0x08006000：Application初始MSP
0x08006004：Application Reset_Handler地址
```

有效性检查至少应验证：

1. MSP位于主SRAM范围内
2. MSP满足4字节对齐
3. Reset_Handler最低位为1，表示Thumb状态
4. Reset_Handler实际地址位于Application分区内

实现如下：

```c
// BootLoader/Core/Src/bootloader.c
#include "bootloader.h"
#include "flash_if.h"
#include "usart.h"
#include <string.h>

typedef void (*ApplicationEntry_t)(void);

static bool BootLoader_IsStackPointerValid(uint32_t stack_pointer)
{
    if ((stack_pointer & 0x3U) != 0U)
    {
        return false;
    }

    return (stack_pointer >= MAIN_SRAM_START_ADDR) &&
           (stack_pointer <= MAIN_SRAM_END_ADDR);
}

void BootInfo_Read(BootInfo_t *info)
{
    if (info != NULL)
    {
        memcpy(info, (const void *)BOOTINFO_START_ADDR, sizeof(BootInfo_t));
    }
}

bool BootInfo_IsUpdating(void)
{
    BootInfo_t info;
    BootInfo_Read(&info);

    return (info.magic == BOOTINFO_MAGIC) &&
           (info.state == BOOTINFO_STATE_UPDATING);
}

bool BootLoader_IsVectorTableValid(void)
{
    uint32_t app_stack = *(__IO uint32_t *)APP_START_ADDR;
    uint32_t app_reset = *(__IO uint32_t *)(APP_START_ADDR + 4U);
    uint32_t reset_address = app_reset & ~1U;

    if (!BootLoader_IsStackPointerValid(app_stack))
    {
        return false;
    }

    if ((app_reset & 1U) == 0U)
    {
        return false;
    }

    return (reset_address >= APP_START_ADDR) &&
           (reset_address < APP_END_ADDR);
}

bool BootLoader_IsApplicationBootable(void)
{
    BootInfo_t info;

    if (!BootLoader_IsVectorTableValid())
    {
        return false;
    }

    BootInfo_Read(&info);

    /* 方便开发阶段使用ST-Link直接烧录Application。 */
    if ((info.magic == 0xFFFFFFFFU) &&
        (info.state == 0xFFFFFFFFU))
    {
        return true;
    }

    if ((info.magic != BOOTINFO_MAGIC) ||
        (info.state != BOOTINFO_STATE_VALID) ||
        (info.app_addr != APP_START_ADDR) ||
        (info.app_size == 0U) ||
        (info.app_size > APP_SIZE))
    {
        return false;
    }

    return FlashIf_CalculateCrc32(APP_START_ADDR, info.app_size) ==
           info.app_crc32;
}
```

这里保留了一个开发阶段兼容逻辑：如果BootInfo区域从未写入，仍允许通过向量表检查后启动Application。这样可以先用ST-Link分别烧录两个ELF文件，验证跳转，再进行IAP测试。

一旦BootInfo中出现`UPDATING`状态，BootLoader将不会启动Application，即使前几个固件数据包已经写入了有效向量表，也必须等完整CRC校验通过后才能启动。

### 步骤7：关闭外设并跳转Application

不能只调用Application的Reset_Handler。BootLoader已经使用了SysTick、USART1、RCC和NVIC，如果把这些状态直接留给Application，可能出现以下问题：

- SysTick在Application完成初始化前触发
- USART1中断或其他挂起中断使用错误的向量表
- Application根据错误的时钟状态配置PLL
- NVIC仍保留BootLoader设置的使能位和挂起位

完整跳转代码如下：

```c
void BootLoader_JumpToApplication(void)
{
    uint32_t app_stack;
    uint32_t app_reset;
    ApplicationEntry_t app_entry;

    if (!BootLoader_IsVectorTableValid())
    {
        return;
    }

    app_stack = *(__IO uint32_t *)APP_START_ADDR;
    app_reset = *(__IO uint32_t *)(APP_START_ADDR + 4U);
    app_entry = (ApplicationEntry_t)app_reset;

    __disable_irq();

    HAL_UART_DeInit(&huart1);

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL = 0U;

    for (uint32_t index = 0U; index < 8U; index++)
    {
        NVIC->ICER[index] = 0xFFFFFFFFU;
        NVIC->ICPR[index] = 0xFFFFFFFFU;
    }

    HAL_RCC_DeInit();
    HAL_DeInit();

    SCB->VTOR = APP_START_ADDR;
    __DSB();
    __ISB();

    __set_CONTROL(0U);
    __set_MSP(app_stack);
    __DSB();
    __ISB();

    __enable_irq();
    app_entry();

    while (1)
    {
    }
}
```

跳转后的执行路径为：

```text
BootLoader_JumpToApplication()
        │
        ├── 设置MSP为Application向量表中的初始值
        │
        └── 跳转Application Reset_Handler
                    │
                    ├── 复制.data
                    ├── 清零.bss
                    ├── 调用SystemInit()
                    ├── 设置VTOR = 0x08006000
                    └── 调用Application main()
```

### 步骤8：实现STM32G4 Flash操作

STM32G431与原教程常见的STM32F407扇区模型不同：

- STM32G431按2KB页擦除
- Application占Page12~Page59，共48页
- Flash编程单位为64位双字
- 写入地址必须8字节对齐
- Flash只能把位从1写成0，重新写入前必须擦除对应页

创建Flash接口头文件：

```c
// BootLoader/Core/Inc/flash_if.h
#ifndef FLASH_IF_H
#define FLASH_IF_H

#include "bootloader.h"

HAL_StatusTypeDef FlashIf_EraseApplication(void);
HAL_StatusTypeDef FlashIf_WriteApplication(uint32_t address,
                                           const uint8_t *data,
                                           uint32_t length);
HAL_StatusTypeDef FlashIf_WriteBootInfo(const BootInfo_t *info);
uint32_t FlashIf_CalculateCrc32(uint32_t address, uint32_t length);

#endif
```

实现页擦除、双字写入和软件CRC32：

```c
// BootLoader/Core/Src/flash_if.c
#include "flash_if.h"
#include <string.h>

static HAL_StatusTypeDef FlashIf_ErasePages(uint32_t first_page,
                                            uint32_t page_count)
{
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0xFFFFFFFFU;
    HAL_StatusTypeDef status;

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
    {
        return status;
    }

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks = FLASH_BANK_1;
    erase.Page = first_page;
    erase.NbPages = page_count;

    status = HAL_FLASHEx_Erase(&erase, &page_error);
    HAL_FLASH_Lock();

    return status;
}

static HAL_StatusTypeDef FlashIf_Program(uint32_t address,
                                         const uint8_t *data,
                                         uint32_t length,
                                         uint32_t region_start,
                                         uint32_t region_end)
{
    HAL_StatusTypeDef status;

    if ((data == NULL) ||
        (length == 0U) ||
        ((address & 0x7U) != 0U) ||
        ((length & 0x7U) != 0U) ||
        (address < region_start) ||
        (address > region_end) ||
        (length > (region_end - address)))
    {
        return HAL_ERROR;
    }

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
    {
        return status;
    }

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);

    for (uint32_t offset = 0U; offset < length; offset += 8U)
    {
        uint64_t value;

        memcpy(&value, &data[offset], sizeof(value));

        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                                   address + offset,
                                   value);
        if (status != HAL_OK)
        {
            break;
        }

        if (*(__IO uint64_t *)(address + offset) != value)
        {
            status = HAL_ERROR;
            break;
        }
    }

    HAL_FLASH_Lock();
    return status;
}

HAL_StatusTypeDef FlashIf_EraseApplication(void)
{
    return FlashIf_ErasePages(APP_FIRST_PAGE, APP_PAGE_COUNT);
}

HAL_StatusTypeDef FlashIf_WriteApplication(uint32_t address,
                                           const uint8_t *data,
                                           uint32_t length)
{
    return FlashIf_Program(address,
                           data,
                           length,
                           APP_START_ADDR,
                           APP_END_ADDR);
}

HAL_StatusTypeDef FlashIf_WriteBootInfo(const BootInfo_t *info)
{
    HAL_StatusTypeDef status;

    if (info == NULL)
    {
        return HAL_ERROR;
    }

    status = FlashIf_ErasePages(BOOTINFO_FIRST_PAGE,
                                BOOTINFO_PAGE_COUNT);
    if (status != HAL_OK)
    {
        return status;
    }

    return FlashIf_Program(BOOTINFO_START_ADDR,
                           (const uint8_t *)info,
                           sizeof(BootInfo_t),
                           BOOTINFO_START_ADDR,
                           BOOTINFO_START_ADDR + BOOTINFO_SIZE);
}

uint32_t FlashIf_CalculateCrc32(uint32_t address, uint32_t length)
{
    const uint8_t *data = (const uint8_t *)address;
    uint32_t crc = 0xFFFFFFFFU;

    for (uint32_t index = 0U; index < length; index++)
    {
        crc ^= data[index];

        for (uint32_t bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 1U) != 0U)
            {
                crc = (crc >> 1U) ^ 0xEDB88320U;
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    return ~crc;
}
```

应用区擦除参数必须是：

```c
erase.Page = 12U;
erase.NbPages = 48U;
```

因为Page12~Page59是包含首尾页的范围：

```text
59 - 12 + 1 = 48页
48 × 2KB = 96KB
```

Data分区的Page60~Page61和BootInfo分区的Page62~Page63不会被Application擦除函数影响。

### 步骤9：设计USART基础IAP协议

当前工程没有升级按键，因此使用串口字符`U`作为升级触发符：

1. BootLoader复位后等待1秒
2. 上位机在1秒内发送ASCII字符`U`
3. BootLoader进入IAP命令循环
4. 如果Application无效或BootInfo状态为`UPDATING`，不等待触发字符，直接进入IAP

#### 请求帧格式

```text
+--------+--------+--------+--------+----------+--------+--------+
| SOF    | CMD    | LEN_L  | LEN_H  | PAYLOAD  | CRC_L  | CRC_H  |
+--------+--------+--------+--------+----------+--------+--------+
| 0x5A   | 1字节  | 2字节小端       | LEN字节   | CRC16小端       |
+--------+--------+--------+--------+----------+--------+--------+
```

CRC16采用CRC-16/CCITT-FALSE：

- 初值：`0xFFFF`
- 多项式：`0x1021`
- 输入不反转
- 输出不反转
- 计算范围：`CMD + LEN_L + LEN_H + PAYLOAD`

#### 响应帧格式

```text
+--------+--------+--------+--------+--------+----------+--------+--------+
| SOF    | CMD    | STATUS | LEN_L  | LEN_H  | PAYLOAD  | CRC_L  | CRC_H  |
+--------+--------+--------+--------+--------+----------+--------+--------+
| 0xA5   | 1字节  | 1字节  | 2字节小端       | LEN字节   | CRC16小端       |
+--------+--------+--------+--------+--------+----------+--------+--------+
```

命令定义如下：

| 命令 | 数值 | 请求数据 | 作用 |
|---|---:|---|---|
| `IAP_CMD_GET_INFO` | `0xA1` | 无 | 读取BootInfo |
| `IAP_CMD_ERASE_APP` | `0xA2` | size、crc32、version | 标记升级中并擦除Application |
| `IAP_CMD_WRITE_BLOCK` | `0xA3` | offset、固件数据 | 将一个数据块写入Application |
| `IAP_CMD_VERIFY` | `0xA4` | 无 | 计算CRC32并写入有效BootInfo |
| `IAP_CMD_JUMP_APP` | `0xA5` | 无 | 校验并跳转Application |

创建协议头文件：

```c
// BootLoader/Core/Inc/iap_uart.h
#ifndef IAP_UART_H
#define IAP_UART_H

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

typedef enum
{
    IAP_STATUS_OK = 0x00U,
    IAP_STATUS_BAD_FRAME = 0x01U,
    IAP_STATUS_BAD_LENGTH = 0x02U,
    IAP_STATUS_BAD_STATE = 0x03U,
    IAP_STATUS_FLASH_ERROR = 0x04U,
    IAP_STATUS_CRC_ERROR = 0x05U,
    IAP_STATUS_INVALID_APP = 0x06U,
    IAP_STATUS_UNKNOWN_COMMAND = 0x07U
} IapStatus_t;

bool IapUart_WaitForTrigger(uint32_t timeout_ms);
void IapUart_Run(void);

#endif
```

基础协议实现如下：

```c
// BootLoader/Core/Src/iap_uart.c
#include "iap_uart.h"
#include "flash_if.h"
#include "usart.h"
#include <string.h>

typedef struct
{
    uint8_t command;
    uint16_t length;
    uint8_t payload[IAP_MAX_PAYLOAD_SIZE];
} IapFrame_t;

typedef struct
{
    bool active;
    uint32_t app_size;
    uint32_t app_crc32;
    uint32_t app_version;
} IapSession_t;

static IapSession_t session;

static uint16_t IapUart_Crc16Update(uint16_t crc, uint8_t data)
{
    crc ^= (uint16_t)data << 8U;

    for (uint32_t bit = 0U; bit < 8U; bit++)
    {
        if ((crc & 0x8000U) != 0U)
        {
            crc = (uint16_t)((crc << 1U) ^ 0x1021U);
        }
        else
        {
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

    for (uint32_t index = 0U; index < frame->length; index++)
    {
        crc = IapUart_Crc16Update(crc, frame->payload[index]);
    }

    return crc;
}

static uint32_t IapUart_ReadLe32(const uint8_t *data)
{
    return ((uint32_t)data[0]) |
           ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

static HAL_StatusTypeDef IapUart_ReceiveFrame(IapFrame_t *frame,
                                              IapStatus_t *frame_status)
{
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
                                 uint16_t length)
{
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

static IapStatus_t IapUart_HandleErase(const IapFrame_t *frame)
{
    BootInfo_t info = {0};

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
    info.reserved0 = 0xFFFFFFFFU;
    info.reserved1 = 0xFFFFFFFFU;

    if (FlashIf_WriteBootInfo(&info) != HAL_OK)
    {
        return IAP_STATUS_FLASH_ERROR;
    }

    if (FlashIf_EraseApplication() != HAL_OK)
    {
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

    if (!session.active)
    {
        return IAP_STATUS_BAD_STATE;
    }

    if (frame->length < 12U)
    {
        return IAP_STATUS_BAD_LENGTH;
    }

    offset = IapUart_ReadLe32(&frame->payload[0]);
    data_length = (uint32_t)frame->length - 4U;
    padded_firmware_size = (session.app_size + 7U) & ~7U;

    if (((offset & 0x7U) != 0U) ||
        ((data_length & 0x7U) != 0U) ||
        (data_length > IAP_MAX_BLOCK_SIZE) ||
        (offset > padded_firmware_size) ||
        (data_length > (padded_firmware_size - offset)))
    {
        return IAP_STATUS_BAD_LENGTH;
    }

    if (FlashIf_WriteApplication(APP_START_ADDR + offset,
                                 &frame->payload[4],
                                 data_length) != HAL_OK)
    {
        return IAP_STATUS_FLASH_ERROR;
    }

    return IAP_STATUS_OK;
}

static IapStatus_t IapUart_HandleVerify(void)
{
    BootInfo_t info = {0};
    uint32_t calculated_crc;

    if (!session.active)
    {
        return IAP_STATUS_BAD_STATE;
    }

    calculated_crc = FlashIf_CalculateCrc32(APP_START_ADDR,
                                            session.app_size);
    if (calculated_crc != session.app_crc32)
    {
        return IAP_STATUS_CRC_ERROR;
    }

    info.magic = BOOTINFO_MAGIC;
    info.state = BOOTINFO_STATE_VALID;
    info.app_addr = APP_START_ADDR;
    info.app_size = session.app_size;
    info.app_crc32 = session.app_crc32;
    info.app_version = session.app_version;
    info.reserved0 = 0xFFFFFFFFU;
    info.reserved1 = 0xFFFFFFFFU;

    if (FlashIf_WriteBootInfo(&info) != HAL_OK)
    {
        return IAP_STATUS_FLASH_ERROR;
    }

    session.active = false;
    return IAP_STATUS_OK;
}

bool IapUart_WaitForTrigger(uint32_t timeout_ms)
{
    uint8_t trigger;

    if (HAL_UART_Receive(&huart1, &trigger, 1U, timeout_ms) != HAL_OK)
    {
        return false;
    }

    return (trigger == (uint8_t)IAP_TRIGGER_CHAR) ||
           (trigger == (uint8_t)'u');
}

void IapUart_Run(void)
{
    IapFrame_t frame;
    BootInfo_t info;
    const char ready_message[] = "IAP ready\r\n";

    memset(&session, 0, sizeof(session));
    BootInfo_Read(&info);

    if ((info.magic == BOOTINFO_MAGIC) &&
        (info.state == BOOTINFO_STATE_UPDATING) &&
        (info.app_addr == APP_START_ADDR) &&
        (info.app_size > 0U) &&
        (info.app_size <= APP_SIZE))
    {
        session.active = true;
        session.app_size = info.app_size;
        session.app_crc32 = info.app_crc32;
        session.app_version = info.app_version;
    }

    HAL_UART_Transmit(&huart1,
                      (uint8_t *)ready_message,
                      sizeof(ready_message) - 1U,
                      HAL_MAX_DELAY);

    while (1)
    {
        IapStatus_t receive_status;
        IapStatus_t command_status;

        if (IapUart_ReceiveFrame(&frame, &receive_status) != HAL_OK)
        {
            IapUart_SendResponse(frame.command,
                                 receive_status,
                                 NULL,
                                 0U);
            continue;
        }

        switch (frame.command)
        {
            case IAP_CMD_GET_INFO:
                if (frame.length != 0U)
                {
                    command_status = IAP_STATUS_BAD_LENGTH;
                    IapUart_SendResponse(frame.command,
                                         command_status,
                                         NULL,
                                         0U);
                    break;
                }

                BootInfo_Read(&info);
                IapUart_SendResponse(frame.command,
                                     IAP_STATUS_OK,
                                     (const uint8_t *)&info,
                                     sizeof(info));
                break;

            case IAP_CMD_ERASE_APP:
                command_status = IapUart_HandleErase(&frame);
                IapUart_SendResponse(frame.command,
                                     command_status,
                                     NULL,
                                     0U);
                break;

            case IAP_CMD_WRITE_BLOCK:
                command_status = IapUart_HandleWrite(&frame);
                IapUart_SendResponse(frame.command,
                                     command_status,
                                     NULL,
                                     0U);
                break;

            case IAP_CMD_VERIFY:
                command_status = (frame.length == 0U) ?
                                 IapUart_HandleVerify() :
                                 IAP_STATUS_BAD_LENGTH;
                IapUart_SendResponse(frame.command,
                                     command_status,
                                     NULL,
                                     0U);
                break;

            case IAP_CMD_JUMP_APP:
                command_status = (frame.length == 0U) &&
                                 BootLoader_IsApplicationBootable() ?
                                 IAP_STATUS_OK :
                                 IAP_STATUS_INVALID_APP;
                IapUart_SendResponse(frame.command,
                                     command_status,
                                     NULL,
                                     0U);

                if (command_status == IAP_STATUS_OK)
                {
                    HAL_Delay(20U);
                    BootLoader_JumpToApplication();
                }
                break;

            default:
                IapUart_SendResponse(frame.command,
                                     IAP_STATUS_UNKNOWN_COMMAND,
                                     NULL,
                                     0U);
                break;
        }
    }
}
```

!!! note "关于断点续传"
    BootInfo会保存本次升级的固件大小、CRC32和版本，因此复位后仍能识别“升级中”状态，但这个最小实现没有记录已经写到哪个数据块。重新连接后，最稳妥的做法是再次发送`ERASE_APP`并从头下载。

### 步骤10：整合BootInfo和主程序

BootInfo状态变化如下：

```text
Flash全擦除
    │
    ├── BootInfo为0xFFFFFFFF：允许ST-Link直接烧录的Application启动
    │
收到ERASE_APP
    │
    ├── 写入UPDATING状态
    ├── 擦除Application
    └── 接收并写入固件
            │
收到VERIFY且CRC正确
            │
            ├── 写入VALID状态
            └── 允许跳转Application
```

在`BootLoader/Core/Src/main.c`中加入用户头文件：

```c
/* USER CODE BEGIN Includes */
#include "bootloader.h"
#include "iap_uart.h"
/* USER CODE END Includes */
```

然后在外设初始化完成后的`USER CODE BEGIN 2`区域加入：

```c
/* USER CODE BEGIN 2 */
const char banner[] =
    "\r\n"
    "========================================\r\n"
    "  STM32G431 Simple BootLoader\r\n"
    "  APP: 0x08006000, USART1: 115200 8N1\r\n"
    "========================================\r\n";

HAL_UART_Transmit(&huart1,
                  (uint8_t *)banner,
                  sizeof(banner) - 1U,
                  HAL_MAX_DELAY);

if (BootInfo_IsUpdating() ||
    !BootLoader_IsApplicationBootable())
{
    const char wait_message[] =
        "Application unavailable, enter IAP\r\n";

    HAL_UART_Transmit(&huart1,
                      (uint8_t *)wait_message,
                      sizeof(wait_message) - 1U,
                      HAL_MAX_DELAY);
    IapUart_Run();
}

{
    const char trigger_message[] =
        "Send 'U' within 1000ms to enter IAP\r\n";

    HAL_UART_Transmit(&huart1,
                      (uint8_t *)trigger_message,
                      sizeof(trigger_message) - 1U,
                      HAL_MAX_DELAY);
}

if (IapUart_WaitForTrigger(1000U))
{
    IapUart_Run();
}

{
    const char jump_message[] = "Jump to Application\r\n";

    HAL_UART_Transmit(&huart1,
                      (uint8_t *)jump_message,
                      sizeof(jump_message) - 1U,
                      HAL_MAX_DELAY);
}

HAL_Delay(20U);
BootLoader_JumpToApplication();

IapUart_Run();
/* USER CODE END 2 */
```

最后一行`IapUart_Run()`是跳转失败后的保护路径。正常情况下，`BootLoader_JumpToApplication()`不会返回。

### 步骤11：配置和验证Application

Application需要完成两项关键配置：

1. 链接脚本Flash起始地址为`0x08006000`，长度为96KB
2. `system_stm32g4xx.c`设置`VECT_TAB_OFFSET = 0x00006000U`

Application当前会创建FreeRTOS任务并启动调度器。为了验证BootLoader确实跳转成功，可以采用以下任一方式：

- 在`Application/Core/Src/main.c`的`main()`入口设置断点
- 在`osKernelStart()`前设置断点
- 在现有`App_DebugTask`中增加一个`volatile`计数器并观察递增
- 使用当前硬件上已确定的LED或调试引脚输出运行状态

不要在不知道开发板原理图的情况下随意假设某个GPIO连接了LED。

## 实践示例

### 示例1：先验证最小跳转

在实现完整IAP前，先只验证链接地址、向量表和跳转逻辑。

**步骤1：编译BootLoader**

```bash
cd BootLoader
cmake --preset Debug
cmake --build --preset Debug
```

构建产物位于：

```text
BootLoader/build/Debug/BootLoader.elf
BootLoader/build/Debug/BootLoader.map
```

**步骤2：检查BootLoader向量表地址**

```bash
arm-none-eabi-objdump -h build/Debug/BootLoader.elf
```

预期`.isr_vector`地址为：

```text
08000000
```

**步骤3：编译Application**

```bash
cd ../Application
cmake --preset Debug
cmake --build --preset Debug
```

**步骤4：检查Application向量表地址**

```bash
arm-none-eabi-objdump -h build/Debug/Application.elf
```

预期`.isr_vector`地址为：

```text
08006000
```

**步骤5：分别烧录两个ELF**

1. 将`BootLoader.elf`烧录到芯片
2. 将`Application.elf`烧录到芯片
3. 复位芯片
4. 打开USART1串口，设置115200、8N1

预期BootLoader输出：

```text
========================================
  STM32G431 Simple BootLoader
  APP: 0x08006000, USART1: 115200 8N1
========================================
Send 'U' within 1000ms to enter IAP
Jump to Application
```

如果没有发送`U`，BootLoader应在1秒后跳转到Application。

### 示例2：生成Application二进制文件

IAP上位机需要发送不带ELF元数据的原始二进制文件：

```bash
cd Application
arm-none-eabi-objcopy \
  -O binary \
  build/Debug/Application.elf \
  build/Debug/Application.bin
```

检查文件大小：

```bash
ls -lh build/Debug/Application.bin
```

文件必须满足：

```text
0 < Application.bin大小 <= 98304字节
```

Application的链接地址已经是`0x08006000`，上位机发送时不需要把前面的24KB BootLoader空间补成`0xFF`。

### 示例3：使用Python上位机下载固件

下面的脚本与步骤9中的协议对应。它会：

1. 打开串口
2. 发送`U`进入IAP
3. 计算原始Application.bin的CRC32
4. 发送`ERASE_APP`
5. 按256字节分包，末包补`0xFF`到8字节对齐
6. 发送`VERIFY`
7. 发送`JUMP_APP`

```python
#!/usr/bin/env python3
import argparse
import struct
import time
import zlib

import serial

REQUEST_SOF = 0x5A
RESPONSE_SOF = 0xA5

CMD_GET_INFO = 0xA1
CMD_ERASE_APP = 0xA2
CMD_WRITE_BLOCK = 0xA3
CMD_VERIFY = 0xA4
CMD_JUMP_APP = 0xA5

STATUS_OK = 0x00
BLOCK_SIZE = 256
MAX_APP_SIZE = 96 * 1024


def crc16_ccitt(data: bytes) -> int:
    crc = 0xFFFF
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def read_exact(port: serial.Serial, length: int) -> bytes:
    data = port.read(length)
    if len(data) != length:
        raise TimeoutError(f"expected {length} bytes, received {len(data)}")
    return data


def send_frame(port: serial.Serial, command: int, payload: bytes = b"") -> None:
    body = bytes([command]) + struct.pack("<H", len(payload)) + payload
    frame = bytes([REQUEST_SOF]) + body + struct.pack("<H", crc16_ccitt(body))
    port.write(frame)
    port.flush()


def receive_response(port: serial.Serial, expected_command: int) -> bytes:
    while True:
        sof = read_exact(port, 1)[0]
        if sof == RESPONSE_SOF:
            break

    header = read_exact(port, 4)
    command = header[0]
    status = header[1]
    length = struct.unpack_from("<H", header, 2)[0]
    payload = read_exact(port, length)
    received_crc = struct.unpack("<H", read_exact(port, 2))[0]
    calculated_crc = crc16_ccitt(header + payload)

    if command != expected_command:
        raise RuntimeError(
            f"response command mismatch: 0x{command:02X}"
        )
    if received_crc != calculated_crc:
        raise RuntimeError("response CRC16 mismatch")
    if status != STATUS_OK:
        raise RuntimeError(
            f"device returned status 0x{status:02X} for command 0x{command:02X}"
        )

    return payload


def request(port: serial.Serial, command: int, payload: bytes = b"") -> bytes:
    send_frame(port, command, payload)
    return receive_response(port, command)


def upload(port_name: str, firmware_path: str, version: int) -> None:
    with open(firmware_path, "rb") as firmware_file:
        firmware = firmware_file.read()

    if not firmware:
        raise ValueError("firmware file is empty")
    if len(firmware) > MAX_APP_SIZE:
        raise ValueError(
            f"firmware is {len(firmware)} bytes, maximum is {MAX_APP_SIZE}"
        )

    firmware_crc32 = zlib.crc32(firmware) & 0xFFFFFFFF
    padded_firmware = firmware + b"\xFF" * ((-len(firmware)) % 8)

    print(f"Firmware size : {len(firmware)} bytes")
    print(f"Firmware CRC32: 0x{firmware_crc32:08X}")
    print(f"Version       : {version}")

    with serial.Serial(port_name, 115200, timeout=5) as port:
        time.sleep(0.1)

        # 设备复位后1秒内发送；若设备已因无有效App停留在IAP，也可直接发送。
        port.write(b"U")
        port.flush()
        time.sleep(0.1)
        port.reset_input_buffer()

        erase_payload = struct.pack(
            "<III",
            len(firmware),
            firmware_crc32,
            version,
        )

        print("Erasing Application...")
        request(port, CMD_ERASE_APP, erase_payload)

        total_blocks = (len(padded_firmware) + BLOCK_SIZE - 1) // BLOCK_SIZE

        for block_index, offset in enumerate(
            range(0, len(padded_firmware), BLOCK_SIZE),
            start=1,
        ):
            block = padded_firmware[offset:offset + BLOCK_SIZE]
            payload = struct.pack("<I", offset) + block
            request(port, CMD_WRITE_BLOCK, payload)
            print(
                f"Writing block {block_index}/{total_blocks}",
                end="\r",
                flush=True,
            )

        print()
        print("Verifying CRC32...")
        request(port, CMD_VERIFY)

        print("Jumping to Application...")
        request(port, CMD_JUMP_APP)

    print("Upload complete")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="STM32G431 SimpleBootLoader uploader"
    )
    parser.add_argument("port", help="serial port, for example /dev/tty.usbserial-0001")
    parser.add_argument("firmware", help="Application.bin path")
    parser.add_argument("--version", type=int, default=1)
    args = parser.parse_args()

    upload(args.port, args.firmware, args.version)


if __name__ == "__main__":
    main()
```

安装依赖：

```bash
python3 -m pip install pyserial
```

使用示例：

```bash
python3 firmware_uploader.py \
  /dev/tty.usbserial-0001 \
  Application/build/Debug/Application.bin \
  --version 1
```

Windows示例：

```powershell
python firmware_uploader.py COM3 Application.bin --version 1
```

!!! note "进入IAP的时机"
    如果设备中已有有效Application，需要先复位设备，并在BootLoader输出等待提示后的1秒内启动上传脚本。若Application无效或上次升级未完成，BootLoader会一直停留在IAP，不受1秒窗口限制。

## 深入理解

### Cortex-M4复位时读取了什么

STM32G431复位后，处理器首先读取Flash起始位置的两个字：

```text
0x08000000 -> BootLoader初始MSP
0x08000004 -> BootLoader Reset_Handler
```

Application位于`0x08006000`，CPU不会在硬件复位时自动读取这里。BootLoader跳转时需要模拟这个过程：

```c
uint32_t app_stack = *(__IO uint32_t *)0x08006000U;
uint32_t app_reset = *(__IO uint32_t *)0x08006004U;

SCB->VTOR = 0x08006000U;
__set_MSP(app_stack);
((void (*)(void))app_reset)();
```

### 为什么Application必须重新运行Reset_Handler

Application不是从`main()`直接开始。它的`Reset_Handler`负责：

- 设置运行时环境
- 把`.data`初值从Flash复制到RAM
- 清零`.bss`
- 调用`SystemInit()`
- 调用C/C++运行库初始化
- 最后进入`main()`

直接跳转到Application的`main()`会跳过这些步骤，普通全局变量、静态变量和系统时钟都可能处于错误状态。

### 为什么Flash必须先擦除

Flash擦除后所有位都为1：

```text
擦除状态：0xFF
```

编程只能把1变为0。例如：

```text
0xFF -> 0xA5  可以
0xA5 -> 0xFF  不可以，必须先擦除
```

STM32G431最小擦除单位为2KB页，但最小编程单位为8字节双字。因此：

- 擦除地址按页规划
- 写入地址按8字节对齐
- 上位机把固件末尾补`0xFF`到8字节整数倍
- CRC32只计算原始固件长度，不计算补齐字节

### 为什么先写UPDATING再擦除Application

如果先擦除Application，再写升级状态，中间掉电时BootInfo可能仍然显示旧固件有效。先写`UPDATING`可以保证：

1. 任何后续复位都会停留在BootLoader
2. 不会误跳转到已经擦除或只写入一部分的Application
3. 用户可以重新执行完整升级

### BootInfo为什么独占两个Flash页

BootInfo结构只有32字节，但Flash擦除单位是2KB。README为BootInfo预留4KB，也就是两个页。这样后续可以扩展：

- 固件构建时间
- 硬件版本
- 升级计数
- 错误码
- 下载进度
- 多份BootInfo轮换记录

本教程为了保持简单，每次更新BootInfo都会擦除Page62~Page63并从起始位置重写一份结构体。

### 当前方案的掉电行为

| 掉电位置 | 下次启动行为 |
|---|---|
| 未进入升级 | 正常校验并启动Application |
| 已写UPDATING，尚未擦除App | 停留IAP |
| 正在擦除或写入App | 停留IAP |
| 固件写完但未VERIFY | 停留IAP |
| VERIFY成功并写入VALID | 校验CRC后启动Application |

没有备份分区意味着升级中断后不能恢复旧固件，只能重新下载。

### BootLoader与Application时钟可以不同吗

可以。当前工程就是这种情况：

- BootLoader使用HSI 16MHz，不启用PLL
- Application使用HSE和PLL，目标系统时钟更高

BootLoader跳转前调用`HAL_RCC_DeInit()`，Application进入自己的`Reset_Handler`后重新执行`SystemClock_Config()`。两个工程不需要使用相同系统时钟，但必须各自正确初始化。

## 常见问题

### Q1：为什么Application链接后仍从0x08000000开始？

**A**：检查以下内容：

1. 修改的是`Application/STM32G431XX_FLASH.ld`
2. CMake工具链确实使用`${CMAKE_SOURCE_DIR}/STM32G431XX_FLASH.ld`
3. 修改后重新执行CMake配置和构建
4. 使用`arm-none-eabi-objdump -h`检查`.isr_vector`

正确结果必须是：

```text
.isr_vector  0x08006000
```

### Q2：为什么跳转后立刻HardFault？

**A**：重点检查：

1. Application初始MSP是否在`0x20000000 ~ 0x20005800`
2. 两个链接脚本是否仍错误使用连续32KB RAM
3. Application Reset_Handler是否位于`0x08006000 ~ 0x0801DFFF`
4. Reset_Handler地址最低位是否为1
5. `SCB->VTOR`是否为`0x08006000`
6. BootLoader是否清理SysTick和NVIC

可以在跳转前通过调试器查看：

```c
uint32_t app_stack = *(__IO uint32_t *)APP_START_ADDR;
uint32_t app_reset = *(__IO uint32_t *)(APP_START_ADDR + 4U);
```

### Q3：为什么Application进入main后不响应中断？

**A**：通常是向量表未重定位。确认`Application/Core/Src/system_stm32g4xx.c`中：

```c
#define USER_VECT_TAB_ADDRESS
#define VECT_TAB_OFFSET 0x00006000U
```

运行后检查：

```c
SCB->VTOR == 0x08006000U
```

### Q4：为什么Flash写入返回错误？

**A**：检查以下几点：

1. 写入地址是否8字节对齐
2. 数据长度是否为8字节整数倍
3. 地址是否位于Application区域
4. 对应Flash页是否已经擦除
5. 是否错误擦除了BootLoader、Data或BootInfo区域
6. 芯片是否启用了写保护

STM32G431应使用：

```c
HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                  address,
                  value64);
```

不要照搬STM32F4教程中的32位Word编程方式。

### Q5：为什么CRC16正确但最终CRC32失败？

**A**：两个CRC用途不同：

- CRC16检查单个串口帧
- CRC32检查完整Application.bin

常见原因包括：

1. 上位机发送的文件不是原始`.bin`
2. offset计算错误，数据块写入位置不正确
3. 最后一包补齐后，把补齐字节也算进了CRC32
4. 某一包重复写入或漏写
5. 固件实际大小与`ERASE_APP`携带的size不同

本教程规定CRC32只计算原始固件长度。

### Q6：Python脚本发送U后没有收到响应怎么办？

**A**：检查：

1. TX和RX是否交叉连接
2. 是否连接公共GND
3. 串口是否为115200、8N1
4. 是否在设备复位后1秒内发送`U`
5. 是否有其他串口助手占用同一端口
6. Python脚本中的串口设备名是否正确

macOS可以列出串口：

```bash
ls /dev/tty.*
ls /dev/cu.*
```

### Q7：为什么直接烧录Application.elf可以运行，IAP下载后不能运行？

**A**：依次检查：

1. IAP发送的是`Application.bin`，不是ELF文件
2. `Application.bin`由链接到`0x08006000`的ELF生成
3. 第一包offset为0
4. Flash实际地址为`APP_START_ADDR + offset`
5. VERIFY返回成功并写入VALID状态
6. 读取`0x08006000`和`0x08006004`确认向量表内容合理

### Q8：CubeMX重新生成代码后跳转失效怎么办？

**A**：CubeMX可能重新生成`system_stm32g4xx.c`和CMake辅助文件。重新生成后检查：

- Application的`USER_VECT_TAB_ADDRESS`
- Application的`VECT_TAB_OFFSET`
- 两个链接脚本的Flash和RAM配置
- 用户源文件是否仍在根`CMakeLists.txt`的`target_sources()`中
- `main.c`代码是否放在`USER CODE`区间

### Q9：如何缩短正常启动时间？

**A**：当前正常启动固定等待1秒串口触发。可以采用：

- 将等待时间缩短到100~300ms
- 由Application在BootInfo中写升级请求，再复位进入BootLoader
- 使用一个专用GPIO按键进入升级
- 只在接收到特定串口前导字节时延长等待

开发阶段建议保留1秒，便于观察和调试。

### Q10：这个BootLoader能防止恶意固件吗？

**A**：不能。CRC32只能检查传输错误和数据损坏，不能证明固件来源。要防止恶意固件，需要增加数字签名验证、密钥管理、读写保护和安全启动，这些不属于当前“基础IAP”目标。

## 总结

通过本教程，我们基于当前STM32G431工程完成了一个基础BootLoader设计，核心内容包括：

- **Flash分区**：BootLoader 24KB、Application 96KB、Data 4KB、BootInfo 4KB
- **地址纠正**：按页号把Data区域确定为`0x0801E000 ~ 0x0801EFFF`
- **链接配置**：两个工程使用不同Flash起始地址
- **RAM配置**：主RAM按连续22KB使用，CCMSRAM单独声明
- **向量表重定位**：Application使用`VTOR = 0x08006000`
- **Application检查**：验证MSP、Thumb位和Reset_Handler范围
- **安全跳转**：清理USART、SysTick、NVIC和RCC后进入Application Reset_Handler
- **Flash操作**：按2KB页擦除、按64位双字编程
- **基础IAP**：USART1分包传输、CRC16帧校验、CRC32固件校验
- **BootInfo状态**：使用UPDATING和VALID防止启动不完整固件
- **上位机示例**：使用Python和pyserial发送Application.bin

这个实现适合作为BootLoader验证工程和后续扩展基础。它刻意没有加入备份恢复区，因此结构简单、占用空间小，但升级过程中掉电后必须重新下载固件。

后续可以在当前结构上继续增加：

- Application主动请求升级
- 数据包序号、重传和断点续传
- BootInfo双副本或磨损均衡
- 硬件CRC加速
- CAN、USB或RS485升级接口
- 固件数字签名
- BootLoader写保护
- 双区升级和失败回滚

## 延伸阅读

建议进一步阅读：

- STM32G4系列参考手册中的Flash章节
- STM32G431数据手册中的存储器映射章节
- ARM Cortex-M4 Devices Generic User Guide中的VTOR、MSP和异常模型
- ST应用笔记AN2606：STM32系统存储器Bootloader
- ST应用笔记AN3155：USART Bootloader协议
- STM32CubeG4 HAL Flash驱动接口说明

当前工程中可以直接对照的文件：

- `README.md`：项目目标和Flash分区
- `BootLoader/BootLoader.ioc`：USART1和时钟配置
- `BootLoader/Core/Src/main.c`：BootLoader入口
- `BootLoader/Core/Src/usart.c`：USART1初始化
- `BootLoader/STM32G431XX_FLASH.ld`：BootLoader链接地址
- `Application/Core/Src/main.c`：Application和FreeRTOS入口
- `Application/Core/Src/system_stm32g4xx.c`：向量表重定位
- `Application/STM32G431XX_FLASH.ld`：Application链接地址

## 练习题

1. **基础练习**：只实现Application向量表检查和跳转，使用ST-Link分别烧录两个ELF，确认Application能够运行。

2. **地址练习**：根据2KB页大小，手工计算Page11、Page12、Page59、Page60和Page62的起始地址。

3. **Flash练习**：增加一个函数，在不擦除Data分区的前提下，只擦除BootInfo的Page62~Page63。

4. **协议练习**：为`WRITE_BLOCK`命令增加数据包序号，并拒绝乱序数据包。

5. **重传练习**：在Python上位机中为每个命令增加最多3次重试。

6. **断点续传练习**：在BootInfo中保存最后成功写入的offset，复位后从该位置继续传输。

7. **启动优化练习**：把串口升级窗口从1000ms缩短到200ms，并测试上位机是否仍能稳定进入IAP。

8. **Application通信练习**：让Application写入升级请求状态并调用`NVIC_SystemReset()`，使设备无需人工复位即可进入BootLoader。

9. **可靠性练习**：在固件下载的不同阶段人为复位设备，记录BootInfo状态和BootLoader行为。

10. **挑战练习**：重新规划128KB Flash，在保证Application可用空间的前提下，设计一个最小备份或恢复方案，并说明其容量限制。

---

**下一步**：先完成“链接地址 + RAM配置 + VTOR + 最小跳转”验证，再加入Flash写入和串口IAP。BootLoader调试应始终分阶段进行，不要在基础跳转尚未稳定时同时排查协议和Flash编程问题。
