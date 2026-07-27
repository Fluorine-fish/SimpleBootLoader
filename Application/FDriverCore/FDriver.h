/**
*   @file FDriver.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_FDRIVER_H
#define APPLICATION_FDRIVER_H

#include "stdint.h"
#include "Algorithm.h"
#include "Vfcontrol.h"
#include "IfControl.h"
#include "Runtime_StateMachine.h"

typedef enum {
    FDRIVER_VF,
    FDRIVER_IF,
}FDriver_Mode_e;

typedef struct {
    AppEvent event;
    AppState state;
}FDriver_State_s;

typedef struct {
    float rs;
    float ls;
    uint8_t pole_pairs;
}FDriver_MotorParams_s;

typedef struct {
    float theta_e;// 0~1
    float theta_m;// 0~1
    float velocity_m;// rad/s
    float vdc;  // 母线电压
    Alg_3Sys_s phase_current;
    Alg_2Sys_s iAlphaBeta;
    Alg_2Sys_s iDq;
}FDriver_Foc_s;

typedef struct {
    uint16_t encoder_raw; // 编码器原始值
    uint16_t encoder_offset;
    Alg_3Sys_s adc_current_raw; // ADC原始值
    Alg_3Sys_s adc_curren_offset;
    float Vdc;
}FDriver_Feedback_s;

typedef struct {
    Controller_VfControl_s vfController;
    Controller_IfControl_s ifController;
}FDriver_Controller_s;

typedef struct {
    FDriver_Mode_e mode;
    FDriver_State_s state_machine;
    FDriver_MotorParams_s motor_params;
    FDriver_Foc_s foc;
    FDriver_Feedback_s fdb;
    FDriver_Controller_s controllers;
}FDriver_s;

void FDriver_Init(FDriver_s *fdriver);
void FDriver_GetFeedback(FDriver_s *fdriver);

#endif //APPLICATION_FDRIVER_H
