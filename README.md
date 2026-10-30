# ML-ESD Learnathon & Hackathon: Supply-Sag Emulator (NUCLEO-F446RE Edition)

## Variant 2.7 Ported to NUCLEO-F446RE
**Target Board:** STMicroelectronics NUCLEO-F446RE (STM32F446RET6 @ 180 MHz, Cortex-M4 with FPU)  
**Toolchain:** STM32CubeIDE (`stmide`) + ARM GNU GCC 14.3  
**Actuator:** 6-12V Geared DC motor with quadrature encoder + 3.3V logic H-bridge  

---

### Hardware Pinout for NUCLEO-F446RE (Morpho / Arduino Headers)

| Physical Signal | Pin on NUCLEO-F446RE | Header Location | Description |
|:---|:---|:---|:---|
| **ST-LINK VCOM TX** | **PA2** (USART2_TX) | Hardwired to onboard ST-LINK | 115200 Baud Telemetry Stream |
| **ST-LINK VCOM RX** | **PA3** (USART2_RX) | Hardwired to onboard ST-LINK | Advisory Host Commands |
| **Motor PWM** | **PA8** (TIM1_CH1) | Arduino D7 / Morpho CN10-23 | 20 kHz Motor PWM drive to H-Bridge |
| **Motor DIR1** | **PB5** (GPIO) | Arduino D4 / Morpho CN10-29 | Direction Logic IN1 |
| **Motor DIR2** | **PB6** (GPIO) | Arduino D10 / Morpho CN10-17 | Direction Logic IN2 |
| **Encoder Channel A** | **PA0** (TIM2_CH1) | Arduino A0 / Morpho CN7-28 | Motor quadrature pulse A |
| **Encoder Channel B** | **PA1** (TIM2_CH2) | Arduino A1 / Morpho CN7-30 | Motor quadrature pulse B |
| **Current Sense ADC** | **PA4** (ADC1_IN4) | Arduino A2 / Morpho CN7-32 | 0 - 3.3V Conditioned Shunt Voltage |
| **Supply Proxy ADC** | **PC0** (ADC1_IN10) | Arduino A5 / Morpho CN7-38 | Voltage Divider / Potentiometer |
| **LM35 Temperature** | **PA6** (ADC1_IN6) | Arduino D12 / Morpho CN10-13 | 10 mV / deg C Temperature Sensor |
| **User Pushbutton B1** | **PC13** | Onboard Blue Button | Trial Start / Reset Fault |
| **User Status LED** | **PA5** (LD2 Green) | Onboard Green LED | Solid = Running, Blink = Warning/Derate |

---

### How to Import, Build and Flash in STM32CubeIDE
1. Open **STM32CubeIDE**.
2. Click `File` -> `Open Projects from File System...`.
3. Set **Import source** to `C:\ml-esd\2.7_f446re`.
4. Click `Finish`. The IDE imports `Variant_2_7_F446RE`.
5. Click **Build** (Hammer icon) or press `Ctrl + B`.
6. Connect your **NUCLEO-F446RE** via USB.
7. Click `Run` -> `Run As STM32 C/C++ Application`.
8. The firmware will begin streaming 100 Hz telemetry packets over the ST-LINK Virtual COM port on **USART2** at 115200 baud!
