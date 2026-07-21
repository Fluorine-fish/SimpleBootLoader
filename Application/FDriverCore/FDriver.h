/**
*   @file FDriver.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_FDRIVER_H
#define APPLICATION_FDRIVER_H

#include "stdint.h"

typedef struct {
    float rs;
    float ls;
    uint8_t pole_pairs;
}FDriver_MotorParams_s;

typedef struct {
    float theta_e;
    float theta_m;
    float velocity_m;

}FDriver_Foc_s;

typedef struct {
    uint32_t encoder_raw; // 编码器原始值
    uint8_t encoder_rev; // 编码器分辨率
}FDriver_Feedback_s;

typedef struct {

}FDriver_s;

#endif //APPLICATION_FDRIVER_H
