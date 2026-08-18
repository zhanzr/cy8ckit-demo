# coremark_150m (app_ext) - CoreMark 1.0 on the CM4 @ 150 MHz (SMIF XIP)

CM4 app linked at `0x18000000` (external NOR). After the CM0+ stub enables
XIP and releases this core, it re-clocks to **150 MHz** (PLL on CLKPATH1)
and runs CoreMark 1.0 (2K performance run, 6000 iterations).

```
bash build.sh                          # build/cm4_coremark_150m.hex + build/cm0p_xip_stub.hex
# program the CM4 app to the NOR (probe-rs), flash + boot the stub - see ../README.md
```

Verified on hardware:
`CoreMark 1.0 : 288.406076 / GCC 15.3.1 ... / Static` (the SMIF-XIP fetch
throttle keeps it below the internal-flash figure).
