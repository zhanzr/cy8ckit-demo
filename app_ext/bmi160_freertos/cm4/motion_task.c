/******************************************************************************
* File Name:   motion_task.c
*
* Description: This file contains the task that initializes and configures the
*              BMI160/BMI270 Motion Sensor; and displays the sensor orientation.
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

#include "motion_task.h"
#if (SENSOR_TYPE == SENSOR_TYPE_BMI270)
#include "mtb_bmi270.h"
#else
#include "mtb_bmi160.h"
#endif
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "cy_retarget_io.h"
#include "stdlib.h"


/******************************************************************************
* Macros
******************************************************************************/
/* Macro to check if the result of an operation was successful. When it has
 * failed, print the error message and suspend the task.
 */
#define CHECK_RESULT(result, error_message...)                        \
                     do                                               \
                     {                                                \
                         if ((cy_rslt_t)result != CY_RSLT_SUCCESS)    \
                         {                                            \
                             printf(error_message);                   \
                             vTaskSuspend(NULL);                      \
                         }                                            \
                     } while(0)

/* Check if the EPD and TFT shields are being used with non Pioneer kits and
 * throw a compile-time error accordingly.
 */
#if (!defined(CYBSP_D0) && (INTERFACE_USED == CY8CKIT_028_EPD || INTERFACE_USED == CY8CKIT_028_TFT))
    #error "Error: Incorrect configuration for the macro 'INTERFACE_USED' in motion_task.h. " \
           "The kits that do not have Arduino form factor do not support the EPD or TFT shields. "\
           "Use a custom interface instead."
#else
    /* Assign the PSoC 6 GPIO Interrupt pin according to the interface (shield)
     * and the interrupt channel being configured in  'motion_task.h'.
     */
    #if (INTERFACE_USED == CY8CKIT_028_EPD)
        #if (IMU_INTERRUPT_CHANNEL == 1)
            #define IMU_INTERRUPT_PIN        (CYBSP_D9)
        #elif (IMU_INTERRUPT_CHANNEL == 2)
            #define IMU_INTERRUPT_PIN        (CYBSP_D8)
        #endif
    #elif (INTERFACE_USED == CY8CKIT_028_TFT)
        #if (IMU_INTERRUPT_CHANNEL == 1)
            #define IMU_INTERRUPT_PIN        (CYBSP_A2)
        #elif (IMU_INTERRUPT_CHANNEL == 2)
            #define IMU_INTERRUPT_PIN        (CYBSP_A3)
        #endif
    #elif (INTERFACE_USED == SHIELD_XENSIV_A)
        #define IMU_INTERRUPT_PIN            (CYBSP_D7)
    #elif (INTERFACE_USED == CY8CKIT_062S2_AI)
        #if (IMU_INTERRUPT_CHANNEL == 1)
            #define IMU_INTERRUPT_PIN        (CYBSP_IMU_INT1)
        #elif (IMU_INTERRUPT_CHANNEL == 2)
            #define IMU_INTERRUPT_PIN        (CYBSP_IMU_INT2)
        #endif
    #elif (INTERFACE_USED == CUSTOM_INTERFACE)
        #define IMU_INTERRUPT_PIN            (CUSTOM_INTERRUPT_PIN)
    #else
        #error "Error: Incorrect configuration for the macro 'INTERFACE_USED' in motion_task.h"
    #endif

    /* Compile-time error checking for other macros in 'motion_task.h' */
    #if (IMU_INTERRUPT_CHANNEL != 1 && IMU_INTERRUPT_CHANNEL != 2)
        #error "Error: Incorrect configuration for the macro 'IMU_INTERRUPT_CHANNEL' in motion_task.h"
    #elif !defined(IMU_INTERRUPT_PIN)
        #error "Error: Incorrect configuration for the macro 'IMU_INTERRUPT_PIN' in motion_task.h"
    #endif
#endif

