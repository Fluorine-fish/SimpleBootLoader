/**
*   @file Application.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#include "Application.h"
#include "Algorithm.h"
#include "Alg_math.h"
#include "Alg_Svpwm.h"
#include "Encoder_Mt6701.h"
#include "Timer_Pwm.h"

uint32_t encoder_raw_angle = 0;

float openloop_theta_e = 0.f;
float omega_e = 300.f;
float iAmp = 0.15f;
Alg_2Sys_s uAlphaBeta = {};
Alg_3Sys_s switchTim = {};
Alg_3Sys_s duty = {};

void App_TimerIt() {
    Encoder_GetRaw(&encoder_raw_angle);

    openloop_theta_e = (openloop_theta_e > 1.0f) ? 0.f : openloop_theta_e + (1.f / 20000) * (omega_e / (2 * PI));
    Alg_DeComp(iAmp, openloop_theta_e, &uAlphaBeta);
    Alg_SvpwmWithSector(&uAlphaBeta, &switchTim, &duty);
    Timer_PwmSetDuty(&switchTim);
}

void App_Init() {

}
