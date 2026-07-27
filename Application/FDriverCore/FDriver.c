/**
*   @file FDriver.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/24
*   @version 1.0
*   @note
*/

#include "Bsp_Adc.h"
#include "Alg_math.h"
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

static void FDriver_CurrentRestruct(FDriver_s* fdriver){
    switch (fdriver->mode) {
        case FDRIVER_VF:
            Alg_Clarke(&fdriver->foc.phase_current, &fdriver->foc.iAlphaBeta);
            Alg_Park(fdriver->controllers.vfController.voltageVector.arg, &fdriver->foc.iAlphaBeta, &fdriver->foc.iDq);
            break;
        case FDRIVER_IF:
            Alg_Clarke(&fdriver->foc.phase_current, &fdriver->foc.iAlphaBeta);
            Alg_Park(fdriver->controllers.ifController.currentVector.arg, &fdriver->foc.iAlphaBeta, &fdriver->foc.iDq);
            break;
        default:
            Alg_Clarke(&fdriver->foc.phase_current, &fdriver->foc.iAlphaBeta);
            Alg_Park(fdriver->foc.theta_e, &fdriver->foc.iAlphaBeta, &fdriver->foc.iDq);
            break;
    }
}

static void FDriver_ControllerInit(FDriver_s* fdriver) {
    // VfController
    fdriver->controllers.vfController.omega_e = OMEGA_E;
    fdriver->controllers.vfController.voltageVector.module = V_MODULE;

    // IfController
    IfController_Init(&fdriver->controllers.ifController, fdriver->foc.vdc / sqrt3);
}

static void 

void FDriver_Init(FDriver_s *fdriver) {
    // Mode
    fdriver->mode = CONTROLMODE,

    // Motor Params
    fdriver->motor_params.rs = MOTOR_RS;
    fdriver->motor_params.ls = MOTOR_LS;
    fdriver->motor_params.pole_pairs = MOTOR_POLE_PAIRS;

    // StateMachine 不需要初始化

    // Foc
    fdriver->foc.vdc = 24.f;

    // Controller Init
    FDriver_ControllerInit(fdriver);
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
    FDriver_CurrentRestruct(&FDriver);
}