/*******************************************************************************
* Global Variables
********************************************************************************/
/* Orientation types:
 * Indicates which edge of the board is pointing towards the ceiling/sky
 */
typedef enum
{
    ORIENTATION_NULL            = 0,    /* Default orientation state used for initialization purposes */
    ORIENTATION_TOP_EDGE        = 1,    /* Top edge of the board points towards the ceiling */
    ORIENTATION_BOTTOM_EDGE     = 2,    /* Bottom edge of the board points towards the ceiling */
    ORIENTATION_LEFT_EDGE       = 3,    /* Left edge of the board (USB connector side) points towards the ceiling */
    ORIENTATION_RIGHT_EDGE      = 4,    /* Right edge of the board points towards the ceiling */
    ORIENTATION_DISP_UP         = 5,    /* Display faces up (towards the sky/ceiling) */
    ORIENTATION_DISP_DOWN       = 6     /* Display faces down (towards the ground) */
} orientation_t;

#if (SENSOR_TYPE == SENSOR_TYPE_BMI160)
/* Instance of BMI160 sensor structure */
static mtb_bmi160_t motion_sensor;
#else
/* Instance of BMI270 sensor structure */
mtb_bmi270_data_t data;
mtb_bmi270_t sensor_bmi270;
#endif

/* HAL structure for I2C */
static cyhal_i2c_t kit_i2c;

/* Motion sensor task handle */
static TaskHandle_t motion_sensor_task_handle;

/* Handle for the semaphore that locks the I2C resource while reading the
 * Motion Sensor data
 */
static SemaphoreHandle_t i2c_semaphore;

/*******************************************************************************
* Function Prototypes
********************************************************************************/
/* These static functions are used by the Motion Sensor Task. These are not
 * available outside this file. See the respective function definitions for
 * more details.
 */
static void task_motion(void* pvParameters);
static cy_rslt_t motionsensor_init(void);
static cy_rslt_t motionsensor_config_interrupt(void);
static cy_rslt_t motionsensor_update_orientation(orientation_t *orientation_result);
static void motionsensor_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event);

/*******************************************************************************
* Function Name: create_motion_sensor_task
********************************************************************************
* Summary:
*  Function that creates the motion sensor task.
*
* Parameters:
*  None
*
* Return:
*  CY_RSLT_SUCCESS upon successful creation of the motion sensor task, else a
*  non-zero value that indicates the error.
*
*******************************************************************************/
cy_rslt_t create_motion_sensor_task(void)
{
    BaseType_t status;

    status = xTaskCreate(task_motion, "Motion Sensor Task", TASK_MOTION_SENSOR_STACK_SIZE,
                         NULL, TASK_MOTION_SENSOR_PRIORITY, &motion_sensor_task_handle);

    return (status == pdPASS) ? CY_RSLT_SUCCESS : (cy_rslt_t) status;
}

