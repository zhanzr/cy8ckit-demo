# SPI NOR Flash benchmark — S25FL512S @ 100 MHz (CY8CKIT-062-BLE)

Benchmarks the on-board **S25FL512S** SPI NOR (64 MB) in device mode (no
address remapping / no XIP). Both cores run at **100 MHz**:

- **CM0p** runs the benchmark once, reports, then **idles**.
- **CM4** runs the benchmark once, reports, then runs the **LED blink + die
  temperature (SAR ADC) loop**.

## Important: GPIO bit-bang, not SMIF hardware

The SPI interface is bit-banged on the flash pins (SCK=P11.7, CS=P11.2,
SI=P11.6, SO=P11.5). The **SMIF hardware block would not drive the SPI bus**
in a direct-PDL setup: `Cy_SMIF_MemCmdWriteEnable()` returned success but the
flash's write-enable-latch (WEL) stayed 0, i.e. the command never reached the
flash. This was debugged extensively (all `rxClockSel` options, clock speeds,
HOLD#/WP# states, CS pre-drive, software reset, peri-divider enable) without
success, while a **GPIO bit-bang JEDEC ID read returns `01 02 20`
(verified S25FL512S)** — so the flash and pins are good.

Therefore the numbers below reflect **GPIO bit-bang throughput** (a few
hundred KB/s), NOT the SMIF hardware's capability (the SMIF, via the HAL
`cyhal_qspi`/serial-flash middleware, is expected to reach several MB/s).

## Results (measured on hardware, 100 MHz, bit-bang)

| Core | Erase 256 KB | Program 256 KB | Read 256 KB |
|------|--------------|----------------|-------------|
| CM0p | 432 ms (592 KB/s)  | 673 ms (380 KB/s)  | 628 ms (407 KB/s) |
| CM4  | 419 ms (610 KB/s)  | 545 ms (469 KB/s)  | 557 ms (459 KB/s) |

Notes:
- Erase is the S25FL512S **256 KB sector erase** (spec max ~2.6 s; measured
  ~0.42 s). Program uses **256-byte page program**, read uses **0x03 read**
  with 4-byte addresses.
- Each core prints its own atomic result block on the shared UART (no
  interleaving), then CM0p idles and CM4 starts the LED/ADC loop
  (`[CM4] LED/ADC loop, DieTemp = 35 C`).

## Build / flash / boot

```
./build.sh           # Git Bash / MSYS2 -> build/combined.hex
```

Program + boot (see `D:\cy8ckit-prj\BOOT_ISSUE.md` for why manual boot is needed):

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe -c "set HEX D:/cy8ckit-prj/app/nor_benchmark/build/combined.hex" -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```

Open the KitProg3 COM port at **115200 8N1**. Capture ~5 s: CM0p reports and
idles, then CM4 reports and starts the LED/ADC loop.

## Caveats

- **Erases and rewrites flash sector 0** (the first 256 KB of the external
  NOR). Nothing critical lives there on this kit (the app runs from internal
  flash), but it will overwrite any prior contents of that sector.
- CM0p RAM enlarged to 20 KB (0x08000000–0x08005000) for the 4 KB buffer;
  shared UART flags at 0x08005040/44, CM4 RAM origin at 0x08005100.
- To benchmark with the real SMIF hardware, integrate the HAL
  `cyhal_qspi` (see `git@github.com:Infineon/mtb-example-serial-flash-readwrite.git`).
