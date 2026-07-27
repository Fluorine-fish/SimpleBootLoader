/**
*   @file CurrerntControl.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/26
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_CURRERNTCONTROL_H
#define APPLICATION_CURRERNTCONTROL_H

typedef struct {
    float kp;
    float ki;
    float ka;
    float omega_c;
    float out_max;

    float error_sum;
    float kp_out;
    float ki_out;
    float anti_windup_error;
    float output;
}CurrentController_s;

void CurrentController_Init(CurrentController_s* controller, float out_max);
float CurrentControl(CurrentController_s* controller, float error);

#endif //APPLICATION_CURRERNTCONTROL_H
