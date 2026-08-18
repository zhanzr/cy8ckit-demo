#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_sysclk.h"
#include "cy_wdt.h"
#include "system_psoc6.h"
#include <stdio.h>

#include "smif_nor.h"
#include "utils.h"

extern void uart_init(void);

/* Shared cross-core UART mutex + readiness flag (fixed SRAM addresses,
 * above the CM0p RAM region 0x08000000..0x08005000). */
#define UART_LOCK_ADDR   ((volatile uint32_t *)0x08005040u)
#define UART_READY_ADDR  ((volatile uint32_t *)0x08005044u)

static void uart_lock(void)
{
    while (*(volatile uint32_t *)UART_LOCK_ADDR != 0u)
    {
    }
    *(volatile uint32_t *)UART_LOCK_ADDR = 1u;
}

static void uart_unlock(void)
{
    *(volatile uint32_t *)UART_LOCK_ADDR = 0u;
}

static void clock_init(void)
{
    static const cy_stc_fll_manual_config_t fllConfig = {
        .fllMult = 500U,                 /* 8 MHz / 20 * 500 / 2 = 100 MHz */
        .refDiv = 20U,
        .ccoRange = CY_SYSCLK_FLL_CCO_RANGE4,
        .enableOutputDiv = true,
        .lockTolerance = 10U,
        .igain = 9U,
        .pgain = 5U,
        .settlingCount = 8U,
        .outputMode = CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT,
        .cco_Freq = 355U,
    };

    /* Raise flash read wait states before increasing the clock (100 MHz). */
    Cy_SysLib_SetWaitStates(false, 100UL);

    /* The ROM may leave the FLL enabled at its default frequency; disable it
     * first, otherwise FllManualConfigure() refuses to program our values. */
    Cy_SysClk_FllDisable();

    /* IMO -> CLKPATH0 (FLL reference), FLL -> HFCLK0 -> CLK_FAST */
    Cy_SysClk_ClkPathSetSource(0U, CY_SYSCLK_CLKPATH_IN_IMO);
    (void)Cy_SysClk_FllManualConfigure(&fllConfig);
    (void)Cy_SysClk_FllEnable(100000u);
    Cy_SysClk_ClkHfSetSource(0U, CY_SYSCLK_CLKHF_IN_CLKPATH0);
    Cy_SysClk_ClkFastSetDivider(0U);
    SystemCoreClockUpdate();
}

static void enable_cm4(void)
{
    Cy_SysEnableCM4(CY_CORTEX_M4_APPL_ADDR);
}

int main(void)
{
    Cy_WDT_Unlock();
    Cy_WDT_Disable();

    clock_init();   /* run both cores at 100 MHz (FLL) */

    *UART_LOCK_ADDR = 0u;   /* clear shared UART mutex (flash work area leftovers) */

    uart_init();
    TICK_Init();

    smif_nor_init();        /* SMIF + S25FL512S, device mode, no remapping */

    *UART_READY_ADDR = 1u;   /* UART + SMIF ready for the CM4 */

    enable_cm4();

    /* Run the NOR benchmark once and report, then idle. */
    uart_lock();
    smif_nor_benchmark("CM0p");
    uart_unlock();

    uart_lock();
    printf("\r\n[CM0p] benchmark done, idling.\r\n");
    uart_unlock();

    for (;;)
    {
    }
}
