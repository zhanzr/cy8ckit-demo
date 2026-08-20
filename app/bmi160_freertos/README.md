# app/bmi160_freertos - BMI160 motion-sensor orientation demo (FreeRTOS)

A port of the Infineon
[`mtb-example-psoc6-motion-sensor-freertos`](https://github.com/Infineon/mtb-example-psoc6-motion-sensor-freertos)
to run on the **CY8CKIT-062-BLE + CY8CKIT-028-EPD** (retargeted from
`CY8CPROTO-062S3-4343W`). Only change from the official example:
`motion_task.h` `INTERFACE_USED = CY8CKIT_028_EPD` (selects the BMI160 on the
EPD shield over the CYBSP I2C bus, interrupt on `CYBSP_D9` = P13_1).

It uses:
- **FreeRTOS** (`app/mtb_shared/freertos/latest-v10.X`, `COMPONENTS=FREERTOS
  RTOS_AWARE`) + the RTOS abstraction (`abstraction-rtos`), `clib-support`,
  `retarget-io`.
- The **BMI160 middleware** (`sensor-motion-bmi160/release-v1.1.2`, already
  vendored) + the Bosch **BMI160_driver** (`bmi160_v3.9.1`, already vendored).

Behavior: a FreeRTOS task reads the BMI160 over I2C (CYBSP_I2C, P6_1/P6_0) and
prints the board orientation (TOP/BOTTOM/LEFT/RIGHT_EDGE, DISP_UP/DOWN) on
orientation-change interrupts.

## Build (offline)

Same offline ModusToolbox flow as `app/eink_test` / `app/capsense_test`:

```bash
./build.sh          # (Git Bash / modus-shell)
```

Produces `build/CY8CKIT-062-BLE/Debug/bmi160_freertos.hex`.

`DEFINES=CY_USING_PREBUILT_CM0P_IMAGE` is required (as in `capsense_test`) so
`cybsp_init()` runs the device config (100 MHz FLL) on the CM4.

## Flash + boot

Merge the CM4-only hex with the cat1cm0p (0x10000000) image (see
`tools/merge_hex.ps1`), then program + boot with `tools/flash_and_boot.tcl`.
UART: CYBSP_DEBUG_UART (P5_1 TX / P5_0 RX, 115200 8N1, COM26).
