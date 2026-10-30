#ifndef STM32F446XX_H
#define STM32F446XX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define __IO volatile
#define __I  volatile const
#define __O  volatile

/* Peripheral Base Addresses for STM32F446 */
#define PERIPH_BASE           0x40000000UL
#define APB1PERIPH_BASE       PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE       (PERIPH_BASE + 0x00020000UL)

/* GPIO on AHB1 */
#define GPIOA_BASE            (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE            (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE            (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE            (AHB1PERIPH_BASE + 0x0C00UL)

/* Timers */
#define TIM1_BASE             (APB2PERIPH_BASE + 0x0000UL)
#define TIM2_BASE             (APB1PERIPH_BASE + 0x0000UL)
#define TIM3_BASE             (APB1PERIPH_BASE + 0x0400UL)

/* I2C1 on APB1 */
#define I2C1_BASE             (APB1PERIPH_BASE + 0x5400UL)

/* USART2 on APB1 (Connected to ST-LINK on NUCLEO-F446RE) */
#define USART2_BASE           (APB1PERIPH_BASE + 0x4400UL)

/* ADC1 on APB2 */
#define ADC1_BASE             (APB2PERIPH_BASE + 0x2000UL)

/* RCC on AHB1 */
#define RCC_BASE              (AHB1PERIPH_BASE + 0x3800UL)

typedef struct {
  __IO uint32_t MODER;
  __IO uint32_t OTYPER;
  __IO uint32_t OSPEEDR;
  __IO uint32_t PUPDR;
  __IO uint32_t IDR;
  __IO uint32_t ODR;
  __IO uint32_t BSRR;
  __IO uint32_t LCKR;
  __IO uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct {
  __IO uint32_t CR1;
  __IO uint32_t CR2;
  __IO uint32_t SMCR;
  __IO uint32_t DIER;
  __IO uint32_t SR;
  __IO uint32_t EGR;
  __IO uint32_t CCMR1;
  __IO uint32_t CCMR2;
  __IO uint32_t CCER;
  __IO uint32_t CNT;
  __IO uint32_t PSC;
  __IO uint32_t ARR;
  __IO uint32_t RCR;
  __IO uint32_t CCR1;
  __IO uint32_t CCR2;
  __IO uint32_t CCR3;
  __IO uint32_t CCR4;
  __IO uint32_t BDTR;
} TIM_TypeDef;

typedef struct {
  __IO uint32_t CR1;
  __IO uint32_t CR2;
  __IO uint32_t OAR1;
  __IO uint32_t OAR2;
  __IO uint32_t DR;
  __IO uint32_t SR1;
  __IO uint32_t SR2;
  __IO uint32_t CCR;
  __IO uint32_t TRISE;
  __IO uint32_t FLTR;
} I2C_TypeDef;

typedef struct {
  __IO uint32_t SR;
  __IO uint32_t DR;
  __IO uint32_t BRR;
  __IO uint32_t CR1;
  __IO uint32_t CR2;
  __IO uint32_t CR3;
  __IO uint32_t GTPR;
} USART_TypeDef;

typedef struct {
  __IO uint32_t SR;
  __IO uint32_t CR1;
  __IO uint32_t CR2;
  __IO uint32_t SMPR1;
  __IO uint32_t SMPR2;
  __IO uint32_t JOFR1;
  __IO uint32_t JOFR2;
  __IO uint32_t JOFR3;
  __IO uint32_t JOFR4;
  __IO uint32_t HTR;
  __IO uint32_t LTR;
  __IO uint32_t SQR1;
  __IO uint32_t SQR2;
  __IO uint32_t SQR3;
  __IO uint32_t JSQR;
  __IO uint32_t JDR1;
  __IO uint32_t JDR2;
  __IO uint32_t JDR3;
  __IO uint32_t JDR4;
  __IO uint32_t DR;
} ADC_TypeDef;

typedef struct {
  __IO uint32_t CR;            /* 0x00 */
  __IO uint32_t PLLCFGR;       /* 0x04 */
  __IO uint32_t CFGR;          /* 0x08 */
  __IO uint32_t CIR;           /* 0x0C */
  __IO uint32_t AHB1RSTR;      /* 0x10 */
  __IO uint32_t AHB2RSTR;      /* 0x14 */
  __IO uint32_t AHB3RSTR;      /* 0x18 */
  uint32_t      RESERVED0;     /* 0x1C */
  __IO uint32_t APB1RSTR;      /* 0x20 */
  __IO uint32_t APB2RSTR;      /* 0x24 */
  uint32_t      RESERVED1[2];  /* 0x28, 0x2C */
  __IO uint32_t AHB1ENR;       /* 0x30 */
  __IO uint32_t AHB2ENR;       /* 0x34 */
  __IO uint32_t AHB3ENR;       /* 0x38 */
  uint32_t      RESERVED2;     /* 0x3C */
  __IO uint32_t APB1ENR;       /* 0x40 */
  __IO uint32_t APB2ENR;       /* 0x44 */
} RCC_TypeDef;

#define GPIOA  ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB  ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC  ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD  ((GPIO_TypeDef *) GPIOD_BASE)
#define TIM1   ((TIM_TypeDef *) TIM1_BASE)
#define TIM2   ((TIM_TypeDef *) TIM2_BASE)
#define TIM3   ((TIM_TypeDef *) TIM3_BASE)
#define I2C1   ((I2C_TypeDef *) I2C1_BASE)
#define USART2 ((USART_TypeDef *) USART2_BASE)
#define ADC1   ((ADC_TypeDef *) ADC1_BASE)
#define RCC    ((RCC_TypeDef *) RCC_BASE)

/* RCC Enable Bits */
#define RCC_AHB1ENR_GPIOAEN   (1U << 0)
#define RCC_AHB1ENR_GPIOBEN   (1U << 1)
#define RCC_AHB1ENR_GPIOCEN   (1U << 2)
#define RCC_APB1ENR_TIM2EN    (1U << 0)
#define RCC_APB1ENR_USART2EN  (1U << 17)
#define RCC_APB1ENR_I2C1EN    (1U << 21)
#define RCC_APB2ENR_TIM1EN    (1U << 0)
#define RCC_APB2ENR_ADC1EN    (1U << 8)

/* Cortex-M4 SysTick */
typedef struct {
  __IO uint32_t CTRL;
  __IO uint32_t LOAD;
  __IO uint32_t VAL;
  __I  uint32_t CALIB;
} SysTick_TypeDef;
#define SysTick_BASE (0xE000E010UL)
#define SysTick      ((SysTick_TypeDef *) SysTick_BASE)
#define SysTick_CTRL_CLKSOURCE_Msk (1UL << 2)
#define SysTick_CTRL_TICKINT_Msk   (1UL << 1)
#define SysTick_CTRL_ENABLE_Msk    (1UL << 0)

#ifdef __cplusplus
}
#endif

#endif /* STM32F446XX_H */
