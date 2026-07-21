/**
*   @file Bsp_Init.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#include "Bsp_Init.h"
#include "Timer_It.h"
#include "Timer_Pwm.h"

void Bsp_Init() {
    Timer_ItInit();
    Timer_PwmInit();
}
