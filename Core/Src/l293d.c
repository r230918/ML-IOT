/**
 * @file l293d.c
 * @brief High-performance baremetal L293D driver implementation for NUCLEO-F446RE.
 */

#include "l293d.h"
#include "main.h"

static bool currentDirectionForward = true;

void L293D_Init(void) {
    /* 1. Ensure 1A (PB5) and 2A (PB6) are driven LOW initially */
    GPIOB->ODR &= ~((1U << 5) | (1U << 6));
    
    /* 2. Zero PWM compare register on TIM1_CH1 (PA8 / 1,2EN) */
    TIM1->CCR1 = 0;
    currentDirectionForward = true;
}

void L293D_SetDirection(bool forward) {
    currentDirectionForward = forward;
    if (forward) {
        /* Forward: 1A (PB5) = HIGH, 2A (PB6) = LOW */
        GPIOB->ODR |= (1U << 5);
        GPIOB->ODR &= ~(1U << 6);
    } else {
        /* Reverse: 1A (PB5) = LOW, 2A (PB6) = HIGH */
        GPIOB->ODR &= ~(1U << 5);
        GPIOB->ODR |= (1U << 6);
    }
}

void L293D_SetSpeed(uint16_t duty_permille) {
    if (duty_permille > 1000) duty_permille = 1000;

    /* Set compare value on TIM1 Channel 1 (PA8 -> Pin 1 of L293D / 1,2EN) */
    TIM1->CCR1 = (uint32_t)duty_permille;

    if (duty_permille > 0) {
        if (currentDirectionForward) {
            /* Active Forward: 1A = HIGH, 2A = LOW */
            GPIOB->ODR |= (1U << 5);
            GPIOB->ODR &= ~(1U << 6);
        } else {
            /* Active Reverse: 1A = LOW, 2A = HIGH */
            GPIOB->ODR &= ~(1U << 5);
            GPIOB->ODR |= (1U << 6);
        }
    }
}

void L293D_Brake(void) {
    /* Active Electric Brake: Drive both inputs LOW and disable PWM */
    GPIOB->ODR &= ~((1U << 5) | (1U << 6));
    TIM1->CCR1 = 0;
}

void L293D_Coast(void) {
    /* Freewheel Coast: Turn off PWM on Enable pin */
    TIM1->CCR1 = 0;
}
