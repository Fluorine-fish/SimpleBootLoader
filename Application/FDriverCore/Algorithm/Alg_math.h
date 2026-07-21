/**
*   @file Alg_math.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_ALG_MATH_H
#define APPLICATION_ALG_MATH_H

#include "Algorithm.h"

float Alg_FastSin(float angle);
float Alg_FastCos(float angle);

void Alg_DeComp(float amp, float theta_e ,Alg_2Sys_s* outAlphaBeta);

#endif //APPLICATION_ALG_MATH_H
