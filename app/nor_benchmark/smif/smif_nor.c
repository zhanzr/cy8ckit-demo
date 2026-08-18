#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_gpio.h"
#include "system_psoc6.h"
#include <stdio.h>
#include <string.h>

#include "smif_nor.h"
#include "utils.h"

/* ---------------------------------------------------------------------------
 * The on-board S25FL512S SPI NOR is connected to:
 *   SCK  = P11.7   CS  = P11.2   SI(IO0) = P11.6
 *   SO(IO1) = P11.5   WP(IO2) = P11.4   HOLD(IO3) = P11.3
 *
 * The SMIF hardware block would not drive the bus in the direct-PDL setup
 * (write-enable "completed" but never reached the flash -- WEL stayed 0),
 * so this driver bit-bangs the SPI interface on the same pins. The flash
 * itself is verified working (JEDEC ID 0x01 0x02 0x20 = S25FL512S).
 *
 * NOTE: This measures GPIO bit-bang throughput, NOT the SMIF hardware speed.
 * ------------------------------------------------------------------------- */

#define NOR_CMD_READ_STATUS     0x05u   /* read status register 1 */
#define NOR_CMD_WRITE_ENABLE    0x06u
#define NOR_CMD_PAGE_PROGRAM    0x02u   /* page program, 4-byte address */
#define NOR_CMD_SECTOR_ERASE    0xDCu   /* sector erase 4KB? no: 256 KB sectors, 4-byte addr */

#define NOR_STATUS_WIP          (0x01u) /* write-in-progress bit */
#define NOR_STATUS_WEL          (0x02u) /* write-enable-latch bit */
#define NOR_SECTOR_SIZE         (256u * 1024u)   /* S25FL512S 256 KB sectors */
#define NOR_PAGE_SIZE           (256u)           /* program page */
#define NOR_BUF_SIZE            (4096u)

/* ---- benchmark parameters ---- */
#define NOR_ERASE_SIZE   (256u * 1024u)   /* one 256 KB sector */
#define NOR_TOTAL_SIZE   (256u * 1024u)   /* total bytes programmed / read */
#define NOR_CHUNK_SIZE   (4096u)

static uint8_t nor_buf[NOR_CHUNK_SIZE];

static void spi_delay(void)
{
    /* a few NOPs to stretch the bit clock a little */
    __asm volatile("nop");
    __asm volatile("nop");
}

static void spi_cs(uint32_t level)
{
    Cy_GPIO_Write(GPIO_PRT11, 2u, level);   /* CS  = P11.2 */
}

static void spi_clk(uint32_t level)
{
    Cy_GPIO_Write(GPIO_PRT11, 7u, level);   /* SCK = P11.7 */
}

static void spi_mosi(uint32_t level)
{
    Cy_GPIO_Write(GPIO_PRT11, 6u, level);   /* SI  = P11.6 */
}

static uint32_t spi_miso(void)
{
    return Cy_GPIO_Read(GPIO_PRT11, 5u);    /* SO  = P11.5 */
}

static void spi_send_byte(uint8_t b)
{
    for (int bit = 7; bit >= 0; bit--)
    {
        spi_mosi((b >> bit) & 1u);
        spi_clk(1u);
        spi_delay();
        spi_clk(0u);
        spi_delay();
    }
}

static uint8_t spi_recv_byte(void)
{
    uint8_t val = 0u;
    for (int bit = 7; bit >= 0; bit--)
    {
        spi_clk(1u);
        spi_delay();
        val |= (uint8_t)(spi_miso() << bit);
        spi_clk(0u);
        spi_delay();
    }
    return val;
}

static void spi_send_addr(uint32_t addr)
{
    spi_send_byte((uint8_t)(addr >> 24) & 0xFFu);
    spi_send_byte((uint8_t)(addr >> 16) & 0xFFu);
    spi_send_byte((uint8_t)(addr >> 8) & 0xFFu);
    spi_send_byte((uint8_t)(addr) & 0xFFu);
}

static void nor_init_pins(void)
{
    static const cy_stc_gpio_pin_config_t out_cfg = {
        .outVal = 0u, .driveMode = CY_GPIO_DM_STRONG, .hsiom = HSIOM_SEL_GPIO,
    };
    static const cy_stc_gpio_pin_config_t in_cfg = {
        .outVal = 0u, .driveMode = CY_GPIO_DM_HIGHZ, .hsiom = HSIOM_SEL_GPIO,
    };

    Cy_GPIO_Pin_Init(GPIO_PRT11, 7u, &out_cfg);  /* SCK */
    Cy_GPIO_Pin_Init(GPIO_PRT11, 6u, &out_cfg);  /* MOSI */
    Cy_GPIO_Pin_Init(GPIO_PRT11, 5u, &in_cfg);   /* MISO */
    Cy_GPIO_Pin_Init(GPIO_PRT11, 2u, &out_cfg);  /* CS  */
    /* WP#/HOLD# (P11.4/P11.3) left as-is (strong GPIO high default is fine) */
    Cy_GPIO_Write(GPIO_PRT11, 4u, 1u);
    Cy_GPIO_Write(GPIO_PRT11, 3u, 1u);
    Cy_GPIO_Write(GPIO_PRT11, 2u, 1u);
}

