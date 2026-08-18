# xip_test — PSoC 6 QSPI XIP test (external NOR + XIP mode)

MTB (ModusToolbox) application that proves two things on the CY8CKIT-062-BLE:

1. **The external-NOR flashing path** — writes a string into the on-board
   S25FL512S (device address 0) through the HAL SMIF driver.
2. **XIP mode** — puts the SMIF into XIP mode and reads the string back via
   the memory-mapped address `0x18000000` (the string is fetched by the CPU
   directly from the external NOR), verifying the two match.

## Expected output

```
*************** PSoC 6: XIP test (external NOR) ***************

1. SMIF initialized, total flash: 67108864 bytes
2. Programming string to external NOR (device addr 0x00000000)...
   Erased + wrote 38 bytes.
3. Entering XIP mode...
4. Reading back via XIP address 0x18000000...
   XIP read: "Hello from the external NOR via XIP!

"
================================================================================
SUCCESS: XIP read matches the programmed string! (external NOR + XIP work)
================================================================================
```

## Build

ModusToolbox 3.8 app (same as `app/nor_benchmark_hal`):

```
cd app/xip_test
make getlibs MTB_SHARED_DIR=<dir-with-fetched-libs>
make build TOOLCHAIN=GCC_ARM CONFIG=Debug -j4 MTB_SHARED_DIR=<dir-with-fetched-libs>
```

## Flash / run

Program + boot (see `D:\cy8ckit-prj\BOOT_ISSUE.md` for why manual boot is needed):

```
C:\Infineon\Tools\ModusToolboxProgtools-1.9\openocd\bin\openocd.exe -c "set HEX D:/cy8ckit-prj/app/xip_test/build/APP_CY8CKIT-062-BLE/Debug/mtb-example-psoc6-qspi-xip.hex" -c "adapter speed 1000" -c "source [find interface/kitprog3.cfg]" -c "source [find target/infineon/cy8c6xx.cfg]" -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```

## Relation to the "CM4 app in external NOR" boot test

This validates the two capabilities the `cm4_external_app` test needs:
external-NOR storage and XIP access. The full "CM0p boots a separate CM4 app
from `0x18000000`" flow is documented in `app/cm4_external_app`; the CM4's
autonomous boot from the CPUSS registers is still affected by the board's
boot-ROM hold (the same class of issue as the CM0+ auto-boot), so the
external-NOR programming + XIP access is demonstrated here with the HAL.
