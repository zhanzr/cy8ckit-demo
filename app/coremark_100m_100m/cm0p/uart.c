#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_sysclk.h"
#include "cy_gpio.h"
#include "cy_scb_uart.h"
#include <stdio.h>

static cy_stc_scb_uart_context_t uart_context;

void uart_init(void)
{
    static const cy_stc_gpio_pin_config_t pin_cfg = {
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

    Cy_GPIO_Pin_Init(GPIO_PRT5, 0u, &pin_cfg);
    Cy_GPIO_Pin_Init(GPIO_PRT5, 1u, &pin_cfg);

    HSIOM_PRT5->PORT_SEL0 = (HSIOM_PRT5->PORT_SEL0 & ~((uint32_t)HSIOM_PRT_PORT_SEL0_IO0_SEL_Msk | (uint32_t)HSIOM_PRT_PORT_SEL0_IO1_SEL_Msk))
                           | ((uint32_t)18u << HSIOM_PRT_PORT_SEL0_IO0_SEL_Pos)
                           | ((uint32_t)18u << HSIOM_PRT_PORT_SEL0_IO1_SEL_Pos);

    /* SCB5 clock = SystemCoreClock / divide, target 115200 baud @ 16x oversample
     * (fractional 16.5-bit divider: divide = INT16 + 1 + FRAC/32) */
    {
        uint32_t scbClkHz = 115200u * 16u;
        uint32_t divx32 = (uint32_t)(((uint64_t)SystemCoreClock * 32u) / scbClkHz);
        uint32_t int16 = divx32 / 32u;
        uint32_t frac5 = divx32 % 32u;

        Cy_SysClk_PeriphAssignDivider(PCLK_SCB5_CLOCK, CY_SYSCLK_DIV_16_5_BIT, 0u);
        Cy_SysClk_PeriphSetFracDivider(CY_SYSCLK_DIV_16_5_BIT, 0u, (int16 - 1u), frac5);
        Cy_SysClk_PeriphEnableDivider(CY_SYSCLK_DIV_16_5_BIT, 0u);
    }

    static const cy_stc_scb_uart_config_t uart_cfg = {
        .uartMode                   = CY_SCB_UART_STANDARD,
        .oversample                 = 16u,
        .dataWidth                  = 8u,
        .enableMsbFirst             = false,
        .stopBits                   = CY_SCB_UART_STOP_BITS_1,
        .parity                     = CY_SCB_UART_PARITY_NONE,
        .enableInputFilter          = false,
        .dropOnParityError          = false,
        .dropOnFrameError           = false,
        .enableMutliProcessorMode   = false,
        .receiverAddress            = 0u,
        .receiverAddressMask        = 0u,
        .acceptAddrInFifo           = false,
        .irdaInvertRx               = false,
        .irdaEnableLowPowerReceiver = false,
        .smartCardRetryOnNack       = false,
        .enableCts                  = false,
        .ctsPolarity                = CY_SCB_UART_ACTIVE_LOW,
        .rtsRxFifoLevel            = 0u,
        .rtsPolarity                = CY_SCB_UART_ACTIVE_LOW,
        .breakWidth                 = 0x10u,
        .rxFifoTriggerLevel        = 0u,
        .rxFifoIntEnableMask       = 0u,
        .txFifoTriggerLevel        = 0u,
        .txFifoIntEnableMask       = 0u,
    };

    Cy_SCB_UART_Init(SCB5, &uart_cfg, &uart_context);
    Cy_SCB_UART_Enable(SCB5);
}

void uart_putc(char c)
{
    while (Cy_SCB_UART_Put(SCB5, (uint32_t)(uint8_t)c) == 0u)
    {
    }
}

void uart_puts(const char *s)
{
    while (*s)
    {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
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
