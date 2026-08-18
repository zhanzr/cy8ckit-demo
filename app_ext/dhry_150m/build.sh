#!/usr/bin/env bash
# dhry_150m - app_ext CM4 Dhrystone (external NOR XIP @ 150 MHz).
# Run from this directory in Git Bash / MSYS2:  ./build.sh
#
# Outputs build/cm4_dhry_150m.elf/.hex (0x18000000) + build/cm0p_xip_stub.hex.
# Program the elf to the external NOR with probe-rs, flash the stub, boot it.

set -e

CC="D:/arm-none-eabi-tc/bin/arm-none-eabi-gcc.exe"
OBJCOPY="D:/arm-none-eabi-tc/bin/arm-none-eabi-objcopy.exe"

PDL="D:/board_database/main-cy8ckit-062/mtb-pdl-cat1-release-v3.23.0"
DEVICES="$PDL/devices/COMPONENT_CAT1A"
CAT1A_SRC="$DEVICES/source"

COMMON="../common/cm4"
STUB="../common/cm0p_xip_stub"

BUILD="build"
mkdir -p "$BUILD"

DEFINES=("-DCY8C6347BZI_BLD53" "-DCOMPONENT_PSOC6_01" "-DCY_IPC_DEFAULT_CFG_DISABLE")
INC=("-I." "-Icm4/bench" "-I$COMMON" "-I$COMMON/ext" "-I$DEVICES/include" "-I$DEVICES/include/ip" "-I$PDL/drivers/include")

CFLAGS=("-mcpu=cortex-m4" "-mthumb" "-mfloat-abi=soft" "-Os" "-g" "-Wall" "-ffunction-sections" "-fdata-sections" "-Wno-unused-variable" "${DEFINES[@]}" "${INC[@]}")
ASMFLAGS=("-mcpu=cortex-m4" "-mthumb" "-mfloat-abi=soft" "-x" "assembler-with-cpp" "${DEFINES[@]}" "${INC[@]}")

PDL_ASM="$PDL/drivers/source/TOOLCHAIN_GCC_ARM"
OBJS=()

"$CC" "${ASMFLAGS[@]}" -c "$COMMON/startup_psoc6_01_cm4.S" -o "$BUILD/startup.o"
OBJS+=("$BUILD/startup.o")

"$CC" "${ASMFLAGS[@]}" -c "$PDL_ASM/cy_syslib_ext.S" -o "$BUILD/syslib_ext.o"
OBJS+=("$BUILD/syslib_ext.o")

for f in "cm4/main.c" "cm4/bench/utils.c" "cm4/bench/dhry_1.c" "cm4/bench/dhry_2.c" "$COMMON/uart.c" "$COMMON/system_psoc6_cm4.c"; do
  base=$(basename "$f" .c)
  "$CC" "${CFLAGS[@]}" -c "$f" -o "$BUILD/$base.o"
  OBJS+=("$BUILD/$base.o")
done

"$CC" "${CFLAGS[@]}" -c "$CAT1A_SRC/cy_device.c" -o "$BUILD/device.o"
OBJS+=("$BUILD/device.o")

for f in cy_gpio cy_scb_uart cy_scb_common cy_sysclk cy_sysint cy_syslib cy_systick cy_wdt; do
  "$CC" "${CFLAGS[@]}" -c "$PDL/drivers/source/$f.c" -o "$BUILD/$f.o"
  OBJS+=("$BUILD/$f.o")
done

"$CC" -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -T "$COMMON/cm4.ld" -Wl,--gc-sections -Wl,-Map="$BUILD/cm4.map" \
  "${OBJS[@]}" -lm -lc -lnosys -o "$BUILD/cm4_dhry_150m.elf"

"$OBJCOPY" -O ihex "$BUILD/cm4_dhry_150m.elf" "$BUILD/cm4_dhry_150m.hex"

( cd "$STUB" && bash build.sh )
cp "$STUB/build/cm0p_xip_stub.hex" "$BUILD/"

echo "=== built $BUILD/cm4_dhry_150m.hex (0x18000000) + $BUILD/cm0p_xip_stub.hex ==="
