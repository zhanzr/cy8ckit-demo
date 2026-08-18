# dhry_150m (app_ext) - Dhrystone 2.1 on the CM4 @ 150 MHz (SMIF XIP)

CM4 app linked at `0x18000000` (external NOR). After the CM0+ stub enables
XIP and releases this core, it re-clocks to **150 MHz** (PLL on CLKPATH1)
and runs Dhrystone 2.1 (1,000,000 runs).

```
bash build.sh                          # build/cm4_dhry_150m.hex + build/cm0p_xip_stub.hex
# program the CM4 app to the NOR (probe-rs), flash + boot the stub - see ../README.md
```

Verified on hardware:
`Dhrystones per Second: 251952.641` / `DMIPS/MHz: 0.956` (the lower
DMIPS/MHz vs internal flash is the SMIF-XIP code-fetch throttle).
