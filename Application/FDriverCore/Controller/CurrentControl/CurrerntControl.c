/**
*   @file CurrerntControl.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/26
*   @version 1.0
*   @note
*/
#include "CurrerntControl.h"

#include "Algorithm.h"
#include "FDriver_config.h"

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

float CurrentControl(CurrentController_s* controller, float error) {
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
