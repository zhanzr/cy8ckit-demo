# Minimal CM0+ only blink build script

$TC = "D:\arm-none-eabi-tc\bin\arm-none-eabi"
$CC = "$TC-gcc"
$AS = "$TC-gcc"
$LD = "$TC-gcc"
$OBJCOPY = "$TC-objcopy"
$OBJDUMP = "$TC-objdump"

$PDL = "D:\board_database\main-cy8ckit-062\mtb-pdl-cat1-release-v3.23.0"
$CMSIS = "D:\modubus_wks\mtb_shared\cmsis\release-v5.8.0\Core\Include"
$CORELIB = "D:\modubus_wks\mtb_shared\core-lib\release-v1.8.0\include"

$buildDir = "build"
if (!(Test-Path $buildDir)) { New-Item -ItemType Directory -Path $buildDir | Out-Null }

Write-Host "=== Compiling sources ===" -ForegroundColor Cyan

$cpuFlags = @("-mcpu=cortex-m0plus", "-mthumb", "-mfloat-abi=soft")
$incFlags = @(
    "-Iext\cmsis",
    "-I$CMSIS",
    "-I$CORELIB",
    "-I$PDL\drivers\include",
    "-I$PDL\devices\COMPONENT_CAT1A\include",
    "-I$PDL\devices\COMPONENT_CAT1A\template\board",
    "-Isystem"
)
$defines = @("-D__NO_SYSTEM_INIT", "-DCY8C6347BZI_BLD53", "-DCOMPONENT_PSOC6_01", "-DCY_IPC_DEFAULT_CFG_DISABLE")
$dbgFlags = @("-g", "-O0")
$warnFlags = @("-Wall", "-Wextra", "-Wno-unused-parameter")

$cFlags = $cpuFlags + $warnFlags + $defines + $dbgFlags + $incFlags + @("-ffunction-sections", "-fdata-sections", "-std=gnu11")
$asFlags = $cpuFlags + $defines + @("-x", "assembler-with-cpp")
$ldFlags = $cpuFlags + @("-T", "linker\linker.ld", "-Wl,--gc-sections", "-Wl,--print-memory-usage", "-specs=nosys.specs", "-specs=nano.specs")

# Compile C sources
$cSources = @(
    "cm0p\main.c",
    "$PDL\drivers\source\cy_gpio.c",
    "$PDL\drivers\source\cy_wdt.c",
    "$PDL\devices\COMPONENT_CAT1A\source\cy_device.c"
)

$objs = @()
foreach ($src in $cSources) {
    $base = [System.IO.Path]::GetFileNameWithoutExtension($src)
    $obj = "$buildDir\$base.o"
    $objs += $obj
    Write-Host "  CC  $src"
    & $CC @cFlags -c $src -o $obj
    if ($LASTEXITCODE -ne 0) { Write-Error "Failed to compile $src"; exit 1 }
}

# Compile startup assembly
$startupSrc = "startup\startup_psoc6_01_cm0plus.S"
$startupObj = "$buildDir\startup.o"
Write-Host "  AS  $startupSrc"
& $AS @asFlags -c $startupSrc -o $startupObj
if ($LASTEXITCODE -ne 0) { Write-Error "Failed to compile $startupSrc"; exit 1 }
$objs += $startupObj

Write-Host "`n=== Linking ===" -ForegroundColor Cyan
& $LD @ldFlags $objs -o "$buildDir\cm0p.elf"
if ($LASTEXITCODE -ne 0) { Write-Error "Link failed"; exit 1 }

Write-Host "`n=== Output ===" -ForegroundColor Cyan
& $OBJCOPY -O ihex "$buildDir\cm0p.elf" "$buildDir\cm0p.hex"
& $OBJDUMP -h "$buildDir\cm0p.elf"
Write-Host "`nDone: build\cm0p.hex" -ForegroundColor Green
