#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_gpio.h"
#include "cy_sysclk.h"
#include "cy_syslib.h"
#include "cy_wdt.h"
#include "system_psoc6.h"
#include <stdio.h>

extern void uart_init(void);
extern void uart_puts(const char *s);

static void led_init(void)
{
    static const cy_stc_gpio_pin_config_t led_cfg = {
        .outVal    = 1u,
        .driveMode = CY_GPIO_DM_STRONG_IN_OFF,
        .hsiom     = HSIOM_SEL_GPIO,
        .intEdge   = CY_GPIO_INTR_DISABLE,
        .intMask   = 0u,
        .vtrip     = CY_GPIO_VTRIP_CMOS,
        .slewRate  = CY_GPIO_SLEW_FAST,
        .driveSel  = CY_GPIO_DRIVE_FULL,
        .vregEn    = 0u,
        .ibufMode  = 0u,
        .vtripSel  = 0u,
        .vrefSel   = 0u,
        .vohSel    = 0u,
    };

    Cy_GPIO_Pin_Init(GPIO_PRT1, 5u, &led_cfg);    /* LED4 = P1.5 */
    Cy_GPIO_Pin_Init(GPIO_PRT13, 7u, &led_cfg);   /* LED5 = P13.7 */
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

static void delay_loop(uint32_t n)
{
    for (volatile uint32_t i = 0; i < n; i++)
    {
        __asm volatile("nop");
    }
}

int main(void)
{
    Cy_PDL_Init(CY_DEVICE_CFG);

    Cy_WDT_Unlock();
    Cy_WDT_Disable();

    clock_init();   /* run at 100 MHz (FLL) */

    led_init();
    uart_init();

    uart_puts("\r\n=== CM0+ Boot Demo @ 100 MHz ===\r\n");
    uart_puts("If you see this, the app was booted manually\r\n");
    uart_puts("(boot ROM held CM0+ in PWR_MODE=1 otherwise)\r\n");

    uint32_t alive = 0;

    for (;;)
    {
        Cy_GPIO_Inv(GPIO_PRT1, 5u);    /* LED4 */
        delay_loop(4000000u);

        Cy_GPIO_Inv(GPIO_PRT13, 7u);   /* LED5 */
        delay_loop(4000000u);

        if (++alive >= 4u)
        {
            alive = 0u;
            printf("[CM0+] alive, SysClk = %lu Hz\r\n", (unsigned long)SystemCoreClock);
        }
    }
}
