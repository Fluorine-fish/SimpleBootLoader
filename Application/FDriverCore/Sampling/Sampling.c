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

void Sampling_encoder(uint16_t encoderRaw, float* theta_e, float* theta_m, float* velocity_m) {
    // 编码器反向
    *theta_e = (uint16_t)(65536 - encoderRaw * MOTOR_POLE_PAIRS) / 65536.0f * 2 * PI;
    *theta_m = encoderRaw / 65536.0f * 2 * PI;

}
