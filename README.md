# CY8CKIT-062-BLE — PSoC 6 BLE bring-up, apps and boot workaround

Everything in this repo is tied to a **CY8CKIT-062-BLE** board (PSoC 6 BLE,
**CY8C6347BZI-BLD53**, dual core: CM0p + CM4, 1 MB flash, 288 KB SRAM).

> **The one important caveat:** the PSoC 6 boot ROM on this board does **not**
> auto-boot any application after reset / power-cycle. The CM0+ is left *held*
> by the ROM, so apps never start on their own — a **debugger-assisted boot**
> (one OpenOCD command) is required. See [The boot issue](#the-boot-issue) and
> [BOOT_ISSUE.md](BOOT_ISSUE.md).

## Board

![Board photo](board_images/board_0.png)
![Board photo 1](board_images/board_1.png)
![Board photo 2](board_images/board_2.png)
![Board photo 3](board_images/board_3.png)
![Board photo 4](board_images/board_4.png)
![Board photo 5](board_images/board_5.png)

Block diagrams (board peripherals):

| Block | Diagram |
|-------|---------|
| MCU / pinout | ![MCU block](board_images/mcu_block.png) |
| Power | ![Power block](board_images/power_block.png) |
| UART | ![UART block](board_images/uart_block.png) |
| SPI | ![SPI block](board_images/spi_block.png) |
| I2C (IIC) | ![IIC block](board_images/iic_block.png) |
| USB Type-C | ![Type-C block](board_images/typec_block.png) |

Key onboard resources used by the demos:

- **LEDs** (active-low): LED0 `P11.1`, LED1 `P0.3`, LED2 `P1.1`, LED4 `P1.5`, LED5 `P13.7`
- **UART** (KitProg3 virtual COM port): **SCB5**, `P5[1]` TX / `P5[0]` RX, **115200 8N1**
- **Debug/programming**: on-board KitProg3 (SWD), SW3 = mode select

## Apps

| App | Core(s) | What it does | Status |
|-----|---------|--------------|--------|
| [`app/blink_hello_m0p`](app/blink_hello_m0p) | CM0+ only | Boot-issue demo: runs at **100 MHz**, blinks LED4/LED5, prints alive messages on UART | working |
| [`app/blink_hello_dualcore`](app/blink_hello_dualcore) | CM0p + CM4 | Dual-core blink + shared-UART `printf`, both cores at **100 MHz** | working |
| [`app/dhry_100m_100m`](app/dhry_100m_100m) | CM0p + CM4 | Dhrystone 2.1 on both cores at **100 MHz**, atomic result blocks | working |
| [`app/coremark_100m_100m`](app/coremark_100m_100m) | CM0p + CM4 | CoreMark 1.0 on both cores at **100 MHz**, atomic result blocks | working |
| [`app/nor_benchmark`](app/nor_benchmark) | CM0p + CM4 | S25FL512S SPI NOR erase/program/read benchmark (GPIO bit-bang) | working |
| [`app/nor_benchmark_hal`](app/nor_benchmark_hal) | CM4 (MTB) | S25FL512S benchmark with the **hardware SMIF** (HAL), 50 MHz | working |
| [`app/xip_test`](app/xip_test) | CM4 (MTB) | QSPI **XIP** test: program external NOR + read back via 0x18000000 | working |
| [`app/cm4_external_app`](app/cm4_external_app) | CM0p + CM4 | CM4 app stored in the external NOR, read back + booted by the CM0p | partial (SMIF/XIP blocked) |

## Apps in `app_ext/` (external-NOR XIP)

The `app_ext/` projects run the **CM4 entirely from the external S25FL512S NOR
via SMIF XIP** at `0x18000000`. A tiny CM0+ XIP stub (internal flash) inits the
SMIF and releases the CM4 to the NOR vector table. The shared stub and UART
source live in `app_ext/common/`.

| App | What it does | Status |
|-----|--------------|--------|
| [`app_ext/blink_hello`](app_ext/blink_hello) | CM4 @ 150 MHz from XIP, blinks LEDs + prints internal DieTemp over SCB5 UART | working |
| [`app_ext/coremark_150m`](app_ext/coremark_150m) | CoreMark 1.0 on the CM4 @ 150 MHz, running from XIP | working |
| [`app_ext/dhry_150m`](app_ext/dhry_150m) | Dhrystone 2.1 on the CM4 @ 150 MHz, running from XIP | working |
| [`app_ext/eink_test`](app_ext/eink_test) | E2271CS021 E-ink demo (cyhal SPI) from XIP, cycles 4 full-screen patterns | working |

Key facts (shared across `app_ext`):

- The CM4 is re-clocked to **150 MHz** (PLL on CLKPATH1); the SMIF interface
  stays on CLKPATH0 (stub FLL) at **25 MHz** for extra XIP read margin. XIP
  data reads are reliable at 150 MHz and marginal at 100 MHz on this board.
- The UART uses peripheral divider **#1** so the cyhal SPI stack (which
  allocates the first free divider) never reuses divider #0 and breaks the UART.
- cyhal-based apps (`eink_test`) must call `__enable_irq()` - the cyhal SPI
  transfers complete via SCB interrupts.


### blink_hello_m0p — the boot-issue demo

CM0+-only, built to demonstrate the boot issue and the workaround:

