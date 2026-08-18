#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_gpio.h"
#include "cy_sar.h"
#include "cy_scb_uart.h"
#include "cy_syslib.h"
#include "cy_wdt.h"
#include "system_psoc6.h"
#include <stdio.h>

#include "sar.h"
#include "smif_nor.h"
#include "utils.h"

/* Shared cross-core UART mutex + readiness flag (same addresses as the CM0p). */
#define UART_LOCK_ADDR   ((volatile uint32_t *)0x08005040u)
#define UART_READY_ADDR  ((volatile uint32_t *)0x08005044u)

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

    TICK_Init();

    /* Wait until the CM0p has initialized the shared SCB5 UART and SMIF */
    while (*(volatile uint32_t *)UART_READY_ADDR == 0u)
    {
    }

    led_init();
    sar_temp_init();

    /* Run the NOR benchmark once and report. */
    uart_lock();
    smif_nor_benchmark("CM4");
    uart_unlock();

    /* Afterwards: LED blink + die-temperature (SAR) sampling loop. */
    uint32_t last_print = 0;

    for (;;)
    {
        Cy_GPIO_Inv(GPIO_PRT11, 1u);
        HAL_Delay(250);

        Cy_GPIO_Inv(GPIO_PRT0, 3u);
        HAL_Delay(250);

        Cy_GPIO_Inv(GPIO_PRT1, 1u);
        HAL_Delay(250);

        if (HAL_GetTick() - last_print >= 1000u)
        {
            last_print = HAL_GetTick();
            int16_t die_temp = sar_temp_read_celsius();
            uart_lock();
            printf("[CM4] LED/ADC loop, DieTemp = %d C\r\n", (int)die_temp);
            uart_unlock();
        }
    }
}
