#!/usr/bin/env bash
# app/eink-freertos-emwin - official emWin + FreeRTOS E-Ink example, vendored.
#
# Offline ModusToolbox build using the vendored workspace mtb_shared
# (app/mtb_shared, deps recorded in VENDORED_DEPS.md).
# Produces build/CY8CKIT-062-BLE/Debug/mtb-example-psoc6-emwin-eink-freertos.hex
#
# Usage (from Git Bash / modus-shell):
#   ./build.sh
set -e
cd "$(dirname "$0")"

export CY_TOOLS_PATHS='C:/Users/user1/ModusToolbox/tools_3.8'
export CY_GETLIBS_SHARED_PATH='D:/cy8ckit-prj/app'
BSP="D:/cy8ckit-prj/app/eink-freertos-emwin/bsps/TARGET_APP_CY8CKIT-062-BLE"
CORE_MAKE="D:/cy8ckit-prj/app/mtb_shared/core-make/release-v3.9.0"
RECIPE="D:/cy8ckit-prj/app/mtb_shared/recipe-make-cat1a/release-v2.8.0"

"$CY_TOOLS_PATHS/modus-shell/bin/bash.exe" -l -c "
  cd /cygdrive/d/cy8ckit-prj/app/eink-freertos-emwin &&
  make build \
    SEARCH_TARGET_CY8CKIT-062-BLE='$BSP' \
    SEARCH_core-make='$CORE_MAKE' \
    CY_BASELIB_PATH='$RECIPE'
"
echo "=== built build/CY8CKIT-062-BLE/Debug/mtb-example-psoc6-emwin-eink-freertos.hex ==="
