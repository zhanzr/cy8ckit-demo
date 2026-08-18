# CY8CKIT-062-BLE — Boot ROM Holds the CM0+ (no auto-boot)

**Status:** Board and apps are fully functional. The only defect is that the PSoC6
boot ROM will **not auto-boot** any application image after reset / power-cycle.
A debugger-assisted "program + boot" workflow (see `tools/`) works reliably.

## Hardware / Software

- Board: **CY8CKIT-062-BLE** (on-board KitProg3 + PSoC 6 BLE, SW3 = mode select)
- Target: **CY8C6347BZI-BLD53** (PSoC 6 BLE, 1 MB flash, 288 KB SRAM, dual core)
- Programmer: on-board KitProg3, firmware **2.82.1735** (current for ModusToolbox Progtools-1.9)
- Debugger: **Infineon OpenOCD** `C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe`
- Tools: ModusToolbox VS Code Extension projects in `D:\mtb_vs_prj` (`t1`, `hw`)
- Custom minimal CM0+ project: `app/blink_hello_m0p` (this repo)

## Symptoms

- All applications (MTB hello-world, MTB gpio-pins, minimal CM0+ blink) build and
  program with **Verify OK**.
- After programming / reset / power-cycle / USB re-plug, **no LED, no UART** — the
  app never runs.
- The CM0+ core sits parked in the ROM/boot wait state at `0x1f2c`
  (`lr = 0x160022a7` = flash-boot dispatch).

## What is verified working

The **whole application chain runs** when the CM0+ image is started manually via the
debugger (proven multiple times):

1. Jump to the CM0+ image Reset_Handler (`SP=0x08003000`, `PC=0x1000015b` for the
   cat1cm0p BLESS image, or `0x10000732` for the minimal CM0+ blink app).
2. The CM0+ BLESS image boots, starts the CM4 app at `0x10020000`, and the
   **hello-world LED on P1.5 blinks at 1 Hz** (`GPIO_PRT1.OUT` / `0x40320080` toggles
   between `0x00` and `0x20`).
3. UART (P5_1/P5_0, 115200) works once the app runs.

So: **the board, the KitProg, and the application images are all good.**

## Diagnosis (what the boot ROM does)

On reset the boot ROM checks CPUSS boot state and parks the CM0+ instead of booting:

| Register            | Value           | Meaning                                             |
|---------------------|-----------------|-----------------------------------------------------|
| `0x40210080`        | `0xfa050001`    | CM0+ power control = **PWR_MODE 1 (held/reset)**     |
| `0x40210400`        | `0x00000f03`    | bits[11:8]=0x0f → ROM takes the "hold" boot branch  |
| `0x402102b0`        | `0xffffff00`    | CM0+ app vector-table pointer = "no app" marker     |
| `0x402102c0`        | `0x00000000`    | CM0+ app vector-table pointer (path A) = 0          |
| SFLASH `0x16007C00` | (TOC2) empty    | user-app boot table erased/absent                   |

- The ROM reset handler reads `0x40210400 & 0x0f00`; non-zero routes it to a branch
  that only boots when `0x40210080 & 3` is 2 or 3. With PWR_MODE = 1 it parks the CM0+.
- Every reset re-initializes these registers to the held/"no app" state, and they
  persist across power cycles performed via the KitProg.
- Programming a valid TOC2 (`magic 0x01211220`, `FIRST_USER_APP=0x10000000`,
  `SECOND_USER_APP=0x10020000`, OBJECT_SIZE 0x1FC, correct CRC16-CCITT) did **not**
  change the boot behavior.
- Pressing **SW3 MODE SELECT** only switches the KitProg USB interface (Proprietary
  SWD ↔ Mass-storage/CMSIS-DAP). It does **not** release the CM0+.

## Root-cause analysis

