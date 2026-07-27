/**
*   @file Alg_Svpwm.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#include "Alg_Svpwm.h"
#include "Algorithm.h"
#include "Alg_math.h"
#include "FDriver_config.h"

/**
 * @brief 带扇区判断的SVPWM
 * @param Vdc 母线电压
 * @param uAlphaBeta Ualpha，Ubeta
 * @param switchTim 三相开关时间
 * @param duty 三相pwm实际占空比(0~1)
 */
void Alg_SvpwmWithSector(float Vdc, Alg_2Sys_s* uAlphaBeta, Alg_3Sys_s* switchTim, Alg_3Sys_s* duty) {
    float vRef1 = uAlphaBeta->b;
    float vRef2 = sqrt3 / 2.f * uAlphaBeta->a - 0.5f * uAlphaBeta->b;
    float vRef3 = - sqrt3 / 2.f * uAlphaBeta->a - 0.5f * uAlphaBeta->b;

    uint8_t a = (vRef1 > 0) ? 1 : 0;
    uint8_t b = (vRef2 > 0) ? 1 : 0;
    uint8_t c = (vRef3 > 0) ? 1 : 0;

    uint8_t N = 4 * c + 2 * b + a;
    // 计算归一化时间的中间变量
    float X = sqrt3 * uAlphaBeta->b / Vdc;
    float Y = sqrt3 * (sqrt3 * uAlphaBeta->a + uAlphaBeta->b) / (2.f * Vdc);
    float Z = sqrt3 * (- sqrt3 * uAlphaBeta->a + uAlphaBeta->b) / (2.f * Vdc);

    float T4 = 0.f, T6 = 0.f;
    switch (N) {
        case 1:
            T4 = Z;
            T6 = Y;
            break;
        case 2:
            T4 = Y;
            T6 = -X;
            break;
        case 3:
            T4 = -Z;
            T6 = X;
            break;
        case 4:
            T4 = -X;
            T6 = Z;
            break;
        case 5:
            T4 = X;
            T6 = -Y;
            break;
        case 6:
            T4 = -Y;
            T6 = -Z;
            break;
        default:
            switchTim->a = 0;
            switchTim->b = 0;
            switchTim->c = 0;
            duty->a = 1.f - 2.f * switchTim->a;
            duty->b = 1.f - 2.f * switchTim->b;
            duty->c = 1.f - 2.f * switchTim->c;
            return;
    }

    if (T4 + T6 > 1.f) {
        float sum = T4 + T6;
        T4 /= sum;
        T6 /= sum;
    }

    float ta = (1.f - T4 - T6) / 4.f;
    float tb = ta + T4 / 2.f;
    float tc = tb + T6 / 2.f;

    switch (N) {
        case 1:
            switchTim->a = tb;
            switchTim->b = ta;
            switchTim->c = tc;
            duty->a = 1.f - 2.f * switchTim->a;
            duty->b = 1.f - 2.f * switchTim->b;
            duty->c = 1.f - 2.f * switchTim->c;
            break;

        case 2:
            switchTim->a = ta;
            switchTim->b = tc;
            switchTim->c = tb;
            duty->a = 1.f - 2.f * switchTim->a;
            duty->b = 1.f - 2.f * switchTim->b;
            duty->c = 1.f - 2.f * switchTim->c;
            break;

        case 3:
            switchTim->a = ta;
            switchTim->b = tb;
            switchTim->c = tc;
            duty->a = 1.f - 2.f * switchTim->a;
            duty->b = 1.f - 2.f * switchTim->b;
            duty->c = 1.f - 2.f * switchTim->c;
            break;

        case 4:
            switchTim->a = tc;
            switchTim->b = tb;
            switchTim->c = ta;
            duty->a = 1.f - 2.f * switchTim->a;
            duty->b = 1.f - 2.f * switchTim->b;
            duty->c = 1.f - 2.f * switchTim->c;
            break;

        case 5:
            switchTim->a = tc;
            switchTim->b = ta;
            switchTim->c = tb;
            duty->a = 1.f - 2.f * switchTim->a;
            duty->b = 1.f - 2.f * switchTim->b;
            duty->c = 1.f - 2.f * switchTim->c;
            break;

        case 6:
            switchTim->a = tb;
            switchTim->b = tc;
            switchTim->c = ta;
            duty->a = 1.f - 2.f * switchTim->a;
            duty->b = 1.f - 2.f * switchTim->b;
            duty->c = 1.f - 2.f * switchTim->c;
            break;

        default:
            switchTim->a = 0;
            switchTim->b = 0;
            switchTim->c = 0;
            duty->a = 1.f - 2.f * switchTim->a;
            duty->b = 1.f - 2.f * switchTim->b;
            duty->c = 1.f - 2.f * switchTim->c;
            return;
    }
}

void Alg_SvpwmZeroInject(float Vdc, Alg_2Sys_s* uAlphaBeta, Alg_3Sys_s* switchTim, Alg_3Sys_s* duty) {
    Alg_3Sys_s uvw = {
        .a = uAlphaBeta->a,
        .b = -0.5f * uAlphaBeta->a + sqrt3 / 2.f * uAlphaBeta->b,
        .c = -0.5f * uAlphaBeta->a - sqrt3 / 2.f * uAlphaBeta->b,
    };

    float u0 = - 0.5f * (Alg_Max3(&uvw) + Alg_Min3(&uvw));
    duty->a = 0.5f + (uvw.a + u0) / Vdc;
    duty->b = 0.5f + (uvw.b + u0) / Vdc;
    duty->c = 0.5f + (uvw.c + u0) / Vdc;

    // Convert normalized phase voltage (-1..1) to centered PWM duty (0..1).
    if (duty->a > 1.f) duty->a = 1.f;
    if (duty->a < 0.f) duty->a = 0.f;
    if (duty->b > 1.f) duty->b = 1.f;
    if (duty->b < 0.f) duty->b = 0.f;
    if (duty->c > 1.f) duty->c = 1.f;
    if (duty->c < 0.f) duty->c = 0.f;

    switchTim->a = 0.5f * (1 - duty->a);
    switchTim->b = 0.5f * (1 - duty->b);
    switchTim->c = 0.5f * (1 - duty->c);
}
