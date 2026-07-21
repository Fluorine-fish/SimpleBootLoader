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

/**
 * @brief 带扇区判断的SVPWM
 * @param uAlphaBeta 母线电压标幺化的Ualpha，Ubeta(0~1)
 * @param switchTim 三相开关时间
 * @param duty 三相pwm实际占空比(0~1)
 */
void Alg_SvpwmWithSector(Alg_2Sys_s* uAlphaBeta, Alg_3Sys_s* switchTim, Alg_3Sys_s* duty) {
    float vRef1 = uAlphaBeta->b;
    float vRef2 = sqrt3 / 2.f * uAlphaBeta->a - 0.5f * uAlphaBeta->b;
    float vRef3 = - sqrt3 / 2.f * uAlphaBeta->a - 0.5f * uAlphaBeta->b;

    uint8_t a = (vRef1 > 0) ? 1 : 0;
    uint8_t b = (vRef2 > 0) ? 1 : 0;
    uint8_t c = (vRef3 > 0) ? 1 : 0;

    uint8_t N = 4 * c + 2 * b + a;
    float X = uAlphaBeta -> b;
    float Y = 0.5f * (sqrt3 * uAlphaBeta->a + uAlphaBeta->b);
    float Z = 0.5f * (- sqrt3 * uAlphaBeta->a + uAlphaBeta->b);

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
