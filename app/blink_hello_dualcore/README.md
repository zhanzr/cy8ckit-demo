# blink_hello_dualcore

Dual-core PSoC 6 (CY8C6347BZI-BLD53 / CY8CKIT-062-BLE) demo:

- **CM0+ core**: blinks two onboard LEDs sequentially and prints its core info over UART.
- **CM4 core**: blinks three onboard LEDs sequentially and prints its core info over UART.
- Both cores share one SCB5 UART (115200 8N1) protected by a simple cross-core mutex.

## LED assignments and blink patterns

| Core | LED | Pin  | Pattern |
|------|-----|------|---------|
| CM4  | LED0 | P11.1 | INV → 250 ms delay → LED1 → 250 ms → LED2 → 250 ms (repeat) |
| CM4  | LED1 | P0.3  |                                                              |
| CM4  | LED2 | P1.1  |                                                              |
| CM0+ | LED4 | P1.5  | INV → 250 ms delay → LED5 → 250 ms (repeat)                  |
| CM0+ | LED5 | P13.7 |                                                              |

Every ~1 s each core prints (mutex-protected):

```
[CM0] Core ID = 0, SysClk = 8000000 Hz
[CM4] Core ID = 1, SysClk = 8000000 Hz
```

## UART

- **SCB5**, pins **P5[0] (RX) / P5[1] (TX)**, **115200 8N1**, connected to the KitProg3 virtual COM port.
- The SCB5 peripheral clock uses the **16.5-bit fractional divider** to produce ~115200 baud from the 8 MHz IMO:
  `Cy_SysClk_PeriphSetFracDivider(CY_SYSCLK_DIV_16_5_BIT, 0, 3, 11)` (divide factor 4.34).
- `printf` is retargeted via `_write()` on both cores.

## Shared cross-core UART mutex

Both cores share one UART, so a simple spinlock serializes `printf`:

- Fixed SRAM flags:
  - `UART_LOCK_ADDR  = 0x08003040` (0 = free, 1 = held)
  - `UART_READY_ADDR = 0x08003044` (1 after the CM0+ initializes SCB5)
- Each core: `while (*LOCK != 0); *LOCK = 1; printf(...); *LOCK = 0;`
- Both cores clear the lock to 0 at startup (the region overlaps the OpenOCD flash
  work area, which can leave stale data after programming).
- The region `0x08003000..0x080030FF` is reserved for shared flags: it sits above the
  CM0+ stack top (0x08003000, stack grows down) and below the CM4 RAM
  (CM4 `ram` origin moved to 0x08003100 in `linker/cm4.ld`).

## Build

- `build.sh` (run in **Git Bash / MSYS2**: `./build.sh`).
- Outputs in `build/`:
  - `cm0p.elf` / `cm0p.hex` — CM0+ image (placed at 0x10000000)
  - `cm4.elf` / `cm4.hex` — CM4 image (placed at 0x10020000, with the CM0+ binary embedded)
  - `combined.hex` — full image (CM0+ at 0x10000000 + CM4 at 0x10020000)

Key build defines:

```
-DCY_CORTEX_M4_APPL_ADDR=0x10020000
```

(`system_psoc6.h` defaults to 0x10002000, which does NOT match the linker
`FLASH_CM0P_SIZE = 0x20000` — the CM0+ would start the CM4 at the wrong address.)

## Notes / gotchas fixed during bring-up

1. **CM4 start address**: `CY_CORTEX_M4_APPL_ADDR` must be 0x10020000 (matches the
   linker's CM4 placement after the 128 KB CM0+ region).
2. **UART `breakWidth`**: PDL asserts `breakWidth >= DATA_WIDTH + 3`; use `0x10`.
3. **SysTick clock**: use `CY_SYSTICK_CLOCK_SOURCE_CLK_CPU`; `CLK_IMO` selects the
   external clock which does not tick on this part.
4. **Interrupts**: the startup does `cpsid i`; each core must call `__enable_irq()`
   for SysTick/ISR callbacks to run.
5. **P5.5**: not an onboard LED; the CM0+ LED uses **P1.5**.
6. **Shared flags**: must be outside the OpenOCD flash work area (0x08000000–0x08008000)
   and initialized to 0 at startup, or both cores can deadlock in `uart_lock`.

## Programming / running

> The PSoC 6 boot ROM currently does **not** auto-boot any image on this board
> (the CM0+ is left held; see `D:\cy8ckit-prj\BOOT_ISSUE.md`). Use the
> debugger-assisted scripts in `D:\cy8ckit-prj\tools\`.

First time / after a code change (program + boot):

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe ^
  -c "set HEX D:/cy8ckit-prj/app/blink_hello_dualcore/build/combined.hex" ^
  -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" ^
  -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```

After a power cycle / reset (re-boot, no re-flash):

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe ^
  -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" ^
  -c "init" -c "source D:/cy8ckit-prj/tools/boot_only.tcl"
```

Open a serial terminal on the KitProg3 COM port at **115200 8N1** to see the prints.

## Project layout

- `cm0p/main.c` — CM0+ application (LED4/LED5, UART init, starts CM4, printf)
- `cm0p/uart.c` — SCB5 UART init, putc/puts, printf retarget (`_write`)
- `cm4/main.c` — CM4 application (LED0/LED1/LED2, printf)
- `linker/cm0plus.ld` / `linker/cm4.ld` — linker scripts (CM4 embeds the CM0+ image, shared region reserved)
- `system/` — system_psoc6_cm0plus.c / cm4.c
- `startup/` — startup_psoc6_01_cm0plus.S / cm4.S
