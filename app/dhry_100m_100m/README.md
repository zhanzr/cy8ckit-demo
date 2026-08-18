# Dhrystone 2.1 @ 100 MHz — PSoC6 (CY8CKIT-062-BLE)

Classic Dhrystone 2.1 (dhry_1.c / dhry_2.c / dhry.h), ported from the STM32
`h723-mini` benchmark. Runs **on both cores simultaneously** at **100 MHz**
(FLL), each core printing its own result block on the shared SCB5 UART
(115200 8N1). **1,000,000 runs** per run (~5 s).

## Results (measured on hardware, 100 MHz, `-Ofast -funroll-loops`, no LTO)

| Core | Dhrystones/s | DMIPS/MHz |
|------|--------------|-----------|
| CM0p | 176,554      | 1.005     |
| CM4  | 217,817      | 1.240     |

Both cores print correct final values (Int_Glob=5, Arr_2_Glob = runs+10, …).

> **Do not use LTO for Dhrystone.** GCC `-flto` sees the whole program and
> hoists loop-invariant work out of the timed loop, inflating the score.

## Atomic result blocks

Both cores share one SCB5 UART, so each core holds a **spinlock
(`uart_lock()`/`uart_unlock()`, shared flag at `0x08005040`) for the whole
benchmark run** — the header line, the Dhrystone execution and its result block
are printed as one atomic unit. The two cores therefore alternate their result
blocks on the console and never interleave, so a block can be copied verbatim.

## Build / flash / boot

```
./build.sh           # Git Bash / MSYS2 -> build/combined.hex
```

Program + boot (see `D:\cy8ckit-prj\BOOT_ISSUE.md` for why manual boot is needed):

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe -c "set HEX D:/cy8ckit-prj/app/dhry_100m_100m/build/combined.hex" -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```

Open the KitProg3 COM port at **115200 8N1**. Each core prints a block every
~5–6 s; capture ~12 s for one full block per core.

## Notes

- Clock: both cores share CLK_FAST via the 100 MHz FLL; `clock_init()` in
  `cm0p/main.c` disables the ROM's default FLL first (see the m0p boot demo).
- Timing: per-core SysTick (1 ms) provides `HAL_GetTick()`; Dhrystone's
  `Too_Small_Time` gate (2 s) is satisfied (~5 s per run).
- CM0p RAM enlarged to 20 KB (0x08000000–0x08005000) to fit Dhrystone's
  10 KB `Arr_2_Glob[50][50]`; the shared UART flags moved to 0x08005040/44 and
  the CM4 RAM origin to 0x08005100.
- `RUN_NUMBER` can be adjusted in `bench/dhry.h`.
