#include "main.h"
#include "safety_supervisor.h"
#include "telemetry.h"
#include "motor_controller.h"
#include "sensor_acquisition.h"
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart2;
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
ADC_HandleTypeDef hadc1;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);

/* =========================================================================
   BULLETPROOF 16x2 I2C LCD DRIVER (Broadcasts to BOTH 0x27 and 0x3F)
   PB8 = SCL (Arduino D15), PB9 = SDA (Arduino D14)
   ========================================================================= */

static void i2c_delay(void) {
    /* ~10 us delay at 16 MHz for stable ~50 kHz I2C clock */
    for (volatile int i = 0; i < 120; i++) __asm__ volatile("nop");
}

static void i2c_scl_hi(void) { GPIOB->BSRR = (1U << 8); }
static void i2c_scl_lo(void) { GPIOB->BSRR = (1U << (8 + 16)); }
static void i2c_sda_hi(void) { GPIOB->BSRR = (1U << 9); }
static void i2c_sda_lo(void) { GPIOB->BSRR = (1U << (9 + 16)); }
static uint8_t i2c_sda_read(void) { return (GPIOB->IDR & (1U << 9)) ? 1 : 0; }

static void i2c_start(void) {
    i2c_sda_hi(); i2c_scl_hi(); i2c_delay();
    i2c_sda_lo(); i2c_delay();
    i2c_scl_lo(); i2c_delay();
}

static void i2c_stop(void) {
    i2c_sda_lo(); i2c_scl_lo(); i2c_delay();
    i2c_scl_hi(); i2c_delay();
    i2c_sda_hi(); i2c_delay();
}

static bool i2c_write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        if (byte & 0x80) i2c_sda_hi(); else i2c_sda_lo();
        i2c_delay();
        i2c_scl_hi(); i2c_delay();
        i2c_scl_lo(); i2c_delay();
        byte <<= 1;
    }
    i2c_sda_hi(); i2c_delay();
    i2c_scl_hi(); i2c_delay();
    bool ack = (i2c_sda_read() == 0);
    i2c_scl_lo(); i2c_delay();
    return ack;
}

static void i2c_write_packet(uint8_t addr, const uint8_t *data, uint8_t len) {
    i2c_start();
    if (!i2c_write_byte(addr)) {
        i2c_stop();
        return;
    }
    for (uint8_t i = 0; i < len; i++) {
        i2c_write_byte(data[i]);
    }
    i2c_stop();
}

static void LCD_Send_Cmd(uint8_t cmd) {
    uint8_t data_u = cmd & 0xF0;
    uint8_t data_l = (cmd << 4) & 0xF0;
    uint8_t d[4];
    d[0] = data_u | 0x08 | 0x04; /* Backlight | EN */
    d[1] = data_u | 0x08;        /* Backlight */
    d[2] = data_l | 0x08 | 0x04; /* Backlight | EN */
    d[3] = data_l | 0x08;        /* Backlight */

    /* Broadcast to both standard backpack addresses */
    i2c_write_packet(0x27 << 1, d, 4);
    i2c_write_packet(0x3F << 1, d, 4);
}

static void LCD_Send_Data(uint8_t data) {
    uint8_t data_u = data & 0xF0;
    uint8_t data_l = (data << 4) & 0xF0;
    uint8_t d[4];
    d[0] = data_u | 0x08 | 0x04 | 0x01; /* Backlight | EN | RS */
    d[1] = data_u | 0x08 | 0x01;        /* Backlight | RS */
    d[2] = data_l | 0x08 | 0x04 | 0x01; /* Backlight | EN | RS */
    d[3] = data_l | 0x08 | 0x01;        /* Backlight | RS */

    /* Broadcast to both standard backpack addresses */
    i2c_write_packet(0x27 << 1, d, 4);
    i2c_write_packet(0x3F << 1, d, 4);
}

static void LCD_Init(void) {
    HAL_Delay(100);

    /* 4-bit wake-up sequence */
    LCD_Send_Cmd(0x30); HAL_Delay(10);
    LCD_Send_Cmd(0x30); HAL_Delay(5);
    LCD_Send_Cmd(0x30); HAL_Delay(5);
    LCD_Send_Cmd(0x20); HAL_Delay(5);

    /* 2 lines, 5x8 font */
    LCD_Send_Cmd(0x28); HAL_Delay(2);
    /* Display ON, cursor OFF */
    LCD_Send_Cmd(0x0C); HAL_Delay(2);
    /* Clear display */
    LCD_Send_Cmd(0x01); HAL_Delay(5);
    /* Entry mode */
    LCD_Send_Cmd(0x06); HAL_Delay(2);
}

