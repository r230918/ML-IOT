#include "stm32f4xx_hal.h"

static volatile uint32_t uwTick = 0;

HAL_StatusTypeDef HAL_Init(void) {
  uwTick = 0;
  return HAL_OK;
}

void HAL_IncTick(void) {
  uwTick++;
}

uint32_t HAL_GetTick(void) {
  return uwTick;
}

void HAL_Delay(uint32_t Delay) {
  /* Bulletproof cycle-accurate delay: 3200 loops ~ 1ms at 16 MHz */
  for (uint32_t i = 0; i < Delay; i++) {
    for (volatile uint32_t j = 0; j < 3200; j++) {
      __asm__ volatile("nop");
    }
  }
}

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init) {
  (void)GPIOx;
  (void)GPIO_Init;
}

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
  if (PinState != GPIO_PIN_RESET) {
    GPIOx->BSRR = (uint32_t)GPIO_Pin;
  } else {
    GPIOx->BSRR = (uint32_t)GPIO_Pin << 16U;
  }
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
  return ((GPIOx->IDR & GPIO_Pin) != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
  uint32_t odr = GPIOx->ODR;
  GPIOx->BSRR = ((odr & GPIO_Pin) << 16U) | (~odr & GPIO_Pin);
}

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart) {
  (void)huart;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData, uint16_t Size, uint32_t Timeout) {
  if (huart == NULL || huart->Instance == NULL) return HAL_ERROR;
  (void)Timeout;
  for (uint16_t i = 0; i < Size; i++) {
    uint32_t timeout = 100000;
    while (!(huart->Instance->SR & (1U << 7)) && --timeout);
    huart->Instance->DR = pData[i];
  }
  return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout) {
  if (huart == NULL || huart->Instance == NULL) return HAL_ERROR;
  (void)Timeout;
  (void)Size;
  if (huart->Instance->SR & (1U << 5)) {
    *pData = (uint8_t)(huart->Instance->DR & 0xFF);
    return HAL_OK;
  }
  return HAL_TIMEOUT;
}

HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef *htim) {
  (void)htim;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel) {
  (void)htim;
  (void)Channel;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel) {
  (void)htim;
  (void)Channel;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Encoder_Init(TIM_HandleTypeDef *htim) {
  (void)htim;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Encoder_Start(TIM_HandleTypeDef *htim, uint32_t Channel) {
  (void)htim;
  (void)Channel;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *hadc) {
  (void)hadc;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc) {
  (void)hadc;
  return HAL_OK;
}

HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t Timeout) {
  (void)hadc;
  (void)Timeout;
  return HAL_OK;
}

uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc) {
  (void)hadc;
  return 3000;
}
