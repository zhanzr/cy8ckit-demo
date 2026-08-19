/***********************************************************************************************//**
 * \file cy8ckit_028_epd.h
 *
 * Description: This file is the top level interface for the CY8CKIT-028-EPD
 *              shield board.
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

#include "cy8ckit_028_epd_pins.h"
#include "cy_result.h"
#include "cybsp.h"
#include "cyhal_i2c.h"
#include "cyhal_pdmpcm.h"
#include "mtb_bmi160.h"
#include "mtb_thermistor_ntc_gpio.h"
#include "mtb_e2271cs021.h"

/**
 * \defgroup group_board_libs_shield Shield
 * \defgroup group_board_libs_pins Pins
 * \defgroup group_board_libs_microphone Microphone
 * \defgroup group_board_libs_motion Motion Sensor
 * \defgroup group_board_libs_temp Temperature Sensor
 * \defgroup group_board_libs_display E-Ink Display
 *
 * \addtogroup group_board_libs_motion
 * \{
 * The motion sensor is handled by the sensor-motion-bmi160 library, details are available at
 * https://github.com/cypresssemiconductorco/sensor-motion-bmi160.
 * \}
 * \addtogroup group_board_libs_temp
 * \{
 * The temperature sensor is handled by the thermistor library, details are available at
 * https://github.com/cypresssemiconductorco/thermistor.
 * \}
 * \addtogroup group_board_libs_display
 * \{
 * The display is handled by the display-eink-e2271cs021 library, details are available at
 * https://github.com/cypresssemiconductorco/display-eink-e2271cs021.
 * \}
 */

/**
 * \addtogroup group_board_libs_shield
 * \{
 * Basic set of APIs for interacting with the CY8CKIT-028-EPD shield board.
 * This provides pin definitions and initialization code for the shield.
 * Initialization of the shield configures the internal peripherals to allow
 * them to be used.
 */

#if defined(__cplusplus)
extern "C"
{
#endif

/**
 * Initialize the shield board and all peripherals contained on it.
 * @param[in] i2c_inst  An optional I2C instance to use for communicating with the motion
 * sensor on the shield. If NULL, a new instance will be allocated using the CYBSP_I2C_SCL &
 * CYBSP_I2C_SDA pins.
 * @param[in] spi_inst  An optional SPI instance to use for communicating with the display
 * on the shield. If NULL, a new instance will be allocated using the CYBSP_D11 (mosi),
 * CYBSP_D12 (miso), CYBSP_D13 (sclk) & CYBSP_D14 (cs) pins.
 * @param[in] adc_inst  An optional ADC instance used for reading the
 * thermistor. If NULL, a new instance will be allocated.
 * @param[in] pdm_pcm_cfg The configuration for the PDM object used with the microphone.
 * If NULL, the PDM object will not be initialized.
 * @param[in] audio_clock_inst The audio clock used with the microphone.
 * If NULL, the PDM object will not be initialized.
 * @return CY_RSLT_SUCCESS if properly initialized, else an error indicating what went wrong.
 */
cy_rslt_t cy8ckit_028_epd_init(cyhal_i2c_t* i2c_inst, cyhal_spi_t* spi_inst, cyhal_adc_t* adc_inst,
                               const cyhal_pdm_pcm_cfg_t* pdm_pcm_cfg,
                               cyhal_clock_t* audio_clock_inst);
// For more information about the pdm_pcm_cfg and audio_clock_inst parameters,
// see ../common/microphone_spk0838ht4hb.h

/**
 * Gives the user access to the thermistor object
 * @return A reference to the thermistor object on this shield.
 */
mtb_thermistor_ntc_gpio_t* cy8ckit_028_epd_get_thermistor(void);

/**
 * Gives the user access to the thermistor config object
 * @return A reference to the thermistor configuration.
 */
mtb_thermistor_ntc_gpio_cfg_t* cy8ckit_028_epd_get_thermistor_cfg(void);

/**
 * Gives the user access to the motion sensor object
 * @return A reference to the motion sensor object on this shield.
 */
mtb_bmi160_t* cy8ckit_028_epd_get_motion_sensor(void);

/**
 * Gives the user access to a buffer that is allocated by the shield library
 * and contains the previous eink frame. The size of this buffer is defined
 * by the display driver's PV_EINK_IMAGE_SIZE variable.
 * @return A reference to the previous display frame.
 */
uint8_t* cy8ckit_028_epd_get_previous_frame(void);

/**
 * Gives the user access to a buffer that is allocated by the shield library
 * and contains the current eink frame. Note: If EMWIN is enabled the
 * allocated memory is instead allocated and owned by the EMWIN library.
 * Either way this function will return a valid pointer to that memory.
 * The size of this buffer is defined by the display driver's
 * PV_EINK_IMAGE_SIZE variable.
 * @return A reference to the current display frame.
 */
uint8_t* cy8ckit_028_epd_get_current_frame(void);

/**
 * Gives the user access to the PDM object used with the microphone.
 * This will be null if the arguments to setup the PDM interface were not provided.
 * @return A reference to the PDM microphone object on this shield.
 */
cyhal_pdm_pcm_t* cy8ckit_028_epd_get_pdm(void);

/**
 * Frees up any resources allocated as part of \ref cy8ckit_028_epd_init().
 */
void cy8ckit_028_epd_free(void);

#if defined(__cplusplus)
}
#endif

/** \} group_board_libs */
