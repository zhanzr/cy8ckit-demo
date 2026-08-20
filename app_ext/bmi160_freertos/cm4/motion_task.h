/******************************************************************************
* File Name:   motion_task.h
*
* Description: This file is the public interface of motion_task.c. This file
*              also contains the BMI160/270 motion sensor configuration parameters.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2021-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#ifndef MOTION_TASK_H_
#define MOTION_TASK_H_

#include "FreeRTOS.h"
#include "task.h"
#include "cybsp.h"

/*******************************************************************************
* Macros
********************************************************************************/
#define CY8CKIT_028_EPD                 (0u)
#define CY8CKIT_028_TFT                 (1u)
#define SHIELD_XENSIV_A                 (2u)
#define CY8CKIT_062S2_AI                (3u)
#define CUSTOM_INTERFACE                (4u)

#define SENSOR_TYPE_BMI160              (0)
#define SENSOR_TYPE_BMI270              (1)

/*******************************************************************************
* =================== MOTION SENSOR INTERFACE CONFIGURATION ====================
********************************************************************************/
/* Specify the interface being used with the CE.
 * Valid choices: CY8CKIT_028_EPD, CY8CKIT_028_TFT, SHIELD_XENSIV_A,
 * CY8CKIT_062S2_AI, CUSTOM_INTERFACE
 *  - If you are using a kit that is not a Pioneer kit (Pioneer kits have
 *    Arduino compatible headers), choose CUSTOM_INTERFACE for this macro.
 *  - For CUSTOM_INTERFACE setting, specify the Interrupt pin being used under
 *    the IMU_INTERRUPT_PIN macro.
 */
#define INTERFACE_USED                  (CY8CKIT_028_EPD)

/* BMI160/270 motion sensor has two interrupt channels (INT1 and INT2).
 * Specify the interrupt channel being used in this example.
 * Valid choices: 1, 2
 */
#define IMU_INTERRUPT_CHANNEL           (1)

#if (INTERFACE_USED == CY8CKIT_028_EPD || INTERFACE_USED == CY8CKIT_028_TFT)
    #define SENSOR_TYPE                 (SENSOR_TYPE_BMI160)
#elif (INTERFACE_USED == SHIELD_XENSIV_A || INTERFACE_USED == CY8CKIT_062S2_AI)
    #define SENSOR_TYPE                 (SENSOR_TYPE_BMI270)
#elif (INTERFACE_USED == CUSTOM_INTERFACE)
    /* Specify the PSoC 6 GPIO pin that interfaces with the BMI160/BMI270 motion
     * sensor's interrupt pin (INT1 when BMI160_INTERRUPT_CHANNEL = 1, and
     * INT2 when BMI160_INTERRUPT_CHANNEL = 2).
     */
    #define CUSTOM_INTERRUPT_PIN        (P10_0)

    /* Specify the type of sensor interfaced */
    #define SENSOR_TYPE                 (SENSOR_TYPE_BMI270)
#endif

/*******************************************************************************
* ====================== OTHER CONFIGURATION MACROS ============================
********************************************************************************/
/* Interrupt pin initial value and interrupt priority */
#define IMU_INTERRUPT_PIN_INITVAL    (0u)
#define IMU_INTERRUPT_PRIORITY       (5u)

/* Task priority and stack size for the Motion sensor task */
#define TASK_MOTION_SENSOR_PRIORITY     (configMAX_PRIORITIES - 1)
#define TASK_MOTION_SENSOR_STACK_SIZE   (512u)

/* I2C Clock frequency in Hz */
#define I2C_CLK_FREQ_HZ                 (1000000u)

/*******************************************************************************
* Function Prototypes
********************************************************************************/
cy_rslt_t create_motion_sensor_task(void);

#endif /* MOTION_TASK_H_ */

/* [] END OF FILE */
