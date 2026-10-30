#include "stm32f446xx.h"

static void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms * 3200; i++) {
        __asm__ volatile("nop");
    }
}

int main(void) {
    // 1. Enable GPIOA, GPIOB, GPIOC clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;

    // 2. PA5 (Onboard Green LED LD2) -> Output
    GPIOA->MODER &= ~(3U << (5 * 2));
    GPIOA->MODER |=  (1U << (5 * 2));

    // 3. PA8 (Arduino D7 -> L293D Pin 1 / Enable) -> Output HIGH
    GPIOA->MODER &= ~(3U << (8 * 2));
    GPIOA->MODER |=  (1U << (8 * 2));
    GPIOA->ODR   |=  (1U << 8);

    // 4. PB5 (Arduino D4 -> L293D Pin 2 / 1A) -> Output HIGH (Forward)
    //    PB6 (Arduino D10 -> L293D Pin 7 / 2A) -> Output LOW
    GPIOB->MODER &= ~((3U << (5 * 2)) | (3U << (6 * 2)));
    GPIOB->MODER |=  ((1U << (5 * 2)) | (1U << (6 * 2)));
    GPIOB->ODR   |=  (1U << 5);  // 1A = HIGH
    GPIOB->ODR   &= ~(1U << 6);  // 2A = LOW

    // 5. Blink Green LED every 500 ms to confirm code is alive
    while (1) {
        GPIOA->ODR ^= (1U << 5);
        delay_ms(500);
    }
}