/*******************************************************************************
* Function Name: task_motion
********************************************************************************
* Summary:
*  Task that configures the Motion Sensor and processes the sensor data to
*  display the sensor orientation.
*
* Parameters:
*  void *pvParameters : Task parameter defined during task creation (unused)
*
* Return:
*  None
*
*******************************************************************************/
static void task_motion(void* pvParameters)
{
    /* Status variable to indicate the result of various operations */
    cy_rslt_t result;

    /* Variable used to store the orientation information */
    orientation_t orientation;

    /* Remove warning for unused parameter */
    (void)pvParameters;

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("***************************************************************************\n");
    printf("            PSoC 6 MCU: Motion sensor interfacing using I2C                \n");
    printf("***************************************************************************\n");

    /* Create binary semaphore and suspend the task upon failure */
    i2c_semaphore = xSemaphoreCreateBinary();
    CHECK_RESULT((i2c_semaphore == NULL), " Error : Motion Sensor - Failed to create semaphore !!\n");
    xSemaphoreGive(i2c_semaphore);

    /* Initialize IMU motion sensor and suspend the task upon failure */
    result = motionsensor_init();
    CHECK_RESULT(result, " Error : Motion Sensor initialization failed !!\n Check hardware connection. [Error code: 0x%lx]\n", (long unsigned int)result);
    printf(" Motion Sensor successfully initialized.\n");

    /* Configure orientation interrupt and suspend the task upon failure */
    result = motionsensor_config_interrupt();
    CHECK_RESULT(result, " Error : Motion Sensor interrupt configuration failed !!\n [Error code: 0x%lx]\n", (long unsigned int)result);
    printf(" Motion Sensor interrupts successfully configured and enabled.\n\n\n");

    for(;;)
    {
        /* Get current orientation */
        result = motionsensor_update_orientation(&orientation);
        CHECK_RESULT(result, " Error : Could not read motion sensor data !!\n [Error code: 0x%lx]\n", (long unsigned int)result);

        /* Print the orientation in the same line.
         * \x1b[1F - ANSI ESC sequence to move cursor to the previous line.
         */
        switch(orientation)
        {
            case ORIENTATION_TOP_EDGE:
                printf("\x1b[1FOrientation = TOP_EDGE   \n");
                break;
            case ORIENTATION_BOTTOM_EDGE:
                printf("\x1b[1FOrientation = BOTTOM_EDGE\n");
                break;
            case ORIENTATION_LEFT_EDGE:
                printf("\x1b[1FOrientation = LEFT_EDGE  \n");
                break;
            case ORIENTATION_RIGHT_EDGE:
                printf("\x1b[1FOrientation = RIGHT_EDGE \n");
                break;
            case ORIENTATION_DISP_UP:
                printf("\x1b[1FOrientation = DISP_UP    \n");
                break;
            case ORIENTATION_DISP_DOWN:
                printf("\x1b[1FOrientation = DISP_DOWN  \n");
            default:
                break;
        }

        /* Wait for notification from ISR. The ISR will notify the task upon
         * receiving interrupt from the Motion Sensor on orientation change.
         */
        xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
    }
}

/*******************************************************************************
* Function Name: motionsensor_init
********************************************************************************
* Summary:
*  Function that configures the I2C master interface and then initializes
*  the motion sensor.
*
* Parameters:
*  None
*
* Return:
*  CY_RSLT_SUCCESS upon successful initialization of the motion sensor, else a
*  non-zero value that indicates the error.
*
*******************************************************************************/
static cy_rslt_t motionsensor_init(void)
{
    /* Status variable */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* I2C configuration structure */
    cyhal_i2c_cfg_t kit_i2c_cfg = {
        .is_slave = false,
        .address = 0,
        .frequencyhal_hz = I2C_CLK_FREQ_HZ
    };

    /* Block the I2C resource while initializing I2C and the motion sensor */
    xSemaphoreTake(i2c_semaphore, portMAX_DELAY);

    /* Initialize the I2C master interface for BMI160 motion sensor */
    result = cyhal_i2c_init(&kit_i2c, (cyhal_gpio_t) CYBSP_I2C_SDA,
                            (cyhal_gpio_t) CYBSP_I2C_SCL, NULL);
    CHECK_RESULT(result, " Error : I2C initialization failed !!\n [Error code: 0x%lx]\n", (long unsigned int)result);

    /* Configure the I2C master interface with the desired clock frequency */
    result = cyhal_i2c_configure(&kit_i2c, &kit_i2c_cfg);
    CHECK_RESULT(result, " Error : I2C configuration failed !!\n [Error code: 0x%lx]\n", (long unsigned int)result);

    /* Initialize the IMU motion sensor */
#if (SENSOR_TYPE == SENSOR_TYPE_BMI160)
    result = mtb_bmi160_init_i2c(&motion_sensor, &kit_i2c, MTB_BMI160_DEFAULT_ADDRESS);
#else
    #if (INTERFACE_USED == CY8CKIT_062S2_AI)
        result = mtb_bmi270_init_i2c(&sensor_bmi270, &kit_i2c, MTB_BMI270_ADDRESS_DEFAULT);
    #else
        result = mtb_bmi270_init_i2c(&sensor_bmi270, &kit_i2c, MTB_BMI270_ADDRESS_SEC);
    #endif
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    result = mtb_bmi270_config_default(&sensor_bmi270);
#endif

    /* Release the I2C resource after initializing the motion sensor */
    xSemaphoreGive(i2c_semaphore);

    return result;
}

