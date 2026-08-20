# app_ext/ntc_test - NTC thermistor + DieTemp temperature demo from external NOR XIP

Port of `app/ntc_test` to the app_ext flow: the CM4 runs **entirely from the
external S25FL512S NOR via SMIF XIP** (`0x18000000`), using the shared
`app_ext/common` CM0+ XIP stub + UART.

Same NTC circuit and code as `app/ntc_test`: the 0R strap resistors to
VIO_REF/GND are not populated, so A0 is driven high (3.3 V supply) and A3 low
(return); A1/A2 (the divider output) are sampled by the SAR ADC. The internal
DieTemp uses the 1.2 V bandgap reference + the factory calibration (like
`blink_hello`), with the SAR aperture sized to ~1.15 us.

```
A1=47136 A2=47136 DTS=1200
mV: 1448 1448   NTC:  31.62 C   DTS:   33 C
```

Differences from `app/ntc_test`:

- **CPU re-clocked to 150 MHz** (PLL on CLKPATH1, SMIF stays on CLKPATH0) for
  reliable XIP reads.
- **No `init_cycfg_*` calls** (like `blink_hello`): the generated device
  config would reconfigure the PASS/analog routing and break the DieTemp read.
  The SAR clock + UART are set up manually in `ntc_init()`/`uart_init()`.
- `uart_init()` (SCB5, P5_1/P5_0 @ 115200) replaces `cy_retarget_io`.
- The VBG-calibrated `cyhal_adc_read_uv()` is unavailable here, so the
  millivolts fall back to the raw-count scale (~2015 mV full-scale, which
  reproduces the VBG result on this board).

## Build / flash / boot

```bash
./build.sh   # -> build/cm4_ntc_test.hex (0x18000000) + build/cm0p_xip_stub.hex

probe-rs download build/cm4_ntc_test.elf \
    --chip-description-path ../../tools/psoc6_smif_algo/algo/target_cy8c6347_smif.yaml \
    --chip CY8C6347BZI-BLD53-S25FL512S --protocol swd --allow-erase-all

# flash the stub (internal flash) + boot:
#   C:/Infineon/Tools/ModusToolboxProgtools-1.9/openocd/bin/openocd.exe \
#     -c "set HEX D:/cy8ckit-prj/app_ext/ntc_test/build/cm0p_xip_stub.hex" \
#     -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" \
#     -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" \
#     -c "source D:/cy8ckit-prj/tools/flash_stub_boot.tcl"
```

UART: P5_1/P5_0 @ 115200 8N1, COM26.
