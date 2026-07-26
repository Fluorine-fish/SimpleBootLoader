/**
*   @file Sampling.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/24
*   @version 1.0
*   @note
*/
#include "Sampling.h"
#include "Bsp_Adc.h"
#include "FDriver_config.h"

static uint16_t lastEncoderRaw = 0;

/**
 * @brief 电流采样
 * @param currentRaw 带符号的ADC采样值
 * @param phase_current 实际的相电流mA
 */
void Sampling_Current(Alg_3Sys_s* currentRaw, Alg_3Sys_s* phaseCurrent, Alg_3Sys_s* offset) {



}

void Sampling_Vdc(float vdcRaw, float* vdc) {

}

void Sampling_encoder(uint16_t encoderRaw, float* theta_e, float* theta_m, float* velocity_m) {
    *theta_e = (uint16_t)(encoderRaw * MOTOR_POLE_PAIRS) / 65536.0f;
    *theta_m = encoderRaw / 65536.0f;
}
