#!/usr/bin/env bash
# Dual-core (CM0+ + CM4) build script. Produces combined.hex.
# Run from this directory in Git Bash / MSYS2:
#   ./build.sh

set -e

CC="D:/arm-none-eabi-tc/bin/arm-none-eabi-gcc.exe"
OBJCOPY="D:/arm-none-eabi-tc/bin/arm-none-eabi-objcopy.exe"
SIZE="D:/arm-none-eabi-tc/bin/arm-none-eabi-size.exe"

PDL="D:/board_database/main-cy8ckit-062/mtb-pdl-cat1-release-v3.23.0"
CMSIS="D:/modubus_wks/mtb_shared/cmsis/release-v5.8.0/Core/Include"
UTILS="D:/modubus_wks/mtb_shared/core-lib/release-v1.8.0/include"
DEVICES="$PDL/devices/COMPONENT_CAT1A"
CAT1A_SRC="$DEVICES/source"

BUILD="build"
mkdir -p "$BUILD"

DEFINES=("-DCY8C6347BZI_BLD53" "-DCOMPONENT_PSOC6_01" "-DCY_IPC_DEFAULT_CFG_DISABLE" "-DCY_CORTEX_M4_APPL_ADDR=0x10020000")
INC=("-Isystem" "-Iext/cmsis" "-I$CMSIS" "-I$UTILS" "-I$DEVICES/include" "-I$DEVICES/include/ip" "-I$PDL/drivers/include")

CM0P_CFLAGS=("-mcpu=cortex-m0plus" "-mthumb" "-mfloat-abi=soft" "-Os" "-g" "-Wall" "-ffunction-sections" "-fdata-sections" "-Wno-unused-variable" "${DEFINES[@]}" "${INC[@]}")
CM4_CFLAGS=("-mcpu=cortex-m4" "-mthumb" "-mfloat-abi=hard" "-mfpu=fpv4-sp-d16" "-Os" "-g" "-Wall" "-ffunction-sections" "-fdata-sections" "-Wno-unused-variable" "${DEFINES[@]}" "${INC[@]}")
ASMFLAGS_CM0P=("-mcpu=cortex-m0plus" "-mthumb" "-x" "assembler-with-cpp" "${DEFINES[@]}" "${INC[@]}")
ASMFLAGS_CM4=("-mcpu=cortex-m4" "-mthumb" "-mfpu=fpv4-sp-d16" "-mfloat-abi=hard" "-x" "assembler-with-cpp" "${DEFINES[@]}" "${INC[@]}")

PDL_ASM="$PDL/drivers/source/TOOLCHAIN_GCC_ARM"
OBJS_CM0P=()
OBJS_CM4=()

# ============ CM0+ ============
echo "=== Building CM0+ ==="

"$CC" "${ASMFLAGS_CM0P[@]}" -c "startup/startup_psoc6_01_cm0plus.S" -o "$BUILD/cm0p_startup.o"
OBJS_CM0P+=("$BUILD/cm0p_startup.o")

"$CC" "${ASMFLAGS_CM0P[@]}" -c "$PDL_ASM/cy_syslib_ext.S" -o "$BUILD/cm0p_syslib_ext.o"
OBJS_CM0P+=("$BUILD/cm0p_syslib_ext.o")

"$CC" "${CM0P_CFLAGS[@]}" -c "cm0p/main.c" -o "$BUILD/cm0p_main.o"
OBJS_CM0P+=("$BUILD/cm0p_main.o")

"$CC" "${CM0P_CFLAGS[@]}" -c "cm0p/uart.c" -o "$BUILD/cm0p_uart.o"
OBJS_CM0P+=("$BUILD/cm0p_uart.o")

"$CC" "${CM0P_CFLAGS[@]}" -c "system/system_psoc6_cm0plus.c" -o "$BUILD/cm0p_sysinit.o"
OBJS_CM0P+=("$BUILD/cm0p_sysinit.o")

"$CC" "${CM0P_CFLAGS[@]}" -c "$CAT1A_SRC/cy_device.c" -o "$BUILD/cm0p_device.o"
OBJS_CM0P+=("$BUILD/cm0p_device.o")

