#!/usr/bin/env bash
# CM0+ XIP stub - single-core CM0+ build (internal flash 0x10000000).
# Run from this directory in Git Bash / MSYS2:  ./build.sh
# Produces build/cm0p_xip_stub.hex. Boot it with the usual
# D:/cy8ckit-prj/tools/flash_and_boot.tcl (the ROM does not auto-boot).

set -e

CC="D:/arm-none-eabi-tc/bin/arm-none-eabi-gcc.exe"
OBJCOPY="D:/arm-none-eabi-tc/bin/arm-none-eabi-objcopy.exe"

REPO="$(cygpath -m "$(cd "$(dirname "$0")/../../.." && pwd)")"
PDL="$REPO/app/mtb_shared/mtb-pdl-cat1/release-v3.23.0"
DEVICES="$PDL/devices/COMPONENT_CAT1A"
CAT1A_SRC="$DEVICES/source"

BUILD="build"
mkdir -p "$BUILD"

DEFINES=("-DCY8C6347BZI_BLD53" "-DCOMPONENT_PSOC6_01" "-DCY_IPC_DEFAULT_CFG_DISABLE")
INC=("-Isystem" "-Iext/cmsis" "-I$DEVICES/include" "-I$DEVICES/include/ip" "-I$PDL/drivers/include")

CFLAGS=("-mcpu=cortex-m0plus" "-mthumb" "-mfloat-abi=soft" "-Os" "-g" "-Wall" "-ffunction-sections" "-fdata-sections" "-Wno-unused-variable" "${DEFINES[@]}" "${INC[@]}")
ASMFLAGS=("-mcpu=cortex-m0plus" "-mthumb" "-x" "assembler-with-cpp" "${DEFINES[@]}" "${INC[@]}")

PDL_ASM="$PDL/drivers/source/TOOLCHAIN_GCC_ARM"
OBJS=()

"$CC" "${ASMFLAGS[@]}" -c "startup/startup_psoc6_01_cm0plus.S" -o "$BUILD/startup.o"
OBJS+=("$BUILD/startup.o")

"$CC" "${ASMFLAGS[@]}" -c "$PDL_ASM/cy_syslib_ext.S" -o "$BUILD/syslib_ext.o"
OBJS+=("$BUILD/syslib_ext.o")

for f in "main.c" "system/system_psoc6_cm0plus.c"; do
  base=$(basename "$f" .c)
  "$CC" "${CFLAGS[@]}" -c "$f" -o "$BUILD/$base.o"
  OBJS+=("$BUILD/$base.o")
done

"$CC" "${CFLAGS[@]}" -c "$CAT1A_SRC/cy_device.c" -o "$BUILD/device.o"
OBJS+=("$BUILD/device.o")

for f in cy_gpio cy_scb_uart cy_scb_common cy_sysclk cy_wdt cy_sysint cy_syslib; do
  "$CC" "${CFLAGS[@]}" -c "$PDL/drivers/source/$f.c" -o "$BUILD/$f.o"
  OBJS+=("$BUILD/$f.o")
done

"$CC" -mcpu=cortex-m0plus -mthumb -T "linker/cm0plus.ld" -Wl,--gc-sections -Wl,-Map="$BUILD/stub.map" \
  "${OBJS[@]}" -lm -lc -lnosys -o "$BUILD/cm0p_xip_stub.elf"

"$OBJCOPY" -O ihex "$BUILD/cm0p_xip_stub.elf" "$BUILD/cm0p_xip_stub.hex"

echo "=== built $BUILD/cm0p_xip_stub.hex ==="
