#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_sysclk.h"
#include "cy_wdt.h"
#include "cy_gpio.h"
#include "cy_systick.h"
#include "system_psoc6.h"
#include <stdio.h>

extern void uart_init(void);
extern void uart_puts(const char *s);

/* Shared cross-core UART mutex + readiness flag (fixed SRAM addresses).
 * CM0+ SRAM is 0x08000000..0x08003000; 0x08001000 is unused by code/stack. */
#define UART_LOCK_ADDR   ((volatile uint32_t *)0x08003040u)
#define UART_READY_ADDR  ((volatile uint32_t *)0x08003044u)

static volatile uint32_t systick_ms = 0;

void SysTick_Handler(void)
{
    systick_ms++;
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = systick_ms;
    while ((systick_ms - start) < ms)
    {
    }
}

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

static void led_init(void)
{
    static const cy_stc_gpio_pin_config_t led_cfg = {
        .outVal = 1u,
        .driveMode = CY_GPIO_DM_STRONG_IN_OFF,
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

    Cy_GPIO_Pin_Init(GPIO_PRT1, 5u, &led_cfg);   /* LED4 = P1.5 */
    Cy_GPIO_Pin_Init(GPIO_PRT13, 7u, &led_cfg);  /* LED5 = P13.7 */
}

static void enable_cm4(void)
{
    Cy_SysEnableCM4(CY_CORTEX_M4_APPL_ADDR);
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

    /* IMO -> CLKPATH0 (FLL reference), FLL -> HFCLK0 -> CLK_FAST (both cores) */
    Cy_SysClk_ClkPathSetSource(0U, CY_SYSCLK_CLKPATH_IN_IMO);
    (void)Cy_SysClk_FllManualConfigure(&fllConfig);
    (void)Cy_SysClk_FllEnable(100000u);
    Cy_SysClk_ClkHfSetSource(0U, CY_SYSCLK_CLKHF_IN_CLKPATH0);
    Cy_SysClk_ClkFastSetDivider(0U);
    SystemCoreClockUpdate();
}

int main(void)
{
    Cy_WDT_Unlock();
    Cy_WDT_Disable();

    clock_init();   /* run both cores at 100 MHz (FLL) */

    *UART_LOCK_ADDR = 0u;   /* clear shared UART mutex (flash work area leftovers) */

    led_init();
    uart_init();

    *UART_READY_ADDR = 1u;   /* UART ready for the CM4 */

    enable_cm4();

    uart_puts("\r\n=== Dual-Core Blink + UART Demo ===\r\n");

    Cy_SysTick_Init(CY_SYSTICK_CLOCK_SOURCE_CLK_CPU, SystemCoreClock / 1000u);
    Cy_SysTick_SetCallback(0, SysTick_Handler);
    __enable_irq();

    uart_puts("CM4 started\r\n");
    uart_puts("UART: SCB5 @ 115200 8N1 on P5 P5[1](TX)\r\n");
    uart_puts("================================\r\n\r\n");

    uint32_t last_print = 0;

    for (;;)
    {
        /* CM0 LED pattern: LED4 INV, delay, LED5 INV, delay */
        Cy_GPIO_Inv(GPIO_PRT1, 5u);
        delay_ms(250);

        Cy_GPIO_Inv(GPIO_PRT13, 7u);
        delay_ms(250);

        /* Periodic core-info print over the shared (mutex-protected) UART */
        if (systick_ms - last_print >= 1000u)
        {
            last_print = systick_ms;
            uart_lock();
            printf("[CM0] Core ID = 0, SysClk = %lu Hz\r\n", (unsigned long)SystemCoreClock);
            uart_unlock();
        }
    }
}