static uint8_t nor_read_status(void)
{
    uint8_t st;
    spi_cs(0u);
    spi_send_byte(NOR_CMD_READ_STATUS);
    st = spi_recv_byte();
    spi_cs(1u);
    return st;
}

static void nor_wait_not_busy(void)
{
    while ((nor_read_status() & NOR_STATUS_WIP) != 0u)
    {
    }
}

static void nor_write_enable(void)
{
    spi_cs(0u);
    spi_send_byte(NOR_CMD_WRITE_ENABLE);
    spi_cs(1u);
}

static int nor_erase_sector(uint32_t addr)
{
    nor_write_enable();
    spi_cs(0u);
    spi_send_byte(NOR_CMD_SECTOR_ERASE);
    spi_send_addr(addr);
    spi_cs(1u);
    nor_wait_not_busy();
    return 0;
}

static int nor_page_program(uint32_t addr, const uint8_t *data, uint32_t len)
{
    nor_write_enable();
    spi_cs(0u);
    spi_send_byte(NOR_CMD_PAGE_PROGRAM);
    spi_send_addr(addr);
    for (uint32_t i = 0u; i < len; i++)
    {
        spi_send_byte(data[i]);
    }
    spi_cs(1u);
    nor_wait_not_busy();
    return 0;
}

static int nor_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    spi_cs(0u);
    spi_send_byte(0x03u);   /* read data, 4-byte address */
    spi_send_addr(addr);
    for (uint32_t i = 0u; i < len; i++)
    {
        buf[i] = spi_recv_byte();
    }
    spi_cs(1u);
    return 0;
}

static uint32_t kb_per_sec(uint32_t bytes, uint32_t ms)
{
    if (ms == 0u)
    {
        ms = 1u;
    }
    return (uint32_t)(((uint64_t)bytes * 1000u) / (ms * 1024u));
}

void smif_nor_init(void)
{
    nor_init_pins();
}

void smif_nor_benchmark(const char *core_label)
{
    uint32_t t0, t1, ms;

    printf("\r\n=== SPI NOR (S25FL512S) benchmark [%s] @ %lu Hz (GPIO bit-bang) ===\r\n",
           core_label, (unsigned long)SystemCoreClock);

    /* Erase one 256 KB sector at address 0. */
    t0 = HAL_GetTick();
    nor_erase_sector(0u);
    t1 = HAL_GetTick();
    ms = t1 - t0;
    printf("  Erase   %u KB: %lu ms (%lu KB/s)\r\n",
           (unsigned)(NOR_ERASE_SIZE / 1024u), (unsigned long)ms,
           (unsigned long)kb_per_sec(NOR_ERASE_SIZE, ms));

    /* Program NOR_TOTAL_SIZE bytes, one 256-byte page at a time. */
    memset(nor_buf, 0x5Au, sizeof(nor_buf));
    t0 = HAL_GetTick();
    for (uint32_t addr = 0u; addr < NOR_TOTAL_SIZE; addr += NOR_PAGE_SIZE)
    {
        nor_page_program(addr, nor_buf, NOR_PAGE_SIZE);
    }
    t1 = HAL_GetTick();
    ms = t1 - t0;
    printf("  Program %u KB: %lu ms (%lu KB/s)\r\n",
           (unsigned)(NOR_TOTAL_SIZE / 1024u), (unsigned long)ms,
           (unsigned long)kb_per_sec(NOR_TOTAL_SIZE, ms));

    /* Read NOR_TOTAL_SIZE bytes back. */
    memset(nor_buf, 0u, sizeof(nor_buf));
    t0 = HAL_GetTick();
    for (uint32_t addr = 0u; addr < NOR_TOTAL_SIZE; addr += NOR_CHUNK_SIZE)
    {
        nor_read(addr, nor_buf, NOR_CHUNK_SIZE);
    }
    t1 = HAL_GetTick();
    ms = t1 - t0;
    printf("  Read    %u KB: %lu ms (%lu KB/s)\r\n",
           (unsigned)(NOR_TOTAL_SIZE / 1024u), (unsigned long)ms,
           (unsigned long)kb_per_sec(NOR_TOTAL_SIZE, ms));

    printf("  Benchmark complete [%s]\r\n", core_label);
}
