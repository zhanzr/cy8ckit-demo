#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_wdt.h"
#include "system_psoc6.h"
#include <stdio.h>

#include "custom_def.h"
#include "dhry.h"
#include "utils.h"

/* Shared cross-core UART mutex + readiness flag (same addresses as the CM0p). */
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

int main(void)
{
    Cy_WDT_Unlock();
    Cy_WDT_Disable();

    TICK_Init();

    /* Wait until the CM0p has initialized the shared SCB5 UART */
    while (*(volatile uint32_t *)UART_READY_ADDR == 0u)
    {
    }

    const uint32_t cpu_hz = SystemCoreClock;

    for (;;)
    {
        /* Hold the shared UART lock for the whole run so the two cores'
         * result blocks never interleave on the console. */
        uart_lock();
        printf("\r\n=== Dhrystone 2.1 on PSoC6 [CM4] @ %lu Hz ===\r\n",
               (unsigned long)cpu_hz);
        dhry_main(cpu_hz);
        uart_unlock();

        HAL_Delay(500);
    }
}
