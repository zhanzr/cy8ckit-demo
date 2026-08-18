# nor_benchmark_hal — SPI NOR (S25FL512S) speed benchmark with the HAL SMIF

MTB (ModusToolbox) application that benchmarks the on-board **S25FL512S** SPI
NOR using the **hardware SMIF** (via the HAL `cy_serial_flash_qspi` driver).
Runs on the CM4 at 100 MHz, 50 MHz QSPI clock. This is the **real hardware
throughput** (the bit-bang versions in `app/nor_benchmark` are kept as an
"in case" software fallback).

## Results (measured on hardware, HAL SMIF @ 50 MHz)

| Operation | Size | Time | Speed |
|-----------|------|------|-------|
| Erase   | 512 KB (2 × 256 KB sectors) | 1300 ms | 393 KB/s |
| Program | 512 KB (256 B pages)        |  413 ms | 1239 KB/s (1.24 MB/s) |
| Read    | 512 KB                     |   53 ms | 9660 KB/s (9.66 MB/s) |

Compare with the GPIO bit-bang fallback (`app/nor_benchmark`): read ~0.4 MB/s,
program ~0.4 MB/s. The SMIF is roughly **20x faster on read** and **3x faster
on program**; erase is flash-limited and similar.

## How the SMIF got working

The SMIF hardware block does not drive the SPI bus with the raw PDL
`Cy_SMIF_*` calls used earlier (write-enable "completed" but never reached the
flash). The **HAL** `cy_serial_flash_qspi_init` configures the SMIF correctly
(interface clock divider, per AN228740) — this is the working path.
The vendor OpenOCD SMIF flash algorithm (`CY8C6xxA_SMIF*.FLM`) still fails
its Init on this board, so external-NOR programming is done at runtime via
the HAL.

## Build

This is a ModusToolbox app. From a modus-shell with ModusToolbox 3.8:

```
cd app/nor_benchmark_hal
make getlibs MTB_SHARED_DIR=<dir-with-fetched-libs>
make build TOOLCHAIN=GCC_ARM CONFIG=Debug -j4 MTB_SHARED_DIR=<dir-with-fetched-libs>
```

The hex is `build/APP_CY8CKIT-062-BLE/Debug/mtb-example-serial-flash-readwrite.hex`.

## Flash / run

Program + boot (see `D:\cy8ckit-prj\BOOT_ISSUE.md` for why manual boot is needed):

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe -c "set HEX D:/cy8ckit-prj/app/nor_benchmark_hal/build/APP_CY8CKIT-062-BLE/Debug/mtb-example-serial-flash-readwrite.hex" -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```

Open the KitProg3 COM port at **115200 8N1**; the benchmark runs once and
reports the three lines, then the board blinks the user LED.

## Notes

- Erases/programs **flash sector 0** (first 256 KB of the external NOR).
- `main.c` uses the DWT cycle counter to time the operations.
- The SMIF is a shared peripheral; the CM4's numbers represent the hardware
  (the bit-bang benchmark in `app/nor_benchmark` gives per-core figures).
