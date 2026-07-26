/**
*   @file vfcontrol.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/24
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_VFCONTROL_H
#define APPLICATION_VFCONTROL_H

#include "Algorithm.h"

typedef struct {
    Alg_vector_s voltageVector;
    float omega_e; // 电角速度 rad/s
}Controller_VfControl_s;

void VfControl(Controller_VfControl_s* vfController,
               float target_voltage_module,
               float omega_e,
               uint16_t update_frequency);

#endif //APPLICATION_VFCONTROL_H
