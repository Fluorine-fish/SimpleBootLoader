/**
*   @file Timer_It.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#include "Timer_It.h"
#include "tim.h"
#include "Encoder_Mt6701.h"
#include "Application.h"

void Timer_ItInit() {
    HAL_TIM_Base_Start_IT(&htim1);
    HAL_TIM_Base_Start(&htim1);
}

void Timer_It() {
    App_TimerIt();
}
