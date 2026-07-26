/**
*   @file Adc.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/23
*   @version 1.0
*   @note
*/
#include "Bsp_Adc.h"
#include "adc.h"
#include "opamp.h"

uint16_t adc_offset[3] = {};

void Adc_Init() {
    // ADC Init
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
    HAL_ADCEx_InjectedStart_IT(&hadc1);
    HAL_ADCEx_InjectedStart_IT(&hadc2);

    // OPAMP Init
    HAL_OPAMP_Start(&hopamp1);
    HAL_OPAMP_Start(&hopamp2);
    HAL_OPAMP_Start(&hopamp3);
}

void Adc_GetPhaseCurrent(Alg_3Sys_s* phaseCurrent) {
    phaseCurrent->a = - (((float)hadc1.Instance->JDR1 - adc_offset[0]) / 2048.f) / 3.3f * 9 / 0.005f;
    phaseCurrent->b = - (((float)hadc2.Instance->JDR1 - adc_offset[1]) / 2048.f) / 3.3f * 9 / 0.005f;
    phaseCurrent->c = - (((float)hadc1.Instance->JDR3 - adc_offset[2]) / 2048.f) / 3.3f * 9 / 0.005f;
}

void Adc_GetVdc(float *vdcRaw) {
    *vdcRaw = (float)hadc1.Instance->JDR2; // v bus voltage
}
