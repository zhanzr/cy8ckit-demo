# app_ext/pdm2pcm_test - PDM/PCM mic capture from external NOR XIP

Port of `app/pdm2pcm_test` to the app_ext flow: the CM4 runs **entirely from
the external S25FL512S NOR via SMIF XIP** (`0x18000000`), using the shared
`app_ext/common` CM0+ XIP stub + UART.

Differences from `app/pdm2pcm_test`:

- **CPU re-clocked to 150 MHz** (PLL on CLKPATH1, SMIF stays on CLKPATH0) for
  reliable XIP reads.
- No `cybsp_init()`: the generated device config is applied selectively
  (`init_cycfg_clocks/routing/peripherals/pins`, **not** `init_cycfg_system`).
- **`uart_init()` runs LAST** (after the PDM setup). The PDM/audio clock setup
  (PLL0 for CLK_HF[1]) reconfigures the peripheral dividers and would otherwise
  clobber the UART's 16.5-bit divider, putting the SCB5 baud off (~100 k instead
  of 115200). A short settle delay follows `uart_init()`.

`PDM_DATA` P10_5 / `PDM_CLK` P10_4 (EPD shield mic) - same as the `app` version.

## Build / flash / boot

```bash
./build.sh   # -> build/cm4_pdm2pcm_test.hex (0x18000000) + build/cm0p_xip_stub.hex

probe-rs download build/cm4_pdm2pcm_test.elf \
    --chip-description-path ../../tools/psoc6_smif_algo/algo/target_cy8c6347_smif.yaml \
    --chip CY8C6347BZI-BLD53-S25FL512S --protocol swd --allow-erase-all

# flash the stub (internal flash) + boot:
#   C:/Infineon/Tools/ModusToolboxProgtools-1.9/openocd/bin/openocd.exe \
#     -c "set HEX D:/cy8ckit-prj/app_ext/pdm2pcm_test/build/cm0p_xip_stub.hex" \
#     -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" \
#     -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" \
#     -c "source D:/cy8ckit-prj/tools/flash_stub_boot.tcl"
```

UART: P5_1/P5_0 @ 115200 8N1, COM26.
