/**
*   @file VelocityObserver.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/29
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_VELOCITYOBSERVER_H
#define APPLICATION_VELOCITYOBSERVER_H

typedef struct {
    float freq;
    float wn;
    float kp;
    float ki;
    float ts;
    float kaw;

    float omega_max;
    float omega_integral;
    float omega_hat;
    float theta_hat;
}Observer_Pll_s;

void Observer_PllInit(Observer_Pll_s *pll, float omega_max);
void Observer_PllUpdate(Observer_Pll_s *pll, float theta);

#endif //APPLICATION_VELOCITYOBSERVER_H
