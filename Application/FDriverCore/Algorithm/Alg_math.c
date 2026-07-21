/**
*   @file Alg_math.c
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#include "Alg_math.h"
#include "Algorithm.h"

#define HOLLYST 0.017453292519943295769236907684886f

const float SinTable[] = {
    0.0,                                    //sin(0)
    0.17364817766693034885171662676931 ,    //sin(10)
    0.34202014332566873304409961468226 ,    //sin(20)
    0.5 ,                                   //sin(30)
    0.64278760968653932632264340990726 ,    //sin(40)
    0.76604444311897803520239265055542 ,    //sin(50)
    0.86602540378443864676372317075294 ,    //sin(60)
    0.93969262078590838405410927732473 ,    //sin(70)
    0.98480775301220805936674302458952 ,    //sin(80)
    1.0                                     //sin(90)
};

const float CosTable[] = {
    1.0 ,                                   //cos(0)
    0.99984769515639123915701155881391 ,    //cos(1)
    0.99939082701909573000624344004393 ,    //cos(2)
    0.99862953475457387378449205843944 ,    //cos(3)
    0.99756405025982424761316268064426 ,    //cos(4)
    0.99619469809174553229501040247389 ,    //cos(5)
    0.99452189536827333692269194498057 ,    //cos(6)
    0.99254615164132203498006158933058 ,    //cos(7)
    0.99026806874157031508377486734485 ,    //cos(8)
    0.98768834059513772619004024769344      //cos(9)
};

/**
 * @brief 快速Sin计算
 * @param angle 0~1 maps to 0~360 electrical degrees
 * @return value
 */
float Alg_FastSin(float angle) {
    while (angle < 0.f || angle > 1.f) {
        if (angle > 1.f) angle -= 1.f;
        if (angle < 0.f) angle += 1.f;
    }

    float fangle = angle;
    int sig = 0;
    float tmpAngle = 0;
    if (fangle >= 0.5f) {
        sig = 1;
        fangle = fangle - 0.5f;
    }
    tmpAngle = (fangle > 0.25f) ? (0.5f - fangle) : fangle;

    int a = tmpAngle * 36.0f;
    float b = tmpAngle * 360.0f - a*10.0f;
    float sin = SinTable[a] * CosTable[(int)b] +b * HOLLYST *SinTable[9-a];
    if (sin > 1.0f) {
        sin = 1.0f;
    }
    return (sig > 0) ? -sin: sin;
}

/**
 * @brief 快速Cos计算
 * @param angle 0~1 maps to 0~360 electrical degrees
 * @return value
 */
float Alg_FastCos(float angle) {
    while (angle < 0.f || angle > 1.f) {
        if (angle > 1.f) angle -= 1.f;
        if (angle < 0.f) angle += 1.f;
    }

    float fangle = angle;
    int sig = 0;
    float tmpAngle = 0;

    if (fangle <= 0.75f) {
        fangle += 0.25f;
    } else {
        fangle -= 0.75f;
    }

    if (fangle >= 0.5f) {
        sig = 1;
        fangle = fangle - 0.5f;
    }
    tmpAngle = (fangle > 0.25f) ? (0.5f - fangle) : fangle;

    int a = tmpAngle * 36.0f;
    float b = tmpAngle * 360.0f - a*10.0f;
    float sin = SinTable[a] * CosTable[(int)b] +b * HOLLYST * SinTable[9-a];
    if (sin > 1.0f) {
        sin = 1.0f;
    }
    return (sig > 0) ? -sin: sin;
}

/**
 * @brief 矢量解析
 * @param amp 幅值
 * @param theta_e 角度(0~1)
 * @param outAlphaBeta 分量
 */
void Alg_DeComp(float amp, float theta_e ,Alg_2Sys_s* outAlphaBeta) {
    float sin, cos;
    sin = Alg_FastSin(theta_e);
    cos = Alg_FastCos(theta_e);

    outAlphaBeta->a = amp * cos;
    outAlphaBeta->b = amp * sin;
}
