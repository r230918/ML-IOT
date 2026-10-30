#include "motor_controller.h"
#include "main.h"

extern TIM_HandleTypeDef htim1;

void Motor_Init(void) {
    /* Initialize direction pins PB5 and PB6 low */
    GPIOB->ODR &= ~((1U << 5) | (1U << 6));
    TIM1->CCR1 = 0;
}

void Motor_SetDirection(bool forward) {
    if (forward) {
        /* Forward: PB5 = HIGH, PB6 = LOW */
        GPIOB->ODR |= (1U << 5);
        GPIOB->ODR &= ~(1U << 6);
    } else {
        /* Reverse: PB5 = LOW, PB6 = HIGH */
        GPIOB->ODR &= ~(1U << 5);
        GPIOB->ODR |= (1U << 6);
    }
}

void Motor_SetPWM(uint16_t duty_permille) {
    if (duty_permille > 1000) duty_permille = 1000;
    /* Set TIM1 Channel 1 compare value */
    TIM1->CCR1 = (uint32_t)duty_permille;
    if (duty_permille > 0) {
        /* Ensure H-Bridge forward direction is actively driven: PB5 = HIGH, PB6 = LOW */
        GPIOB->ODR |= (1U << 5);
        GPIOB->ODR &= ~(1U << 6);
    }
}

void Motor_EmergencyBrake(void) {
    /* Hard active brake on H-bridge by driving both outputs LOW */
    GPIOB->ODR &= ~((1U << 5) | (1U << 6));
    TIM1->CCR1 = 0;
}
