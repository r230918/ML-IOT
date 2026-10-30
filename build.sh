#!/bin/bash
set -e

GCC="arm-none-eabi-gcc"
FLAGS="-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32F446xx -DUSE_HAL_DRIVER -ICore/Inc -IDrivers/CMSIS/Include -O2 -Wall -fdata-sections -ffunction-sections"

SRCS=(
    "Core/Src/main.c"
    "Core/Src/safety_supervisor.c"
    "Core/Src/telemetry.c"
    "Core/Src/l293d.c"
    "Core/Src/motor_controller.c"
    "Core/Src/sensor_acquisition.c"
    "Core/Src/stm32f4xx_it.c"
    "Core/Src/system_stm32f4xx.c"
    "Core/Src/stm32f4xx_hal.c"
    "Core/Src/syscalls.c"
    "Core/Src/sysmem.c"
    "Core/Startup/startup_stm32f446xx.c"
)

mkdir -p build
OBJS=()

for src in "${SRCS[@]}"; do
    base=$(basename "$src" .c)
    obj="build/$base.o"
    OBJS+=("$obj")
    echo "Compiling $src..."
    $GCC $FLAGS -c "$src" -o "$obj"
done

LDFLAGS="-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -TSTM32F446RETX_FLASH.ld -Wl,--gc-sections -specs=nano.specs -specs=nosys.specs"

echo "Linking build/Variant_02_07_F446RE.elf..."
$GCC "${OBJS[@]}" $LDFLAGS -o "build/Variant_02_07_F446RE.elf"

arm-none-eabi-objcopy -O binary build/Variant_02_07_F446RE.elf build/Variant_02_07_F446RE.bin
arm-none-eabi-size build/Variant_02_07_F446RE.elf

echo "BUILD SUCCESS: build/Variant_02_07_F446RE.elf and build/Variant_02_07_F446RE.bin generated cleanly!"
