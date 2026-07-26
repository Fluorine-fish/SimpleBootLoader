/**
*   @file FDriver.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/24
*   @version 1.0
*   @note
*/

#include "Bsp_Adc.h"
#include "Encoder_Mt6701.h"
#include "Sampling.h"
#include "FDriver.h"
#include "FDriver_config.h"

FDriver_s FDriver = {
    .state_machine = {
        .state = APP_STATE_RESET,
        .event = APP_EVENT_RESET,
    }
};

void FDriver_Init(FDriver_s *fdriver) {
    // Motor Params
    fdriver->motor_params.rs = MOTOR_RS;
    fdriver->motor_params.ls = MOTOR_LS;
    fdriver->motor_params.pole_pairs = MOTOR_POLE_PAIRS;

    // StateMachine 不需要初始化
}

void FDriver_GetFeedback(FDriver_s *fdriver) {
    // Encoder
    Encoder_GetRaw(&fdriver->fdb.encoder_raw);
    Sampling_encoder(fdriver->fdb.encoder_raw,
        &fdriver->foc.theta_e,
        &fdriver->foc.theta_m,
        &fdriver->foc.velocity_m);

    // CurrentSampling
    Adc_GetPhaseCurrent(&FDriver.foc.phase_current);
}