static void LCD_Set_Cursor(uint8_t row, uint8_t col) {
    uint8_t addr = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_Send_Cmd(addr);
}

static void LCD_Send_String(const char *str) {
    while (*str) LCD_Send_Data((uint8_t)(*str++));
}

static void LCD_Update_Dashboard(const char *line1, const char *line2) {
    char b1[17], b2[17];
    snprintf(b1, sizeof(b1), "%-16s", line1);
    snprintf(b2, sizeof(b2), "%-16s", line2);
    LCD_Set_Cursor(0, 0);
    LCD_Send_String(b1);
    LCD_Set_Cursor(1, 0);
    LCD_Send_String(b2);
}

/* =========================================================================
   APPLICATION ENTRY POINT
   ========================================================================= */
static uint16_t currentRunId = 1;
static uint8_t currentConditionId = 0;

int main(void) {
    HAL_Init();
    __asm__ volatile("cpsie i");
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_TIM1_Init();
    MX_TIM2_Init();
    MX_ADC1_Init();

    /* Send Boot Banner immediately over USART2 */
    const char *bootMsg = "\r\n=== VARIANT 2.7 ML-ESD RUNNING ON NUCLEO-F446RE ===\r\n";
    HAL_UART_Transmit(&huart2, (const uint8_t *)bootMsg, (uint16_t)strlen(bootMsg), 100);

    SafetySupervisor_Init();
    Telemetry_Init();
    Motor_Init();
    Sensors_Init();

    /* Set Motor Forward Direction: PB5 = HIGH, PB6 = LOW */
    Motor_SetDirection(true);

    /* Initialize LCD Screen */
    LCD_Init();
    LCD_Update_Dashboard("VARIANT 2.7 ML", "SYSTEM ACTIVE...");

    /* Auto-start motor trial so it ROTATES ON BOOT */
    HAL_Delay(300);
    SafetySupervisor_ArmStart();

    uint32_t lastLoopTick = 0;
    uint32_t lastLcdTick = 0;
    uint32_t lastButtonTick = 0;
    uint32_t lastLedBlinkTick = 0;
    uint32_t loop_counter = 0;

    while (1) {
        loop_counter++;
        uint32_t now = HAL_GetTick();
        if (now == 0) now = loop_counter * 10; /* Guaranteed tick fallback */

        /* Blink Onboard Green LED LD2 (PA5) every 500 ms to prove MCU is running */
        if (now - lastLedBlinkTick >= 500) {
            lastLedBlinkTick = now;
            GPIOA->ODR ^= (1U << 5);
        }

        /* 100 Hz Main Control Loop (every 10 ms) */
        if (now - lastLoopTick >= 10) {
            uint32_t delta_ms = now - lastLoopTick;
            lastLoopTick = now;

            /* Check User Button B1 (PC13) - Toggle Motor Running / Stopped */
            if ((GPIOC->IDR & (1U << 13)) == 0) {
                if (now - lastButtonTick > 350) {
                    lastButtonTick = now;
                    MotorSystemState_t curSt = SafetySupervisor_GetState();
                    if (curSt == STATE_SAFE_REST || curSt == STATE_TRIP_SHUTDOWN) {
                        currentRunId++;
                        SafetySupervisor_Init();
                        SafetySupervisor_ArmStart();
                    } else {
                        SafetySupervisor_ManualStop();
                        Motor_EmergencyBrake();
                    }
                }
            }

            /* Sample Sensors */
            SensorData_t sensorData;
            Sensors_Sample(&sensorData, delta_ms);

            /* Safety Supervisor State Evaluation */
            SafetySupervisor_Update(sensorData.current_ma, 
                                    sensorData.speed_rpm, 
                                    sensorData.temp_c, 
                                    sensorData.supply_mv, 
                                    now);

            /* Actuate Motor based on Supervisor State */
            if (SafetySupervisor_IsTripped()) {
                Motor_EmergencyBrake();
            } else {
                uint16_t targetPWM = SafetySupervisor_GetTargetPWM();
                Motor_SetPWM(targetPWM);
            }

            /* Refresh 16x2 LCD Display every 400 ms */
            if (now - lastLcdTick >= 400) {
                lastLcdTick = now;
                char l1[32], l2[32];
                MotorSystemState_t st = SafetySupervisor_GetState();
                const char *st_str = "STOPPED";
                if (st == STATE_RUNNING) st_str = "ROTATING";
                else if (st == STATE_STARTUP_RAMPING) st_str = "RAMPING ";
                else if (st == STATE_DERATED) st_str = "DERATED ";

                snprintf(l1, sizeof(l1), "MOT: %-8s", st_str);
                snprintf(l2, sizeof(l2), "PWM:%2u%% T:%2dC", SafetySupervisor_GetTargetPWM() / 10, sensorData.temp_c);
                LCD_Update_Dashboard(l1, l2);
            }

            /* Transmit Telemetry Frame to Host via USART2 */
            Telemetry_SendFrame(currentRunId, 
                                currentConditionId, 
                                now, 
                                SafetySupervisor_GetState(), 
                                SafetySupervisor_GetTargetPWM(), 
                                sensorData.current_ma, 
                                sensorData.speed_rpm, 
                                sensorData.encoder_total_pulses, 
                                sensorData.supply_mv, 
                                sensorData.temp_c, 
                                SafetySupervisor_GetLastReason());
        }

        /* Check for Incoming Host Recommendation Commands over USART2 */
        uint8_t rx_char;
        if (HAL_UART_Receive(&huart2, &rx_char, 1, 0) == HAL_OK) {
            HostCommandFrame_t cmd;
            if (Telemetry_ParseByte(rx_char, &cmd)) {
                SafetySupervisor_ProcessHostRecommendation(cmd.rec, cmd.seq, cmd.confidence_pct, cmd.expiry_ms, now);
            }
        }
    }
}

