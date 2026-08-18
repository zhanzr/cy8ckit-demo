# PSoC6 SMIF external-NOR probe-rs flash algorithm

Programs the CY8CKIT-062 on-board **S25FL512S (64 MB) external NOR** through
the PSoC6 SMIF peripheral using **probe-rs**, via a custom target extension +
RAM flash algorithm. This replaces the failing OpenOCD vendor SMIF FLM
(`CY8C6xxA_SMIF*.FLM` → "Init operation failed (1)").

## Layout

```
psoc6_smif_algo/
  algo/
    flash_s25fl512s_smif.c   # the algorithm (register-level SMIF, PIC, single-SPI 4-byte-address)
    algo.ld                  # links at 0x08000000 (PSoC6 SRAM) for the PIC blob
    build_algo.py            # compiles + emits target_cy8c6347_smif.yaml
    target_cy8c6347_smif.yaml  # probe-rs chip description (auto-generated)
  harness/
    build.sh                 # CM0+ single-core firmware build (modus-shell bash)
    main.c                   # runs the algorithm code as a debuggable app over SCB5 UART
    ...
```

## Usage

### 1. Build the algorithm + target YAML

```bash
cd algo
python build_algo.py          # needs arm-none-eabi-gcc on PATH or edit GCC/LD/NM/OBJCOPY
```

### 2. Program the external NOR with probe-rs

```bash
probe-rs download <image.elf> \
    --chip-description-path algo/target_cy8c6347_smif.yaml \
    --chip CY8C6347BZI-BLD53-S25FL512S \
    --protocol swd --allow-erase-all
```

Any loadable section in `0x18000000..0x1C000000` is programmed (erase =
256 KB sectors via `0xDC`, program = 256 B pages via `0x12`, single-SPI,
4-byte addresses). Verified on hardware: a 64-byte pattern written by
probe-rs reads back byte-exact.

### 3. Debug the algorithm on hardware (harness)

The harness compiles the *exact* algorithm source as a normal firmware that
replicates the probe-rs "connect under reset" context (reset clock, no HAL).
It prints `Init` (JEDEC), erase, program and read-back over SCB5 UART instead
of cryptic probe-rs error codes:

```bash
cd harness
bash build.sh                 # -> build/harness.hex (internal flash)
# flash + boot with the usual D:/cy8ckit-prj/tools/flash_and_boot.tcl,
# watch COM26 @ 115200
```

Read-only dump mode (verifies what probe-rs wrote, no erase/program):

```bash
READ_ONLY=1 bash build.sh
```

## How the algorithm works (and what the vendor FLM got wrong)

The PSoC6 SMIF needs three things a bare RAM algorithm must set up itself:

1. **SMIF pins** `P11[2..7]` → HSIOM **17** (SMIF function). The SS0 pin
   (`P11_2`) uses a 5-bit HSIOM field; setting it to `1` (GPIO) instead of
   `17` silently leaves the slave-select unconnected and the SMIF stuck BUSY.
2. **SMIF interface clock**: `SRSS_CLK_ROOT_SELECT[2]` = `0x80000010`
   (`CLK_HF[2] = CLKPATH0 / 2`, 50 MHz at the 100 MHz FLL). Without it the
   SMIF never consumes the command FIFO (stays BUSY, `STATUS=0x80000000`).
3. **SMIF command mode**: `SMIF0_CTL = 0x80071000`,
   `DEVICE[0]` CTL/ADDR/MASK/ADDR_CTL as the HAL programs, and the command
   FIFO protocol (opcode/addr bytes with `SS0=1<<8`, TX/RX-count data phases).

Commands (single-SPI, 4-byte addressing, no QE dependency):
`0x9F` JEDEC, `0x13` read, `0x12` page program, `0xDC` 256 KB sector erase,
`0x60` chip erase, `0x06`/`0x04` write enable/disable, `0x05` status (WIP).

The debug harness (`probers_alg`-style, after the h723-mini tool) is what
located the two root causes: the SS0 HSIOM bug and the missing SMIF clock.

## Internal-flash flashing benchmark (OpenOCD vs probe-rs)

Same 178 KB internal image, KitProg3 SWD:

| Flasher | time | boot after reset |
|---------|------|------------------|
| OpenOCD `program ... verify reset` | ~3.36 s | needs manual CM0+ resume (boot-ROM hold) |
| probe-rs `download --allow-erase-all` | ~6.58 s | needs manual CM0+ resume (boot-ROM hold) |

**OpenOCD remains the default** internal flasher (faster + the
`flash_and_boot.tcl` boot tooling exists). probe-rs's niche is the external
NOR (this tool) and as an OpenOCD-free fallback.
