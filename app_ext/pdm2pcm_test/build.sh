#!/usr/bin/env bash
# pdm2pcm_test (app_ext) - PDM/PCM microphone demo on CY8CKIT-062-BLE +
# CY8CKIT-028-EPD, CM4 running from the external S25FL512S NOR via SMIF XIP
# (0x18000000).
# Port of app/pdm2pcm_test (Infineon mtb-example-psoc6-pdm-pcm).
#
# Run from this directory in Git Bash / MSYS2:  ./build.sh
#
# Outputs:
#   build/cm4_pdm2pcm_test.elf / .hex  - link at 0x18000000 (program to the
#     external NOR: probe-rs download ... --chip CY8C6347BZI-BLD53-S25FL512S)
#   build/cm0p_xip_stub.hex            - the CM0+ stub (internal flash)
#
# To run: program this ELF to the NOR, flash the stub, boot the stub.

set -e

CC="D:/arm-none-eabi-tc/bin/arm-none-eabi-gcc.exe"
OBJCOPY="D:/arm-none-eabi-tc/bin/arm-none-eabi-objcopy.exe"

PDL="D:/board_database/main-cy8ckit-062/mtb-pdl-cat1-release-v3.23.0"
BSP="D:/board_database/main-cy8ckit-062/TARGET_CY8CKIT-062-BLE-release-v4.2.0"
MS="D:/cy8ckit-prj/app/mtb_shared"
HAL="$MS/mtb-hal-cat1/release-v2.7.4"
CORE_LIB="$MS/core-lib/release-v1.8.0"
CMSIS="$MS/cmsis/release-v6.1.0"

COMMON="../common/cm4"
STUB="../common/cm0p_xip_stub"

BUILD="build"
mkdir -p "$BUILD"

DEFINES=("-DCY8C6347BZI_BLD53" "-DCOMPONENT_PSOC6_01" "-DCOMPONENT_CAT1A" "-DCY_IPC_DEFAULT_CFG_DISABLE" "-DCY_USING_HAL")
INC=("-I." "-Icm4" "-I$COMMON" "-I$COMMON/ext" \
     "-I$BSP" "-I$BSP/config" "-I$BSP/config/GeneratedSource" \
     "-I$PDL/drivers/include" "-I$PDL/devices/COMPONENT_CAT1A/include" "-I$PDL/devices/COMPONENT_CAT1A/include/ip" \
     "-I$HAL/include" "-I$HAL/include_pvt" "-I$HAL/COMPONENT_CAT1A/include" \
     "-I$CORE_LIB/include" "-I$CMSIS/Core/Include" \
     "-I$MS/retarget-io/latest-v1.X/include")

CFLAGS=("-mcpu=cortex-m4" "-mthumb" "-mfloat-abi=soft" "-Os" "-g" "-Wall" "-ffunction-sections" "-fdata-sections" "-Wno-unused-variable" "-Wno-implicit-function-declaration" "${DEFINES[@]}" "${INC[@]}")
ASMFLAGS=("-mcpu=cortex-m4" "-mthumb" "-mfloat-abi=soft" "-x" "assembler-with-cpp" "${DEFINES[@]}" "${INC[@]}")

PDL_ASM="$PDL/drivers/source/TOOLCHAIN_GCC_ARM"
OBJS=()

"$CC" "${ASMFLAGS[@]}" -c "$COMMON/startup_psoc6_01_cm4.S" -o "$BUILD/startup.o"
OBJS+=("$BUILD/startup.o")
"$CC" "${ASMFLAGS[@]}" -c "$PDL_ASM/cy_syslib_ext.S" -o "$BUILD/syslib_ext.o"
OBJS+=("$BUILD/syslib_ext.o")

# App + common + generated device config
for f in "cm4/main.c" "$COMMON/uart.c" "$COMMON/system_psoc6_cm4.c" \
         "cycfg.c" "cycfg_clocks.c" "cycfg_peripherals.c" \
         "cycfg_pins.c" "cycfg_routing.c" "cycfg_system.c"; do
  base=$(basename "$f" .c)
  "$CC" "${CFLAGS[@]}" -c "$f" -o "$BUILD/$base.o"
  OBJS+=("$BUILD/$base.o")
done

"$CC" "${CFLAGS[@]}" -c "$PDL/devices/COMPONENT_CAT1A/source/cy_device.c" -o "$BUILD/device.o"
OBJS+=("$BUILD/device.o")

# cyhal sources (all) + device pin map + trigger data
for f in "$HAL"/source/*.c; do
  base=$(basename "$f" .c)
  "$CC" "${CFLAGS[@]}" -c "$f" -o "$BUILD/hal_$base.o"
  OBJS+=("$BUILD/hal_$base.o")
done
"$CC" "${CFLAGS[@]}" -c "$HAL/COMPONENT_CAT1A/source/pin_packages/cyhal_psoc6_01_116_bga_ble.c" -o "$BUILD/hal_pinpkg.o"
OBJS+=("$BUILD/hal_pinpkg.o")
"$CC" "${CFLAGS[@]}" -c "$HAL/COMPONENT_CAT1A/source/triggers/cyhal_triggers_psoc6_01.c" -o "$BUILD/hal_triggers.o"
OBJS+=("$BUILD/hal_triggers.o")

# PDL drivers (pdm / hal usage)
for f in cy_dma cy_gpio cy_pdm_pcm cy_scb_common cy_scb_uart cy_scb_spi cy_sysclk cy_sysint cy_syslib cy_syspm cy_syspm_bus cy_systick cy_wdt cy_trigmux cy_tcpwm cy_tcpwm_counter cy_tcpwm_pwm; do
  if [ -f "$PDL/drivers/source/$f.c" ]; then
    "$CC" "${CFLAGS[@]}" -c "$PDL/drivers/source/$f.c" -o "$BUILD/$f.o"
    OBJS+=("$BUILD/$f.o")
  fi
done

"$CC" -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -T "$COMMON/cm4.ld" -Wl,--gc-sections -Wl,-Map="$BUILD/cm4.map" \
  "${OBJS[@]}" -lm -lc -lnosys -o "$BUILD/cm4_pdm2pcm_test.elf"

"$OBJCOPY" -O ihex "$BUILD/cm4_pdm2pcm_test.elf" "$BUILD/cm4_pdm2pcm_test.hex"

( cd "$STUB" && bash build.sh )
cp "$STUB/build/cm0p_xip_stub.hex" "$BUILD/"

echo "=== built $BUILD/cm4_pdm2pcm_test.hex (0x18000000) + $BUILD/cm0p_xip_stub.hex ==="
