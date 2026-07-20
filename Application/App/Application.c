/**
*   @file Application.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#include "Application.h"
#include "Encoder_Mt6701.h"
#include "tim.h"

uint32_t encoder_raw_angle = 0;

void App_TimerIt() {
    Encoder_GetRaw(&encoder_raw_angle);
}

void App_Init() {

}
