/**
*   @file IfControl.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/21
*   @version 1.0
*   @note
*/
#include "IfControl.h"
#include "Alg_Svpwm.h"
#include "Timer_Pwm.h"

void IfController_Init(Controller_IfControl_s* controller, float out_max) {
    CurrentController_Init(&controller->idController, out_max);
    CurrentController_Init(&controller->iqController, out_max);

    controller->target_idq.a = 0;
    controller->target_idq.b = 0;
    controller->out_uAlphaBeta.a = 0;
    controller->out_uAlphaBeta.b = 0;
    controller->out_uDq.a = 0;
    controller->out_uDq.b = 0;
}

void IfControl(Controller_IfControl_s* ifController,
               Alg_2Sys_s fdb_idq,
               float Vdc,
               float target_current_module,
               float omega_e,
               uint16_t update_frequency) {
    float* theta_e = &ifController->currentVector.arg;
    Alg_2Sys_s tmp_iAlphaBeta;
    Alg_3Sys_s tmp_switchTim;
    Alg_3Sys_s duty;
    ifController->currentVector.module = target_current_module;
    ifController->omega_e = omega_e;

    if (omega_e > 0) {
        *theta_e = *theta_e - (1.f / update_frequency) * (ifController->omega_e);
        if (*theta_e < 0.f) *theta_e += 2 * PI;
    }else {
        *theta_e = *theta_e + (1.f / update_frequency) * ( - ifController->omega_e);
        if (*theta_e > 2 * PI) *theta_e -= 2 * PI;
    }

    Alg_DeComp(&ifController->currentVector, &tmp_iAlphaBeta);
    Alg_Park(*theta_e, &tmp_iAlphaBeta, &ifController->target_idq);

    ifController->out_uDq.a = CurrentControl(&ifController->idController, ifController->target_idq.a  - fdb_idq.a);
    ifController->out_uDq.b = CurrentControl(&ifController->iqController, ifController->target_idq.b  - fdb_idq.b);

    Alg_InvPark(*theta_e, &ifController->out_uDq, &ifController->out_uAlphaBeta);
    Alg_SvpwmZeroInject(Vdc, &ifController->out_uAlphaBeta, &tmp_switchTim, &duty);
    Timer_PwmSetDuty(&tmp_switchTim);
}