/* =========================================================================
   HARDWARE PERIPHERAL INITIALIZATION (BARE-METAL DRIVERS)
   ========================================================================= */
static void SystemClock_Config(void) {
    /* Set up SysTick to interrupt every 1 ms at 16 MHz HSI */
    SysTick->LOAD = (16000000UL / 1000UL) - 1UL;
    SysTick->VAL = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

static void MX_GPIO_Init(void) {
    /* 1. Enable GPIO Clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN;

    /* 2. PA5: Output (Green LED LD2) */
    GPIOA->MODER &= ~(3U << (5 * 2));
    GPIOA->MODER |=  (1U << (5 * 2));

    /* 3. PA8: Alternate Function AF1 (TIM1_CH1 Motor PWM output) */
    GPIOA->MODER &= ~(3U << (8 * 2));
    GPIOA->MODER |=  (2U << (8 * 2)); /* AF Mode */
    GPIOA->AFR[1] &= ~(0xFU << ((8 - 8) * 4));
    GPIOA->AFR[1] |=  (1U << ((8 - 8) * 4)); /* AF1 = TIM1 */
    GPIOA->OSPEEDR |= (3U << (8 * 2));

    /* 4. PB5 (DIR1), PB6 (DIR2): Output Mode for H-Bridge Direction */
    GPIOB->MODER &= ~((3U << (5 * 2)) | (3U << (6 * 2)));
    GPIOB->MODER |=  ((1U << (5 * 2)) | (1U << (6 * 2)));
    GPIOB->ODR |= (1U << 5);  /* PB5 = HIGH (Forward) */
    GPIOB->ODR &= ~(1U << 6); /* PB6 = LOW */

    /* 5. PB0 (Yellow LED), PB7 (Red LED): Output Mode */
    GPIOB->MODER &= ~((3U << (0 * 2)) | (3U << (7 * 2)));
    GPIOB->MODER |=  ((1U << (0 * 2)) | (1U << (7 * 2)));

    /* 6. PB8 (SCL), PB9 (SDA): Open Drain Output with Pull-up for I2C LCD */
    GPIOB->MODER &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
    GPIOB->MODER |=  ((1U << (8 * 2)) | (1U << (9 * 2))); /* Output Mode */
    GPIOB->OTYPER |= (1U << 8) | (1U << 9);                /* Open Drain */
    GPIOB->PUPDR  &= ~((3U << (8 * 2)) | (3U << (9 * 2)));
    GPIOB->PUPDR  |=  ((1U << (8 * 2)) | (1U << (9 * 2))); /* Pull-up */
    GPIOB->BSRR = (1U << 8) | (1U << 9);

    /* 7. PC13: Input Mode with Pull-up (User Button B1) */
    GPIOC->MODER &= ~(3U << (13 * 2));
    GPIOC->PUPDR &= ~(3U << (13 * 2));
    GPIOC->PUPDR |=  (1U << (13 * 2));

    /* 8. Analog Inputs: PA4 (Current Sense A2), PC0 (Supply Proxy A5), PA6 (Temp D12) */
    GPIOA->MODER |= (3U << (4 * 2)) | (3U << (6 * 2));
    GPIOC->MODER |= (3U << (0 * 2));

    /* 9. PA0 (Arduino A0), PA1 (Arduino A1): Alternate Function AF1 for TIM2 Encoder */
    GPIOA->MODER &= ~((3U << (0 * 2)) | (3U << (1 * 2)));
    GPIOA->MODER |=  ((2U << (0 * 2)) | (2U << (1 * 2))); /* AF Mode */
    GPIOA->AFR[0] &= ~((0xFU << (0 * 4)) | (0xFU << (1 * 4)));
    GPIOA->AFR[0] |=  ((1U << (0 * 4)) | (1U << (1 * 4))); /* AF1 = TIM2 */
    GPIOA->PUPDR  &= ~((3U << (0 * 2)) | (3U << (1 * 2)));
    GPIOA->PUPDR  |=  ((1U << (0 * 2)) | (1U << (1 * 2))); /* Internal Pull-Up */
}

static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    /* PA2 (TX), PA3 (RX): AF7 (USART2) */
    GPIOA->MODER &= ~((3U << (2 * 2)) | (3U << (3 * 2)));
    GPIOA->MODER |=  ((2U << (2 * 2)) | (2U << (3 * 2)));
    GPIOA->AFR[0] &= ~((0xFU << (2 * 4)) | (0xFU << (3 * 4)));
    GPIOA->AFR[0] |=  ((7U << (2 * 4)) | (7U << (3 * 4)));

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    USART2->BRR = 0x008B; /* 115200 Baud @ 16 MHz HSI */
    USART2->CR1 = (1U << 3) | (1U << 2) | (1U << 13); /* TE, RE, UE */
}

