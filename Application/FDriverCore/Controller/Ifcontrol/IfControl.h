/**
*   @file IfControl.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/21
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_IFCONTROL_H
#define APPLICATION_IFCONTROL_H

#include "Alg_math.h"
#include "CurrerntControl.h"

typedef struct {
    CurrentController_s idController;
    CurrentController_s iqController;
    Alg_vector_s currentVector;
    float omega_e; // 电角速度 rad/s
    Alg_2Sys_s out_uDq;
    Alg_2Sys_s out_uAlphaBeta;
    Alg_2Sys_s target_idq;
}Controller_IfControl_s;

void IfController_Init(Controller_IfControl_s* controller, float out_max);

void IfControl(Controller_IfControl_s* ifController,
               Alg_2Sys_s fdb_idq,
               float Vdc,
               float target_current_module,
               float omega_e,
               uint16_t update_frequency);

#endif //APPLICATION_IFCONTROL_H
