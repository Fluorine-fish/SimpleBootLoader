/**
*   @file VelocityObserver.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/29
*   @version 1.0
*   @note
*/
#include "VelocityObserver.h"
#include "Algorithm.h"
#include "Alg_math.h"
#include "FDriver_config.h"

void Observer_PllInit(Observer_Pll_s *pll, float omega_max) {
    pll->freq = PLL_FREQ;
    pll->wn = 2 * PI * PLL_FREQ;
    pll->kp = 2 * PLL_ZETA * pll->wn;
    pll->ki = pll->wn * pll->wn;
    pll->ts = PLL_TS;
    pll->kaw = pll->ki / pll->kp;

    pll->omega_max = omega_max;
    pll->omega_integral = 0.f;
    pll->omega_hat = 0.f;
    pll->theta_hat = 0.f;
}

void Observer_PllUpdate(Observer_Pll_s *pll, float theta) {
    // 开放调参
    pll->wn = 2 * PI * pll->freq;
    pll->kp = 2 * PLL_ZETA * pll->wn;
    pll->ki = pll->wn * pll->wn;
    pll->kaw = pll->ki / pll->kp;

    // The motor-positive direction is defined as decreasing encoder angle.
    float error = pll->theta_hat - theta;
    if (error > PI) error -= 2 * PI;
    else if (error < - PI) error += 2 * PI;

    float omega_unsat = pll->kp * error + pll->omega_integral;
    pll->omega_hat = Alg_Clamp(omega_unsat, -pll->omega_max, pll->omega_max);

    // Anti-windup backCalc
    pll->omega_integral += pll->ts * (pll->ki * error + pll->kaw * (pll->omega_hat - omega_unsat));

    pll->theta_hat -= pll->ts * pll->omega_hat;
    if (pll->theta_hat > 2 * PI) pll->theta_hat -= 2 * PI;
    else if (pll->theta_hat < 0) pll->theta_hat += 2 * PI;
}
