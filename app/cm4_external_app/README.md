# cm4_external_app — run a CM4 app stored in the external SPI NOR

Tests the chain requested: **store a CM4 app in the external S25FL512S NOR
(at `0x18000000`) and have the CM0+ read it back and boot the CM4.**

Because the SMIF hardware block does not drive the SPI bus in a direct-PDL
setup on this board (see `app/nor_benchmark/README.md`), the flash is accessed
with a **GPIO bit-bang** SPI driver (same pins: SCK=P11.7, CS=P11.2, SI=P11.6,
SO=P11.5).

## What it does

1. **CM0p** boots from internal flash (0x10000000), initialises clocks/UART,
   then:
   - erases one 256 KB sector and **programs the CM4 image into the external
     flash** at device address 0 (GPIO bit-bang page-program),
   - **reads it back** into internal SRAM at `0x08030000` and does a full
     byte-for-byte verification,
   - resets and enables the CM4 with the vector table at `0x08030000`.
2. **CM4** (the image stored in the external NOR) runs from SRAM, reports
   `=== CM4 running from the external-flash image ===` on the shared SCB5
   UART (115200 8N1) and blinks LED1 (P0.3).

## Results (measured on hardware)

```
=== cm4_external_app: CM0p (boot the CM4 from external flash) ===
  CM4 image: 4832 bytes, stored at external flash 0x18000000,
  run address: SRAM 0x08030000
  Programming CM4 image to external flash (sector erase + page program)...
  Done in 446 ms
  Reading image back to SRAM 0x08030000...
  verify: SRAM[0]=0x08040000, image[0]=0x08040000  OK
  full verify: 4832 bytes OK
  Booting CM4 from SRAM (VTOR = 0x08030000)...
```

## Honest status / blockers

- **The external-flash storage + CM0+ read-back works and is verified**
  (bit-bang page-program, full-image read-back verification).
- **The CM4 executes the external-flash-sourced image** — proven by booting
  it from the debugger (reset, load the image, set SP/PC to the SRAM copy);
  the CM4 then runs and reports. This mirrors the CM0+ boot-issue workaround.
- **The CM0+ autonomous boot (`Cy_SysEnableCM4(0x08030000)`) faults on this
  board**: the CM4 ends in the boot ROM (instruction-access violation,
  `IACCVIOL` at `0x3c`, VTOR/PC in ROM). This is the same class of
  boot-ROM-hold behaviour that stops the CM0+ auto-boot (see
  `D:\cy8ckit-prj\BOOT_ISSUE.md`).
- **True XIP** (CM4 executing directly at `0x18000000`) is the standard
  PSoC 6 pattern (`Cy_SysEnableCM4(0x18000000)` after SMIF XIP init, see the
  Infineon AN228740 flow) but is **blocked here by the SMIF interface-clock
  issue** — the SMIF must be working before the external flash is readable
  in place.

## Build / flash / boot

```
./build.sh           # Git Bash / MSYS2 -> build/combined.hex
```

Program + boot (see `D:\cy8ckit-prj\BOOT_ISSUE.md` for why manual boot is needed):

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe -c "set HEX D:/cy8ckit-prj/app/cm4_external_app/build/combined.hex" -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```

To boot the CM4 manually from the debugger (the working method), after the
CM0p has copied the image to SRAM:

```
openocd -c "set ENABLE_ACQUIRE 0" -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" -c "targets 1" -c "halt" -c "reg sp 0x08040000" -c "reg pc 0x08030291" -c "resume"
```

## Notes

- `bench/` provides the per-core SysTick tick/delay used by both images.
- `linker/cm4_ram.ld` links the CM4 image self-contained in SRAM at
  `0x08030000` (LMA == VMA, so the startup's data/vector copies are no-ops;
  init arrays are included for libc).
- The CM4 image is embedded in the CM0p binary (`build.sh` uses objcopy into
  a `.cy_m4_image` section) and written to the external NOR at device
  address 0 (== XIP `0x18000000`).
- Erasing/programming writes **flash sector 0** (first 256 KB of the external
  NOR). Nothing critical lives there on this kit.
