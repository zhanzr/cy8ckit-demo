# app_ext/eink_test - E2271CS021 E-ink from external NOR XIP (EXPERIMENTAL)

A port of `app/eink_test` (the working cyhal demo) to run the CM4 from the
external S25FL512S NOR via SMIF XIP at `0x18000000`.

It uses the **same cyhal SPI stack** as the working `app/eink_test` (compiled
from the vendored `app/mtb_shared`), the display-eink-e2271cs021 driver, and
the shared app_ext CM0+ XIP stub (SMIF init + `Cy_SysEnableCM4(0x18000000)`).
No FreeRTOS / emWin.

## Status: WORKING (confirmed display + cycling loop)
- Builds cleanly (`./build.sh` -> `build/cm4_eink_test.hex` at 0x18000000).
- The panel **displays** and the main loop **cycles** all four patterns
  (checkerboard / horizontal bars / vertical bars / box) over external-NOR XIP.
- Two fixes made it reliable:
  1. **Re-clock the CPU to 150 MHz** (PLL on CLKPATH1) while keeping CLKPATH0
     (the stub FLL) as the SMIF interface clock source. XIP data reads are
     reliable at 150 MHz (proven by `blink_hello`) and marginal at 100 MHz.
  2. **`__enable_irq()`** - the cyhal SPI stack completes its transfers via
     SCB interrupts, so global interrupts must be enabled.
- The stub uses a 25 MHz SMIF interface clock (extra XIP read margin), and the
  UART uses peripheral divider #1 so the cyhal SPI stack (which allocates the
  first free divider) never steals divider #0 and breaks the UART.

## Build / flash / boot
```
./build.sh   # -> build/cm4_eink_test.hex (0x18000000) + build/cm0p_xip_stub.hex
probe-rs download build/cm4_eink_test.elf \
    --chip-description-path ../../tools/psoc6_smif_algo/algo/target_cy8c6347_smif.yaml \
    --chip CY8C6347BZI-BLD53-S25FL512S --protocol swd --allow-erase-all
# then flash the stub (internal flash) and boot it:
#   C:/Infineon/Tools/ModusToolboxProgtools-1.9/openocd/bin/openocd.exe \
#     -c "set HEX D:/cy8ckit-prj/app_ext/eink_test/build/cm0p_xip_stub.hex" \
#     -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" \
#     -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" \
#     -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```

EPD pin map and UART (P5_1/P5_0 @ 115200) are the same as `app/eink_test`.
