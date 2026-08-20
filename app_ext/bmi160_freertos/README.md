# app_ext/bmi160_freertos - BMI160 motion sensor from external NOR XIP (FreeRTOS)

Port of `app/bmi160_freertos` to the app_ext flow: the CM4 runs **entirely
from the external S25FL512S NOR via SMIF XIP** (`0x18000000`), using the shared
`app_ext/common` CM0+ XIP stub + UART, and starts the **FreeRTOS scheduler**
from XIP.

Differences from `app/bmi160_freertos`:

- **CPU re-clocked to 150 MHz** (PLL on CLKPATH1, SMIF stays on CLKPATH0) for
  reliable XIP reads.
- No `cybsp_init()`: the generated device config is applied selectively
  (`init_cycfg_clocks/routing/peripherals/pins`, **not** `init_cycfg_system`),
  the shared `app_ext/common/cm4/uart.c` provides the SCB5 UART.
- The FreeRTOS kernel + CM4 port + heap (scheme 3 / newlib malloc) +
  abstraction-rtos + BMI160 driver are compiled into the XIP image; the
  `-mfloat-abi=softfp -mfpu=fpv4-sp-d16` flags are required for the FreeRTOS
  CM4 port context switch.

`motion_task.h` `INTERFACE_USED = CY8CKIT_028_EPD` (BMI160 via I2C, INT on
P13_1) - same as the `app` version.

## Build / flash / boot

```bash
./build.sh   # -> build/cm4_bmi160_freertos.hex (0x18000000) + build/cm0p_xip_stub.hex

probe-rs download build/cm4_bmi160_freertos.elf \
    --chip-description-path ../../tools/psoc6_smif_algo/algo/target_cy8c6347_smif.yaml \
    --chip CY8C6347BZI-BLD53-S25FL512S --protocol swd --allow-erase-all

# flash the stub (internal flash) + boot:
#   C:/Infineon/Tools/ModusToolboxProgtools-1.9/openocd/bin/openocd.exe \
#     -c "set HEX D:/cy8ckit-prj/app_ext/bmi160_freertos/build/cm0p_xip_stub.hex" \
#     -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" \
#     -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" \
#     -c "source D:/cy8ckit-prj/tools/flash_stub_boot.tcl"
```

UART: P5_1/P5_0 @ 115200 8N1, COM26.
