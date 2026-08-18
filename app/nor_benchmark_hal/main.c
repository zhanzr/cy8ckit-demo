/*******************************************************************************
* File Name: main.c
*
* Description: SPI NOR (S25FL512S) speed benchmark using the HAL SMIF driver.
* Runs on the CM4 at 100 MHz, measures erase/program/read throughput and
* reports KB/s on the UART.
*******************************************************************************/

#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "cy_serial_flash_qspi.h"
#include "cycfg_qspi_memslot.h"
#include <string.h>

#define PACKET_SIZE             (64u)
#define NUM_BYTES_PER_LINE      (16u)
#define QSPI_BUS_FREQUENCY_HZ   (50000000lu)
#define MEM_SLOT_NUM            (0u)

/* Benchmark size: 2 x 256 KB sectors = 512 KB */
#define BENCH_SIZE              (512u * 1024u)
#define BENCH_ERASE_SECTOR      (256u * 1024u)
#define BENCH_BUF_SIZE          (4096u)

static uint8_t tx_buf[BENCH_BUF_SIZE];
static uint8_t rx_buf[BENCH_BUF_SIZE];

static uint32_t get_ms(void)
{
    /* DWT cycle counter -> milliseconds (SystemCoreClock is the CPU clock) */
    return (uint32_t)(DWT->CYCCNT / (SystemCoreClock / 1000u));
}

static uint32_t kb_per_sec(uint32_t bytes, uint32_t ms)
{
    if (ms == 0u) { ms = 1u; }
    return (uint32_t)(((uint64_t)bytes * 1000u) / (ms * 1024u));
}

int main(void)
{
    cy_rslt_t result = cybsp_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    __enable_irq();

    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
                                 CY_RETARGET_IO_BAUDRATE);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    printf("\x1b[2J\x1b[;H");
    printf("****************** SPI NOR (S25FL512S) speed benchmark (HAL SMIF) ******************\r\n");

    /* Enable the DWT cycle counter for timing. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0u;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    result = cy_serial_flash_qspi_init(smifMemConfigs[MEM_SLOT_NUM],
            CYBSP_QSPI_D0, CYBSP_QSPI_D1, CYBSP_QSPI_D2, CYBSP_QSPI_D3, NC, NC,
            NC, NC, CYBSP_QSPI_SCK, CYBSP_QSPI_SS, QSPI_BUS_FREQUENCY_HZ);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    printf("\r\nTotal Flash Size: %lu bytes\r\n",
           (unsigned long)cy_serial_flash_qspi_get_size());
    printf("Benchmark: %u KB (erase %u KB sectors, page program, read)\r\n",
           (unsigned)(BENCH_SIZE / 1024u),
           (unsigned)(BENCH_ERASE_SECTOR / 1024u));

    /* ---- Erase ---- */
    printf("\r\n1. Erasing %u KB (2 sectors)...\r\n", (unsigned)(BENCH_SIZE / 1024u));
    uint32_t t0 = get_ms();
    result = cy_serial_flash_qspi_erase(0u, BENCH_SIZE);
    uint32_t t1 = get_ms();
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    printf("   Erase:  %lu ms  (%lu KB/s)\r\n", (unsigned long)(t1 - t0),
           (unsigned long)kb_per_sec(BENCH_SIZE, t1 - t0));

    /* ---- Program ---- */
    printf("2. Programming %u KB...\r\n", (unsigned)(BENCH_SIZE / 1024u));
    memset(tx_buf, 0x5Au, sizeof(tx_buf));
    t0 = get_ms();
    for (uint32_t addr = 0u; addr < BENCH_SIZE; addr += BENCH_BUF_SIZE)
    {
        result = cy_serial_flash_qspi_write(addr, BENCH_BUF_SIZE, tx_buf);
        CY_ASSERT(result == CY_RSLT_SUCCESS);
    }
    t1 = get_ms();
    printf("   Program: %lu ms  (%lu KB/s)\r\n", (unsigned long)(t1 - t0),
           (unsigned long)kb_per_sec(BENCH_SIZE, t1 - t0));

    /* ---- Read ---- */
    printf("3. Reading %u KB...\r\n", (unsigned)(BENCH_SIZE / 1024u));
    memset(rx_buf, 0u, sizeof(rx_buf));
    t0 = get_ms();
    for (uint32_t addr = 0u; addr < BENCH_SIZE; addr += BENCH_BUF_SIZE)
    {
        result = cy_serial_flash_qspi_read(addr, BENCH_BUF_SIZE, rx_buf);
        CY_ASSERT(result == CY_RSLT_SUCCESS);
    }
    t1 = get_ms();
    printf("   Read:   %lu ms  (%lu KB/s)\r\n", (unsigned long)(t1 - t0),
           (unsigned long)kb_per_sec(BENCH_SIZE, t1 - t0));

    printf("\r\nSUCCESS: SPI NOR benchmark complete (HAL SMIF, 50 MHz)\r\n");

    for (;;)
    {
        cyhal_gpio_toggle(CYBSP_USER_LED);
        cyhal_system_delay_ms(500);
    }
}
