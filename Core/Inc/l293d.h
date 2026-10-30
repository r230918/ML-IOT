/**
 * @file l293d.h
 * @brief Dedicated L293D Quadruple Half-H Driver Header for STM32F446RE.
 * 
 * Pinout Mapping:
 * - L293D Pin 1 (1,2EN / Enable 1): Connected to PA8 (Arduino D7, TIM1_CH1 PWM)
 * - L293D Pin 2 (1A / Input 1):     Connected to PB5 (Arduino D4, GPIO Output)
 * - L293D Pin 7 (2A / Input 2):     Connected to PB6 (Arduino D10, GPIO Output)
 * - L293D Pin 3, 6 (1Y, 2Y Outputs): Connected to DC Motor terminals
 * - L293D Pin 4, 5, 12, 13 (GND):   Connected to Common Ground (Nucleo GND + Power GND)
 * - L293D Pin 8 (VCC2 / Motor Pwr): Connected to 6V - 12V Motor Power
 * - L293D Pin 16 (VCC1 / Logic Pwr): Connected to Nucleo 5V Pin
 */

#ifndef L293D_H
#define L293D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initializes the L293D GPIO direction pins and PWM timer channel.
 */
void L293D_Init(void);

/**
 * @brief Sets the motor direction.
 * @param forward true for forward (1A=HIGH, 2A=LOW), false for reverse (1A=LOW, 2A=HIGH)
 */
void L293D_SetDirection(bool forward);

/**
 * @brief Sets the motor PWM speed duty cycle.
 * @param duty_permille Speed from 0 (stopped) to 1000 (100% full speed)
 */
void L293D_SetSpeed(uint16_t duty_permille);

/**
 * @brief Active electric brake (drives both inputs LOW and disables PWM).
 */
void L293D_Brake(void);

/**
 * @brief Freewheel coasting stop.
 */
void L293D_Coast(void);

#ifdef __cplusplus
}
#endif

#endif /* L293D_H */