for f in cy_gpio cy_scb_uart cy_scb_common cy_sysclk cy_systick cy_wdt cy_sysint cy_syslib; do
  "$CC" "${CM0P_CFLAGS[@]}" -c "$PDL/drivers/source/$f.c" -o "$BUILD/cm0p_$f.o"
  OBJS_CM0P+=("$BUILD/cm0p_$f.o")
done

"$CC" -mcpu=cortex-m0plus -mthumb -T "linker/cm0plus.ld" -Wl,--gc-sections -Wl,-Map="$BUILD/cm0p.map" \
  "${OBJS_CM0P[@]}" -lm -lc -lnosys -o "$BUILD/cm0p.elf"

# ============ CM4 ============
echo "=== Building CM4 ==="

"$CC" "${ASMFLAGS_CM4[@]}" -c "startup/startup_psoc6_01_cm4.S" -o "$BUILD/cm4_startup.o"
OBJS_CM4+=("$BUILD/cm4_startup.o")

"$CC" "${ASMFLAGS_CM4[@]}" -c "$PDL_ASM/cy_syslib_ext.S" -o "$BUILD/cm4_syslib_ext.o"
OBJS_CM4+=("$BUILD/cm4_syslib_ext.o")

"$CC" "${CM4_CFLAGS[@]}" -c "cm4/main.c" -o "$BUILD/cm4_main.o"
OBJS_CM4+=("$BUILD/cm4_main.o")

"$CC" "${CM4_CFLAGS[@]}" -c "system/system_psoc6_cm4.c" -o "$BUILD/cm4_sysinit.o"
OBJS_CM4+=("$BUILD/cm4_sysinit.o")

"$CC" "${CM4_CFLAGS[@]}" -c "$CAT1A_SRC/cy_device.c" -o "$BUILD/cm4_device.o"
OBJS_CM4+=("$BUILD/cm4_device.o")

for f in cy_gpio cy_systick cy_wdt cy_sysint cy_syslib cy_sysclk cy_scb_uart cy_scb_common; do
  "$CC" "${CM4_CFLAGS[@]}" -c "$PDL/drivers/source/$f.c" -o "$BUILD/cm4_$f.o"
  OBJS_CM4+=("$BUILD/cm4_$f.o")
done

# ============ Merge CM0+ into CM4 ============
echo "=== Creating CM0+ binary image ==="

"$OBJCOPY" -O binary "$BUILD/cm0p.elf" "$BUILD/cm0p.bin"
"$OBJCOPY" -I binary -O elf32-littlearm -B arm \
  --rename-section .data=.cy_m0p_image \
  --redefine-sym _binary_cm0p_bin_start=_binary_cm0p_image_start \
  --redefine-sym _binary_cm0p_bin_end=_binary_cm0p_image_end \
  --redefine-sym _binary_cm0p_bin_size=_binary_cm0p_image_size \
  "$BUILD/cm0p.bin" "$BUILD/m0p_image.o"

echo "=== Linking CM4 with CM0+ image ==="

"$CC" -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
  -T "linker/cm4.ld" -Wl,--gc-sections -Wl,-Map="$BUILD/cm4.map" \
  "${OBJS_CM4[@]}" "$BUILD/m0p_image.o" -lm -lc -lnosys -o "$BUILD/cm4.elf"

echo "=== Generating combined hex ==="

"$OBJCOPY" -O ihex "$BUILD/cm4.elf" "$BUILD/combined.hex"
"$OBJCOPY" -O ihex "$BUILD/cm0p.elf" "$BUILD/cm0p.hex"
"$OBJCOPY" -O ihex "$BUILD/cm4.elf" "$BUILD/cm4.hex"

echo ""
echo "=== CM0+ size ==="
"$SIZE" "$BUILD/cm0p.elf"

echo ""
echo "=== CM4 size ==="
"$SIZE" "$BUILD/cm4.elf"

echo ""
echo "=== Build complete ==="
echo "  Combined hex: $BUILD/combined.hex"
echo "  CM0+ hex:     $BUILD/cm0p.hex"
echo "  CM4 hex:      $BUILD/cm4.hex"
