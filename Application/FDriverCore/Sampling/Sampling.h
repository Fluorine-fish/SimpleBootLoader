/**
*   @file Sampling.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/24
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_SAMPLING_H
#define APPLICATION_SAMPLING_H

#include "Algorithm.h"

void Sampling_AdcCurrentCalibrate(Alg_3Sys_s* currentRaw, Alg_3Sys_s* offset);
void Sampling_Current(Alg_3Sys_s* currentRaw, Alg_3Sys_s* phaseCurrent, Alg_3Sys_s* offset);
void Sampling_Vdc(float vdcRaw, float* vdc);
void Sampling_encoder(uint16_t encoderRaw, float* theta_e, float* theta_m, float* velocity_m);

#endif //APPLICATION_SAMPLING_H
