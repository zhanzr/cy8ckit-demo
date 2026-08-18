#!/usr/bin/env bash
# cm4_external_app build script.
#
# Builds two images:
#   1. A CM4 app linked for SRAM execution at 0x08030000 (cm4_ram.ld).
#   2. A CM0p app that programs the CM4 image into the external S25FL512S
#      (via GPIO bit-bang), reads it back into SRAM and boots the CM4.
#
# Run from this directory in Git Bash / MSYS2:  ./build.sh

set -e

CC="D:/arm-none-eabi-tc/bin/arm-none-eabi-gcc.exe"
OBJCOPY="D:/arm-none-eabi-tc/bin/arm-none-eabi-objcopy.exe"
SIZE="D:/arm-none-eabi-tc/bin/arm-none-eabi-size.exe"

PDL="D:/board_database/main-cy8ckit-062/mtb-pdl-cat1-release-v3.23.0"
DEVICES="$PDL/devices/COMPONENT_CAT1A"
CAT1A_SRC="$DEVICES/source"

BUILD="build"
mkdir -p "$BUILD"

DEFINES=("-DCY8C6347BZI_BLD53" "-DCOMPONENT_PSOC6_01" "-DCY_IPC_DEFAULT_CFG_DISABLE")
INC=("-Isystem" "-Iext/cmsis" "-Ibench" "-Ismif" "-I$DEVICES/include" "-I$DEVICES/include/ip" "-I$PDL/drivers/include")

CM0P_CFLAGS=("-mcpu=cortex-m0plus" "-mthumb" "-mfloat-abi=soft" "-Os" "-g" "-Wall" "-ffunction-sections" "-fdata-sections" "-Wno-unused-variable" "${DEFINES[@]}" "${INC[@]}")
CM4_CFLAGS=("-mcpu=cortex-m4" "-mthumb" "-mfloat-abi=hard" "-mfpu=fpv4-sp-d16" "-Os" "-g" "-Wall" "-ffunction-sections" "-fdata-sections" "-Wno-unused-variable" "${DEFINES[@]}" "${INC[@]}")
ASMFLAGS_CM0P=("-mcpu=cortex-m0plus" "-mthumb" "-x" "assembler-with-cpp" "${DEFINES[@]}" "${INC[@]}")
ASMFLAGS_CM4=("-mcpu=cortex-m4" "-mthumb" "-mfpu=fpv4-sp-d16" "-mfloat-abi=hard" "-x" "assembler-with-cpp" "${DEFINES[@]}" "${INC[@]}")

PDL_ASM="$PDL/drivers/source/TOOLCHAIN_GCC_ARM"

# ============ CM4 RAM image (the app that will live in external flash) ============
echo "=== Building CM4 image (linked for SRAM 0x08030000) ==="

CM4_OBJS=()
"$CC" "${ASMFLAGS_CM4[@]}" -c "startup/startup_psoc6_01_cm4.S" -o "$BUILD/cm4_startup.o"
CM4_OBJS+=("$BUILD/cm4_startup.o")
"$CC" "${ASMFLAGS_CM4[@]}" -c "$PDL_ASM/cy_syslib_ext.S" -o "$BUILD/cm4_syslib_ext.o"
CM4_OBJS+=("$BUILD/cm4_syslib_ext.o")
"$CC" "${CM4_CFLAGS[@]}" -c "cm4/main.c" -o "$BUILD/cm4_main.o"
CM4_OBJS+=("$BUILD/cm4_main.o")
"$CC" "${CM4_CFLAGS[@]}" -c "bench/utils.c" -o "$BUILD/cm4_utils.o"
CM4_OBJS+=("$BUILD/cm4_utils.o")
"$CC" "${CM4_CFLAGS[@]}" -c "system/system_psoc6_cm4.c" -o "$BUILD/cm4_sysinit.o"
CM4_OBJS+=("$BUILD/cm4_sysinit.o")
"$CC" "${CM4_CFLAGS[@]}" -c "$CAT1A_SRC/cy_device.c" -o "$BUILD/cm4_device.o"
CM4_OBJS+=("$BUILD/cm4_device.o")
for f in cy_gpio cy_systick cy_wdt cy_sysint cy_syslib cy_sysclk cy_scb_uart cy_scb_common; do
  "$CC" "${CM4_CFLAGS[@]}" -c "$PDL/drivers/source/$f.c" -o "$BUILD/cm4_$f.o"
  CM4_OBJS+=("$BUILD/cm4_$f.o")
