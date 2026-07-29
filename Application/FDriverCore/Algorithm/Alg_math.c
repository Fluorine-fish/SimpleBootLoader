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
 * @param angle 0～2pi
 * @return value
 */
float Alg_FastSin(float angle) {
    while (angle < 0.f || angle > 2 * PI) {
        if (angle > 2 * PI) angle -= 2 * PI;
        if (angle < 0.f) angle += 2 * PI;
    }


    float fangle = angle / (2 * PI);
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
 * @param angle 0~2pi
 * @return value
 */
float Alg_FastCos(float angle) {
    while (angle < 0.f || angle > 2 * PI) {
        if (angle > 2 * PI) angle -= 2 * PI;
        if (angle < 0.f) angle += 2 * PI;
    }

    float fangle = fangle = angle / (2 * PI);
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

// 三元素最大值
float Alg_Max3(Alg_3Sys_s* input) {
    if (input->a > input->b) {
        if (input->a > input->c) return input->a;
        else return input->c;
    }else {
        if (input->b > input->c) return input->b;
        else return input->c;
    }
}

// 三元素最小值
float Alg_Min3(Alg_3Sys_s* input) {
    if (input->a < input->b) {
        if (input->a < input->c) return input->a;
        else return input->c;
    }else {
        if (input->b < input->c) return input->b;
        else return input->c;
    }
}

/**
 * @brief 矢量分解
 * @param vector 输入矢量
 * @param outAlphaBeta 分量
 */
void Alg_DeComp(Alg_vector_s* vector,Alg_2Sys_s* outAlphaBeta) {
    float sin, cos;
    sin = Alg_FastSin(vector->arg);
    cos = Alg_FastCos(vector->arg);

    outAlphaBeta->a = vector->module * cos;
    outAlphaBeta->b = vector->module * sin;
}

/**
 * @brief Clarke变换
 * @param input 输入静止三相坐标系
 * @param outAlphaBeta 输出静止两相坐标系
 */
void Alg_Clarke(Alg_3Sys_s* input, Alg_2Sys_s* outAlphaBeta) {
    outAlphaBeta->a = (2.f / 3.f) * (input->a - 0.5f * input->b - 0.5f * input->c);
    outAlphaBeta->b = (2.f / 3.f) * (0.5f * sqrt3 * input->b - 0.5f * sqrt3 *input->c);
}

/**
 * @brief Park变换
 * @param theta_e 0~1 maps to 0~360 electrical degrees
 * @param AlphaBeta 输入静止两相坐标系
 * @param outDq 输出旋转两相坐标系
 */
void Alg_Park(float theta_e, Alg_2Sys_s* AlphaBeta, Alg_2Sys_s* outDq) {
    outDq->a = Alg_FastCos(theta_e) * AlphaBeta->a + Alg_FastSin(theta_e) * AlphaBeta->b;
    outDq->b = - Alg_FastSin(theta_e) * AlphaBeta->a + Alg_FastCos(theta_e) * AlphaBeta->b;
}


/**
 * @brief InvClarke变换
 * @param inAlphaBeta 输出静止两相坐标系
 * @param out 输出静止三相坐标系
 */
void Alg_InvClarke(Alg_2Sys_s* inAlphaBeta, Alg_3Sys_s* out) {
    out->a = inAlphaBeta->a;
    out->b = -0.5f * inAlphaBeta->a + 0.5f * sqrt3 * inAlphaBeta->b;
    out->c = -0.5f * inAlphaBeta->a - 0.5f * sqrt3 * inAlphaBeta->b;
}
/**
 * @brief InvPark变换
 * @param theta_e 0~1 maps to 0~360 electrical degrees
 * @param inDq 输入旋转两相坐标系
 * @param outAlphaBeta 输出静止两相坐标系
 */
void Alg_InvPark(float theta_e, Alg_2Sys_s* inDq, Alg_2Sys_s* outAlphaBeta) {
    outAlphaBeta->a = Alg_FastCos(theta_e) * inDq->a - Alg_FastSin(theta_e) * inDq->b;
    outAlphaBeta->b = Alg_FastSin(theta_e) * inDq->a + Alg_FastCos(theta_e) * inDq->b;
}

float Alg_Clamp(float input, float min, float max) {
    if (input < min) return min;
    if (input > max) return max;
    return input;
}
