#include "stm32f446xx.h"
#include <stdio.h>
#include <stdlib.h>

static void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 3200; i++) __asm__ volatile("nop");
}

// 1. USART2 (PA2 = TX @ 115200 Baud)
static void uart2_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    GPIOA->MODER &= ~(3U << (2 * 2));
    GPIOA->MODER |=  (2U << (2 * 2));
    GPIOA->AFR[0] |= (7U << (2 * 4));
    USART2->BRR = 0x8B;                   // 115200 baud @ 16 MHz
    USART2->CR1 = (1U << 13) | (1U << 3); // UE | TE
}

static void uart_send_str(const char *s) {
    while (*s) {
        while (!(USART2->SR & (1U << 7)));
        USART2->DR = (uint8_t)(*s++);
    }
}

// 2. ADC1: PC0 (A5 - Bus Sag Proxy) & PA4 (A2 - ACS712 Current)
static void adc_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    GPIOA->MODER |= (3U << (4 * 2));      // PA4 Analog
    GPIOC->MODER |= (3U << (0 * 2));      // PC0 Analog
    ADC1->CR2 = (1U << 0);                // ADON
}

static uint16_t adc_read(uint8_t ch) {
    ADC1->SQR3 = ch;
    ADC1->CR2 |= (1U << 30);              // SWSTART
    while (!(ADC1->SR & (1U << 1)));      // Wait EOC
    return (uint16_t)ADC1->DR;
}

// 3. GPIO: Motor D4 (PB5), D10 (PB6), Green LED LD2 (PA5)
static void gpio_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    GPIOA->MODER &= ~(3U << (5 * 2));
    GPIOA->MODER |=  (1U << (5 * 2));
    GPIOB->MODER &= ~((3U << (5 * 2)) | (3U << (6 * 2)));
    GPIOB->MODER |=  ((1U << (5 * 2)) | (1U << (6 * 2)));
}

int main(void) {
    gpio_init();
    uart2_init();
    adc_init();

    // Auto-calibrate Current Sensor baseline (Vzero ~ 2.5V)
    uint32_t zero_adc = 0;
    for (int i = 0; i < 20; i++) { zero_adc += adc_read(4); delay_ms(10); }
    zero_adc /= 20;

    uint32_t t_ms = 0;
    uint16_t run_id = 101;

    while (1) {
        // Read Potentiometer on A5 (PC0) -> Supply Proxy mV (0 to 15000 mV)
        uint16_t v_raw = adc_read(10);
        uint32_t supply_mv = ((uint32_t)v_raw * 15000U) / 4095U;

        // Read ACS712 on A2 (PA4) -> Current mA
        uint16_t c_raw = adc_read(4);
        int32_t delta = abs((int32_t)c_raw - (int32_t)zero_adc);
        uint16_t current_ma = (uint16_t)((delta * 3300U * 1000U) / (4095U * 185U));

        // Evaluate Condition & Action matching data/run_manifest.csv
        uint8_t cond_id;
        uint8_t state;
        uint16_t pwm;
        uint16_t speed_rpm;

        if (supply_mv >= 11000) {
            // Condition 0: Nominal_12V -> ACCEPT_START (PWM 800)
            cond_id = 0;
            state = 3; // RUNNING
            pwm = 800;
            speed_rpm = (uint16_t)((supply_mv * 75U) / 1000U);
            GPIOB->ODR |=  (1U << 5);  // D4 = 1
            GPIOB->ODR &= ~(1U << 6);  // D10 = 0
            GPIOA->ODR |=  (1U << 5);  // LED Solid ON
        } else if (supply_mv >= 9500) {
            // Condition 1: Mild_Sag_10.5V -> DERATE (PWM 400)
            cond_id = 1;
            state = 4; // DERATED
            pwm = 400;
            speed_rpm = (uint16_t)((supply_mv * 55U) / 1000U);
            GPIOB->ODR |=  (1U << 5);  // D4 = 1 (Running)
            GPIOB->ODR &= ~(1U << 6);  // D10 = 0
            GPIOA->ODR ^=  (1U << 5);  // LED Fast Blink
        } else if (supply_mv >= 8000) {
            // Condition 2: Moderate_Sag_9.0V -> DERATE (PWM 400)
            cond_id = 2;
            state = 4; // DERATED
            pwm = 400;
            speed_rpm = (uint16_t)((supply_mv * 45U) / 1000U);
            GPIOB->ODR |=  (1U << 5);
            GPIOB->ODR &= ~(1U << 6);
            GPIOA->ODR ^=  (1U << 5);
        } else {
            // Condition 3: Severe_Sag_7.5V -> STOP (PWM 0)
            cond_id = 3;
            state = 6; // STOPPED
            pwm = 0;
            speed_rpm = 0;
            GPIOB->ODR &= ~((1U << 5) | (1U << 6)); // Motor OFF
            GPIOA->ODR &= ~(1U << 5);               // LED OFF
        }

        // Emit telemetry matching data/seed_data_runs.csv format
        char buf[96];
        snprintf(buf, sizeof(buf), "$DAT,%u,%u,%lu,%u,%u,%u,%u,0,%u,24,0*\r\n",
                 run_id, cond_id, t_ms, state, pwm, current_ma, speed_rpm, (unsigned int)supply_mv);
        uart_send_str(buf);

        t_ms += 100;
        delay_ms(100);
    }
}