done

"$CC" -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
  -T "linker/cm4_ram.ld" -Wl,--gc-sections -Wl,-Map="$BUILD/cm4_ram.map" \
  "${CM4_OBJS[@]}" -lm -lc -lnosys -specs=nano.specs -o "$BUILD/cm4_ram.elf"

"$OBJCOPY" -O binary "$BUILD/cm4_ram.elf" "$BUILD/cm4_ram.bin"
"$OBJCOPY" -I binary -O elf32-littlearm -B arm \
  --rename-section .data=.cy_m4_image \
  --redefine-sym _binary_build_cm4_ram_bin_start=_binary_cm4_image_start \
  --redefine-sym _binary_build_cm4_ram_bin_end=_binary_cm4_image_end \
  --redefine-sym _binary_build_cm4_ram_bin_size=_binary_cm4_image_size \
  "$BUILD/cm4_ram.bin" "$BUILD/m4_image.o"

# ============ CM0p (programs + boots the CM4 image) ============
echo "=== Building CM0p ==="

CM0P_OBJS=()
"$CC" "${ASMFLAGS_CM0P[@]}" -c "startup/startup_psoc6_01_cm0plus.S" -o "$BUILD/cm0p_startup.o"
CM0P_OBJS+=("$BUILD/cm0p_startup.o")
"$CC" "${ASMFLAGS_CM0P[@]}" -c "$PDL_ASM/cy_syslib_ext.S" -o "$BUILD/cm0p_syslib_ext.o"
CM0P_OBJS+=("$BUILD/cm0p_syslib_ext.o")
"$CC" "${CM0P_CFLAGS[@]}" -c "cm0p/main.c" -o "$BUILD/cm0p_main.o"
CM0P_OBJS+=("$BUILD/cm0p_main.o")
"$CC" "${CM0P_CFLAGS[@]}" -c "cm0p/uart.c" -o "$BUILD/cm0p_uart.o"
CM0P_OBJS+=("$BUILD/cm0p_uart.o")
"$CC" "${CM0P_CFLAGS[@]}" -c "bench/utils.c" -o "$BUILD/cm0p_utils.o"
CM0P_OBJS+=("$BUILD/cm0p_utils.o")
"$CC" "${CM0P_CFLAGS[@]}" -c "smif/nor_bb.c" -o "$BUILD/cm0p_nor_bb.o"
CM0P_OBJS+=("$BUILD/cm0p_nor_bb.o")
"$CC" "${CM0P_CFLAGS[@]}" -c "system/system_psoc6_cm0plus.c" -o "$BUILD/cm0p_sysinit.o"
CM0P_OBJS+=("$BUILD/cm0p_sysinit.o")
"$CC" "${CM0P_CFLAGS[@]}" -c "$CAT1A_SRC/cy_device.c" -o "$BUILD/cm0p_device.o"
CM0P_OBJS+=("$BUILD/cm0p_device.o")
for f in cy_gpio cy_scb_uart cy_scb_common cy_sysclk cy_systick cy_wdt cy_sysint cy_syslib; do
  "$CC" "${CM0P_CFLAGS[@]}" -c "$PDL/drivers/source/$f.c" -o "$BUILD/cm0p_$f.o"
  CM0P_OBJS+=("$BUILD/cm0p_$f.o")
done

"$CC" -mcpu=cortex-m0plus -mthumb -T "linker/cm0plus.ld" -Wl,--gc-sections -Wl,-Map="$BUILD/cm0p.map" \
  "${CM0P_OBJS[@]}" "$BUILD/m4_image.o" -lm -lc -lnosys -o "$BUILD/cm0p.elf"

echo "=== Generating hex ==="
"$OBJCOPY" -O ihex "$BUILD/cm0p.elf" "$BUILD/combined.hex"
"$OBJCOPY" -O ihex "$BUILD/cm4_ram.elf" "$BUILD/cm4_ram.hex"

echo ""
echo "=== CM0p size ==="
"$SIZE" "$BUILD/cm0p.elf"
echo ""
echo "=== CM4 image size ==="
"$SIZE" "$BUILD/cm4_ram.elf"
ls -l "$BUILD/cm4_ram.bin" | awk '{print "  cm4_ram.bin: " $5 " bytes"}'

echo ""
echo "=== Build complete ==="
echo "  CM0p+image hex: $BUILD/combined.hex"
echo "  CM4 image bin:  $BUILD/cm4_ram.bin"