static void MX_TIM1_Init(void) {
    /* Enable TIM1 Clock on APB2 */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    TIM1->PSC = 15;   /* 16 MHz / 16 = 1 MHz count (1us tick) */
    TIM1->ARR = 999;  /* 1ms period = 1 kHz PWM frequency */
    TIM1->CCR1 = 0;   /* Initial duty cycle = 0 */

    /* Configure Channel 1 as PWM Mode 1 with preload */
    TIM1->CCMR1 &= ~((7U << 4) | (1U << 3));
    TIM1->CCMR1 |=  ((6U << 4) | (1U << 3)); /* OC1M = 0110 (PWM Mode 1), OC1PE = 1 */

    TIM1->CCER |= (1U << 0); /* CC1E = 1 (Channel 1 Output Enable) */

    /* CRITICAL FOR TIM1 (Advanced Timer): Main Output Enable (MOE) bit */
    TIM1->BDTR |= (1U << 15);

    /* Generate update event to latch ARR & PSC into active shadow registers */
    TIM1->EGR |= (1U << 0);

    /* Enable Counter with Auto-reload preload */
    TIM1->CR1 |= (1U << 7) | (1U << 0); /* ARPE = 1, CEN = 1 */
}

static void MX_TIM2_Init(void) {
    htim2.Instance = TIM2;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* Configure TIM2 as 32-bit Hardware Quadrature Encoder (Mode 3: both edges) */
    TIM2->CR1 = 0;
    TIM2->PSC = 0;
    TIM2->ARR = 0xFFFFFFFF;
    TIM2->CCMR1 = (1U << 0) | (1U << 8); /* CC1S = 01 (TI1), CC2S = 01 (TI2) */
    TIM2->CCER = 0;                      /* Non-inverted, active high */
    TIM2->SMCR = (3U << 0);              /* SMS = 011: Encoder mode 3 */
    TIM2->CNT = 0;
    TIM2->CR1 |= (1U << 0);              /* CEN = 1 (Enable counter) */
}

static void MX_ADC1_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    ADC1->CR2 |= (1U << 0); /* ADON = 1 */
}

void Error_Handler(void) {
    while (1) {
    }
}
