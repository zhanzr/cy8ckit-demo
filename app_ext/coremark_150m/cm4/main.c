/*
 * coremark_150m - app_ext CM4 app: CoreMark on the CM4 at 150 MHz,
 * running from the external NOR via SMIF XIP (0x18000000).
 *
 * The CM0+ XIP stub configured the SMIF XIP (50 MHz) and released this core.
 * Here we raise the CPU to 150 MHz (PLL on CLKPATH1; SMIF stays on
 * CLKPATH0/FLL@100) and run coremark_main(). SCB5 UART is owned exclusively
 * by this core (the CM0+ is in IDLE).
 */

#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_sysclk.h"
#include "cy_syslib.h"
#include "cy_wdt.h"
#include "system_psoc6.h"
#include <stdio.h>

#include "custom_def.h"
#include "core_portme.h"
#include "utils.h"

int coremark_main(void);

extern void uart_init(void);

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
    Cy_WDT_Unlock();
    Cy_WDT_Disable();

    clock_init_150mhz_pll();
    uart_init();
    TICK_Init();

    printf("\r\n=== CoreMark on PSoC6 [CM4] @ %lu Hz (XIP) ===\r\n",
           (unsigned long)SystemCoreClock);
    coremark_main();

    for (;;)
    {
    }
}
