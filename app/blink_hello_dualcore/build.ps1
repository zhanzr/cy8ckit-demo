$ErrorActionPreference = "Continue"

$CC = "D:\arm-none-eabi-tc\bin\arm-none-eabi-gcc.exe"
$AS = "D:\arm-none-eabi-tc\bin\arm-none-eabi-gcc.exe"
$OBJCOPY = "D:\arm-none-eabi-tc\bin\arm-none-eabi-objcopy.exe"
$SIZE = "D:\arm-none-eabi-tc\bin\arm-none-eabi-size.exe"

$PDL = "D:\board_database\main-cy8ckit-062\mtb-pdl-cat1-release-v3.23.0"
$CMSIS = "D:\modubus_wks\mtb_shared\cmsis\release-v5.8.0\Core\Include"
$UTILS = "D:\modubus_wks\mtb_shared\core-lib\release-v1.8.0\include"
$DEVICES = "$PDL\devices\COMPONENT_CAT1A"
$CAT1A_SRC = "$DEVICES\source"

$BUILD = "build"
if (!(Test-Path $BUILD)) { New-Item -ItemType Directory -Path $BUILD | Out-Null }

$DEFINES = @("-DCY8C6347BZI_BLD53", "-DCOMPONENT_PSOC6_01", "-DCY_IPC_DEFAULT_CFG_DISABLE", "-DCY_CORTEX_M4_APPL_ADDR=0x10020000")

$INC = @(
    "-Isystem",
    "-Iext\cmsis",
    "-I$CMSIS",
    "-I$UTILS",
    "-I$DEVICES\include",
    "-I$DEVICES\include\ip",
    "-I$PDL\drivers\include"
)

