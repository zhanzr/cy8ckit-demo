# app/eink_test - E2271CS021 E-ink demo (no FreeRTOS / no emWin)

A port of the official `mtb-example-psoc6-emwin-eink-freertos` example to the
CY8CKIT-062-BLE + CY8CKIT-028-EPD, with the **FreeRTOS and emWin dependencies
removed** and the display content reduced to a few patterns.

It uses:
- **cybsp + cyhal** (the exact SPI/clock stack the official example uses, so
  the level-translator timing / SCB6 SPI work correctly on the hardware).
- The **display-eink-e2271cs021** driver (`mtb_e2271cs021_*`).
- No RTOS, no GUI: `main()` cycles four full-screen patterns every 4 s
  (checkerboard / horizontal bars / vertical bars / box).

## EPD pin map (CY8CKIT-028-EPD shield on CY8CKIT-062-BLE)
```
MOSI P12[0] (D11)   MISO P12[1] (D12)   SCLK P12[2] (D13)
CS   P12[3] (D10)   RST  P5[2]  (D2)   BUSY P5[3]   (D3)
EN   P5[4]  (D4)    DISCHARGE P5[5] (D5) BORDER P5[6] (D6)
IOEN P0[2]  (D7)    (IOEN active-low: LOW enables the level translator)
```

## Build (offline)
Dependencies live in the workspace `mtb_shared`
(`D:/board_database/main-cy8ckit-062/mtb_shared` and the project-local
`../mtb_shared` symlinks). Requires ModusToolbox tools_3.8 at
`C:/Users/user1/ModusToolbox/tools_3.8`.

```bash
./build.sh          # (Git Bash / modus-shell)
```
Produces `build/CY8CKIT-062-BLE/Debug/eink_test.hex`.

Note: the offline build registers the build-system libs (cyhal/PDL/core-lib/
cmsis/cat1cm0p) via `libs/mtb.mk` `SEARCH_*`/`COMPONENTS` and the Makefile
`SOURCES`/`INCLUDES`, and copies the BSP startup/system/generated sources into
the app root. This is why it builds without a full `make getlibs` run.

## Flash + boot
The board has boot-ROM hold and the CM4 stays debug-halted unless the CM0+
image is booted from its vector table AND both cores are resumed:

```
C:/Infineon/Tools/ModusToolboxProgtools-1.9/openocd/bin/openocd.exe \
  -c "set HEX D:/cy8ckit-prj/app/eink_test/build/CY8CKIT-062-BLE/Debug/eink_test.hex" \
  -c "adapter speed 1000" \
  -c "source [find interface/kitprog3.cfg]" \
  -c "source [find target/infineon/cy8c6xx.cfg]" \
  -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```
UART: CYBSP_DEBUG_UART (P5_1 TX / P5_0 RX, 115200 8N1, COM26).

For the full reference example (emWin + FreeRTOS), see
`app/eink-freertos-emwin/README.md` (the migrated official example).

