#!/usr/bin/env bash
# Dual-core (CM0p + CM4) Dhrystone 2.1 benchmark at 100 MHz.
# Run from this directory in Git Bash / MSYS2:
#   ./build.sh

set -e

CC="D:/arm-none-eabi-tc/bin/arm-none-eabi-gcc.exe"
OBJCOPY="D:/arm-none-eabi-tc/bin/arm-none-eabi-objcopy.exe"
SIZE="D:/arm-none-eabi-tc/bin/arm-none-eabi-size.exe"

REPO="$(cygpath -m "$(cd "$(dirname "$0")/../.." && pwd)")"
PDL="$REPO/app/mtb_shared/mtb-pdl-cat1/release-v3.23.0"
DEVICES="$PDL/devices/COMPONENT_CAT1A"
CAT1A_SRC="$DEVICES/source"

BUILD="build"
mkdir -p "$BUILD"

DEFINES=("-DCY8C6347BZI_BLD53" "-DCOMPONENT_PSOC6_01" "-DCY_IPC_DEFAULT_CFG_DISABLE" "-DCY_CORTEX_M4_APPL_ADDR=0x10020000")
INC=("-Isystem" "-Iext/cmsis" "-Ibench" "-I$DEVICES/include" "-I$DEVICES/include/ip" "-I$PDL/drivers/include")

# Benchmark-appropriate flags: -Ofast -funroll-loops (NO LTO -- it hoists the
# Dhrystone timed loop and inflates the score).
CM0P_CFLAGS=("-mcpu=cortex-m0plus" "-mthumb" "-mfloat-abi=soft" "-Ofast" "-g" "-funroll-loops" "-Wall" "-ffunction-sections" "-fdata-sections" "-Wno-unused-variable" "-Wno-unused-parameter" "${DEFINES[@]}" "${INC[@]}")
CM4_CFLAGS=("-mcpu=cortex-m4" "-mthumb" "-mfloat-abi=hard" "-mfpu=fpv4-sp-d16" "-Ofast" "-g" "-funroll-loops" "-Wall" "-ffunction-sections" "-fdata-sections" "-Wno-unused-variable" "-Wno-unused-parameter" "${DEFINES[@]}" "${INC[@]}")
ASMFLAGS_CM0P=("-mcpu=cortex-m0plus" "-mthumb" "-x" "assembler-with-cpp" "${DEFINES[@]}" "${INC[@]}")
ASMFLAGS_CM4=("-mcpu=cortex-m4" "-mthumb" "-mfpu=fpv4-sp-d16" "-mfloat-abi=hard" "-x" "assembler-with-cpp" "${DEFINES[@]}" "${INC[@]}")

PDL_ASM="$PDL/drivers/source/TOOLCHAIN_GCC_ARM"
BENCH_SRCS=("bench/dhry_1.c" "bench/dhry_2.c" "bench/utils.c")
OBJS_CM0P=()
OBJS_CM4=()

compile_cm0p() {
  "$CC" "${CM0P_CFLAGS[@]}" -c "$1" -o "$BUILD/cm0p_$2.o"
  OBJS_CM0P+=("$BUILD/cm0p_$2.o")
}
compile_cm4() {
  "$CC" "${CM4_CFLAGS[@]}" -c "$1" -o "$BUILD/cm4_$2.o"
  OBJS_CM4+=("$BUILD/cm4_$2.o")
}

# ============ CM0p ============
echo "=== Building CM0p ==="

"$CC" "${ASMFLAGS_CM0P[@]}" -c "startup/startup_psoc6_01_cm0plus.S" -o "$BUILD/cm0p_startup.o"
OBJS_CM0P+=("$BUILD/cm0p_startup.o")

"$CC" "${ASMFLAGS_CM0P[@]}" -c "$PDL_ASM/cy_syslib_ext.S" -o "$BUILD/cm0p_syslib_ext.o"
OBJS_CM0P+=("$BUILD/cm0p_syslib_ext.o")

