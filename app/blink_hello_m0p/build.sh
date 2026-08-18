#!/usr/bin/env bash
# Minimal CM0+ only blink build script (100 MHz boot demo).
# Run from this directory in Git Bash / MSYS2:
#   ./build.sh

set -e

TC="D:/arm-none-eabi-tc/bin/arm-none-eabi"
PDL="D:/board_database/main-cy8ckit-062/mtb-pdl-cat1-release-v3.23.0"
CMSIS="D:/modubus_wks/mtb_shared/cmsis/release-v5.8.0/Core/Include"
CORELIB="D:/modubus_wks/mtb_shared/core-lib/release-v1.8.0/include"
DEVICES="$PDL/devices/COMPONENT_CAT1A"
CAT1A_SRC="$DEVICES/source"

BUILD="build"
mkdir -p "$BUILD"

DEFINES=("-D__NO_SYSTEM_INIT" "-DCY8C6347BZI_BLD53" "-DCOMPONENT_PSOC6_01" "-DCY_IPC_DEFAULT_CFG_DISABLE")
INC=("-Isystem" "-Iext/ext/cmsis" "-I$CMSIS" "-I$CORELIB" "-I$DEVICES/include" "-I$DEVICES/include/ip" "-I$PDL/drivers/include")

CFLAGS=("-mcpu=cortex-m0plus" "-mthumb" "-mfloat-abi=soft" "-Wall" "-Wextra" "-Wno-unused-parameter" "-Wno-unused-variable" "${DEFINES[@]}" "-g" "-Os" "${INC[@]}" "-ffunction-sections" "-fdata-sections" "-std=gnu11")
ASFLAGS=("-mcpu=cortex-m0plus" "-mthumb" "-x" "assembler-with-cpp" "${DEFINES[@]}" "${INC[@]}")

CC="$TC-gcc"
LD="$TC-gcc"
OBJCOPY="$TC-objcopy"
SIZE="$TC-size"

OBJS=()

compile_c() {
  echo "  CC  $1"
  "$CC" "${CFLAGS[@]}" -c "$1" -o "$2"
  OBJS+=("$2")
}

# App + system + device
compile_c "cm0p/main.c" "$BUILD/main.o"
compile_c "cm0p/uart.c" "$BUILD/uart.o"
compile_c "system/system_psoc6_cm0plus.c" "$BUILD/sysinit.o"
compile_c "$CAT1A_SRC/cy_device.c" "$BUILD/cy_device.o"

# PDL driver sources
for f in cy_gpio cy_scb_uart cy_scb_common cy_sysclk cy_wdt cy_sysint cy_syslib; do
  compile_c "$PDL/drivers/source/$f.c" "$BUILD/$f.o"
done

# PDL assembly (critical section + delay cycles)
echo "  AS  cy_syslib_ext.S"
"$CC" "${ASFLAGS[@]}" -c "$PDL/drivers/source/TOOLCHAIN_GCC_ARM/cy_syslib_ext.S" -o "$BUILD/cy_syslib_ext.o"
OBJS+=("$BUILD/cy_syslib_ext.o")

# Startup assembly
echo "  AS  startup/startup_psoc6_01_cm0plus.S"
"$CC" "${ASFLAGS[@]}" -c "startup/startup_psoc6_01_cm0plus.S" -o "$BUILD/startup.o"
OBJS+=("$BUILD/startup.o")

echo "=== Linking ==="
"$LD" -mcpu=cortex-m0plus -mthumb -T "linker/linker.ld" -Wl,--gc-sections -Wl,--print-memory-usage \
  -specs=nosys.specs -specs=nano.specs "${OBJS[@]}" -o "$BUILD/cm0p.elf"

echo "=== Output ==="
"$OBJCOPY" -O ihex "$BUILD/cm0p.elf" "$BUILD/cm0p.hex"
"$SIZE" "$BUILD/cm0p.elf"
echo "Done: build/cm0p.hex"
