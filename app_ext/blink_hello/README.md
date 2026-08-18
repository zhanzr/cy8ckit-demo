# blink_hello (app_ext) - CM4 LED blink + ADC from SMIF XIP @ 150 MHz

The CM0+ XIP stub brings up the SMIF XIP and releases this core; this CM4 app
(linked at `0x18000000`, running from the external NOR) re-clocks to
**150 MHz** via the PLL, blinks LED0/1/2 (P11_1, P0_3, P1_1) and samples the
internal DieTemp SAR ADC, printing over SCB5 UART (115200 8N1).

```
bash build.sh                          # build/cm4_blink_hello.hex + build/cm0p_xip_stub.hex
# program the CM4 app to the NOR (probe-rs), flash + boot the stub - see ../README.md
```

Verified on hardware: `blink_hello [app_ext] CM4 @ 150000000 Hz (XIP)` with
live `DieTemp: NN C` readings.
