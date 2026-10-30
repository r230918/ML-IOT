#ifndef STM32F4XX_HAL_H
#define STM32F4XX_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f446xx.h"

typedef enum {
  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

typedef enum {
  GPIO_PIN_RESET = 0U,
  GPIO_PIN_SET
} GPIO_PinState;

#define GPIO_PIN_0                 ((uint16_t)0x0001)
#define GPIO_PIN_1                 ((uint16_t)0x0002)
#define GPIO_PIN_2                 ((uint16_t)0x0004)
#define GPIO_PIN_3                 ((uint16_t)0x0008)
#define GPIO_PIN_4                 ((uint16_t)0x0010)
#define GPIO_PIN_5                 ((uint16_t)0x0020)
#define GPIO_PIN_6                 ((uint16_t)0x0040)
#define GPIO_PIN_7                 ((uint16_t)0x0080)
#define GPIO_PIN_8                 ((uint16_t)0x0100)
#define GPIO_PIN_9                 ((uint16_t)0x0200)
#define GPIO_PIN_10                ((uint16_t)0x0400)
#define GPIO_PIN_11                ((uint16_t)0x0800)
#define GPIO_PIN_12                ((uint16_t)0x1000)
#define GPIO_PIN_13                ((uint16_t)0x2000)
#define GPIO_PIN_14                ((uint16_t)0x4000)
#define GPIO_PIN_15                ((uint16_t)0x8000)
#define GPIO_PIN_All               ((uint16_t)0xFFFF)

#define GPIO_MODE_INPUT            0x00000000U
#define GPIO_MODE_OUTPUT_PP        0x00000001U
#define GPIO_MODE_AF_PP            0x00000002U
#define GPIO_MODE_ANALOG           0x00000003U

#define GPIO_NOPULL                0x00000000U
#define GPIO_PULLUP                0x00000001U
#define GPIO_PULLDOWN              0x00000002U

#define GPIO_SPEED_FREQ_LOW        0x00000000U
#define GPIO_SPEED_FREQ_HIGH       0x00000002U
#define GPIO_SPEED_FREQ_VERY_HIGH  0x00000003U

#define TIM_CHANNEL_1              0x00000000U
#define TIM_CHANNEL_2              0x00000004U
#define TIM_CHANNEL_3              0x00000008U
#define TIM_CHANNEL_4              0x0000000CU
#define TIM_CHANNEL_ALL            0x0000003CU

#define HAL_MAX_DELAY              0xFFFFFFFFU

typedef struct {
  uint32_t Pin;
  uint32_t Mode;
  uint32_t Pull;
  uint32_t Speed;
  uint32_t Alternate;
} GPIO_InitTypeDef;

typedef struct {
  uint32_t BaudRate;
  uint32_t WordLength;
  uint32_t StopBits;
  uint32_t Parity;
  uint32_t Mode;
} UART_InitTypeDef;

typedef struct {
  USART_TypeDef    *Instance;
  UART_InitTypeDef Init;
} UART_HandleTypeDef;

typedef struct {
  uint32_t Prescaler;
  uint32_t CounterMode;
  uint32_t Period;
  uint32_t ClockDivision;
} TIM_Base_InitTypeDef;

typedef struct {
  TIM_TypeDef          *Instance;
  TIM_Base_InitTypeDef Init;
} TIM_HandleTypeDef;

typedef struct {
  uint32_t ClockPrescaler;
  uint32_t Resolution;
} ADC_InitTypeDef;

typedef struct {
  ADC_TypeDef     *Instance;
  ADC_InitTypeDef Init;
} ADC_HandleTypeDef;

/* Hardware Macros */
#define __HAL_TIM_SET_COMPARE(__HANDLE__, __CHANNEL__, __COMPARE__) \
  (((__CHANNEL__) == TIM_CHANNEL_1) ? ((__HANDLE__)->Instance->CCR1 = (__COMPARE__)) : \
   ((__CHANNEL__) == TIM_CHANNEL_2) ? ((__HANDLE__)->Instance->CCR2 = (__COMPARE__)) : \
   ((__CHANNEL__) == TIM_CHANNEL_3) ? ((__HANDLE__)->Instance->CCR3 = (__COMPARE__)) : \
   ((__HANDLE__)->Instance->CCR4 = (__COMPARE__)))

#define __HAL_TIM_GET_COUNTER(__HANDLE__) ((__HANDLE__)->Instance->CNT)
#define __HAL_TIM_SET_COUNTER(__HANDLE__, __VALUE__) ((__HANDLE__)->Instance->CNT = (__VALUE__))

/* Core Prototypes */
HAL_StatusTypeDef HAL_Init(void);
void HAL_Delay(uint32_t Delay);
uint32_t HAL_GetTick(void);
void HAL_IncTick(void);

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);

HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);

HAL_StatusTypeDef HAL_TIM_Encoder_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_Encoder_Start(TIM_HandleTypeDef *htim, uint32_t Channel);

HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t Timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);
uint32_t ADC_ReadChannel(uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif /* STM32F4XX_HAL_H */
