/*
 * ntc_test (app_ext) - NTC thermistor + internal DTS temperature demo on
 * CY8CKIT-062-BLE + CY8CKIT-028-EPD, CM4 running from the external S25FL512S
 * NOR via SMIF XIP (0x18000000).
 *
 * Port of app/ntc_test to the app_ext XIP flow: the CM0+ stub inits the SMIF
 * and releases this core, this app re-clocks the CPU to 150 MHz (PLL on
 * CLKPATH1, SMIF stays on CLKPATH0), applies the generated device config
 * (clocks/routing/peripherals/pins - not init_cycfg_system), then samples the
 * NTC (A0..A3) and the internal DieTemp sensor and reports the raw SAR counts
 * plus best-effort temperatures over the shared SCB5 UART.
 */

#include "cyhal.h"
#include "cybsp.h"
#include "cy_syslib.h"
#include "cy_wdt.h"
#include "cy_sysclk.h"
#include "system_psoc6.h"
#include "ntc.h"
#include <stdio.h>

extern void uart_init(void);

#define SAMPLE_DELAY_MS    (1000u)

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
    Cy_SysLib_DelayUs(2000u);  /* let the SCB5 TX settle */

    printf("\x1b[2J\x1b[;H");
    printf("****************************************\r\n");
    printf("  NTC thermistor + DTS temperature test\r\n");
    printf("****************************************\r\n\n");

    ntc_init();

    for (;;)
    {
        uint16_t a0, a1, a2, a3, dts;
        ntc_read_raw(&a0, &a1, &a2, &a3, &dts);
        uint16_t m0, m1, m2, m3;
        ntc_read_mv(&m0, &m1, &m2, &m3);
        float t_ntc = ntc_temp_from_raw(a0, a1, a2, a3);
        int16_t t_dts = dts_read_celsius();

        printf("A0=%u A1=%u A2=%u A3=%u DTS=%u\r\n",
               (unsigned)a0, (unsigned)a1, (unsigned)a2, (unsigned)a3, (unsigned)dts);
        printf("mV: %u %u %u %u   NTC: %6.2f C   DTS: %4d C\r\n",
               (unsigned)m0, (unsigned)m1, (unsigned)m2, (unsigned)m3,
               (double)t_ntc, (int)t_dts);

        Cy_SysLib_DelayUs(SAMPLE_DELAY_MS * 1000u);
    }
}
