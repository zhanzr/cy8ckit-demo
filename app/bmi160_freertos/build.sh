#!/usr/bin/env bash
# app/bmi160_freertos - BMI160 motion-sensor orientation demo (FreeRTOS) on
# CY8CKIT-062-BLE + CY8CKIT-028-EPD.
# Port of Infineon mtb-example-psoc6-motion-sensor-freertos (retargeted from
# CY8CPROTO-062S3-4343W; INTERFACE_USED = CY8CKIT_028_EPD -> BMI160 via I2C).
#
# Offline ModusToolbox build (deps already fetched into the workspace
# mtb_shared). Produces build/CY8CKIT-062-BLE/Debug/bmi160_freertos.hex.
#
# Usage (from Git Bash / modus-shell):
#   ./build.sh
set -e
cd "$(dirname "$0")"

export CY_TOOLS_PATHS='C:/Users/user1/ModusToolbox/tools_3.8'
export CY_GETLIBS_SHARED_PATH='D:/cy8ckit-prj/app'
BSP="D:/cy8ckit-prj/app/bmi160_freertos/bsps/TARGET_APP_CY8CKIT-062-BLE"
CORE_MAKE="D:/cy8ckit-prj/app/mtb_shared/core-make/release-v3.9.0"
RECIPE="D:/cy8ckit-prj/app/mtb_shared/recipe-make-cat1a/release-v2.8.0"

"$CY_TOOLS_PATHS/modus-shell/bin/bash.exe" -l -c "
  cd /cygdrive/d/cy8ckit-prj/app/bmi160_freertos &&
  make build \
    SEARCH_TARGET_CY8CKIT-062-BLE='$BSP' \
    SEARCH_core-make='$CORE_MAKE' \
    CY_BASELIB_PATH='$RECIPE'
"
echo "=== built build/CY8CKIT-062-BLE/Debug/bmi160_freertos.hex ==="
