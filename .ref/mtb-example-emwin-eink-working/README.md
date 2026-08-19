# Reference: official emWin E-Ink example (WORKING on CY8CKIT-062-BLE)

This is a copy of the official Cypress/Infineon example
`mtb-example-psoc6-emwin-eink-freertos` (which lives/built at
`D:\board_database\main-cy8ckit-062\mtb-example-psoc6-emwin-eink-freertos`).

It **displays correctly on the CY8CKIT-062-BLE + CY8CKIT-028-EPD** hardware,
so it is the reference for any E-Ink work on this board.

## What works
- E2271CS021 panel driven via cyhal_spi (SCB6, P12: MOSI/P12_0 MISO/P12_1 SCLK/P12_2,
  CS/P12_3) + the display-eink-e2271cs021 driver.
- Full emWin GUI + FreeRTOS demo (Infineon logo, text, 2D graphics pages).
- Retarget-IO on CYBSP_DEBUG_UART (P5_1 TX / P5_0 RX, 115200 8N1, COM26).

## How to rebuild (offline, deps already in mtb_shared)
```
cd /d/board_database/main-cy8ckit-062/mtb-example-psoc6-emwin-eink-freertos
export CY_TOOLS_PATHS='C:/Users/user1/ModusToolbox/tools_3.8'
export CY_GETLIBS_SHARED_PATH='D:/board_database/main-cy8ckit-062'
make build SEARCH_TARGET_CY8CKIT-062-BLE=<abs path>/bsps/TARGET_APP_CY8CKIT-062-BLE \
           SEARCH_core-make=<abs path>/mtb_shared/core-make/release-v3.9.0 \
           CY_BASELIB_PATH=<abs path>/mtb_shared/recipe-make-cat1a/release-v2.8.0
```

## Key build notes (why it works)
- Deps were fetched with `make getlibs` (needs network + the three assets
  emwin / freertos / CY8CKIT-028-EPD).
- The offline build needs the build-system libs + BSP registered in
  `libs/mtb.mk` (SEARCH_mtb-hal-cat1, mtb-pdl-cat1, core-lib, cmsis,
  SEARCH_TARGET, config, config/GeneratedSource) and the COMPONENTS list.
- BSP startup/system/generated sources are copied to the app root
  (startup_psoc6_01_cm4.S, system_psoc6_cm4.c, cybsp.c, cycfg_*.c) because
  the manually-registered BSP does not auto-compile them.
- `cy_ble_eco_stub.c` (uint32_t cy_BleEcoClockFreqHz = 0) satisfies the PDL
  BLE-ECO symbol.
- Makefile `INCLUDES` adds the BSP root + config + config/GeneratedSource.

## Flash + boot (board has boot-ROM hold; CM4 stays debug-halted otherwise)
```
C:/Infineon/Tools/ModusToolboxProgtools-1.9/openocd/bin/openocd.exe \
  -c "set HEX D:/.../mtb-example-psoc6-emwin-eink-freertos.hex" \
  -c "adapter speed 1000" \
  -c "source [find interface/kitprog3.cfg]" \
  -c "source [find target/infineon/cy8c6xx.cfg]" \
  -c "init" -c "source D:/cy8ckit-prj/tools/flash_and_boot.tcl"
```
`flash_and_boot.tcl` (tools/) programs the hex, then boots the CM0+ image
from its vector table at 0x10000000 **and resumes both cores** (the CM4 must
not stay debug-halted or the cat1cm0p image cannot release it).
