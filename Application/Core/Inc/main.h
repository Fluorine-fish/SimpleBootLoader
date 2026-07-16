/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PWM_PERIOD_CYCLES 8500
#define TIM_CLOCK_DIVIDER 1
#define REP_COUNTER 1
#define DEAD_TIME_COUNTS 170
#define TEST_PIN2_Pin GPIO_PIN_0
#define TEST_PIN2_GPIO_Port GPIOC
#define VBUS_Pin GPIO_PIN_1
#define VBUS_GPIO_Port GPIOC
#define SNS_U_Pin GPIO_PIN_2
#define SNS_U_GPIO_Port GPIOC
#define SNS_V_Pin GPIO_PIN_3
#define SNS_V_GPIO_Port GPIOC
#define SNS_W_Pin GPIO_PIN_0
#define SNS_W_GPIO_Port GPIOA
#define M1_NTC_Pin GPIO_PIN_4
#define M1_NTC_GPIO_Port GPIOC
#define GD_WAKE_Pin GPIO_PIN_7
#define GD_WAKE_GPIO_Port GPIOE
#define M1_PWM_UL_Pin GPIO_PIN_8
#define M1_PWM_UL_GPIO_Port GPIOE
#define M1_PWM_UH_Pin GPIO_PIN_9
#define M1_PWM_UH_GPIO_Port GPIOE
#define M1_PWM_VL_Pin GPIO_PIN_10
#define M1_PWM_VL_GPIO_Port GPIOE
#define M1_PWM_VH_Pin GPIO_PIN_11
#define M1_PWM_VH_GPIO_Port GPIOE
#define M1_PWM_WL_Pin GPIO_PIN_12
#define M1_PWM_WL_GPIO_Port GPIOE
#define M1_PWM_VHE13_Pin GPIO_PIN_13
#define M1_PWM_VHE13_GPIO_Port GPIOE
#define GD_Ready_Pin GPIO_PIN_14
#define GD_Ready_GPIO_Port GPIOE
#define GD_NFAULT_Pin GPIO_PIN_15
#define GD_NFAULT_GPIO_Port GPIOE
#define TEST_PIN_Pin GPIO_PIN_8
#define TEST_PIN_GPIO_Port GPIOA
#define SPI3_CS_Pin GPIO_PIN_9
#define SPI3_CS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
