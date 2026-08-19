/***********************************************************************************************//**
 * \file mtb_thermistor_ntc_gpio.c
 *
 * Description: This file contains the functions for interacting with the
 *              thermistor.
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

#include "mtb_thermistor_ntc_gpio.h"
#include <math.h>


#if defined(__cplusplus)
extern "C"
{
#endif

// Zero Kelvin in degree C
#define ABSOLUTE_ZERO            (float)(-273.15)

//--------------------------------------------------------------------------------------------------
// mtb_thermistor_ntc_gpio_init
//--------------------------------------------------------------------------------------------------
cy_rslt_t mtb_thermistor_ntc_gpio_init(mtb_thermistor_ntc_gpio_t* obj, cyhal_adc_t* adc,
                                       cyhal_gpio_t gnd, cyhal_gpio_t vdd, cyhal_gpio_t out,
                                       mtb_thermistor_ntc_gpio_cfg_t* cfg,
                                       mtb_thermistor_ntc_wiring wiring)
{
    CY_ASSERT(adc != NULL);
    CY_ASSERT(obj != NULL);
    CY_ASSERT(cfg != NULL);
    memset(obj, 0, sizeof(mtb_thermistor_ntc_gpio_t));

    obj->cfg = cfg;
    obj->wiring = wiring;

    cy_rslt_t result;

    result = cyhal_gpio_init(vdd, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0u);
    if (CY_RSLT_SUCCESS == result)
    {
        obj->vdd = vdd;
        result = cyhal_gpio_init(gnd, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0u);
    }

    if (CY_RSLT_SUCCESS == result)
    {
        obj->gnd = gnd;

        #if (CYHAL_API_VERSION >= 2)
        static const cyhal_adc_channel_config_t DEFAULT_CHAN_CONFIG =
            { .enable_averaging = false, .min_acquisition_ns = 10u, .enabled = true };
        result = cyhal_adc_channel_init_diff(&obj->channel, adc, out, CYHAL_ADC_VNEG,
                                             &DEFAULT_CHAN_CONFIG);
        #else // HAL API version 1
        result = cyhal_adc_channel_init(&obj->channel, adc, out);
        #endif
    }

    if (CY_RSLT_SUCCESS == result)
    {
        obj->out = out;
    }
    else
    {
        mtb_thermistor_ntc_gpio_free(obj);
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
// mtb_thermistor_ntc_gpio_get_temp
//--------------------------------------------------------------------------------------------------
float mtb_thermistor_ntc_gpio_get_temp(mtb_thermistor_ntc_gpio_t* obj)
{
    // Measure voltage drop on the reference resistor
    cyhal_gpio_write(obj->vdd, 0u);
    cyhal_gpio_write(obj->gnd, 1u);
    uint16_t voltage_ref = cyhal_adc_read_u16(&obj->channel);

    // Measure voltage drop on the thermistor
    cyhal_gpio_write(obj->vdd, 1u);
    cyhal_gpio_write(obj->gnd, 0u);
    uint16_t voltage_therm = cyhal_adc_read_u16(&obj->channel);

    // Disable power to thermistor circuit
    cyhal_gpio_write(obj->vdd, 0u);

    // Calculate thermistor resistance
    float rThermistor = (obj->wiring == MTB_THERMISTOR_NTC_WIRING_VIN_R_NTC_GND)
        ? ((obj->cfg->r_ref * (voltage_therm)) / ((float)(voltage_ref)))
        : ((obj->cfg->r_ref * (voltage_ref)) / ((float)(voltage_therm)));

    // Calculate thermistor temperature
    float temperature = (obj->cfg->b_const / (logf(rThermistor / obj->cfg->r_infinity))) +
                        ABSOLUTE_ZERO;

    // Return the temperature value
    return temperature;
}


//--------------------------------------------------------------------------------------------------
// mtb_thermistor_ntc_gpio_free
//--------------------------------------------------------------------------------------------------
void mtb_thermistor_ntc_gpio_free(mtb_thermistor_ntc_gpio_t* obj)
{
    if (obj->vdd != NC)
    {
        cyhal_gpio_free(obj->vdd);
    }
    if (obj->gnd != NC)
    {
        cyhal_gpio_free(obj->gnd);
    }
    if (obj->out != NC)
    {
        cyhal_adc_channel_free(&obj->channel);
        cyhal_gpio_free(obj->out);
    }
}


#if defined(__cplusplus)
}
#endif
