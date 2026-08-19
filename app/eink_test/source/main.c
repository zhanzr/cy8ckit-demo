/* app/eink_test - E2271CS021 E-ink demo on CY8CKIT-062-BLE + CY8CKIT-028-EPD.
 *
 * A port of the official mtb-example-psoc6-emwin-eink-freertos but WITHOUT
 * the FreeRTOS and emWin dependencies: it drives the display directly via
 * cyhal_spi (SCB6, P12) + the display-eink-e2271cs021 driver and cycles a few
 * reduced-content patterns (checkerboard / horizontal bars / vertical bars /
 * box).
 *
 * EPD pin map (CY8CKIT-028-EPD shield on CY8CKIT-062-BLE):
 *   MOSI P12[0] (D11)  MISO P12[1] (D12)  SCLK P12[2] (D13)
 *   CS   P12[3] (D10)  RST  P5[2]  (D2)   BUSY P5[3]   (D3)
 *   EN   P5[4]  (D4)   DISCHARGE P5[5] (D5) BORDER P5[6] (D6)
 *   IOEN P0[2]  (D7)   (IOEN is active-low: LOW enables the level translator)
 */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "mtb_e2271cs021.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define EPD_WIDTH       MTB_E2271CS021_DISPLAY_SIZE_X
#define EPD_HEIGHT      MTB_E2271CS021_DISPLAY_SIZE_Y
#define EPD_ROW_BYTES   (MTB_E2271CS021_DISPLAY_SIZE_X / 8u)
#define FRAME_BYTES     MTB_E2271CS021_FRAME_SIZE

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
    cy_rslt_t result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                                 CY_RETARGET_IO_BAUDRATE);
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    __enable_irq();

    printf("\r\n=== eink_test: E2271CS021 on CY8CKIT-062-BLE (no RTOS/emWin) ===\r\n");

    cyhal_spi_t spi;
    result = cyhal_spi_init(&spi, eink_pins.spi_mosi, eink_pins.spi_miso,
                            eink_pins.spi_sclk, NC, NULL, 8,
                            CYHAL_SPI_MODE_00_MSB, false);
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }
    result = cyhal_spi_set_frequency(&spi, 20000000u);
    if (result != CY_RSLT_SUCCESS)
    {
        /* The SCB6 clock divider is not pre-assigned by this BSP config; the
         * SPI still runs at cyhal's default frequency, so continue. */
        printf("SPI clock note: using cyhal default frequency\r\n");
    }

    result = mtb_e2271cs021_init(&eink_pins, &spi);
    printf("EPD init rslt 0x%08lX\r\n", (unsigned long)result);
    mtb_e2271cs021_set_temp_factor(20);

    for (;;)
    {
        pattern_checkerboard();
        printf("show checkerboard\r\n");
        mtb_e2271cs021_show_frame(prev_frame, frame, MTB_E2271CS021_FULL_4STAGE, true);
        memcpy(prev_frame, frame, FRAME_BYTES);
        cyhal_system_delay_ms(4000);

        pattern_bars_h();
        printf("show horizontal bars\r\n");
        mtb_e2271cs021_show_frame(prev_frame, frame, MTB_E2271CS021_FULL_4STAGE, true);
        memcpy(prev_frame, frame, FRAME_BYTES);
        cyhal_system_delay_ms(4000);

        pattern_bars_v();
        printf("show vertical bars\r\n");
        mtb_e2271cs021_show_frame(prev_frame, frame, MTB_E2271CS021_FULL_4STAGE, true);
        memcpy(prev_frame, frame, FRAME_BYTES);
        cyhal_system_delay_ms(4000);

        pattern_box();
        printf("show box\r\n");
        mtb_e2271cs021_show_frame(prev_frame, frame, MTB_E2271CS021_FULL_4STAGE, true);
        memcpy(prev_frame, frame, FRAME_BYTES);
        cyhal_system_delay_ms(4000);
    }
}
