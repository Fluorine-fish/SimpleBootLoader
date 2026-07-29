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
#define MOTOR_K_TORQUE 0.64f
#define MOTOR_TORQUE_TO_IQ_SIGN (-1.0f)
#define MOTOR_CURRENT_RATED 3.06f
#define MOTOR_CURRENT_STALL 18.06f
#define MOTOR_MAX_RPM 700
#define MOTOR_MAX_OMEGA_E (MOTOR_GEAR_RATO * MOTOR_MAX_RPM * MOTOR_POLE_PAIRS * 2 * PI / 60.f)
#define MOTOR_MAX_OMEGA_M (MOTOR_GEAR_RATO * MOTOR_MAX_RPM * 2 * PI / 60)
#define MOTOR_GEAR_RATO 6

/* BoardData */
#define ADC_REVOLUTION 12
#define ADC_VREF 3.3f
#define CURRENT_ADC_GAIN 9
#define VDC_ADC_GAIN
#define VDC_SET 24.f
#define VDC_MAX 36.f
#define VDC_MIN 15.f
#define PWM_FREQ 20000.f

/* Align */
#define ALIGN_VOLTAGE (VDC_SET * 0.1f)
#define ALIGN_TIME 10000

/* ControlData */
#define CONTROLMODE FDRIVER_TORQUE
/* OpenLoop */
#define OMEGA_E 250
#define V_MODULE 0.15
/* CurrentController */
#define CURRENT_CUT_OFF_FREQ 800.f
#define CURRENT_TS (1/PWM_FREQ)
#define CURRENT_CROSS_COUPLING_ENABLE 1
/* PLL */
#define PLL_ZETA 0.707f
#define PLL_FREQ 15.f
#define PLL_TS CURRENT_TS

#endif //APPLICATION_FDRIVER_CONFIG_H
