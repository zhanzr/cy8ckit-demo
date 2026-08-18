/*
 * blink_hello - app_ext CM4 app, runs from the external NOR via SMIF XIP
 * (0x18000000) at 150 MHz.
 *
 * The CM0+ XIP stub already configured the SMIF XIP (50 MHz interface clock)
 * and released this core. This app:
 *   1. Raises the CPU clock to 150 MHz using the PLL (CLKPATH1) while keeping
 *      CLKPATH0 (FLL @ 100 MHz) as the SMIF interface clock source, so XIP
 *      reads stay at the proven 50 MHz.
 *   2. Runs the LED blink + internal DieTemp ADC sampling loop, printing
 *      over SCB5 UART (115200 8N1) which this core owns exclusively.
 */

#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_sysclk.h"
#include "cy_syslib.h"
#include "cy_gpio.h"
#include "cy_sar.h"
#include "cy_wdt.h"
#include "system_psoc6.h"
#include <stdio.h>

#include "sar.h"

extern void uart_init(void);

static void clock_init_150mhz_pll(void)
{
    /* PLL on CLKPATH1, reference = IMO (8 MHz) via the CLKPATH1 mux.
     * Fout = 8 MHz * P / (Q * out); PllConfigure auto-picks P/Q/out. */
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

    /* Flash wait states for 150 MHz before switching the CPU clock. */
    Cy_SysLib_SetWaitStates(false, 150UL);

    /* CPU (HFCLK0) <- PLL via CLKPATH1; SMIF stays on CLKPATH0 (FLL). */
    Cy_SysClk_ClkHfSetSource(0u, CY_SYSCLK_CLKHF_IN_CLKPATH1);
    SystemCoreClockUpdate();
}

static void led_init(void)
{
    static const cy_stc_gpio_pin_config_t led_cfg = {
        .outVal = 1u,
        .driveMode = CY_GPIO_DM_STRONG,
        .hsiom = HSIOM_SEL_GPIO,
        .intEdge = CY_GPIO_INTR_DISABLE,
        .intMask = 0u,
        .vtrip = CY_GPIO_VTRIP_CMOS,
        .slewRate = CY_GPIO_SLEW_FAST,
        .driveSel = CY_GPIO_DRIVE_FULL,
        .vregEn = 0u,
        .ibufMode = 0u,
        .vtripSel = 0u,
        .vrefSel = 0u,
        .vohSel = 0u,
    };
    Cy_GPIO_Pin_Init(GPIO_PRT11, 1u, &led_cfg);  /* LED0 = P11.1 */
    Cy_GPIO_Pin_Init(GPIO_PRT0, 3u, &led_cfg);   /* LED1 = P0.3  */
    Cy_GPIO_Pin_Init(GPIO_PRT1, 1u, &led_cfg);   /* LED2 = P1.1  */
}

static void delay_ms(uint32_t ms)
{
    volatile uint32_t d = ms * (SystemCoreClock / 1000u) / 12u;
    while (d--)
    {
    }
}

int main(void)
{
    Cy_WDT_Unlock();
    Cy_WDT_Disable();

    /* Clock first: uart_init() derives the SCB5 baud divider from
     * SystemCoreClock, so it must see the final 150 MHz clock. */
    clock_init_150mhz_pll();
    uart_init();
    led_init();
    sar_temp_init();

    printf("\r\n=== blink_hello [app_ext] CM4 @ %lu Hz (XIP) ===\r\n",
           (unsigned long)SystemCoreClock);

    for (;;)
    {
        Cy_GPIO_Inv(GPIO_PRT11, 1u);
        printf("DieTemp: %d C\r\n", sar_temp_read_celsius());
        delay_ms(250);

        Cy_GPIO_Inv(GPIO_PRT0, 3u);
        delay_ms(250);

        Cy_GPIO_Inv(GPIO_PRT1, 1u);
        delay_ms(250);
    }
}
