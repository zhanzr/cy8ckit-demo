/*
 * ntc_test - NTC thermistor + internal DTS temperature demo.
 * CY8CKIT-062-BLE + CY8CKIT-028-EPD.
 */

#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "cy_syslib.h"
#include "ntc.h"
#include <stdio.h>

#define SAMPLE_DELAY_MS    (1000u)

int main(void)
{
    cy_rslt_t result;

    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    __enable_irq();

    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                                 CY_RETARGET_IO_BAUDRATE);
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    printf("\x1b[2J\x1b[;H");
    printf("****************************************\r\n");
    printf("  NTC thermistor + DTS temperature test\r\n");
    printf("****************************************\r\n\n");

    ntc_init();

    for (;;)
    {
        uint16_t a0, a1, a2, a3, dts;
        ntc_read_raw(&a0, &a1, &a2, &a3, &dts);
        uint16_t m0, m1, m2, m3;
        ntc_read_mv(&m0, &m1, &m2, &m3);
        float t_ntc = ntc_temp_from_raw(a0, a1, a2, a3);
        int16_t t_dts = dts_read_celsius();

        printf("A0=%u A1=%u A2=%u A3=%u DTS=%u\r\n",
               (unsigned)a0, (unsigned)a1, (unsigned)a2, (unsigned)a3, (unsigned)dts);
        printf("mV: %u %u %u %u   NTC: %6.2f C   DTS: %4d C\r\n",
               (unsigned)m0, (unsigned)m1, (unsigned)m2, (unsigned)m3,
               (double)t_ntc, (int)t_dts);

        Cy_SysLib_DelayUs(SAMPLE_DELAY_MS * 1000u);
    }
}
