/*
 * bmi160_freertos (app_ext) - BMI160 motion-sensor orientation demo (FreeRTOS)
 * on CY8CKIT-062-BLE + CY8CKIT-028-EPD, CM4 running from the external S25FL512S
 * NOR via SMIF XIP (0x18000000).
 *
 * Port of app/bmi160_freertos (Infineon mtb-example-psoc6-motion-sensor-freertos)
 * to the app_ext XIP flow: the CM0+ stub inits the SMIF and releases this core,
 * this app re-clocks the CPU to 150 MHz (PLL on CLKPATH1, SMIF stays on
 * CLKPATH0), applies the generated device config (clocks/routing/peripherals/
 * pins - not init_cycfg_system, so the stub's clocks/SMIF are untouched), and
 * starts the FreeRTOS scheduler. The motion task reads the BMI160 on the
 * CY8CKIT-028-EPD shield over I2C (CYBSP_I2C, P6_1/P6_0) and prints the board
 * orientation.
 */

#include "cy_pdl.h"
#include "cyhal.h"
#include "cycfg.h"
#include "cy_syslib.h"
#include "cy_wdt.h"
#include "cy_sysclk.h"
#include "system_psoc6.h"
#include "motion_task.h"
#include "FreeRTOS.h"
#include <stdio.h>

extern void uart_init(void);

/* This enables RTOS aware debugging. */
volatile int uxTopUsedPriority;

/* Raise the CPU to 150 MHz (PLL on CLKPATH1) while keeping CLKPATH0 (the
 * FLL, from the CM0+ stub) as the SMIF interface clock source. XIP data
 * reads are reliable at 150 MHz and marginal at 100 MHz on this board. */
static void clock_init_150mhz_pll(void)
{
    Cy_SysClk_ClkPathSetSource(1u, CY_SYSCLK_CLKPATH_IN_IMO);
    {
        static const cy_stc_pll_config_t pllCfg = {
            .inputFreq  = 8000000u,
            .outputFreq = 150000000u,
            .outputMode = CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT,
        };
        (void)Cy_SysClk_PllConfigure(1u, &pllCfg);
    }
    (void)Cy_SysClk_PllEnable(1u, 100000u);
    Cy_SysLib_SetWaitStates(false, 150UL);
    Cy_SysClk_ClkHfSetSource(0u, CY_SYSCLK_CLKHF_IN_CLKPATH1);
    SystemCoreClockUpdate();
}

int main(void)
{
    cy_rslt_t result;

    /* This enables RTOS aware debugging in OpenOCD. */
    uxTopUsedPriority = configMAX_PRIORITIES - 1;

    Cy_WDT_Unlock();
    Cy_WDT_Disable();
    __enable_irq();

    /* CPU to 150 MHz (PLL, CLKPATH1). SMIF XIP stays on CLKPATH0 (stub FLL). */
    clock_init_150mhz_pll();

    /* Device config: peripheral dividers, routing, peripherals and pins.
     * init_cycfg_system is deliberately NOT called - the CM0+ stub already
     * set the clocks/SMIF. */
    init_cycfg_clocks();
    init_cycfg_routing();
    init_cycfg_peripherals();
    init_cycfg_pins();

    uart_init();  /* SCB5 P5_1/P5_0 @ 115200 (replaces cy_retarget_io) */

    /* Create the Motion Sensor task */
    result = create_motion_sensor_task();
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    /* To avoid compiler warning */
    (void) result;

    /* Start the scheduler */
    vTaskStartScheduler();

    /* Should never get here! */
    /* Halt the CPU if scheduler exits */
    CY_ASSERT(0);
}
