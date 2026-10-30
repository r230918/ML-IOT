#include "stm32f446xx.h"
#include <stdio.h>
#include <stdlib.h>

static void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 3200; i++) {
        __asm__ volatile("nop");
    }
}

// 1. Simple UART2 Print (PA2 = TX at 115200 Baud)
static void uart2_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    
    // PA2 -> Alternate Function AF7 (USART2_TX)
    GPIOA->MODER &= ~(3U << (2 * 2));
    GPIOA->MODER |=  (2U << (2 * 2));
    GPIOA->AFR[0] |= (7U << (2 * 4));
    
    // 16 MHz / 115200 baud = 138.88 => BRR = 0x8B
    USART2->BRR = 0x8B;
    // CR1: UE (bit 13) | TE (bit 3)
    USART2->CR1 = (1U << 13) | (1U << 3);
}

static void uart_send_char(char c) {
    while (!(USART2->SR & (1U << 7))); // Wait until TXE (bit 7) is empty
    USART2->DR = (uint8_t)c;
}

static void uart_send_str(const char *s) {
    while (*s) uart_send_char(*s++);
}

// 2. Simple ADC1 (PA4 = IN4 = A2, PC0 = IN10 = A5)
static void adc_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    
    // PA4 (A2) and PC0 (A5) -> Analog mode (MODER = 11)
    GPIOA->MODER |= (3U << (4 * 2));
    GPIOC->MODER |= (3U << (0 * 2));
    
    ADC1->CR2 = (1U << 0); // ADON (bit 0)
}

static uint16_t adc_read(uint8_t channel) {
    ADC1->SQR3 = channel;
    ADC1->CR2 |= (1U << 30); // SWSTART (bit 30)
    while (!(ADC1->SR & (1U << 1))); // Wait for EOC (bit 1)
    return (uint16_t)ADC1->DR;
}

// 3. Motor Pins (PB5 = D4, PB6 = D10) & Green LED (PA5)
static void gpio_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    
    // PA5 (LED) -> Output
    GPIOA->MODER &= ~(3U << (5 * 2));
    GPIOA->MODER |=  (1U << (5 * 2));
    
    // PB5 (D4), PB6 (D10) -> Output
    GPIOB->MODER &= ~((3U << (5 * 2)) | (3U << (6 * 2)));
    GPIOB->MODER |=  ((1U << (5 * 2)) | (1U << (6 * 2)));
}

int main(void) {
    gpio_init();
    uart2_init();
    adc_init();

    uart_send_str("\r\n==================================================\r\n");
    uart_send_str(" SUPPLY-SAG EMULATOR: SIMPLE REAL-TIME CONTROLLER\r\n");
    uart_send_str("==================================================\r\n");

    // Calibrate current sensor zero-point at boot
    uint32_t zero_adc = 0;
    for (int i = 0; i < 20; i++) {
        zero_adc += adc_read(4); // PA4 (A2)
        delay_ms(10);
    }
    zero_adc /= 20;

    uint32_t loop_count = 0;

    while (1) {
        // Read Potentiometer on A5 (PC0, Channel 10) -> Bus Supply Voltage (0-15V)
        uint16_t v_raw = adc_read(10);
        float bus_voltage = (v_raw * 3.3f / 4095.0f) * (15.0f / 3.3f);
        uint16_t bus_mv = (uint16_t)(bus_voltage * 1000.0f);

        // Read ACS712 Current Sensor on A2 (PA4, Channel 4) -> Current (mA)
        uint16_t c_raw = adc_read(4);
        int32_t delta_adc = abs((int32_t)c_raw - (int32_t)zero_adc);
        float current_ma = (delta_adc * 3300.0f / 4095.0f) / 0.185f;

        char msg[128];

        // --- SIMPLE SUPPLY SAG DECISION LOGIC ---
        if (bus_voltage < 9.5f) {
            // SAG DETECTED! (Voltage < 9.5V) -> Stop Motor, Blink LED
            GPIOB->ODR &= ~(1U << 5); // 1A = LOW (Stop)
            GPIOB->ODR &= ~(1U << 6); // 2A = LOW
            GPIOA->ODR ^= (1U << 5);  // Toggle LED fast

            snprintf(msg, sizeof(msg),
                "[SAG DETECTED] Bus: %4.1fV | Curr: %4.0fmA | Motor: STOPPED (SAFE)\r\n",
                bus_voltage, current_ma);
            uart_send_str(msg);
        } else {
            // NORMAL! (Voltage >= 9.5V) -> Run Motor Forward, LED Solid ON
            GPIOB->ODR |=  (1U << 5); // 1A = HIGH (Forward)
            GPIOB->ODR &= ~(1U << 6); // 2A = LOW
            GPIOA->ODR |=  (1U << 5); // LED ON

            snprintf(msg, sizeof(msg),
                "[NORMAL 12V]   Bus: %4.1fV | Curr: %4.0fmA | Motor: RUNNING\r\n",
                bus_voltage, current_ma);
            uart_send_str(msg);
        }

        // Emit $DAT packet for ML engine compatibility
        char dat[128];
        snprintf(dat, sizeof(dat), "$DAT,1,0,%lu,%u,%u,%d,1200,0,%u,25,0*\r\n",
                 loop_count * 100, (bus_voltage < 9.5f) ? 4 : 3,
                 (bus_voltage < 9.5f) ? 0 : 800, (int)current_ma, bus_mv);
        uart_send_str(dat);

        loop_count++;
        delay_ms(150); // 150 ms loop
    }
}
