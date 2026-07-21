/**
*   @file Algorithm.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_ALGORITHM_H
#define APPLICATION_ALGORITHM_H

#include "stdint.h"

/* 定义常量和结构体 */
#define PI (3.1415926536f)
#define sqrt3 (1.7320508076f)

typedef struct {
    float a;
    float b;
}Alg_2Sys_s;

typedef struct {
    float a;
    float b;
    float c;
}Alg_3Sys_s;

#endif //APPLICATION_ALGORITHM_H
