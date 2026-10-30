TARGET = Variant_2_7_F446RE

CC = "C:/ST/STM32CubeIDE_2.2.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin/arm-none-eabi-gcc.exe"
CP = "C:/ST/STM32CubeIDE_2.2.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin/arm-none-eabi-objcopy.exe"
SZ = "C:/ST/STM32CubeIDE_2.2.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin/arm-none-eabi-size.exe"

CPU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

C_DEFS = -DSTM32F446xx -DUSE_HAL_DRIVER

C_INCLUDES = \
-ICore/Inc \
-IDrivers/CMSIS/Include

CFLAGS = $(CPU) $(C_DEFS) $(C_INCLUDES) -O2 -Wall -fdata-sections -ffunction-sections

LDSCRIPT = STM32F446RETX_FLASH.ld

LDFLAGS = $(CPU) -T$(LDSCRIPT) -Wl,--gc-sections -specs=nano.specs -specs=nosys.specs -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

BUILD_DIR = build

C_SOURCES = \
Core/Src/main.c \
Core/Src/safety_supervisor.c \
Core/Src/telemetry.c \
Core/Src/motor_controller.c \
Core/Src/sensor_acquisition.c \
Core/Src/stm32f4xx_it.c \
Core/Src/system_stm32f4xx.c \
Core/Src/stm32f4xx_hal.c \
Core/Src/syscalls.c \
Core/Src/sysmem.c \
Core/Startup/startup_stm32f446xx.c

OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c
	@powershell -Command "New-Item -ItemType Directory -Force -Path (Split-Path '$@')" >nul 2>&1
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(CP) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(CP) -O binary -S $< $@

clean:
	@powershell -Command "Remove-Item -Recurse -Force -Path $(BUILD_DIR) -ErrorAction SilentlyContinue"