**Test result that rules out the "missing TOC2" theory:** A valid TOC2 was written
(`magic 0x01211220`, `OBJECT_SIZE 0x1FC`, `FIRST_USER_APP=0x10000000`,
`SECOND_USER_APP=0x10020000`, and a CRC16-CCITT that was verified correct against the
factory TOC1 = `0x71A1`). After a clean physical power cycle the app **still does not
boot**. So an empty/invalid TOC2 is **not** the root cause (despite what that theory
claims).

The real blocker is the **CM0+ power/hold state**:

- `0x40210080` = `0xfa050001` → **PWR_MODE = 1 (CM0+ held)**. The boot ROM only boots
  the app when PWR_MODE is 2 or 3; with 1 it parks the CM0+ in a wait loop.
- `0x40210400` = `0x00000f03` (bits[11:8]=0x0f) routes the ROM into that hold branch.
  It is **read-only** — no write (plain or keyed `0x05FA`) changes it, and it persists
  across power cycles, so the ROM always takes the hold path.
- The reset re-initializes the app pointers (`0x402102b0=0xffffff00`,
  `0x402102c0=0`) to "no app", and nothing (ROM, FlashBoot, driver) later sets the
  CM0+ free on this board.

The most likely remaining explanation: **the current ModusToolbox OpenOCD does not
release the CM0+ the way the classic PSoC Creator / PSoC Programmer flow did.**
(The on-board KitProg can hold the target; SW3 MODE SELECT only switches the USB
interface and does not release it.)

**Conclusion:** Not a hardware failure and not a TOC2 configuration problem. A
tool/driver release (or board-KitProg interaction) issue. Until the classic
programming flow is used, the debugger-assisted boot workaround below is required.

## Working workaround (debugger-assisted boot)

Scripts are in **`tools/`**:

- `flash_and_boot.tcl` — program a hex, then jump to the CM0+ image and run it.
- `boot_only.tcl` — after a power cycle / reset, re-jump to the already-flashed CM0+
  image and run it (no re-program needed).

Both do a full reset-and-run: they set `SP`/`PC` from the CM0+ vector table at
`0x10000000` (`SP = mem[0x10000000]`, `Reset = mem[0x10000004]`) and resume.

### First time / after a code change (program + run)

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe -c "set HEX D:/mtb_vs_prj/hw/build/last_config/mtb-example-hal-hello-world.hex" -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```

### After a power cycle / reset (re-run, no flash)

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" -c "source D:/cy8ckit-prj/tools/boot_only.tcl"
```

The app keeps running after the OpenOCD session exits (verified: LED keeps blinking).

## Next steps to fix auto-boot

1. **mtb-programmer / PSoC Programmer (GUI)** — `C:\Infineon\Tools\ModusToolboxProgtools-1.9\mtb-programmer\mtb-programmer.exe`
   (or `D:\Program Files (x86)\Cypress\Programmer\PSoCProgrammer.exe`). These support
   the CY8C6xxx family (FLMs present) and do a full erase+program; one attempt is
   worth it, but note the valid-TOC2 test above suggests it may not change the boot.
2. External **KitProg2 / MiniProg3** on the SWD header (J1) — the classic flow.
3. File an **Infineon support ticket** with this diagnosis (PSoC 6 BLE CY8C6347:
   boot ROM holds the CM0+ at `PWR_MODE=1`; valid TOC2 does not help; only a
   debugger-assisted manual boot runs the app).

## Appendix — useful addresses / notes

- GPIO base `0x40320000`; `GPIO_PRT1` = `0x40320080` (P1.5 = LED8, hello-world LED),
  `GPIO_PRT13` = `0x40320680` (P13.7); `OUT` at offset 0.
- User button (gpio-pins example) = **P0.4**.
- Debug UART = P5_1 (TX) / P5_0 (RX), 115200 8N1 (KitProg3 virtual COM port).
- Flash base `0x10000000` (CM0+ image), CM4 app at `0x10020000`.
- SFLASH TOC1 `0x16007800` (valid factory content), TOC2 `0x16007C00` (user-app
  table, currently empty), FB (flash boot) `0x16002000` (intact).
