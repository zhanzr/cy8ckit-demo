#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_gpio.h"
#include "nor_bb.h"

/* S25FL512S commands (4-byte address variants, work regardless of the
 * flash's default 3-byte address mode) */
#define NOR_CMD_READ_STATUS     0x05u
#define NOR_CMD_WRITE_ENABLE    0x06u
#define NOR_CMD_PAGE_PROGRAM    0x12u   /* 4-byte-address page program */
#define NOR_CMD_SECTOR_ERASE    0xDCu   /* 4-byte-address sector erase */
#define NOR_CMD_READ_4B         0x13u   /* 4-byte-address read */

#define NOR_STATUS_WIP          (0x01u)
#define NOR_SECTOR_SIZE         (256u * 1024u)
#define NOR_PAGE_SIZE           (256u)

static void spi_delay(void)
{
    __asm volatile("nop");
    __asm volatile("nop");
}

static void spi_cs(uint32_t level)  { Cy_GPIO_Write(GPIO_PRT11, 2u, level); }
static void spi_clk(uint32_t level) { Cy_GPIO_Write(GPIO_PRT11, 7u, level); }
static void spi_mosi(uint32_t level){ Cy_GPIO_Write(GPIO_PRT11, 6u, level); }
static uint32_t spi_miso(void)      { return Cy_GPIO_Read(GPIO_PRT11, 5u); }

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

void nor_bb_init(void)
{
    static const cy_stc_gpio_pin_config_t out_cfg = {
        .outVal = 0u, .driveMode = CY_GPIO_DM_STRONG, .hsiom = HSIOM_SEL_GPIO,
    };
    static const cy_stc_gpio_pin_config_t in_cfg = {
        .outVal = 0u, .driveMode = CY_GPIO_DM_HIGHZ, .hsiom = HSIOM_SEL_GPIO,
    };

    Cy_GPIO_Pin_Init(GPIO_PRT11, 7u, &out_cfg);
    Cy_GPIO_Pin_Init(GPIO_PRT11, 6u, &out_cfg);
    Cy_GPIO_Pin_Init(GPIO_PRT11, 5u, &in_cfg);
    Cy_GPIO_Pin_Init(GPIO_PRT11, 2u, &out_cfg);
    Cy_GPIO_Write(GPIO_PRT11, 4u, 1u);  /* WP# high */
    Cy_GPIO_Write(GPIO_PRT11, 3u, 1u);  /* HOLD# high */
    Cy_GPIO_Write(GPIO_PRT11, 2u, 1u);  /* CS high */
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

int nor_bb_program(uint32_t addr, const uint8_t *data, uint32_t len)
{
    (void)nor_erase_sector(addr);
    while (len > 0u)
    {
        uint32_t chunk = (len > NOR_PAGE_SIZE) ? NOR_PAGE_SIZE : len;
        /* do not cross a 256-byte page boundary */
        uint32_t page_left = NOR_PAGE_SIZE - (addr % NOR_PAGE_SIZE);
        if (chunk > page_left)
        {
            chunk = page_left;
        }
        (void)nor_page_program(addr, data, chunk);
        addr += chunk;
        data += chunk;
        len  -= chunk;
    }
    return 0;
}

int nor_bb_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    spi_cs(0u);
    spi_send_byte(NOR_CMD_READ_4B);   /* read data, 4-byte address */
    spi_send_addr(addr);
    for (uint32_t i = 0u; i < len; i++)
    {
        buf[i] = spi_recv_byte();
    }
    spi_cs(1u);
    return 0;
}
