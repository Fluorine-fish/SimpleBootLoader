/**
*   @file Adc.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/23
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_ADC_H
#define APPLICATION_ADC_H

#include "Algorithm.h"

void Adc_Init();
void Adc_GetPhaseCurrent(Alg_3Sys_s* phaseCurrent);
void Adc_GetVdc(float *vdc);

#endif //APPLICATION_ADC_H