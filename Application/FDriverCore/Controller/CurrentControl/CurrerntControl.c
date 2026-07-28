/**
*   @file CurrerntControl.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/26
*   @version 1.0
*   @note
*/
#include "CurrerntControl.h"
#include "math.h"
#include "Algorithm.h"
#include "FDriver_config.h"
#include "string.h"

void CurrentController_Init(CurrentController_s* controller, float out_max) {
    controller->omega_c = CURRENT_CUT_OFF_FREQ * 2 * PI;
    controller->kp = controller->omega_c * MOTOR_LS;
    controller->ki = controller->omega_c * MOTOR_RS * CURRENT_TS;
    controller->ka = 1 / controller->kp;
    controller->out_max = out_max;

    controller->error_sum = 0.f;
    controller->kp_out = 0.f;
    controller->ki_out = 0.f;
    controller->output = 0.f;
    controller->anti_windup_error = 0.f;
}

void CurrentControl_Init(CurrentControl_s* controller, float vdc) {
    controller->target_idq.a = 0.f;
    controller->target_idq.b = 0.f;
    controller->fdb_idq.a = 0.f;
    controller->fdb_idq.b = 0.f;
    controller->vdc = vdc;

    // 最大值初始化时认为是ud，uq均摊
    CurrentController_Init(&controller->id_control, controller->vdc /sqrt3 /sqrt2);
    CurrentController_Init(&controller->iq_control, controller->vdc /sqrt3 /sqrt2);
}

float CurrentControl_single(CurrentController_s* controller, float error) {
    controller->kp = controller->omega_c * MOTOR_LS;
    controller->ki = controller->omega_c * MOTOR_RS * CURRENT_TS;
    controller->ka = 1 / controller->kp;

    controller->kp_out = controller->kp * error;
    controller->error_sum += error;
    controller->ki_out = controller->ki * controller->error_sum;

    // fout = kp * err + ki*Int;
    controller->output = controller->kp_out + controller->ki_out;

    // antiwindup and limit output
    if (controller->output > controller->out_max) {
        controller->anti_windup_error = controller->out_max - controller->output;
        controller->error_sum += controller->anti_windup_error * controller->ka;
        controller->output = controller->out_max;
    } else if (controller->output < -controller->out_max) {
        controller->anti_windup_error = -controller->out_max - controller->output;
        controller->error_sum += controller->anti_windup_error * controller->ka;
        controller->output = -controller->out_max;
    }

    return controller->output;
}

void CurrentControl(CurrentControl_s* controller, Alg_2Sys_s* target_idq, Alg_2Sys_s* fdb_idq, Alg_2Sys_s* out_udq, float vdc) {
    memcpy(&controller->target_idq, target_idq, sizeof(Alg_2Sys_s));
    memcpy(&controller->fdb_idq, fdb_idq, sizeof(Alg_2Sys_s));
    controller->vdc = vdc;

    controller->error.a = controller->target_idq.a - controller->fdb_idq.a;
    controller->error.b = controller->target_idq.b - controller->fdb_idq.b;

    controller->id_control.out_max = controller->vdc / sqrt3;
    controller->out_udq.a = CurrentControl_single(&controller->id_control, controller->error.a);
    controller->iq_control.out_max = sqrtf(controller->vdc * controller->vdc / 3 - controller->out_udq.a * controller->out_udq.a);
    controller->out_udq.b = CurrentControl_single(&controller->iq_control, controller->error.b);

    memcpy(out_udq, &controller->out_udq, sizeof(Alg_2Sys_s));
}
