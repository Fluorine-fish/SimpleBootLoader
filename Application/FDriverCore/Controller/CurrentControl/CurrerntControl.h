/**
*   @file CurrerntControl.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/26
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_CURRERNTCONTROL_H
#define APPLICATION_CURRERNTCONTROL_H

#include "Algorithm.h"

typedef struct {
    float kp;
    float ki;
    float ka;
    float omega_c;
    float out_max;

    float error_sum;
    float kp_out;
    float ki_out;
    float anti_windup_error;
    float output;
}CurrentController_s;

typedef struct {
    CurrentController_s id_control;
    CurrentController_s iq_control;
    Alg_2Sys_s target_idq;
    Alg_2Sys_s fdb_idq;
    Alg_2Sys_s out_udq;
    Alg_2Sys_s error;
    float vdc;
}CurrentControl_s;

void CurrentController_Init(CurrentController_s* controller, float out_max);
float CurrentControl_single(CurrentController_s* controller, float error);
void CurrentControl(CurrentControl_s* controller, Alg_2Sys_s* target_idq, Alg_2Sys_s* fdb_idq, Alg_2Sys_s* out_udq, float vdc);
void CurrentControl_Init(CurrentControl_s* controller, float vdc);

#endif //APPLICATION_CURRERNTCONTROL_H
