#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

#define VARIANT_ID_CODE          0x0207
#define FIRMWARE_VERSION_MAJOR   1
#define FIRMWARE_VERSION_MINOR   0

/* Hardware Pin Assignments for NUCLEO-F446RE (Nucleo-64) */
#define USER_BTN_PIN             GPIO_PIN_13
#define USER_BTN_PORT            GPIOC

#define LED_GREEN_PIN            GPIO_PIN_5   /* Onboard LD2 Green */
#define LED_GREEN_PORT           GPIOA

#define LED_YELLOW_PIN           GPIO_PIN_0   /* Morpho / Breadboard */
#define LED_YELLOW_PORT          GPIOB

#define LED_RED_PIN              GPIO_PIN_7   /* Morpho / Breadboard */
#define LED_RED_PORT             GPIOB

/* Motor PWM & Direction (TIM1_CH1 on PA8, DIR on PB5/PB6) */
#define MOTOR_PWM_PIN            GPIO_PIN_8
#define MOTOR_PWM_PORT           GPIOA

#define MOTOR_DIR1_PIN           GPIO_PIN_5
#define MOTOR_DIR1_PORT          GPIOB

#define MOTOR_DIR2_PIN           GPIO_PIN_6
#define MOTOR_DIR2_PORT          GPIOB

/* Quadrature Encoder (TIM2_CH1 on PA0, TIM2_CH2 on PA1) */
#define ENCODER_CH1_PIN          GPIO_PIN_0
#define ENCODER_CH1_PORT         GPIOA

#define ENCODER_CH2_PIN          GPIO_PIN_1
#define ENCODER_CH2_PORT         GPIOA

/* Analog Sensor Channels (ADC1) */
#define CURRENT_SENSE_PIN        GPIO_PIN_4   /* ADC1_IN4 (PA4, A2 pin) */
#define CURRENT_SENSE_PORT       GPIOA

#define SUPPLY_PROXY_PIN         GPIO_PIN_0   /* ADC1_IN10 (PC0, A5 pin) */
#define SUPPLY_PROXY_PORT        GPIOC

#define TEMP_SENSE_PIN           GPIO_PIN_6   /* ADC1_IN6 (PA6, D12 pin) */
#define TEMP_SENSE_PORT          GPIOA

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
