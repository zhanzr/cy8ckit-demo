################################################################################
# \file library.mk
#
# \brief
# Makefile to identify the presence of MTB_HAL
#
################################################################################
# SPDX-License-Identifier: MIT
#
# (c) 2019-2025, Infineon Technologies AG, or an affiliate of Infineon
# Technologies AG. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# https://www.infineon.com
#
################################################################################

# TrustZone flow
ifneq (,$(filter $(COMPONENTS),FREERTOS_TZ))

  # TZ Secure side
  ifneq (,$(filter $(COMPONENTS), SECURE_DEVICE))
    CY_IGNORE+= $(SEARCH_freertos)/Source 
    CY_IGNORE+= $(SEARCH_abstraction-rtos)
    CY_IGNORE+= $(SEARCH_clib-support)
  endif

else # Non TrustZone flow for all cores

  COMPONENTS+= FREERTOS_NTZ

endif


