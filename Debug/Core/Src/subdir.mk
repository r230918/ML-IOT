################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/l293d.c \
../Core/Src/main.c \
../Core/Src/motor_controller.c \
../Core/Src/safety_supervisor.c \
../Core/Src/sensor_acquisition.c \
../Core/Src/stm32f4xx_hal.c \
../Core/Src/stm32f4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f4xx.c \
../Core/Src/telemetry.c 

OBJS += \
./Core/Src/l293d.o \
./Core/Src/main.o \
./Core/Src/motor_controller.o \
./Core/Src/safety_supervisor.o \
./Core/Src/sensor_acquisition.o \
./Core/Src/stm32f4xx_hal.o \
./Core/Src/stm32f4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f4xx.o \
./Core/Src/telemetry.o 

C_DEPS += \
./Core/Src/l293d.d \
./Core/Src/main.d \
./Core/Src/motor_controller.d \
./Core/Src/safety_supervisor.d \
./Core/Src/sensor_acquisition.d \
./Core/Src/stm32f4xx_hal.d \
./Core/Src/stm32f4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f4xx.d \
./Core/Src/telemetry.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/main.cyclo ./Core/Src/l293d.d \
./Core/Src/main.d ./Core/Src/l293d.o \
./Core/Src/main.o ./Core/Src/main.su ./Core/Src/motor_controller.cyclo ./Core/Src/motor_controller.d ./Core/Src/motor_controller.o ./Core/Src/motor_controller.su ./Core/Src/safety_supervisor.cyclo ./Core/Src/safety_supervisor.d ./Core/Src/safety_supervisor.o ./Core/Src/safety_supervisor.su ./Core/Src/sensor_acquisition.cyclo ./Core/Src/sensor_acquisition.d ./Core/Src/sensor_acquisition.o ./Core/Src/sensor_acquisition.su ./Core/Src/stm32f4xx_hal.cyclo ./Core/Src/stm32f4xx_hal.d ./Core/Src/stm32f4xx_hal.o ./Core/Src/stm32f4xx_hal.su ./Core/Src/stm32f4xx_it.cyclo ./Core/Src/stm32f4xx_it.d ./Core/Src/stm32f4xx_it.o ./Core/Src/stm32f4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f4xx.cyclo ./Core/Src/system_stm32f4xx.d ./Core/Src/system_stm32f4xx.o ./Core/Src/system_stm32f4xx.su ./Core/Src/telemetry.cyclo ./Core/Src/telemetry.d ./Core/Src/telemetry.o ./Core/Src/telemetry.su

.PHONY: clean-Core-2f-Src

