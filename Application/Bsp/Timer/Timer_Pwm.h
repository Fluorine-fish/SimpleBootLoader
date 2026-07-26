/**
*   @file Timer_Pwm.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/21
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_TIMER_PWM_H
#define APPLICATION_TIMER_PWM_H

#include "Algorithm.h"

void Timer_PwmInit();
void Timer_PwmSetDuty(Alg_3Sys_s* duty);
void Timer_PwmDisableOutput();

#endif //APPLICATION_TIMER_PWM_H