- Runs at **100 MHz** (FLL: `8 MHz IMO / 20 * 500 / 2`).
- Blinks **LED4 (P1.5)** and **LED5 (P13.7)** sequentially.
- Prints over the KitProg3 UART (115200 8N1):

  ```
  === CM0+ Boot Demo @ 100 MHz ===
  If you see this, the app was booted manually
  (boot ROM held CM0+ in PWR_MODE=1 otherwise)
  [CM0+] alive, SysClk = 100000000 Hz
  ```

- After a normal reset / power-cycle **nothing appears** on the UART and no LED
  blinks — the app is not running (boot ROM hold). After the debugger-assisted
  boot it runs.

> PDL quirk baked into this demo: `Cy_SysClk_FllManualConfigure()` refuses to
> program the FLL if it is already enabled. The ROM leaves the FLL enabled at
> its default (~25 MHz), so `clock_init()` calls `Cy_SysClk_FllDisable()` first.
> (The dualcore app gets away with this because `SystemInit()` disables the FLL.)

## The boot issue

The short version (full diagnosis in [BOOT_ISSUE.md](BOOT_ISSUE.md)):

- After reset, the ROM parks the CM0+ at `PWR_MODE = 1` (held):
  `0x40210080 = 0xfa050001`, `0x40210400 = 0x00000f03` (read-only).
- A valid TOC2 **does not** fix it (proven with a clean physical power cycle).
- KitProg3 SW3 MODE SELECT only switches the USB interface.
- The whole app chain runs fine when the CM0+ image is started manually — so
  **the board, KitProg, and all images are good**. It is a tool/driver release
  interaction, not a hardware or code defect.

## How to build

Most apps are built with `build.sh` (run in **Git Bash / MSYS2**; no
ModusToolbox project needed — plain arm-gcc + the MTB PDL sources).

Prerequisites (paths are hard-coded in the scripts):

- GNU Arm toolchain at `D:\arm-none-eabi-tc\bin\`
- MTB PDL at `D:\board_database\main-cy8ckit-062\mtb-pdl-cat1-release-v3.23.0\`
- CMSIS / core-lib headers are **vendored** in each app's `ext/cmsis` (no external dependency)

### The ModusToolbox apps (`nor_benchmark_hal`, `xip_test`)

These use the HAL SMIF driver. Their library dependencies are pinned as
**git submodules** in `app/mtb_shared/` (10 libs, ~300 MB — too large to
vendor, per the >200 MiB rule). On a fresh clone:

```
git submodule update --init --recursive
```

Then build with ModusToolbox 3.8 (modus-shell):

```
cd app/nor_benchmark_hal      # or app/xip_test
make build TOOLCHAIN=GCC_ARM CONFIG=Debug -j4 MTB_SHARED_DIR=D:/cy8ckit-prj/app/mtb_shared
```


### The `app_ext/` apps (external-NOR XIP)

Same `./build.sh` flow (Git Bash / MSYS2, plain arm-gcc + PDL). Each produces
`build/cm4_<app>.hex` (linked at `0x18000000`) plus the shared
`build/cm0p_xip_stub.hex`:

```
cd app_ext/blink_hello        # or coremark_150m / dhry_150m / eink_test
./build.sh
```

Program the CM4 image to the external NOR with the probe-rs SMIF algorithm
(`tools/psoc6_smif_algo/`), then flash the stub to internal flash and boot.
See each `app_ext/*/README.md` and "Flash / boot" below.


```
cd app/blink_hello_m0p
./build.sh           # -> build/cm0p.hex

cd ../blink_hello_dualcore
./build.sh           # -> build/cm0p.hex, cm4.hex, combined.hex
```

## How to flash / boot

OpenOCD (Infineon KitProg tools):

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe
```

### First time / after a code change — program + boot

`tools/flash_and_boot.tcl` programs the hex and jumps to the CM0+ image:

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe -c "set HEX D:/cy8ckit-prj/app/blink_hello_m0p/build/cm0p.hex" -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```

For the dualcore app, point `HEX` at `.../blink_hello_dualcore/build/combined.hex`.

### After a power cycle / reset — boot only (no re-flash)

`tools/boot_only.tcl` re-jumps to the already-flashed CM0+ image:

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" -c "source D:/cy8ckit-prj/tools/boot_only.tcl"
```

Then open a serial terminal on the KitProg3 COM port at **115200 8N1**. The
app keeps running after the OpenOCD session exits (verified: LEDs keep blinking).

### `app_ext/` (external-NOR XIP) apps

1. Program the CM4 image to the external S25FL512S NOR (probe-rs + the SMIF
   algorithm in `tools/psoc6_smif_algo/`):

   ```
   probe-rs download app_ext/blink_hello/build/cm4_blink_hello.elf \
       --chip-description-path tools/psoc6_smif_algo/algo/target_cy8c6347_smif.yaml \
       --chip CY8C6347BZI-BLD53-S25FL512S --protocol swd --allow-erase-all
   ```

2. Flash the CM0+ XIP stub to internal flash and boot (programs the stub, then
   jumps the CM0+ to it; the stub inits the SMIF and releases the CM4 to
   `0x18000000`):

   ```
   C:/Infineon/Tools/ModusToolboxProgtools-1.9/openocd/bin/openocd.exe \
     -c "set HEX D:/cy8ckit-prj/app_ext/blink_hello/build/cm0p_xip_stub.hex" \
     -c "adapter speed 1000" \
     -c "source [find interface/kitprog3.cfg]" \
     -c "source [find target/infineon/cy8c6xx.cfg]" \
     -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
   ```

After a power cycle / reset, re-boot with `tools/boot_only.tcl` (the NOR image
and stub stay programmed).



