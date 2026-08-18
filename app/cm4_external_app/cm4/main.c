#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_gpio.h"
#include "cy_scb_uart.h"
#include "cy_syslib.h"
#include "cy_wdt.h"

#include "utils.h"

/* Shared cross-core UART readiness flag (same address as the CM0p). The
 * CM0p is silent after booting the CM4, so no mutex is needed here. */
#define UART_READY_ADDR  ((volatile uint32_t *)0x08005044u)

static void uart_putc(char c)
{
    while (Cy_SCB_UART_Put(SCB5, (uint32_t)(uint8_t)c) == 0u)
    {
    }
}

static void uart_puts(const char *s)
{
    while (*s)
    {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
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

    Cy_GPIO_Pin_Init(GPIO_PRT0, 3u, &led_cfg);   /* LED1 = P0.3 */
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

    led_init();

    uart_puts("\r\n=== CM4 running from the external-flash image ===\r\n");
    uart_puts("  stored in SPI NOR at 0x18000000, read back by the CM0p\r\n");
    uart_puts("  (GPIO bit-bang) into internal SRAM and booted.\r\n");

    for (;;)
    {
        Cy_GPIO_Inv(GPIO_PRT0, 3u);
        HAL_Delay(500);
    }
}
