/******************************************************************************
* File Name: main.c
*
* Description: Self-contained PSoC 6 QSPI XIP test.
*  The CM4 app:
*   1. Initializes the SMIF via the HAL (cy_serial_flash_qspi).
*   2. Programs a string into the external NOR at 0x18000000 (this tests the
*      external-NOR flashing path).
*   3. Puts the SMIF into XIP mode.
*   4. Reads the string back through the memory-mapped (XIP) address and
*      verifies it (this tests XIP read).
*******************************************************************************/

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "cy_serial_flash_qspi.h"
#include "cycfg_qspi_memslot.h"
#include <string.h>

#define PACKET_SIZE             (64u)
#define MEM_SLOT_NUM            (0u)
#define QSPI_BUS_FREQUENCY_HZ   (50000000lu)

/* This string is written to the external NOR (device address 0) and read
 * back via the XIP-mapped address 0x18000000 (device address 0). */
#define XIP_TEST_STRING         "Hello from the external NOR via XIP!\n"
#define XIP_TEST_DEV_ADDR       (0x00000000u)   /* flash device address 0 */
#define XIP_TEST_MEM_ADDR       (0x18000000u)   /* XIP mapping of device 0 */

static void check_status(char *message, uint32_t status)
{
    if (status != 0u)
    {
        printf("\nFAIL: %s (status 0x%08lX)\n", message, (unsigned long)status);
        while (true) {}
    }
}

int main(void)
{
    cy_rslt_t result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    __enable_irq();

    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

    printf("\x1b[2J\x1b[;H");
    printf("*************** PSoC 6: XIP test (external NOR) ***************\n\n");

    /* 1. Initialize the SMIF via the HAL. */
    result = cy_serial_flash_qspi_init(smifMemConfigs[MEM_SLOT_NUM],
            CYBSP_QSPI_D0, CYBSP_QSPI_D1, CYBSP_QSPI_D2, CYBSP_QSPI_D3, NC, NC,
            NC, NC, CYBSP_QSPI_SCK, CYBSP_QSPI_SS, QSPI_BUS_FREQUENCY_HZ);
    check_status("Serial Flash initialization failed", result);
    printf("1. SMIF initialized, total flash: %lu bytes\n",
           (unsigned long)cy_serial_flash_qspi_get_size());

    /* 2. Program a string into the external NOR (device address 0). */
    printf("2. Programming string to external NOR (device addr 0x%08lX)...\n",
           (unsigned long)XIP_TEST_DEV_ADDR);
    size_t sectorSize = cy_serial_flash_qspi_get_erase_size(XIP_TEST_DEV_ADDR);
    result = cy_serial_flash_qspi_erase(XIP_TEST_DEV_ADDR, sectorSize);
    check_status("Erase failed", result);
    result = cy_serial_flash_qspi_write(XIP_TEST_DEV_ADDR, strlen(XIP_TEST_STRING) + 1u,
                                        (const uint8_t *)XIP_TEST_STRING);
    check_status("Write failed", result);
    printf("   Erased + wrote %u bytes.\n", (unsigned)(strlen(XIP_TEST_STRING) + 1u));

    /* 3. Enter XIP mode. */
    printf("3. Entering XIP mode...\n");
    cy_serial_flash_qspi_enable_xip(true);

    /* 4. Read the string back via the memory-mapped (XIP) address. */
    printf("4. Reading back via XIP address 0x%08lX...\n", (unsigned long)XIP_TEST_MEM_ADDR);
    const char *xip_str = (const char *)XIP_TEST_MEM_ADDR;
    printf("   XIP read: \"%s\"\n", xip_str);

    if (strncmp(xip_str, XIP_TEST_STRING, strlen(XIP_TEST_STRING)) == 0)
    {
        printf("\n================================================================================\n");
        printf("SUCCESS: XIP read matches the programmed string! (external NOR + XIP work)\n");
        printf("================================================================================\n");
    }
    else
    {
        printf("\nFAIL: XIP read does not match.\n");
    }

    for (;;)
    {
        cyhal_gpio_toggle(CYBSP_USER_LED);
        cyhal_system_delay_ms(500);
    }
}
