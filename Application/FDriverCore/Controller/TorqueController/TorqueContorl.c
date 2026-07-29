/**
*   @file TorqueContorl.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/27
*   @version 1.0
*   @note
*/
#include "TorqueContorl.h"
#include "Alg_math.h"
#include "Alg_Svpwm.h"
#include "FDriver_config.h"
#include "Timer_Pwm.h"
#include <math.h>

void TorqueControl_Init(TorqueController_s* controller,float vdc) {
    controller->vdc = vdc;
    controller->target_torque = 0.f;
    controller->decoupling_udq.a = 0.f;
    controller->decoupling_udq.b = 0.f;
    controller->voltage_limit_scale = 1.f;

    CurrentControl_Init(&controller->iControl, vdc);
}

void TorqueControl(TorqueController_s* controller,
               Alg_2Sys_s* fdb_idq,
               float Vdc,
               float theta_e,
               float omega_e,
               float target_torque) {
    Alg_3Sys_s tmp_switchTim;
    Alg_3Sys_s duty;

    controller->target_torque = target_torque;
    controller->target_idq.a = 0.f;
    controller->vdc = Vdc;

    float target_iq = MOTOR_TORQUE_TO_IQ_SIGN * controller->target_torque / MOTOR_K_TORQUE;
    if (target_iq > MOTOR_CURRENT_RATED) target_iq = MOTOR_CURRENT_RATED;
    else if (target_iq < -MOTOR_CURRENT_RATED) target_iq = -MOTOR_CURRENT_RATED;
    controller->target_idq.b = target_iq;

    CurrentControl(&controller->iControl, &controller->target_idq, fdb_idq, &controller->out_udq, Vdc);

    // Positive motor speed corresponds to decreasing FOC electrical angle.
    controller->decoupling_udq.a = omega_e * MOTOR_LS * fdb_idq->b;
    controller->decoupling_udq.b = -omega_e * MOTOR_LS * fdb_idq->a;

#if CURRENT_CROSS_COUPLING_ENABLE
    controller->out_udq.a += controller->decoupling_udq.a;
    controller->out_udq.b += controller->decoupling_udq.b;
#endif

    const float voltage_max = Vdc / sqrtf(3.f);
    const float voltage_square = controller->out_udq.a * controller->out_udq.a
                               + controller->out_udq.b * controller->out_udq.b;
    controller->voltage_limit_scale = 1.f;
    if (voltage_square > voltage_max * voltage_max) {
        controller->voltage_limit_scale = voltage_max / sqrtf(voltage_square);
        controller->out_udq.a *= controller->voltage_limit_scale;
        controller->out_udq.b *= controller->voltage_limit_scale;
    }

    Alg_InvPark(theta_e, &controller->out_udq, &controller->out_uAlphaBeta);
    Alg_SvpwmZeroInject(Vdc, &controller->out_uAlphaBeta, &tmp_switchTim, &duty);
    Timer_PwmSetDuty(&tmp_switchTim);
}
