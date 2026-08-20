# app/capsense_test - CAPSENSE buttons + slider demo (CY8CKIT-062-BLE)

A port of the Infineon
[`mtb-example-psoc6-capsense-buttons-slider`](https://github.com/Infineon/mtb-example-psoc6-capsense-buttons-slider)
to run on the **CY8CKIT-062-BLE** (retargeted from `CY8CPROTO-062-4343W`).

It uses:
- **cybsp + cyhal** + the vendored **capsense** middleware **v10.0.0**
  (`app/mtb_shared/capsense/release-v10.0.0`, the prebuilt
  `COMPONENT_SOFTFP/TOOLCHAIN_GCC_ARM/libcy_capsense.a` provides the `_Lib`
  DSP routines) and **retarget-io**.
- The BSP's generated CAPSENSE config (`config/GeneratedSource/cycfg_capsense.*`
  in `bsps/TARGET_APP_CY8CKIT-062-BLE`, Configurator 11.0.0) - the
  CY8CKIT-062-BLE design has **2 CSX buttons + a 5-segment CSD slider**.

Behavior: **Button0** (P8_1) turns the user LED (**P1_5**, `CYBSP_USER_LED`) on,
**Button1** (P8_2) turns it off, and the **slider** (P8_3..P8_7) controls the
brightness. A CAPSENSE Tuner EzI2C bridge (SCB3, P6_1/P6_0, addr 8) is set up
for the Tuner GUI (optional - a failed EzI2C init is non-fatal).

## Build (offline)

Same offline ModusToolbox flow as `app/eink_test` (see its README):

```bash
./build.sh          # (Git Bash / modus-shell)
```

Produces `build/CY8CKIT-062-BLE/Debug/capsense_test.hex`.

**Important:** the Makefile defines `CY_USING_PREBUILT_CM0P_IMAGE`. Without it
`cybsp_init()` skips `cycfg_config_init()` (the CM0+ runs the cat1cm0p prebuilt
image, so the CM4 must run the device config itself), the 100 MHz FLL is never
configured and the CAPSENSE scan stalls.

## Flash + boot

The board has boot-ROM hold; flash the CM0+ cat1cm0p image + the CM4 app. The
CM4-only hex must be merged with the cat1cm0p (0x10000000) image, e.g. from the
`app/eink-freertos-emwin` hex, into a combined hex (see `tools/merge_hex.ps1`),
then:

```
C:/Infineon/Tools/ModusToolboxProgtools-1.9/openocd/bin/openocd.exe \
  -c "set HEX D:/cy8ckit-prj/app/capsense_test/build/.../capsense_test_combined.hex" \
  -c "adapter speed 1000" \
  -c "source [find interface/kitprog3.cfg]" \
  -c "source [find target/infineon/cy8c6xx.cfg]" \
  -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```

UART: CYBSP_DEBUG_UART (P5_1 TX / P5_0 RX, 115200 8N1, COM26).

CAPSENSE pins (CY8CKIT-062-BLE): Cmod P7_7, CintA P7_1, CintB P7_2,
Button0 RX0 P8_1, Button1 RX0 P8_2, TX P1_0 (shared), slider Sns0..4 P8_3..P8_7.
