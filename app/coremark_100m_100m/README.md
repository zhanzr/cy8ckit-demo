# CoreMark 1.0 @ 100 MHz — PSoC6 (CY8CKIT-062-BLE)

EEMBC CoreMark 1.0 (stock `coremark_1_0_1` sources), ported from the STM32
`h723-mini` benchmark. Runs **on both cores simultaneously** at **100 MHz**
(FLL), each core printing its own result block on the shared SCB5 UART
(115200 8N1). **6,000 iterations** per run (~25–35 s).

## Results (measured on hardware, 100 MHz, `-Ofast -funroll-loops`)

| Core | CoreMark 1.0 | Iterations/s | Total time |
|------|--------------|--------------|------------|
| CM0p | 176.15       | 176.15       | 34.1 s     |
| CM4  | 236.03       | 236.03       | 25.4 s     |

Both cores print **`Correct operation validated.`** (valid CRCs).

## Atomic result blocks

Both cores share one SCB5 UART, so each core holds a **spinlock
(`uart_lock()`/`uart_unlock()`, shared flag at `0x08005040`) for the whole
benchmark run** — the header line, the CoreMark run and its result block are
printed as one atomic unit. The two cores therefore alternate their result
blocks on the console and never interleave, so a block can be copied verbatim.

## Build / flash / boot

```
./build.sh           # Git Bash / MSYS2 -> build/combined.hex
```

Program + boot (see `D:\cy8ckit-prj\BOOT_ISSUE.md` for why manual boot is needed):

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe -c "set HEX D:/cy8ckit-prj/app/coremark_100m_100m/build/combined.hex" -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```

Open the KitProg3 COM port at **115200 8N1**. Each core prints one result block
per run (~25 s CM4, ~34 s CM0p); capture ~75 s to get one clean block from both.

## Notes

- Clock: both cores share CLK_FAST via the 100 MHz FLL; `clock_init()` in
  `cm0p/main.c` disables the ROM's default FLL first (see the m0p boot demo).
- Timing: per-core SysTick (1 ms) provides `HAL_GetTick()`, used as
  `CORE_TICKS` (`GETMYTIME`); CoreMark's minimum-time check (10 s) is met.
- Port: `SEED_VOLATILE`, `MEM_LOCATION "Static"`, `HAS_FLOAT 1`.
- CM0p RAM enlarged to 20 KB (0x08000000–0x08005000); the shared UART flags
  moved to 0x08005040/44 and the CM4 RAM origin to 0x08005100.
- `ITERATIONS` can be adjusted in `bench/core_portme.h`.
