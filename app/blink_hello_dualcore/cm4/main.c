#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_sysclk.h"
#include "cy_wdt.h"
#include "cy_gpio.h"
#include "cy_systick.h"
#include "cy_scb_uart.h"
#include "cy_sar.h"
#include "system_psoc6.h"
#include <stdio.h>

#include "sar.h"

/* Shared cross-core UART mutex + readiness flag (fixed SRAM addresses). */
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

static void uart_putc(char c)
{
    while (Cy_SCB_UART_Put(SCB5, (uint32_t)(uint8_t)c) == 0u)
    {
    }
}

int _write(int fd, char *ptr, int len)
{
    (void)fd;
    for (int i = 0; i < len; i++)
    {
        if (ptr[i] == '\n')
            uart_putc('\r');
        uart_putc(ptr[i]);
    }
    return len;
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

    Cy_GPIO_Pin_Init(GPIO_PRT11, 1u, &led_cfg);  /* LED0 = P11.1 */
    Cy_GPIO_Pin_Init(GPIO_PRT0, 3u, &led_cfg);   /* LED1 = P0.3  */
    Cy_GPIO_Pin_Init(GPIO_PRT1, 1u, &led_cfg);   /* LED2 = P1.1  */
}

int main(void)
{
    Cy_WDT_Unlock();
    Cy_WDT_Disable();

    *UART_LOCK_ADDR = 0u;   /* clear shared UART mutex (flash work area leftovers) */

    Cy_SysTick_Init(CY_SYSTICK_CLOCK_SOURCE_CLK_CPU, SystemCoreClock / 1000u);
    Cy_SysTick_SetCallback(0, SysTick_Handler);
    __enable_irq();

    /* Wait until the CM0+ has initialized the shared SCB5 UART */
    while (*(volatile uint32_t *)UART_READY_ADDR == 0u)
    {
    }

    led_init();
    sar_temp_init();   /* SAR ADC configured on the internal DieTemp sensor */

    uint32_t last_print = 0;

    for (;;)
    {
        /* CM4 LED pattern: LED0 INV, delay, LED1 INV, delay, LED2 INV, delay */
        Cy_GPIO_Inv(GPIO_PRT11, 1u);
        delay_ms(250);

        Cy_GPIO_Inv(GPIO_PRT0, 3u);
        delay_ms(250);

        Cy_GPIO_Inv(GPIO_PRT1, 1u);
        delay_ms(250);

        /* Periodic core-info + die-temperature print over the shared
         * (mutex-protected) UART. The DieTemp sensor is an internal SAR
         * channel (no external pin) measured against the 1.2 V bandgap. */
        if (systick_ms - last_print >= 1000u)
        {
            last_print = systick_ms;
            int16_t die_temp = sar_temp_read_celsius();
            uart_lock();
            printf("[CM4] Core ID = 1, SysClk = %lu Hz, DieTemp = %d C\r\n",
                   (unsigned long)SystemCoreClock, (int)die_temp);
            uart_unlock();
        }
    }
}