compile_cm0p "cm0p/main.c" main
compile_cm0p "cm0p/uart.c" uart
compile_cm0p "system/system_psoc6_cm0plus.c" sysinit
compile_cm0p "$CAT1A_SRC/cy_device.c" cy_device
for f in "${BENCH_SRCS[@]}"; do
  base=$(basename "$f" .c)
  compile_cm0p "$f" "$base"
done

for f in cy_gpio cy_scb_uart cy_scb_common cy_sysclk cy_systick cy_wdt cy_sysint cy_syslib; do
  compile_cm0p "$PDL/drivers/source/$f.c" "$f"
done

"$CC" -mcpu=cortex-m0plus -mthumb -T "linker/cm0plus.ld" -Wl,--gc-sections -Wl,-Map="$BUILD/cm0p.map" \
  "${OBJS_CM0P[@]}" -lm -lc -lnosys -o "$BUILD/cm0p.elf"

# ============ CM4 ============
echo "=== Building CM4 ==="

"$CC" "${ASMFLAGS_CM4[@]}" -c "startup/startup_psoc6_01_cm4.S" -o "$BUILD/cm4_startup.o"
OBJS_CM4+=("$BUILD/cm4_startup.o")

"$CC" "${ASMFLAGS_CM4[@]}" -c "$PDL_ASM/cy_syslib_ext.S" -o "$BUILD/cm4_syslib_ext.o"
OBJS_CM4+=("$BUILD/cm4_syslib_ext.o")

compile_cm4 "cm4/main.c" main
compile_cm4 "cm0p/uart.c" uart
compile_cm4 "system/system_psoc6_cm4.c" sysinit
compile_cm4 "$CAT1A_SRC/cy_device.c" cy_device
for f in "${BENCH_SRCS[@]}"; do
  base=$(basename "$f" .c)
  compile_cm4 "$f" "$base"
done

for f in cy_gpio cy_systick cy_wdt cy_sysint cy_syslib cy_sysclk cy_scb_uart cy_scb_common; do
  compile_cm4 "$PDL/drivers/source/$f.c" "$f"
done

# ============ Merge CM0p into CM4 ============
echo "=== Creating CM0p binary image ==="

"$OBJCOPY" -O binary "$BUILD/cm0p.elf" "$BUILD/cm0p.bin"
"$OBJCOPY" -I binary -O elf32-littlearm -B arm \
  --rename-section .data=.cy_m0p_image \
  --redefine-sym _binary_cm0p_bin_start=_binary_cm0p_image_start \
  --redefine-sym _binary_cm0p_bin_end=_binary_cm0p_image_end \
  --redefine-sym _binary_cm0p_bin_size=_binary_cm0p_image_size \
  "$BUILD/cm0p.bin" "$BUILD/m0p_image.o"

echo "=== Linking CM4 with CM0p image ==="

"$CC" -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
  -T "linker/cm4.ld" -Wl,--gc-sections -Wl,-Map="$BUILD/cm4.map" \
  "${OBJS_CM4[@]}" "$BUILD/m0p_image.o" -lm -lc -lnosys -o "$BUILD/cm4.elf"

echo "=== Generating combined hex ==="

"$OBJCOPY" -O ihex "$BUILD/cm4.elf" "$BUILD/combined.hex"
"$OBJCOPY" -O ihex "$BUILD/cm0p.elf" "$BUILD/cm0p.hex"
"$OBJCOPY" -O ihex "$BUILD/cm4.elf" "$BUILD/cm4.hex"

echo ""
echo "=== CM0p size ==="
"$SIZE" "$BUILD/cm0p.elf"

echo ""
echo "=== CM4 size ==="
"$SIZE" "$BUILD/cm4.elf"

echo ""
echo "=== Build complete ==="
echo "  Combined hex: $BUILD/combined.hex"
echo "  CM0p hex:     $BUILD/cm0p.hex"
echo "  CM4 hex:      $BUILD/cm4.hex"
