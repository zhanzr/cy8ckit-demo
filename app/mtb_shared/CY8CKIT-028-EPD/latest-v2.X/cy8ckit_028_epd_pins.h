/***********************************************************************************************//**
 * \file cy8ckit_028_epd_pins.h
 *
 * Description: This file contains the pin definitions and NCP18XH103F03RB
 * thermistor constants for the CY8CKIT-028-EPD shield board.
 *
 * For the details of E-INK display hardware and driver interface, see the
 * documents available at the following website:
 * https://www.pervasivedisplays.com/product/2-71-e-ink-display/
 *
 ***************************************************************************************************
 * \copyright
 * Copyright 2018-2021 Cypress Semiconductor Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************************************/

#pragma once

#include "cybsp.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * \addtogroup group_board_libs_pins Pins
 * \{
 * Pin mapping of the GPIOs used by shield peripherals
 */

// E-Ink display
/** Pin for the E-Ink SPI MOSI signal */
#define CY8CKIT_028_EPD_PIN_DISPLAY_SPI_MOSI    (CYBSP_D11)
/** Pin for the E-Ink SPI MISO signal */
#define CY8CKIT_028_EPD_PIN_DISPLAY_SPI_MISO    (CYBSP_D12)
/** Pin for the E-Ink SPI SCLK signal */
#define CY8CKIT_028_EPD_PIN_DISPLAY_SPI_SCLK    (CYBSP_D13)

/** Pin for the E-Ink SPI Cable Select signal */
#define CY8CKIT_028_EPD_PIN_DISPLAY_CS          (CYBSP_D10)
/** Pin for the E-Ink Display Reset signal */
#define CY8CKIT_028_EPD_PIN_DISPLAY_RST         (CYBSP_D2)
/** Pin for the E-Ink Display Busy signal */
#define CY8CKIT_028_EPD_PIN_DISPLAY_BUSY        (CYBSP_D3)
/** Pin for the E-Ink Display Discharge signal */
#define CY8CKIT_028_EPD_PIN_DISPLAY_DISCHARGE   (CYBSP_D5)
/** Pin for the E-Ink Display Enable signal */
#define CY8CKIT_028_EPD_PIN_DISPLAY_EN          (CYBSP_D4)
/** Pin for the E-Ink Display Border signal */
#define CY8CKIT_028_EPD_PIN_DISPLAY_BORDER      (CYBSP_D6)
/** Pin for the E-Ink Display IO Enable signal */
#define CY8CKIT_028_EPD_PIN_DISPLAY_IOEN        (CYBSP_D7)

// Thermistor
/** Pin for the Thermistor VDD signal */
#define CY8CKIT_028_EPD_PIN_THERM_VDD           (CYBSP_A0)
/** Pin for the Thermistor Output option1 signal */
#define CY8CKIT_028_EPD_PIN_THERM_OUT1          (CYBSP_A1)
/** Pin for the Thermistor Output option2 signal */
#define CY8CKIT_028_EPD_PIN_THERM_OUT2          (CYBSP_A2)
/** Pin for the Thermistor Ground signal */
#define CY8CKIT_028_EPD_PIN_THERM_GND           (CYBSP_A3)

// Internal measurement unit (IMU) : accelerometer + gyroscope
/** Pin for the Accelerometer & Gyroscope I2C SCL signal */
#define CY8CKIT_028_EPD_PIN_IMU_I2C_SCL         (CYBSP_I2C_SCL)
/** Pin for the Accelerometer & Gyroscope I2C SDA signal */
#define CY8CKIT_028_EPD_PIN_IMU_I2C_SDA         (CYBSP_I2C_SDA)
/** Pin for the Accelerometer & Gyroscope Interrupt1 signal */
#define CY8CKIT_028_EPD_PIN_IMU_INT_1           (CYBSP_D9)
/** Pin for the Accelerometer & Gyroscope Interrupt2 signal */
#define CY8CKIT_028_EPD_PIN_IMU_INT_2           (CYBSP_D8)

// PDM Microphone
/** Pin for the PDM Clock */
#define CY8CKIT_028_EPD_PIN_PDM_CLK             (CYBSP_A4)
/** Pin for the PDM Data */
#define CY8CKIT_028_EPD_PIN_PDM_DATA            (CYBSP_A5)

/** \} group_board_libs_pins */

/**
 * \addtogroup group_board_libs_shield
 * \{
 */

// Thermistor Constants
/** Resistance of the reference resistor */
#define CY8CKIT_028_EPD_THERM_R_REF             (float)(10000)
/** Beta constant of the (NCP18XH103F03RB) thermistor (3380 Kelvin).See the
 * thermistor datasheet for more details. */
#define CY8CKIT_028_EPD_THERM_B_CONST           (float)(3380)
/** Resistance of the thermistor is 10K at 25 degrees C (from datasheet)
 * Therefore R0 = 10000 Ohm, and T0 = 298.15 Kelvin, which gives
 * R_INFINITY = R0 e^(-B_CONSTANT / T0) = 0.1192855 */
#define CY8CKIT_028_EPD_THERM_R_INFINITY        (float)(0.1192855)

/** \} group_board_libs_shield */

#if defined(__cplusplus)
}
#endif
