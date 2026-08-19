/*
 * eink_test (app_ext) - E2271CS021 E-ink demo running from the external NOR
 * via SMIF XIP (0x18000000).
 *
 * The CM0+ XIP stub configured the SMIF XIP (50 MHz interface clock) and
 * released this core at 100 MHz (FLL on CLKPATH0). This app drives the panel
 * with the SAME cyhal SPI stack as the working app/eink_test (the level
 * translator timing depends on it). It deliberately does NOT re-clock to
 * 150 MHz yet, so the SMIF XIP interface stays on the proven CLKPATH0 path.
 * TODO(eink_test/app_ext): try re-clocking to 150 MHz (PLL on CLKPATH1,
 * keeping CLKPATH0 for the SMIF) after the display is confirmed at 100 MHz.
 *
 * No FreeRTOS / emWin: a main loop cycles four reduced patterns (checkerboard
 * / horizontal bars / vertical bars / box) and blinks LED0 (P11.1).
 *
 * EPD pin map (CY8CKIT-062-BLE + CY8CKIT-028-EPD):
 *   MOSI P12[0] (D11)  MISO P12[1] (D12)  SCLK P12[2] (D13)
 *   CS   P12[3] (D10)  RST  P5[2]  (D2)   BUSY P5[3]   (D3)
 *   EN   P5[4]  (D4)   DISCHARGE P5[5] (D5) BORDER P5[6] (D6)
 *   IOEN P0[2]  (D7)   (IOEN active-low: LOW enables the level translator)
 */

#include "cyhal.h"
#include "cy_syslib.h"
#include "cy_wdt.h"
#include "system_psoc6.h"
#include "mtb_e2271cs021.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

extern void uart_init(void);

#define EPD_WIDTH       MTB_E2271CS021_DISPLAY_SIZE_X
#define EPD_HEIGHT      MTB_E2271CS021_DISPLAY_SIZE_Y
#define EPD_ROW_BYTES   (MTB_E2271CS021_DISPLAY_SIZE_X / 8u)
#define FRAME_BYTES     MTB_E2271CS021_FRAME_SIZE
#define PATTERN_DELAY_MS 4000u

static void delay_ms(uint32_t ms)
{
    while (ms-- > 0u)
    {
        Cy_SysLib_DelayUs(1000u);
    }
}

static const mtb_e2271cs021_pins_t eink_pins =
{
    .spi_mosi  = P12_0,
    .spi_miso  = P12_1,
    .spi_sclk  = P12_2,
    .spi_cs    = P12_3,
    .reset     = P5_2,
    .busy      = P5_3,
    .discharge = P5_5,
    .enable    = P5_4,
    .border    = P5_6,
    .io_enable = P0_2,
};

static uint8_t frame[FRAME_BYTES];
static uint8_t prev_frame[FRAME_BYTES];

static void set_px(uint32_t x, uint32_t y, bool white)
{
    uint32_t idx = (y * EPD_ROW_BYTES) + (x >> 3u);
    uint8_t  bit = (uint8_t)(0x80u >> (x & 7u));
    if (white)
    {
        frame[idx] |= bit;
    }
    else
    {
        frame[idx] &= (uint8_t)~bit;
    }
}

static void fill_white(void)
{
    memset(frame, 0xFF, sizeof(frame));
}

static void pattern_checkerboard(void)
{
    fill_white();
    for (uint32_t y = 0u; y < EPD_HEIGHT; y++)
    {
        for (uint32_t x = 0u; x < EPD_WIDTH; x++)
        {
            if ((((x >> 4u) + (y >> 4u)) & 1u) != 0u)
            {
                set_px(x, y, false);
            }
        }
    }
}

static void pattern_bars_h(void)
{
    fill_white();
    for (uint32_t y = 0u; y < EPD_HEIGHT; y++)
    {
        if (((y / 16u) & 1u) == 0u)
        {
            for (uint32_t x = 0u; x < EPD_WIDTH; x++)
            {
                set_px(x, y, false);
            }
        }
    }
}

static void pattern_bars_v(void)
{
    fill_white();
    for (uint32_t x = 0u; x < EPD_WIDTH; x++)
    {
        if (((x / 16u) & 1u) == 0u)
        {
            for (uint32_t y = 0u; y < EPD_HEIGHT; y++)
            {
                set_px(x, y, false);
            }
        }
    }
}

static void pattern_box(void)
{
    fill_white();
    for (uint32_t x = 0u; x < EPD_WIDTH; x++)
    {
        set_px(x, 0u, false);
        set_px(x, EPD_HEIGHT - 1u, false);
    }
    for (uint32_t y = 0u; y < EPD_HEIGHT; y++)
    {
        set_px(0u, y, false);
        set_px(EPD_WIDTH - 1u, y, false);
    }
}

int main(void)
{
    Cy_WDT_Unlock();
    Cy_WDT_Disable();

    /* Keep the clock as the CM0+ XIP stub set it (FLL @ 100 MHz, SMIF XIP on
     * CLKPATH0). No re-clock yet - see the file header TODO. */
    uart_init();

    printf("\r\n=== eink_test [app_ext] CM4 @ 100 MHz (XIP) ===\r\n");

    cyhal_spi_t spi;
    cy_rslt_t rslt = cyhal_spi_init(&spi, eink_pins.spi_mosi, eink_pins.spi_miso,
                                    eink_pins.spi_sclk, NC, NULL, 8,
                                    CYHAL_SPI_MODE_00_MSB, false);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("spi_init FAIL 0x%08lX\r\n", (unsigned long)rslt);
    }
    rslt = cyhal_spi_set_frequency(&spi, 20000000u);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("SPI clock note: using cyhal default frequency\r\n");
    }

    rslt = mtb_e2271cs021_init(&eink_pins, &spi);
    printf("EPD init rslt 0x%08lX\r\n", (unsigned long)rslt);
    mtb_e2271cs021_set_temp_factor(20);

    cyhal_gpio_init(P11_1, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, true);

    for (;;)
    {
        pattern_checkerboard();
        printf("show checkerboard\r\n");
        mtb_e2271cs021_show_frame(prev_frame, frame, MTB_E2271CS021_FULL_4STAGE, true);
        memcpy(prev_frame, frame, FRAME_BYTES);
        cyhal_gpio_toggle(P11_1);
        delay_ms(PATTERN_DELAY_MS);

        pattern_bars_h();
        printf("show horizontal bars\r\n");
        mtb_e2271cs021_show_frame(prev_frame, frame, MTB_E2271CS021_FULL_4STAGE, true);
        memcpy(prev_frame, frame, FRAME_BYTES);
        cyhal_gpio_toggle(P11_1);
        delay_ms(PATTERN_DELAY_MS);

        pattern_bars_v();
        printf("show vertical bars\r\n");
        mtb_e2271cs021_show_frame(prev_frame, frame, MTB_E2271CS021_FULL_4STAGE, true);
        memcpy(prev_frame, frame, FRAME_BYTES);
        cyhal_gpio_toggle(P11_1);
        delay_ms(PATTERN_DELAY_MS);

        pattern_box();
        printf("show box\r\n");
        mtb_e2271cs021_show_frame(prev_frame, frame, MTB_E2271CS021_FULL_4STAGE, true);
        memcpy(prev_frame, frame, FRAME_BYTES);
        cyhal_gpio_toggle(P11_1);
        delay_ms(PATTERN_DELAY_MS);
    }
}
