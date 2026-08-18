# app_ext - CM4-only applications running from the external NOR (SMIF XIP)

A set of projects where the CM0+ is reduced to a tiny XIP "bridge" and the
CM4 does everything else, executing its code from the **external S25FL512S
NOR via SMIF XIP** at `0x18000000`.

## Projects

| Project        | CM4 does                                                                  | Verified |
|----------------|---------------------------------------------------------------------------|----------|
| `blink_hello`  | LED blink (P11_1/P0_3/P1_1) + internal DieTemp ADC sampling @ 150 MHz     | **HW OK** |
| `dhry_150m`    | Dhrystone 2.1 @ 150 MHz (251,889 Dhrystones/s, 0.956 DMIPS/MHz)           | **HW OK** |
| `coremark_150m`| CoreMark 1.0 @ 150 MHz (288.4 CoreMarks/s)                                | **HW OK** |

## Architecture (2.1 / 2.2)

**CM0+ XIP stub** (`common/cm0p_xip_stub`, internal flash 0x10000000):
1. Runs both cores at 100 MHz (FLL) so the SMIF XIP reads run at the proven
   50 MHz interface clock (`CLK_HF[2] = CLKPATH0/2`).
2. Initializes the SMIF in XIP/memory mode: pins P11[2..7] -> HSIOM 17,
   `DEVICE[0]` region + quad read 0xEC / quad program 0x34 command config.
3. Releases the CM4 with its vector table at `0x18000000`
   (`Cy_SysEnableCM4(0x18000000)`).
4. Enters an **interruptible IDLE** (`WFI`, interrupts enabled) and stays out
   of the way.

**CM4 apps** (external NOR 0x18000000, programmed with the probe-rs tool):
1. Raise the CPU to **150 MHz** with the PLL on CLKPATH1 (`inputFreq` = IMO
   8 MHz via the CLKPATH1 mux; `Cy_SysClk_PllConfigure(1, 8 MHz, 150 MHz)`).
   The SMIF interface clock stays on CLKPATH0 (FLL@100), so XIP reads keep
   running at the proven 50 MHz through the transition.
2. Own **all the SRAM above the CM0+ region**: 0x08008000..0x08048000
   (256 KB). SCB5 UART (115200 8N1) is owned exclusively by the CM4.

## Brick-risk assessment (2.3) - decision: dedicated CM0+ SRAM + interrupts

Running the CM4 with *all* SRAM and the CM0+ fully disabled is not a
permanent brick (the ROM bootloader always re-runs on reset, and SWD is
independent of the application), but it is an **operability risk**:

- Without dedicated SRAM the CM4 can silently overwrite the CM0+ stack /
  vector table, so a later CM0+ wake (interrupt, debug halt, recovery code)
  crashes and the SWD session can no longer cleanly halt/read the CM0+ core.
- A live, interruptible CM0+ is required for future watchdog / recovery
  features and for robust debugging.

**Therefore:** the CM0+ keeps a **dedicated 32 KB SRAM region
(0x08000000..0x08008000)** and stays **interrupt-enabled in IDLE**
(`__WFI()` loop, PRIMASK clear). The CM4 uses everything from
0x08008000..0x08048000. The cost is ~11% of the 288 KB SRAM for a fully
recoverable system.

## Build & run

```bash
# each project builds its own CM4 app + the shared CM0+ stub:
cd app_ext/dhry_150m && bash build.sh          # (blink_hello, coremark_150m likewise)

# 1. program the CM4 app to the external NOR (probe-rs, custom SMIF algo):
probe-rs download build/cm4_dhry_150m.elf \
    --chip-description-path ../../tools/psoc6_smif_algo/algo/target_cy8c6347_smif.yaml \
    --chip CY8C6347BZI-BLD53-S25FL512S --protocol swd --allow-erase-all

# 2. flash + boot the CM0+ stub (boot-ROM hold => manual boot):
#    openocd -c "set HEX build/cm0p_xip_stub.hex" -c "source ../../tools/flash_and_boot.tcl"
```

## Notes / known constraints

- **XIP throttles the CM4.** The SMIF runs at 50 MHz (quad), so code-fetch
  bandwidth is far below the 150 MHz CPU's demand. Benchmarks are therefore
  lower than the same code in internal flash (e.g. Dhrystone 0.956 vs 1.24
  DMIPS/MHz). Raising the SMIF interface clock to 75 MHz did not measurably
  help the (cache-resident) benchmark loops, so the stub's 50 MHz setting is
  kept.
- The ROM does **not** auto-boot the CM0+ on this board (boot-ROM hold), so
  the stub must be booted manually even with no debugger attached (confirmed
  by a SWD-disconnected power-cycle test).
- The probe-rs external-NOR write takes **~2 s** for a ~40 KB image after
  the SMIF-algorithm speed fix (see tools/psoc6_smif_algo/README.md).
