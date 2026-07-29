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
    IfController_Init(&fdriver->controllers.ifController, fdriver->foc.vdc_set);

    // TorqueController
    TorqueControl_Init(&fdriver->controllers.torqueController, fdriver->foc.vdc_set);
}

void FDriver_Init(FDriver_s *fdriver) {
    // Mode
    fdriver->mode = CONTROLMODE,

    // Motor Params
    fdriver->motor_params.rs = MOTOR_RS;
    fdriver->motor_params.ls = MOTOR_LS;
    fdriver->motor_params.pole_pairs = MOTOR_POLE_PAIRS;

    // StateMachine 不需要初始化

    // Foc
    fdriver->foc.vdc_set = VDC_SET;

    // Fdb
    fdriver->fdb.encoder_offset = 0;
    fdriver->fdb.adc_current_raw.a = 0.f;
    fdriver->fdb.adc_current_raw.b = 0.f;
    fdriver->fdb.adc_current_raw.c = 0.f;
    fdriver->fdb.adc_curren_offset.a = 0.f;
    fdriver->fdb.adc_curren_offset.b = 0.f;
    fdriver->fdb.adc_curren_offset.c = 0.f;
    Adc_GetVdc(&fdriver->fdb.vdc);

    // Controller Init
    FDriver_ControllerInit(fdriver);
}

void FDriver_ObserverInit(FDriver_s* fdriver) {
    Observer_PllInit(&fdriver->observer.pll_e,MOTOR_MAX_OMEGA_E);
    Observer_PllInit(&fdriver->observer.pll_m,MOTOR_MAX_OMEGA_M);
}

void FDriver_ObserverUpdate(FDriver_s* fdriver) {
    Observer_PllUpdate(&fdriver->observer.pll_e, fdriver->foc.theta_e);
    Observer_PllUpdate(&fdriver->observer.pll_m, fdriver->foc.theta_m);
    fdriver->foc.omega_e = fdriver->observer.pll_e.omega_hat;
    fdriver->foc.omega_m = fdriver->observer.pll_m.omega_hat;
}

void FDriver_GetFeedback(FDriver_s *fdriver) {
    // Encoder
    Encoder_GetRaw(&fdriver->fdb.encoder_raw);
    Sampling_encoder(fdriver->fdb.encoder_raw - fdriver->fdb.encoder_offset,
        &fdriver->foc.theta_e,
        &fdriver->foc.theta_m,
        &fdriver->foc.omega_m);

    // CurrentSampling
    Adc_GetPhaseCurrent(&FDriver.foc.phase_current);
    Adc_GetVdc(&FDriver.fdb.vdc);
    FDriver_CurrentRestruct(&FDriver);
}
