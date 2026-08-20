#!/usr/bin/env bash
# app/ntc_test - NTC thermistor + internal DTS temperature demo on
# CY8CKIT-062-BLE + CY8CKIT-028-EPD.
#
# Samples the NCP18XH103F03RB NTC (10k, B=3380) on the EPD shield via the SAR
# ADC (A1 P10.1 + A2 P10.2 averaged) and reports the NTC temperature plus the
# internal Die Temperature Sensor (DTS) value over the UART.
#
# Offline ModusToolbox build (deps already fetched into the workspace
# mtb_shared). Produces build/CY8CKIT-062-BLE/Debug/ntc_test.hex.
#
# Usage (from Git Bash / modus-shell):
#   ./build.sh
set -e
cd "$(dirname "$0")"

export CY_TOOLS_PATHS='C:/Users/user1/ModusToolbox/tools_3.8'
REPO="$(cygpath -m "$(cd "$(dirname "$0")/../.." && pwd)")"
export CY_GETLIBS_SHARED_PATH="$REPO/app"
BSP="$REPO/board/TARGET_CY8CKIT-062-BLE-release-v4.2.0"
CORE_MAKE="D:/cy8ckit-prj/app/mtb_shared/core-make/release-v3.9.0"
RECIPE="D:/cy8ckit-prj/app/mtb_shared/recipe-make-cat1a/release-v2.8.0"

"$CY_TOOLS_PATHS/modus-shell/bin/bash.exe" -l -c "
  cd /cygdrive/d/cy8ckit-prj/app/ntc_test &&
  make build \
    SEARCH_TARGET_CY8CKIT-062-BLE='$BSP' \
    SEARCH_core-make='$CORE_MAKE' \
    CY_BASELIB_PATH='$RECIPE'
"
echo "=== built build/CY8CKIT-062-BLE/Debug/ntc_test.hex ==="
