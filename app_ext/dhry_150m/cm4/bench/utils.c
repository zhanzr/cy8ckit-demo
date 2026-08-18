#include <stdarg.h>
#include <stdio.h>
#include "cy_syslib.h"
#include "cy_systick.h"
#include "system_psoc6.h"
#include "utils.h"

static volatile uint32_t systick_ms = 0;

void SysTick_Handler(void)
{
    systick_ms++;
}

void TICK_Init(void)
{
    Cy_SysTick_Init(CY_SYSTICK_CLOCK_SOURCE_CLK_CPU, SystemCoreClock / 1000u);
    Cy_SysTick_SetCallback(0, SysTick_Handler);
    __enable_irq();
}

uint32_t HAL_GetTick(void)
{
    return systick_ms;
}

void HAL_Delay(uint32_t t)
{
    uint32_t start = systick_ms;
    while ((systick_ms - start) < t)
    {
    }
}

void uart_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}
