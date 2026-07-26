/**
*   @file Application.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#include "FDriver.h"
#include "Application.h"
#include "Runtime_StateMachine.h"

extern FDriver_s FDriver;

void App_TimerIt() {
    StateTransition(FDriver.state_machine.state, FDriver.state_machine.event);
}
