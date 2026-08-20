# app_ext/ntc_test - NTC thermistor + DieTemp temperature demo from external NOR XIP

Port of `app/ntc_test` to the app_ext flow: the CM4 runs **entirely from the
external S25FL512S NOR via SMIF XIP** (`0x18000000`), using the shared
`app_ext/common` CM0+ XIP stub + UART.

Same NTC circuit and code as `app/ntc_test` (A0..A3 = P10.0..P10.3, the ADC
following Infineon `mtb-example-hal-adc-basic`, the factory-calibrated DieTemp
conversion). The app reports the raw SAR counts + pin millivolts + best-effort
temperatures:

```
A0=44512 A1=44672 A2=44784 A3=44880 DTS=445
mV: 1143 1146 1148 1149   NTC:  0.00 C   DTS:  26 C
```

Differences from `app/ntc_test`:

- **CPU re-clocked to 150 MHz** (PLL on CLKPATH1, SMIF stays on CLKPATH0) for
  reliable XIP reads.
- No `cybsp_init()`: the generated device config is applied selectively
  (`init_cycfg_clocks/routing/peripherals/pins`, **not** `init_cycfg_system`).
- `uart_init()` (SCB5, P5_1/P5_0 @ 115200) replaces `cy_retarget_io`.
- The VBG-calibrated `cyhal_adc_read_uv()` is unavailable here, so the
  millivolts fall back to the raw-count scale (VDDA/2 nominal, 1650 mV FS).

> **Note (hardware):** both NTC sides measure ~1.2 V with a meter, so no
> voltage develops across the NTC and the NTC temperature is not meaningful
> right now. The raw counts / millivolts are the primary output.

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