$COMMON = @("-mthumb", "-Os", "-g", "-Wall", "-ffunction-sections", "-fdata-sections", "-Wno-unused-variable") + $DEFINES + $INC
$CM0P_CFLAGS = @("-mcpu=cortex-m0plus", "-mfloat-abi=soft") + $COMMON
$CM4_CFLAGS  = @("-mcpu=cortex-m4", "-mfloat-abi=hard", "-mfpu=fpv4-sp-d16") + $COMMON
$ASMFLAGS_CM0P = @("-mcpu=cortex-m0plus", "-mthumb", "-x", "assembler-with-cpp") + $DEFINES + $INC
$ASMFLAGS_CM4  = @("-mcpu=cortex-m4", "-mthumb", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard", "-x", "assembler-with-cpp") + $DEFINES + $INC

function Invoke-CC($flags, $src, $out, $extra = @()) {
    & $CC @($flags + $extra + @("-c", $src, "-o", $out))
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $src" }
}

function Invoke-AS($flags, $src, $out) {
    & $AS @($flags + @("-c", $src, "-o", $out))
    if ($LASTEXITCODE -ne 0) { throw "Assembly failed: $src" }
}

function Invoke-Link($cpu, $fpu, $floatabi, $ldscript, $objs, $map, $out) {
    $linkflags = @("-mcpu=$cpu", "-mthumb")
    if ($fpu) { $linkflags += @("-mfpu=$fpu", "-mfloat-abi=$floatabi") }
    $linkflags += @("-T", $ldscript, "-Wl,--gc-sections", "-Wl,-Map=$map", "-o", $out)
    & $CC @($linkflags + $objs + @("-lm", "-lc", "-lnosys"))
    if ($LASTEXITCODE -ne 0) { throw "Link failed: $out" }
}

# ============================================
# CM0+
# ============================================
Write-Host "=== Building CM0+ ==="

$CM0P_OBJS = @()

$PDL_ASM = "$PDL\drivers\source\TOOLCHAIN_GCC_ARM"

# Startup assembly
Invoke-AS $ASMFLAGS_CM0P "startup\startup_psoc6_01_cm0plus.S" "$BUILD\cm0p_startup.o"
$CM0P_OBJS += "$BUILD\cm0p_startup.o"

# PDL assembly (critical section + delay cycles)
Invoke-AS $ASMFLAGS_CM0P "$PDL_ASM\cy_syslib_ext.S" "$BUILD\cm0p_syslib_ext.o"
$CM0P_OBJS += "$BUILD\cm0p_syslib_ext.o"

# App sources
Invoke-CC $CM0P_CFLAGS "cm0p\main.c" "$BUILD\cm0p_main.o"
$CM0P_OBJS += "$BUILD\cm0p_main.o"

Invoke-CC $CM0P_CFLAGS "cm0p\uart.c" "$BUILD\cm0p_uart.o"
$CM0P_OBJS += "$BUILD\cm0p_uart.o"

# System source
Invoke-CC $CM0P_CFLAGS "system\system_psoc6_cm0plus.c" "$BUILD\cm0p_sysinit.o"
$CM0P_OBJS += "$BUILD\cm0p_sysinit.o"

# Device source
Invoke-CC $CM0P_CFLAGS "$CAT1A_SRC\cy_device.c" "$BUILD\cm0p_device.o"
$CM0P_OBJS += "$BUILD\cm0p_device.o"

# PDL sources (minimal set, no IPC since CY_IPC_DEFAULT_CFG_DISABLE is set)
$pdl_cm0p = @("cy_gpio", "cy_scb_uart", "cy_scb_common", "cy_sysclk", "cy_systick", "cy_wdt", "cy_sysint", "cy_syslib")
foreach ($f in $pdl_cm0p) {
    Invoke-CC $CM0P_CFLAGS "$PDL\drivers\source\$f.c" "$BUILD\cm0p_$f.o"
    $CM0P_OBJS += "$BUILD\cm0p_$f.o"
}

Invoke-Link "cortex-m0plus" $null "soft" "linker\cm0plus.ld" $CM0P_OBJS "$BUILD\cm0p.map" "$BUILD\cm0p.elf"

# ============================================
# CM4
# ============================================
Write-Host "=== Building CM4 ==="

$CM4_OBJS = @()

# Startup assembly
Invoke-AS $ASMFLAGS_CM4 "startup\startup_psoc6_01_cm4.S" "$BUILD\cm4_startup.o"
$CM4_OBJS += "$BUILD\cm4_startup.o"

# PDL assembly (critical section + delay cycles)
Invoke-AS $ASMFLAGS_CM4 "$PDL_ASM\cy_syslib_ext.S" "$BUILD\cm4_syslib_ext.o"
$CM4_OBJS += "$BUILD\cm4_syslib_ext.o"

# App source
Invoke-CC $CM4_CFLAGS "cm4\main.c" "$BUILD\cm4_main.o"
$CM4_OBJS += "$BUILD\cm4_main.o"

# System source
Invoke-CC $CM4_CFLAGS "system\system_psoc6_cm4.c" "$BUILD\cm4_sysinit.o"
$CM4_OBJS += "$BUILD\cm4_sysinit.o"

# Device source
Invoke-CC $CM4_CFLAGS "$CAT1A_SRC\cy_device.c" "$BUILD\cm4_device.o"
$CM4_OBJS += "$BUILD\cm4_device.o"

# PDL sources
$pdl_cm4 = @("cy_gpio", "cy_systick", "cy_wdt", "cy_sysint", "cy_syslib", "cy_sysclk", "cy_scb_uart", "cy_scb_common")
foreach ($f in $pdl_cm4) {
    Invoke-CC $CM4_CFLAGS "$PDL\drivers\source\$f.c" "$BUILD\cm4_$f.o"
    $CM4_OBJS += "$BUILD\cm4_$f.o"
}

# ============================================
# Merge CM0+ into CM4
# ============================================
Write-Host "=== Creating CM0+ binary image ==="

& $OBJCOPY "-O" "binary" "$BUILD\cm0p.elf" "$BUILD\cm0p.bin"
if ($LASTEXITCODE -ne 0) { throw "Objcopy binary failed" }

& $OBJCOPY "-I" "binary" "-O" "elf32-littlearm" "-B" "arm" `
    "--rename-section" ".data=.cy_m0p_image" `
    "--redefine-sym" "_binary_cm0p_bin_start=_binary_cm0p_image_start" `
    "--redefine-sym" "_binary_cm0p_bin_end=_binary_cm0p_image_end" `
    "--redefine-sym" "_binary_cm0p_bin_size=_binary_cm0p_image_size" `
    "$BUILD\cm0p.bin" "$BUILD\m0p_image.o"
if ($LASTEXITCODE -ne 0) { throw "Objcopy image failed" }

Write-Host "=== Linking CM4 with CM0+ image ==="

$CM4_ALL_OBJS = $CM4_OBJS + @("$BUILD\m0p_image.o")
Invoke-Link "cortex-m4" "fpv4-sp-d16" "hard" "linker\cm4.ld" $CM4_ALL_OBJS "$BUILD\cm4.map" "$BUILD\cm4.elf"

Write-Host "=== Generating combined hex ==="

& $OBJCOPY "-O" "ihex" "$BUILD\cm4.elf" "$BUILD\combined.hex"
if ($LASTEXITCODE -ne 0) { throw "Objcopy hex failed" }

# Generate separate hex for each core for flashing
& $OBJCOPY "-O" "ihex" "$BUILD\cm0p.elf" "$BUILD\cm0p.hex"
& $OBJCOPY "-O" "ihex" "$BUILD\cm4.elf" "$BUILD\cm4.hex"

Write-Host ""
Write-Host "=== CM0+ size ==="
& $SIZE "$BUILD\cm0p.elf"

Write-Host ""
Write-Host "=== CM4 size ==="
& $SIZE "$BUILD\cm4.elf"

Write-Host ""
Write-Host "=== Build complete ==="
Write-Host "  Combined hex: $BUILD\combined.hex"
Write-Host "  CM0+ hex:     $BUILD\cm0p.hex"
Write-Host "  CM4 hex:      $BUILD\cm4.hex"
