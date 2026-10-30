################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Startup/startup_stm32f446xx.c 

OBJS += \
./Core/Startup/startup_stm32f446xx.o 

C_DEPS += \
./Core/Startup/startup_stm32f446xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Startup/%.o Core/Startup/%.su Core/Startup/%.cyclo: ../Core/Startup/%.c Core/Startup/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F446xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Startup

clean-Core-2f-Startup:
	-$(RM) ./Core/Startup/startup_stm32f446xx.cyclo ./Core/Startup/startup_stm32f446xx.d ./Core/Startup/startup_stm32f446xx.o ./Core/Startup/startup_stm32f446xx.su

.PHONY: clean-Core-2f-Startup

