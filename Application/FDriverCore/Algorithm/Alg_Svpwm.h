/**
*   @file Alg_Svpwm.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_ALG_SVPWM_H
#define APPLICATION_ALG_SVPWM_H

#include "Algorithm.h"

void Alg_SvpwmWithSector(float Vdc, Alg_2Sys_s* uAlphaBeta, Alg_3Sys_s* switchTim, Alg_3Sys_s* duty);
void Alg_SvpwmZeroInject(float Vdc, Alg_2Sys_s* uAlphaBeta, Alg_3Sys_s* switchTim, Alg_3Sys_s* duty);

#endif //APPLICATION_ALG_SVPWM_H
