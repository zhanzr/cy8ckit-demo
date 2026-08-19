/***********************************************************************************************//**
 * \file mtb_thermistor_ntc_gpio.h
 *
 * Description: This file is the public interface of mtb_thermistor_ntc_gpio.c source
 *              file.
 *
 ***************************************************************************************************
 * \copyright
 * Copyright 2018-2021 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
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

/**
 * \addtogroup group_board_libs NTC Thermistor
 * \{
 * APIs for interacting with an NTC thermistor e.g the NCP18XH103F03RB resistor
 * on the CY8CPROTO-062-4343W board or the CY8CKIT-028-EPD shield
 * Note: This lib uses floating point calculations during temperature
 * calculation so it is recommended to enable floating point support
 */

#include "cy_result.h"
#include "cyhal_gpio.h"
#include "cyhal_adc.h"

#if defined(__cplusplus)
extern "C"
{
#endif

/** Defines the way the NTC thermistor is wired up: Vin-NTC-R-GND or Vin-R-NTC-GND */
typedef enum
{
    /** The thermistor is connected to Ground with the resister between it and Vin. */
    MTB_THERMISTOR_NTC_WIRING_VIN_R_NTC_GND,
    /** The thermistor is connected to Vin with the resister between it and ground. */
    MTB_THERMISTOR_NTC_WIRING_VIN_NTC_R_GND
} mtb_thermistor_ntc_wiring;

/** Configuration structure containing ntc thermistor constants */
typedef struct
{
    float r_ref;      /**< Resistance of the reference resistor */
    float b_const;    /**< Beta constant of the thermistor */
    float r_infinity; /**< Projected resistance of the thermistor at infinity */
} mtb_thermistor_ntc_gpio_cfg_t;

/** Structure defining the pins and adc channel used to interact with the
 * ntc thermistor. */
typedef struct
{
    cyhal_adc_channel_t            channel; /**< Adc channel obj */
    cyhal_gpio_t                   gnd;     /**< Ground reference pin */
    cyhal_gpio_t                   vdd;     /**< VDD reference pin */
    cyhal_gpio_t                   out;     /**< Voltage output */
    mtb_thermistor_ntc_gpio_cfg_t* cfg;     /**< Ptr to thermistor cfg structure */
    mtb_thermistor_ntc_wiring      wiring;  /**< How the thermistor is wired up */
} mtb_thermistor_ntc_gpio_t;

/**
 * Initialize the ADC Channel and Pins to communicate with the thermistor.
 * @param[in,out] obj Pointer to a thermistor object containing the set of pins
 * and adc channel that are associated with the thermistor. Note: The caller
 * must allocate the memory for this object but the init function will
 * initialize its contents.
 * @param[out] obj    Pointer to a Thermistor object. The caller must allocate the memory
 *  for this object but the init function will initialize its contents.
 * @param[in] adc     Pointer to an already initialized adc object
 * @param[in] gnd     Ground reference pin
 * @param[in] vdd     VDD reference pin
 * @param[in] out     Voltage output pin
 * @param[in] cfg     Pointer to the cfg object containing the thermistor constants
 * @param[in] wiring  How the circuit is wired up
 * @return CY_RSLT_SUCCESS if properly initialized, else an error indicating what went wrong.
 */
cy_rslt_t mtb_thermistor_ntc_gpio_init(mtb_thermistor_ntc_gpio_t* obj, cyhal_adc_t* adc,
                                       cyhal_gpio_t gnd, cyhal_gpio_t vdd, cyhal_gpio_t out,
                                       mtb_thermistor_ntc_gpio_cfg_t* cfg,
                                       mtb_thermistor_ntc_wiring wiring);


/**
 * Gets the temperature reading, in degrees C, from the thermistor.
 * @param[in] obj Pointer to a thermistor object containing the set of pins
 * and adc channel that are associated with the thermistor.
 * @return The temperature reading, in degrees C, from the hardware.
 */
float mtb_thermistor_ntc_gpio_get_temp(mtb_thermistor_ntc_gpio_t* obj);

/**
 * Frees up the ADC channel and Pins allocated for the thermistor.
 * @param[in] obj  The set of pins and adc channel that are associated with the thermistor
 */
void mtb_thermistor_ntc_gpio_free(mtb_thermistor_ntc_gpio_t* obj);

#if defined(__cplusplus)
}
#endif

/** \} group_board_libs */
