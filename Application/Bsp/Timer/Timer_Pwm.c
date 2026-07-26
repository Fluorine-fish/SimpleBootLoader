/**
*   @file Timer_Pwm.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/21
*   @version 1.0
*   @note
*/
#include "Timer_Pwm.h"
#include "tim.h"

#define PWM_PERIOD_CYCLEES (8500)

void Timer_PwmInit() {
    HAL_TIM_Base_Start_IT(&htim1);
    HAL_TIM_Base_Start(&htim1); // initial Timer
    HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1); // enable TIM1 channel 1
    HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_1); // enable TIM1 channel 1 complement ouput
    HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_2); // enable TIM1 channel 2
    HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_2); // enable TIM1 channel 2 complement ouput
    HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_3); // enable TIM1 channel 3
    HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_3); // enable TIM1 channel 3 complement ouput
    HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_4); // enable TIM1 channel 4
    HAL_TIMEx_PWMN_Start(&htim1,TIM_CHANNEL_4); // enable TIM1 channel 4 complement ouput
}

void Timer_PwmSetDuty(Alg_3Sys_s* swTime) {
    TIM1->CCR1 = (uint32_t)(swTime->a * (PWM_PERIOD_CYCLEES));
    TIM1->CCR2 = (uint32_t)(swTime->b * (PWM_PERIOD_CYCLEES));
    TIM1->CCR3 = (uint32_t)(swTime->c * (PWM_PERIOD_CYCLEES));
}

void Timer_PwmDisableOutput() {
    TIM1->CCR1 = (uint32_t)(0.5f* (PWM_PERIOD_CYCLEES));
    TIM1->CCR2 = (uint32_t)(0.5f* (PWM_PERIOD_CYCLEES));
    TIM1->CCR3 = (uint32_t)(0.5f* (PWM_PERIOD_CYCLEES));
}
