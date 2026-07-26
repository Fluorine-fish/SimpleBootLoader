/**
*   @file Runtime_StateMachine.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#include "adc.h"
#include "FDriver.h"
#include "Timer_Pwm.h"
#include "Vfcontrol.h"

float openloop_theta_e = 0.f;
float omega_e = 300.f;
float vModule = 0.15f;

Controller_VfControl_s vfController = {};

extern FDriver_s FDriver;
extern uint16_t adc_offset[3];

void StateReset() {
    uint8_t fnState = 0;

    FDriver.state_machine.state = APP_STATE_RESET;
    FDriver.state_machine.event = APP_EVENT_RESET;

    Timer_PwmDisableOutput();

    fnState = 1;

    if (fnState) {
        FDriver.state_machine.event = APP_EVENT_RESET_DONE;
    }
}

void StateInit() {
    uint8_t fnState = 0;

    FDriver.state_machine.state = APP_STATE_INIT;
    FDriver.state_machine.event = APP_EVENT_INIT;

    Timer_PwmDisableOutput();
    FDriver_Init(&FDriver);

    fnState = 1;

    if (fnState) {
        FDriver.state_machine.event = APP_EVENT_INIT_DONE;
    }
}

void StateReady() {
    FDriver.state_machine.state = APP_STATE_READY;
    FDriver.state_machine.event = APP_EVENT_APP_ON;
}

void StateCalib() {
    uint8_t fnState = 0;
    static uint32_t tmp_offset[3] = {};
    static uint16_t cnt = 0;

    FDriver.state_machine.state = APP_STATE_CALIB;
    FDriver.state_machine.event = APP_EVENT_CALIB;


    if (cnt < 500) {
        tmp_offset[0] += hadc1.Instance->JDR1;
        tmp_offset[1] += hadc2.Instance->JDR1;
        tmp_offset[2] += hadc1.Instance->JDR3;
        cnt ++;
    }else {
        adc_offset[0] = tmp_offset[0] / cnt;
        adc_offset[1] = tmp_offset[1] / cnt;
        adc_offset[2] = tmp_offset[2] / cnt;

        fnState = 1;
    }

    if (fnState) {
        FDriver.state_machine.event = APP_EVENT_CALIB_DONE;
    }
}

void StateAlign() {
    uint8_t fnState = 0;

    FDriver.state_machine.state = APP_STATE_ALIGN;
    FDriver.state_machine.event = APP_EVENT_ALIGN;

    fnState = 1;

    if (fnState) {
        FDriver.state_machine.event = APP_EVENT_ALIGN_DONE;
    }
}

void StateRun() {
    FDriver.state_machine.state = APP_STATE_RUN;
    FDriver.state_machine.event = APP_EVENT_RUN;

    FDriver_GetFeedback(&FDriver);
    VfControl(&vfController, vModule, omega_e, 20000);
}

void StateFault() {
    uint8_t fnState = 0;

    FDriver.state_machine.state = APP_STATE_FAULT;
    FDriver.state_machine.event = APP_EVENT_FAULT;

    Timer_PwmDisableOutput();

    if (fnState) {
        FDriver.state_machine.event = APP_EVENT_FAULT_CLEAR;
    }
}

static StateFunc const stateFuncTable[APP_EVENT_COUNT][APP_STATE_COUNT] = {
/* Current State:          RESET       INIT        READY       CALIB       ALIGN       RUN         FAULT */
/* APP_EVENT_FAULT       */ {StateFault, StateFault, StateFault, StateFault, StateFault, StateFault, StateFault},
/* APP_EVENT_FAULT_CLEAR */ {StateFault, StateFault, StateFault, StateFault, StateFault, StateFault, StateInit},
/* APP_EVENT_RESET       */ {StateReset, StateReset, StateReset, StateFault, StateFault, StateFault, StateFault},
/* APP_EVENT_RESET_DONE  */ {StateInit,  StateFault, StateFault, StateFault, StateFault, StateFault, StateFault},
/* APP_EVENT_INIT        */ {StateFault, StateInit,  StateFault, StateFault, StateFault, StateFault, StateFault},
/* APP_EVENT_INIT_DONE   */ {StateFault, StateReady, StateFault, StateFault, StateFault, StateFault, StateFault},
/* APP_EVENT_APP_OFF     */ {StateFault, StateFault, StateReady, StateInit,  StateInit,  StateInit,  StateFault},
/* APP_EVENT_READY       */ {StateFault, StateFault, StateReady, StateFault, StateFault, StateFault, StateFault},
/* APP_EVENT_APP_ON      */ {StateFault, StateFault, StateCalib, StateFault, StateFault, StateFault, StateFault},
/* APP_EVENT_CALIB       */ {StateFault, StateFault, StateFault, StateCalib, StateFault, StateFault, StateFault},
/* APP_EVENT_CALIB_DONE  */ {StateFault, StateFault, StateFault, StateAlign, StateFault, StateFault, StateFault},
/* APP_EVENT_ALIGN       */ {StateFault, StateFault, StateFault, StateFault, StateAlign, StateFault, StateFault},
/* APP_EVENT_ALIGN_DONE  */ {StateFault, StateFault, StateFault, StateFault, StateRun,   StateFault, StateFault},
/* APP_EVENT_RUN         */ {StateFault, StateFault, StateFault, StateFault, StateFault, StateRun,   StateFault},
/* APP_EVENT_FLASH_ON    */ {StateFault, StateFault, StateRun,   StateFault, StateFault, StateFault, StateFault}
};

void StateTransition(AppState state,AppEvent event) {
    stateFuncTable[event][state]();
}

