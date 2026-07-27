/**
*   @file FDriver_config.h
*   @brief 
*   @author Wenxin HU
*   @date 2026/7/20
*   @version 1.0
*   @note
*/
#ifndef APPLICATION_FDRIVER_CONFIG_H
#define APPLICATION_FDRIVER_CONFIG_H

/* MotorData */
#define MOTOR_RS 0.61f
#define MOTOR_LS 0.000405f
#define MOTOR_TS 0.00005f
#define MOTOR_POLE_PAIRS 14

/* BoardData */
#define ADC_REVOLUTION 12
#define ADC_VREF 3.3f
#define CURRENT_ADC_GAIN 9
#define CURRENT_MAX 36.67f
#define VDC_ADC_GAIN
#define VDC_SET 24.f
#define VDC_MAX 36.f
#define VDC_MIN 15.f
#define PWM_FREQ 20000.f

/* Align */
#define ALIGN_VOLTAGE (VDC_SET * 0.1f)
#define ALIGN_TIME 10000

/* ControlData */
#define CONTROLMODE FDRIVER_IF
/* OpenLoop */
#define OMEGA_E 250
#define V_MODULE 0.15

/* CurrentController */
#define CURRENT_CUT_OFF_FREQ 800.f
#define CURRENT_TS (1/PWM_FREQ)

#endif //APPLICATION_FDRIVER_CONFIG_H