/*******************************************************************************
* Function Name: motionsensor_interrupt_handler
********************************************************************************
* Summary:
*  Interrupt service routine(ISR) for orientation interrupts from motion sensor.
*  The ISR notifies the Motion sensor task.
*
* Parameters:
*  void *handler_arg            : Pointer to variable passed to the ISR (unused)
*  cyhal_gpio_irq_event_t event : GPIO event type (unused)
*
* Return:
*  None
*
*******************************************************************************/
static void motionsensor_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    /* To avoid compiler warnings */
    (void)handler_arg;
    (void)event;

    /* Notify the Motion sensor task */
    xTaskNotifyFromISR(motion_sensor_task_handle, 0, eNoAction, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*******************************************************************************
* Function Name: motionsensor_config_interrupt
********************************************************************************
* Summary:
*  Configures the motion sensor to detect change in orientation. This functions
*  sets up the motion sensor to provide a pulse on orientation change and
*  configures the active level and pulse width.
*
* Parameters:
*  None
*
* Return:
*  CY_RSLT_SUCCESS upon successful orientation interrupt configuration, else a
*  non-zero value that indicates the error.
*
*******************************************************************************/
static cy_rslt_t motionsensor_config_interrupt(void)
{
#if (SENSOR_TYPE == SENSOR_TYPE_BMI160)
    /* Structure for storing interrupt configuration */
    struct bmi160_int_settg int_config;

    /* Status variable */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Map the orientation interrupt to the interrupt pin specified by the
     * 'IMU_INTERRUPT_CHANNEL' macro.
     */
    int_config.int_channel = (IMU_INTERRUPT_CHANNEL == 1) ? BMI160_INT_CHANNEL_1 :
                             BMI160_INT_CHANNEL_2;
    /* Select the Interrupt type as Orientation interrupt */
    int_config.int_type = BMI160_ACC_ORIENT_INT;

    /* Interrupt pin configuration */
    /* Enabling interrupt pins to act as output pin */
    int_config.int_pin_settg.output_en = BMI160_ENABLE;
    /* Choosing push-pull mode for interrupt pin */
    int_config.int_pin_settg.output_mode = BMI160_DISABLE;
    /* Choosing active High output */
    int_config.int_pin_settg.output_type = BMI160_ENABLE;
    /* Choosing edge triggered output */
    int_config.int_pin_settg.edge_ctrl = BMI160_ENABLE;
    /* Disabling interrupt pin to act as input */
    int_config.int_pin_settg.input_en = BMI160_DISABLE;
    /* 5 ms latched output */
    int_config.int_pin_settg.latch_dur = BMI160_LATCH_DUR_5_MILLI_SEC;

    /* Interrupt type configuration */
    /* No exchange axis */
    int_config.int_type_cfg.acc_orient_int.axes_ex = 1;
    /* Set orientation blocking */
    int_config.int_type_cfg.acc_orient_int.orient_blocking = 0;
    /* Set orientation hysteresis */
    int_config.int_type_cfg.acc_orient_int.orient_hyst = 2;
    /* Set orientation interrupt mode */
    int_config.int_type_cfg.acc_orient_int.orient_mode = 0;
    /* Set orientation interrupt theta */
    int_config.int_type_cfg.acc_orient_int.orient_theta = 0;
    /* Enable orientation */
    int_config.int_type_cfg.acc_orient_int.orient_en = 1;
    /* Enable orientation interrupt */
    int_config.int_type_cfg.acc_orient_int.orient_ud_en = 1;

    /* Block the I2C resource while configuring the motion sensor */
    xSemaphoreTake(i2c_semaphore, portMAX_DELAY);

    /* Configure the BMI160 interrupt */
    result = mtb_bmi160_config_int(&motion_sensor, &int_config, (cyhal_gpio_t) IMU_INTERRUPT_PIN,
                                   IMU_INTERRUPT_PRIORITY, CYHAL_GPIO_IRQ_RISE,
                                   motionsensor_interrupt_handler, NULL);

    /* Release the I2C resource after configuring the motion sensor */
    xSemaphoreGive(i2c_semaphore);

    return result;
#else
    /* Structures for storing interrupt configuration */
    struct bmi2_int_pin_config pin_config = {0};

    /* Status variable */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Get the default interrupt pin configurations using the BMI270_SensorAPI library function */
    result = bmi2_get_int_pin_config(&pin_config, &(sensor_bmi270.sensor));
    if (result != BMI2_OK)
    {
        CY_ASSERT(0);
    }

    /* Interrupt pin configuration */
    pin_config.pin_type = (IMU_INTERRUPT_CHANNEL == 1) ? BMI2_INT1 :
                             BMI2_INT2;
    pin_config.int_latch = BMI2_INT_NON_LATCH;
    pin_config.pin_cfg[0].input_en = BMI2_INT_INPUT_DISABLE;
    pin_config.pin_cfg[0].lvl = BMI2_INT_ACTIVE_LOW;
    pin_config.pin_cfg[0].od = BMI2_INT_PUSH_PULL;
    pin_config.pin_cfg[0].output_en = BMI2_INT_OUTPUT_ENABLE;

    /* Map data ready interrupt to interrupt pin using the BMI270_SensorAPI library function */
    result = bmi2_map_data_int(BMI2_DRDY_INT, ((IMU_INTERRUPT_CHANNEL == 1) ?
                BMI2_INT1 : BMI2_INT2), &(sensor_bmi270.sensor));
    if (result != BMI2_OK)
    {
        CY_ASSERT(0);
    }

    /* Configure the BMI270 interrupt */
    result = mtb_bmi270_config_int(&sensor_bmi270, &pin_config, (cyhal_gpio_t) IMU_INTERRUPT_PIN,
                    IMU_INTERRUPT_PRIORITY, CYHAL_GPIO_IRQ_FALL,
                    motionsensor_interrupt_handler, NULL);

    /* Release the I2C resource after configuring the motion sensor */
    xSemaphoreGive(i2c_semaphore);

    return result;
#endif
}

/*******************************************************************************
* Function Name: motionsensor_update_orientation
********************************************************************************
* Summary:
*  Function that updates the orientation status to one of the 6 types, see
*  'orientation_t'. This functions detects the axis that is most perpendicular
*  to the ground based on the absolute value of acceleration in that axis.
*  The sign of the acceleration signifies whether the axis is facing the ground
*  or the opposite.
*
* Parameters:
*  orientation_t *orientation_result: Address of the variable that stores the
*                                     orientation information.
*
* Return:
*  CY_RSLT_SUCCESS upon successful orientation update, else a non-zero value
*  that indicates the error.
*
*******************************************************************************/
static cy_rslt_t motionsensor_update_orientation(orientation_t *orientation_result)
{
    /* Status variable */
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Structure to store the accelerometer and gyroscope data read from the
     * IMU motion sensor
     */
#if (SENSOR_TYPE == SENSOR_TYPE_BMI160)
    mtb_bmi160_data_t data;
#endif

    /* Variables used to store absolute values of the accelerometer data */
    uint16_t abs_x, abs_y, abs_z;

    if(orientation_result == NULL)
    {
#if (SENSOR_TYPE == SENSOR_TYPE_BMI160)
        result = BMI160_E_NULL_PTR;
#else
        result = BMI2_E_NULL_PTR;
#endif
    }
    else
    {
        /* Clear the previous orientation data */
        *orientation_result = ORIENTATION_NULL;

        /* Block the I2C resource while reading the motion sensor data */
        xSemaphoreTake(i2c_semaphore, portMAX_DELAY);

        /* Read x, y, z components of acceleration */
#if (SENSOR_TYPE == SENSOR_TYPE_BMI160)
        result = mtb_bmi160_read(&motion_sensor, &data);
#else
        result =  mtb_bmi270_read(&sensor_bmi270, &data);
#endif

        /* Release the I2C resource after reading the motion sensor data */
        xSemaphoreGive(i2c_semaphore);

        if (result == CY_RSLT_SUCCESS)
        {
#if (SENSOR_TYPE == SENSOR_TYPE_BMI160)
            /* Get the absolute values of the accelerations along each axis */
            abs_x = abs(data.accel.x);
            abs_y = abs(data.accel.y);
            abs_z = abs(data.accel.z);
#else
            abs_x = abs(data.sensor_data.acc.x);
            abs_y = abs(data.sensor_data.acc.y);
            abs_z = abs(data.sensor_data.acc.z);
#endif

            /* Z axis (perpendicular to face of the display) is most aligned with
             * gravity.
             */
            if ((abs_z > abs_x) && (abs_z > abs_y))
            {
#if (SENSOR_TYPE == SENSOR_TYPE_BMI160)
                if (data.accel.z < 0)
                {
                    /* Display faces down (towards the ground) */
                    *orientation_result = ORIENTATION_DISP_DOWN;
                }
#else
                if (data.sensor_data.acc.z < 0)
                {
                    /* Display faces down (towards the ground) */
                    *orientation_result = ORIENTATION_DISP_DOWN;
                }
#endif
                else
                {
                    /* Display faces up (towards the sky/ceiling) */
                    *orientation_result = ORIENTATION_DISP_UP;
                }
            }
            /* Y axis (parallel with shorter edge of board) is most aligned with
             * gravity.
             */
            else if ((abs_y > abs_x) && (abs_y > abs_z))
            {
#if (SENSOR_TYPE == SENSOR_TYPE_BMI160)
                if (data.accel.y > 0)
                {
                    /* Display has an inverted landscape orientation */
                    *orientation_result = ORIENTATION_BOTTOM_EDGE;
                }
                else
                {
                    /* Display has landscape orientation */
                    *orientation_result = ORIENTATION_TOP_EDGE;
                }
#else
                if (data.sensor_data.acc.y > 0)
                {
                    /* Display has an inverted landscape orientation */
                    *orientation_result = ORIENTATION_TOP_EDGE;
                }
                else
                {
                    /* Display has landscape orientation */
                    *orientation_result = ORIENTATION_BOTTOM_EDGE;
                }
#endif
            }
            /* X axis (parallel with longer edge of board) is most aligned with
             * gravity.
             */
            else
            {
#if (SENSOR_TYPE == SENSOR_TYPE_BMI160)
                if (data.accel.x < 0)
                {
                    /* Display has an inverted portrait orientation */
                    *orientation_result = ORIENTATION_RIGHT_EDGE;
                }
                else
                {
                    /* Display has portrait orientation */
                    *orientation_result = ORIENTATION_LEFT_EDGE;
                }
#else
                if (data.sensor_data.acc.x < 0)
                {
                    /* Display has an inverted portrait orientation */
                    *orientation_result = ORIENTATION_LEFT_EDGE;
                }
                else
                {
                    /* Display has portrait orientation */
                    *orientation_result = ORIENTATION_RIGHT_EDGE;
                }
#endif
            }
        }
    }

    return result;
}

/* [] END OF FILE */
