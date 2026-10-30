$gcc = "C:\ST\STM32CubeIDE_2.2.0\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740\tools\bin\arm-none-eabi-gcc.exe"
$flags = @(
    "-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard",
    "-DSTM32F446xx", "-DUSE_HAL_DRIVER",
    "-ICore/Inc", "-IDrivers/CMSIS/Include",
    "-O2", "-Wall", "-fdata-sections", "-ffunction-sections"
)
$srcs = @(
    "Core/Src/main.c",
    "Core/Src/safety_supervisor.c",
    "Core/Src/telemetry.c",
    "Core/Src/motor_controller.c",
    "Core/Src/sensor_acquisition.c",
    "Core/Src/stm32f4xx_it.c",
    "Core/Src/system_stm32f4xx.c",
    "Core/Src/stm32f4xx_hal.c",
    "Core/Src/syscalls.c",
    "Core/Src/sysmem.c",
    "Core/Startup/startup_stm32f446xx.c"
)

New-Item -ItemType Directory -Force -Path build | Out-Null
$objs = @()
foreach ($src in $srcs) {
    $base = [System.IO.Path]::GetFileNameWithoutExtension($src)
    $obj = "build/$base.o"
    $objs += $obj
    Write-Host "Compiling $src..."
    & $gcc @flags -c $src -o $obj
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Compilation failed on $src"
        exit 1
    }
}

$ldflags = @(
    "-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard",
    "-TSTM32F446RETX_FLASH.ld", "-Wl,--gc-sections",
    "-specs=nano.specs", "-specs=nosys.specs"
)
Write-Host "Linking build/Variant_02_07_F446RE.elf..."
& $gcc @objs @ldflags -o "build/Variant_02_07_F446RE.elf"
if ($LASTEXITCODE -eq 0) {
    Write-Host "SUCCESS: build/Variant_02_07_F446RE.elf built successfully!"
} else {
    Write-Error "Linking failed!"
    exit 1
}
