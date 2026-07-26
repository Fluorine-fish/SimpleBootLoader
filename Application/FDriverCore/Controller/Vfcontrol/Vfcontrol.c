/**
*   @file vfcontrol.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/24
*   @version 1.0
*   @note
*/
#include "Vfcontrol.h"
#include "Algorithm.h"
#include "Alg_math.h"
#include "Alg_Svpwm.h"
#include "Timer_Pwm.h"

void VfControl(Controller_VfControl_s* vfController,
               float target_voltage_module,
               float omega_e,
               uint16_t update_frequency){
    float* theta_e = &vfController->voltageVector.arg;
    Alg_2Sys_s tmp_uAlphaBeta;
    Alg_3Sys_s tmp_switchTim;
    Alg_3Sys_s duty;
    vfController->voltageVector.module = target_voltage_module;
    vfController->omega_e = omega_e;

    *theta_e = (*theta_e < 0.0f) ? 1.f : *theta_e -  (1.f / update_frequency) * (vfController->omega_e / (2 * PI));

    Alg_DeComp(&vfController->voltageVector, &tmp_uAlphaBeta);
    Alg_SvpwmZeroInject(&tmp_uAlphaBeta, &tmp_switchTim, &duty);
    Timer_PwmSetDuty(&tmp_switchTim);
}
