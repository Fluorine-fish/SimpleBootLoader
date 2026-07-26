/**
*   @file Runtime_StateMachine.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_RUNTIME_STATEMACHINE_H
#define APPLICATION_RUNTIME_STATEMACHINE_H

#include "FDriver.h"

typedef void (*StateFunc)(void);

typedef enum {
    APP_STATE_RESET = 0,
    APP_STATE_INIT  = 1,
    APP_STATE_READY = 2,
    APP_STATE_CALIB = 3,
    APP_STATE_ALIGN = 4,
    APP_STATE_RUN   = 5,
    APP_STATE_FAULT = 6,
    APP_STATE_COUNT
} AppState; /* Application state identification user type */

typedef enum {
    APP_EVENT_FAULT       = 0,
    APP_EVENT_FAULT_CLEAR = 1,
    APP_EVENT_RESET       = 2,
    APP_EVENT_RESET_DONE  = 3,
    APP_EVENT_INIT        = 4,
    APP_EVENT_INIT_DONE   = 5,
    APP_EVENT_APP_OFF     = 6,
    APP_EVENT_READY       = 7,
    APP_EVENT_APP_ON      = 8,
    APP_EVENT_CALIB       = 9,
    APP_EVENT_CALIB_DONE  = 10,
    APP_EVENT_ALIGN       = 11,
    APP_EVENT_ALIGN_DONE  = 12,
    APP_EVENT_RUN         = 13,
    APP_EVENT_FLASH_ON    = 14, /* Existing calibration data loaded from Flash */
    APP_EVENT_COUNT
} AppEvent; /* Application event identification user type */

void StateTransition(AppState state,AppEvent event);

#endif //APPLICATION_RUNTIME_STATEMACHINE_H
