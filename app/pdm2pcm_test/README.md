# app/pdm2pcm_test - PDM/PCM microphone capture demo (CY8CKIT-062-BLE + CY8CKIT-028-EPD)

A port of the Infineon
[`mtb-example-psoc6-pdm-pcm`](https://github.com/Infineon/mtb-example-psoc6-pdm-pcm)
to run on the **CY8CKIT-062-BLE** (retargeted from `CY8CPROTO-062-4343W`).
Follows the example as-is (no middleware besides `retarget-io`); only the
`TARGET` changed.

The PDM/PCM block captures audio from the **digital microphone on the
CY8CKIT-028-EPD shield** (PDM_DATA **P10_5** / PDM_CLK **P10_4**) and reports
the frame volume as a bar graph over the UART; the user LED (P1_5) lights when
the volume exceeds the noise threshold (the user button resets it).

## Build (offline)

Same offline ModusToolbox flow as the other `app/*` ports:

```bash
./build.sh          # (Git Bash / modus-shell)
```

Produces `build/CY8CKIT-062-BLE/Debug/pdm2pcm_test.hex`.

`DEFINES=CY_USING_PREBUILT_CM0P_IMAGE` is required (as in `capsense_test`) so
`cybsp_init()` runs the device config (100 MHz FLL) on the CM4.

## Flash + boot

Merge the CM4-only hex with the cat1cm0p (0x10000000) image (see
`tools/merge_hex.ps1`), then program + boot with `tools/flash_and_boot.tcl`.
UART: CYBSP_DEBUG_UART (P5_1 TX / P5_0 RX, 115200 8N1, COM26).
