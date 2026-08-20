# app_ext/capsense_test - CAPSENSE buttons + slider from external NOR XIP

Port of `app/capsense_test` to the app_ext flow: the CM4 runs **entirely from
the external S25FL512S NOR via SMIF XIP** (`0x18000000`), using the shared
`app_ext/common` CM0+ XIP stub + UART.

Differences from `app/capsense_test`:

- **CPU re-clocked to 150 MHz** (PLL on CLKPATH1, SMIF stays on CLKPATH0) for
  reliable XIP reads. The capsense config is generated for 100 MHz; the
  middleware still works at 150 MHz (verified: buttons/slider respond).
- No `cybsp_init()`: the generated device config is applied selectively
  (`init_cycfg_clocks/routing/peripherals/pins`, **not** `init_cycfg_system`,
  so the stub's clocks/SMIF are not disturbed), the shared
  `app_ext/common/cm4/uart.c` provides the SCB5 UART (115200).
- The CAPSENSE Tuner EzI2C bridge is set up but non-fatal if the EzI2C init
  fails.

## Build / flash / boot

```bash
./build.sh   # -> build/cm4_capsense_test.hex (0x18000000) + build/cm0p_xip_stub.hex

probe-rs download build/cm4_capsense_test.elf \
    --chip-description-path ../../tools/psoc6_smif_algo/algo/target_cy8c6347_smif.yaml \
    --chip CY8C6347BZI-BLD53-S25FL512S --protocol swd --allow-erase-all

# flash the stub (internal flash) + boot:
#   C:/Infineon/Tools/ModusToolboxProgtools-1.9/openocd/bin/openocd.exe \
#     -c "set HEX D:/cy8ckit-prj/app_ext/capsense_test/build/cm0p_xip_stub.hex" \
#     -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" \
#     -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" \
#     -c "source D:/cy8ckit-prj/tools/flash_stub_boot.tcl"
```

UART: P5_1/P5_0 @ 115200 8N1, COM26. CAPSENSE pins are the same as
`app/capsense_test`.
