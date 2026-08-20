################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Top-level application make file.
#
################################################################################
# \copyright
# Copyright 2018-2023, Cypress Semiconductor Corporation (an Infineon company)
# SPDX-License-Identifier: Apache-2.0
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
################################################################################


################################################################################
# Basic Configuration
################################################################################
MTB_TYPE=COMBINED

# Target board/hardware (BSP).
TARGET=CY8CKIT-062-BLE

# Name of application (used to derive name of final linked file).
APPNAME=capsense_test

# Name of toolchain to use.
TOOLCHAIN=GCC_ARM

# Default build configuration.
CONFIG=Debug

# If set to "true" or "1", display full command-lines when building.
VERBOSE=


################################################################################
# Advanced Configuration
################################################################################
COMPONENTS=

# Like COMPONENTS, but disable optional code that was enabled by default.
DISABLE_COMPONENTS=

# Sources: hand-wired HAL/PDL driver set (the offline build does not run a full
# `make getlibs`, so mtbsearch would not discover these). The middleware
# sources (capsense prebuilt lib, retarget-io) are auto-discovered via
# SEARCH_MTB_MK in libs/mtb.mk.
SOURCES=$(wildcard ../mtb_shared/mtb-hal-cat1/release-v2.7.4/source/*.c) \
        ../mtb_shared/mtb-hal-cat1/release-v2.7.4/COMPONENT_CAT1A/source/pin_packages/cyhal_psoc6_01_116_bga_ble.c \
        ../mtb_shared/mtb-hal-cat1/release-v2.7.4/COMPONENT_CAT1A/source/triggers/cyhal_triggers_psoc6_01.c \
        $(wildcard ../mtb_shared/mtb-pdl-cat1/release-v3.23.0/drivers/source/*.c) \
        ../mtb_shared/mtb-pdl-cat1/release-v3.23.0/drivers/source/TOOLCHAIN_GCC_ARM/cy_syslib_ext.S \
        ../mtb_shared/mtb-pdl-cat1/release-v3.23.0/devices/COMPONENT_CAT1A/source/cy_device.c

# Like SOURCES, but for include directories.
INCLUDES=../../board/TARGET_CY8CKIT-062-BLE-release-v4.2.0 \
         ../../board/TARGET_CY8CKIT-062-BLE-release-v4.2.0/config \
         ../../board/TARGET_CY8CKIT-062-BLE-release-v4.2.0/config/GeneratedSource \
         ../mtb_shared/mtb-hal-cat1/release-v2.7.4/include \
         ../mtb_shared/mtb-hal-cat1/release-v2.7.4/include_pvt \
         ../mtb_shared/mtb-hal-cat1/release-v2.7.4/COMPONENT_CAT1A/include \
         ../mtb_shared/mtb-pdl-cat1/release-v3.23.0/include \
         ../mtb_shared/mtb-pdl-cat1/release-v3.23.0/drivers/include \
         ../mtb_shared/mtb-pdl-cat1/release-v3.23.0/devices/COMPONENT_CAT1A/include \
         ../mtb_shared/mtb-pdl-cat1/release-v3.23.0/devices/COMPONENT_CAT1A/include/ip \
         ../mtb_shared/core-lib/release-v1.8.0/include \
         ../mtb_shared/cmsis/release-v6.1.0/Core/Include \
         ../mtb_shared/cat1cm0p/release-v1.11.0/COMPONENT_CAT1A \
         ../mtb_shared/capsense/release-v10.0.0 \
         ../mtb_shared/retarget-io/latest-v1.X/include \
         ../mtb_shared/retarget-io/latest-v1.X

# Add additional defines to the build process (without a leading -D).
# CY_USING_PREBUILT_CM0P_IMAGE makes cybsp_init() run cycfg_config_init()
# (100 MHz FLL + pins/peripherals) on the CM4 - the CM0+ runs the cat1cm0p
# prebuilt image.
DEFINES=CY_USING_PREBUILT_CM0P_IMAGE

# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT=

CFLAGS=
CXXFLAGS=
ASFLAGS=
LDFLAGS=
LDLIBS=
LINKER_SCRIPT=
PREBUILD=
POSTBUILD=


################################################################################
# Paths
################################################################################
CY_APP_PATH=

# Relative path to the shared repo location.
CY_GETLIBS_SHARED_PATH=../
CY_GETLIBS_SHARED_NAME=mtb_shared

CY_COMPILER_GCC_ARM_DIR=

# Locate ModusToolbox helper tools folders in default installation
# locations for Windows, Linux, and macOS.
CY_WIN_HOME=$(subst \,/,$(USERPROFILE))
CY_TOOLS_PATHS ?= $(wildcard \
    $(CY_WIN_HOME)/ModusToolbox/tools_* \
    $(HOME)/ModusToolbox/tools_* \
    /Applications/ModusToolbox/tools_*)
CY_TOOLS_PATHS+=
CY_TOOLS_DIR=$(lastword $(sort $(wildcard $(CY_TOOLS_PATHS))))
ifeq ($(CY_TOOLS_DIR),)
$(error Unable to find any of the available CY_TOOLS_PATHS -- $(CY_TOOLS_PATHS). On Windows, use forward slashes.)
endif
$(info Tools Directory: $(CY_TOOLS_DIR))

include $(CY_TOOLS_DIR)/make/start.mk
