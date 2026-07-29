/**
*   @file TorqueContorl.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/27
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_TORQUECONTORL_H
#define APPLICATION_TORQUECONTORL_H

#include "CurrerntControl.h"

typedef struct {
    CurrentControl_s iControl;
    float vdc;
    float target_torque;
    Alg_2Sys_s target_idq;
    Alg_2Sys_s decoupling_udq;
    Alg_2Sys_s out_udq;
    Alg_2Sys_s out_uAlphaBeta;
    float voltage_limit_scale;
}TorqueController_s;

void TorqueControl_Init(TorqueController_s* controller,float vdc);
void TorqueControl(TorqueController_s* controller,
               Alg_2Sys_s* fdb_idq,
               float Vdc,
               float theta_e,
               float omega_e,
               float target_torque);

#endif //APPLICATION_TORQUECONTORL_H
