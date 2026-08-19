/***********************************************************************************************//**
 * \file cy8ckit_028_epd.c
 *
 * \brief
 *    Implementation file of the shield support library.
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

#include "cy8ckit_028_epd.h"
#ifdef EMWIN_ENABLED
#include "GUI.h"
#include "LCDConf.h"
#endif

static cyhal_i2c_t i2c;
static cyhal_spi_t spi;
static cyhal_adc_t adc;

static mtb_bmi160_t    motion_sensor;
static cyhal_pdm_pcm_t cy8ckit_028_epd_pdm_pcm;

static uint8_t previous_frame[PV_EINK_IMAGE_SIZE] = { 0 };
// If emwin is used then this will just point to the emwin managed frame buffer, otherwise the
// current frame should be allocated here.
#ifdef EMWIN_ENABLED
static uint8_t* current_frame = NULL;
#else
static uint8_t current_frame[PV_EINK_IMAGE_SIZE] = { 0 };
#endif

static mtb_thermistor_ntc_gpio_t     thermistor;
static mtb_thermistor_ntc_gpio_cfg_t thermistor_cfg =
{
    .r_ref      = CY8CKIT_028_EPD_THERM_R_REF,
    .b_const    = CY8CKIT_028_EPD_THERM_B_CONST,
    .r_infinity = CY8CKIT_028_EPD_THERM_R_INFINITY,
};

#define INITIALIZED_DISPLAY     (0x01)
#define INITIALIZED_MOTION      (0x02)
#define INITIALIZED_THERMISTOR  (0x04)
#define INITIALIZED_MIC         (0x08)

static cyhal_i2c_t* i2c_ptr;
static cyhal_spi_t* spi_ptr;
static cyhal_adc_t* adc_ptr;
static uint8_t      initialized = 0;

const mtb_e2271cs021_pins_t EINK_PINS =
{
    .spi_mosi  = CY8CKIT_028_EPD_PIN_DISPLAY_SPI_MOSI,
    .spi_miso  = CY8CKIT_028_EPD_PIN_DISPLAY_SPI_MISO,
    .spi_sclk  = CY8CKIT_028_EPD_PIN_DISPLAY_SPI_SCLK,
    .spi_cs    = CY8CKIT_028_EPD_PIN_DISPLAY_CS,
    .reset     = CY8CKIT_028_EPD_PIN_DISPLAY_RST,
    .busy      = CY8CKIT_028_EPD_PIN_DISPLAY_BUSY,
    .discharge = CY8CKIT_028_EPD_PIN_DISPLAY_DISCHARGE,
    .enable    = CY8CKIT_028_EPD_PIN_DISPLAY_EN,
    .border    = CY8CKIT_028_EPD_PIN_DISPLAY_BORDER,
    .io_enable = CY8CKIT_028_EPD_PIN_DISPLAY_IOEN,
};

//--------------------------------------------------------------------------------------------------
// cy8ckit_028_epd_init
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy8ckit_028_epd_init(cyhal_i2c_t* i2c_inst, cyhal_spi_t* spi_inst, cyhal_adc_t* adc_inst,
                               const cyhal_pdm_pcm_cfg_t* pdm_pcm_cfg,
                               cyhal_clock_t* audio_clock_inst)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if (NULL == i2c_inst)
    {
        static const cyhal_i2c_cfg_t I2C_CFG =
        {
            .is_slave        = false,
            .address         = 0,
            .frequencyhal_hz = 400000
        };
        result = cyhal_i2c_init(&i2c, CY8CKIT_028_EPD_PIN_IMU_I2C_SDA,
                                CY8CKIT_028_EPD_PIN_IMU_I2C_SCL, NULL);
        if (CY_RSLT_SUCCESS == result)
        {
            i2c_ptr = &i2c;
            result  = cyhal_i2c_configure(&i2c, &I2C_CFG);
        }
    }
    else
    {
        i2c_ptr = i2c_inst;
    }

    if ((CY_RSLT_SUCCESS == result) && (NULL == spi_inst))
    {
        result = cyhal_spi_init(&spi, CY8CKIT_028_EPD_PIN_DISPLAY_SPI_MOSI,
                                CY8CKIT_028_EPD_PIN_DISPLAY_SPI_MISO,
                                CY8CKIT_028_EPD_PIN_DISPLAY_SPI_SCLK, NC, NULL, 8,
                                CYHAL_SPI_MODE_00_MSB, false);
        if (CY_RSLT_SUCCESS == result)
        {
            result  = cyhal_spi_set_frequency(&spi, 20000000);
            spi_ptr = &spi;
        }
    }
    else
    {
        spi_ptr = spi_inst;
    }

    if ((CY_RSLT_SUCCESS == result) && (NULL == adc_inst))
    {
        result = cyhal_adc_init(&adc, CY8CKIT_028_EPD_PIN_THERM_OUT1, NULL);
        if (CY_RSLT_SUCCESS == result)
        {
            adc_ptr = &adc;
        }
    }
    else
    {
        adc_ptr = adc_inst;
    }

    if (CY_RSLT_SUCCESS == result)
    {
        result = mtb_bmi160_init_i2c(&motion_sensor, i2c_ptr, MTB_BMI160_DEFAULT_ADDRESS);
    }
    if (CY_RSLT_SUCCESS == result)
    {
        initialized |= INITIALIZED_MOTION;
        result = mtb_thermistor_ntc_gpio_init(&thermistor, adc_ptr, CY8CKIT_028_EPD_PIN_THERM_GND,
                                              CY8CKIT_028_EPD_PIN_THERM_VDD,
                                              CY8CKIT_028_EPD_PIN_THERM_OUT1,
                                              &thermistor_cfg,
                                              MTB_THERMISTOR_NTC_WIRING_VIN_R_NTC_GND);
    }
    if (CY_RSLT_SUCCESS == result)
    {
        initialized |= INITIALIZED_THERMISTOR;
        #ifdef EMWIN_ENABLED
        current_frame = (uint8_t*)LCD_GetDisplayBuffer();
        #endif
        result = mtb_e2271cs021_init(&EINK_PINS, spi_ptr);
    }

    // Initialize the PDM/PCM block
    if (CY_RSLT_SUCCESS == result)
    {
        initialized |= INITIALIZED_DISPLAY;

        if ((NULL != audio_clock_inst) && (NULL != pdm_pcm_cfg))
        {
            result = cyhal_pdm_pcm_init(&cy8ckit_028_epd_pdm_pcm, CY8CKIT_028_EPD_PIN_PDM_DATA,
                                        CY8CKIT_028_EPD_PIN_PDM_CLK, audio_clock_inst, pdm_pcm_cfg);

            if (CY_RSLT_SUCCESS == result)
            {
                initialized |= INITIALIZED_MIC;
            }
        }
    }

    if (CY_RSLT_SUCCESS != result)
    {
        cy8ckit_028_epd_free();
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
// cy8ckit_028_epd_get_thermistor
//--------------------------------------------------------------------------------------------------
mtb_thermistor_ntc_gpio_t* cy8ckit_028_epd_get_thermistor(void)
{
    return &thermistor;
}


//--------------------------------------------------------------------------------------------------
// cy8ckit_028_epd_get_thermistor_cfg
//--------------------------------------------------------------------------------------------------
mtb_thermistor_ntc_gpio_cfg_t* cy8ckit_028_epd_get_thermistor_cfg(void)
{
    return &thermistor_cfg;
}


//--------------------------------------------------------------------------------------------------
// cy8ckit_028_epd_get_motion_sensor
//--------------------------------------------------------------------------------------------------
mtb_bmi160_t* cy8ckit_028_epd_get_motion_sensor(void)
{
    return &motion_sensor;
}


//--------------------------------------------------------------------------------------------------
// cy8ckit_028_epd_get_previous_frame
//--------------------------------------------------------------------------------------------------
uint8_t* cy8ckit_028_epd_get_previous_frame(void)
{
    return previous_frame;
}


//--------------------------------------------------------------------------------------------------
// cy8ckit_028_epd_get_current_frame
//--------------------------------------------------------------------------------------------------
uint8_t* cy8ckit_028_epd_get_current_frame(void)
{
    return current_frame;
}


//--------------------------------------------------------------------------------------------------
// cy8ckit_028_epd_get_pdm
//--------------------------------------------------------------------------------------------------
cyhal_pdm_pcm_t* cy8ckit_028_epd_get_pdm(void)
{
    return (initialized & INITIALIZED_MIC)
        ? &cy8ckit_028_epd_pdm_pcm
        : NULL;
}


//--------------------------------------------------------------------------------------------------
// cy8ckit_028_epd_free
//--------------------------------------------------------------------------------------------------
void cy8ckit_028_epd_free(void)
{
    if (initialized & INITIALIZED_DISPLAY)
    {
        mtb_e2271cs021_free();
    }
    if (initialized & INITIALIZED_MOTION)
    {
        mtb_bmi160_free(&motion_sensor);
    }
    if (initialized & INITIALIZED_THERMISTOR)
    {
        mtb_thermistor_ntc_gpio_free(&thermistor);
    }
    if (initialized & INITIALIZED_MIC)
    {
        cyhal_pdm_pcm_free(&cy8ckit_028_epd_pdm_pcm);
    }
    initialized = 0;

    if (i2c_ptr == &i2c)
    {
        cyhal_i2c_free(i2c_ptr);
    }
    i2c_ptr = NULL;

    if (spi_ptr == &spi)
    {
        cyhal_spi_free(spi_ptr);
    }
    spi_ptr = NULL;

    // This must be done last, in case other code prior to this frees an ADC channel
    if (adc_ptr == &adc)
    {
        cyhal_adc_free(adc_ptr);
    }
    adc_ptr = NULL;
}
