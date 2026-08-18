#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_gpio.h"
#include "cy_wdt.h"
#include "system_psoc6.h"

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

    Cy_GPIO_Pin_Init(GPIO_PRT1, 5u, &led_cfg);
    Cy_GPIO_Pin_Init(GPIO_PRT13, 7u, &led_cfg);
}

int main(void)
{
    Cy_PDL_Init(CY_DEVICE_CFG);

    led_init();

    for (;;)
    {
        Cy_GPIO_Inv(GPIO_PRT1, 5u);
        Cy_GPIO_Inv(GPIO_PRT13, 7u);

        for (volatile uint32_t i = 0; i < 500000u; i++)
        {
            __asm volatile("nop");
        }
    }
}
