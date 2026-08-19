/*
 * (c) 2024-2025, Infineon Technologies AG, or an affiliate of Infineon
 * Technologies AG. All rights reserved.
 *
 * This software, associated documentation and materials ("Software") is
 * owned by Infineon Technologies AG or one of its affiliates ("Infineon")
 * and is protected by and subject to worldwide patent protection, worldwide
 * copyright laws, and international treaty provisions. Therefore, you may
 * use this Software only as provided in the license agreement accompanying
 * the software package from which you obtained this Software. If no license
 * agreement applies, then any use, reproduction, modification, translation,
 * or compilation of this Software is prohibited without the express written
 * permission of Infineon.
 *
 * Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
 * IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
 * THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS
 * FOR A SPECIFIC USE/PURPOSE OR MERCHANTABILITY. Infineon reserves the right
 * to make changes to the Software without notice. You are responsible for
 * properly designing, programming, and testing the functionality and safety
 * of your intended application of the Software, as well as complying with
 * any legal requirements related to its use. Infineon does not guarantee
 * that the Software will be free from intrusion, data theft or loss, or
 * other breaches ("Security Breaches"), and Infineon shall have no liability
 * arising out of any Security Breaches. Unless otherwise explicitly approved
 * by Infineon, the Software may not be used in any application where a
 * failure of the Product or any consequences of the use thereof can
 * reasonably be expected to result in personal injury.
 */
#include "FreeRTOS.h"
#include "platform_isr.h"
#include "portmacro.h"
#include "cyhal_timer.h"

extern void _cyhal_timer_irq_handler(void);
extern void platform_irq_demuxer(void);
/** @file
 * Implementation of IRQ handling functions for CR4.
 */

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/
void irq_vector_external_interrupt( void ) __attribute__((naked));
void irq_vector_software_interrupt( void ) __attribute__((naked));

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

/* Initial entry point for external interrupts */
void irq_vector_external_interrupt( void )
{
    __asm volatile (
        "B    FreeRTOS_IRQ_Handler    \t\n"
    );
}

/* Initial entry point for software interrupts */
void irq_vector_software_interrupt( void )
{
    __asm volatile (
        "B    FreeRTOS_SVC_Handler    \t\n"
    );
}

static void timer_isr(void *callback_arg, cyhal_timer_event_t event)
{
    (void) (callback_arg);
    (void) (event);
    FreeRTOS_Tick_Handler( );
}

extern void platform_tick_start()
{
    cy_rslt_t result;
    static bool tick_timer_initialized = false;
    static cyhal_timer_t timer;
    const cyhal_timer_cfg_t TimerCfg = {true, CYHAL_TIMER_DIR_UP, true, configCPU_CLOCK_HZ/configTICK_RATE_HZ, (configCPU_CLOCK_HZ/configTICK_RATE_HZ)/2, 0};
    (void) result;
    if( !tick_timer_initialized )
    {
        result = cyhal_timer_init(&timer, 0, NULL);
        CY_ASSERT(result == CY_RSLT_SUCCESS);

        result = cyhal_timer_set_frequency(&timer, configCPU_CLOCK_HZ);
        CY_ASSERT(result == CY_RSLT_SUCCESS);
        
        cyhal_timer_configure(&timer, &TimerCfg);

        cyhal_timer_register_callback(&timer, timer_isr, NULL);
        cyhal_timer_enable_event(&timer, CYHAL_TIMER_IRQ_CAPTURE_COMPARE, 0, true);

        result = cyhal_timer_start(&timer);
        CY_ASSERT(result == CY_RSLT_SUCCESS);

        tick_timer_initialized = true;
    }
}

PLATFORM_DEFINE_ISR(platform_irq_demuxer_wrapper)
{
    platform_irq_demuxer();
}

PLATFORM_MAP_ISR(platform_irq_demuxer_wrapper, vApplicationIRQHandler);

/* This empty function is needed just to allow the linker to discard the symbol from the binary. Without this, the weak function platform_irq_demuxer_default defined in BSP will not be discarded by the linker. */
void platform_irq_demuxer_default(void)
{
    return;
}
