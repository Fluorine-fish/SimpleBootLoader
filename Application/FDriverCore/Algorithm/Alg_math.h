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
float Alg_Max3(Alg_3Sys_s* input);
float Alg_Min3(Alg_3Sys_s* input);
float Alg_Clamp(float input, float min, float max);


void Alg_DeComp(Alg_vector_s* vector,Alg_2Sys_s* outAlphaBeta);
void Alg_Clarke(Alg_3Sys_s* input, Alg_2Sys_s* outAlphaBeta);
void Alg_Park(float theta_e, Alg_2Sys_s* AlphaBeta, Alg_2Sys_s* outDq);
void Alg_InvClarke(Alg_2Sys_s* inAlphaBeta, Alg_3Sys_s* out);
void Alg_InvPark(float theta_e, Alg_2Sys_s* inDq, Alg_2Sys_s* outAlphaBeta);

#endif //APPLICATION_ALG_MATH_H
