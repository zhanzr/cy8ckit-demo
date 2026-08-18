/*
 * flash_s25fl512s_smif.c - S25FL512S (64 MB) probe-rs flash algorithm for the
 * CY8C6347 (CY8CKIT-062) via the PSoC6 SMIF peripheral.
 *
 * Register-level, position-independent (no globals, no libc). Runs ON the
 * target CPU: probe-rs loads it into SRAM and calls Init/EraseSector/
 * ProgramPage/etc. The exact same file is compiled into the ../harness
 * firmware to debug it on hardware (results over SCB5 UART instead of
 * cryptic probe-rs error codes).
 *
 * SMIF in command mode, single-SPI (1-1-1), 4-byte addresses:
 *   - JEDEC ID  0x9F  (3 bytes RX)
 *   - Read      0x13  (4-byte addr + RX data)
 *   - Program   0x12  (4-byte addr + TX data, 256 B page)
 *   - Erase     0xDC  (4-byte addr, 256 KB sector)
 *   - Chip erase 0x60
 *   - W/R enable 0x06 / 0x04, Read status 0x05 (WIP = bit 0)
 * This matches the CY8CKIT-062 QSPI configurator command set (0xDC/0x34),
 * except data is single-SPI so no QE setup is needed and reads/writes work
 * in any prior device state.
 *
 * Verified against the hardware register map (captured from the working
 * HAL cy_serial_flash_qspi app):
 *   SMIF0_CTL       = 0x80071000  (EN + DESELECT_DELAY=7 + CLOCK_IF_RX_SEL=1)
 *   SMIF0_DEVICE[0] CTL=0x80000001 ADDR=0x18000000 MASK=0xFC000000
 *                    ADDR_CTL=0x03 (SIZE2=3 -> 4 address bytes)
 *   P11[2..7] HSIOM = 17 (SMIF), GPIO_PRT11.CFG = 0xEEEEE600
 */

typedef volatile unsigned long vu32;

#ifdef ALGO_DEBUG
extern int printf(const char *fmt, ...);
#define DBG(...) printf(__VA_ARGS__)
#else
#define DBG(...)
#endif

#define SMIF0_BASE          0x40420000UL
#define SMIF0_CTL           (SMIF0_BASE + 0x00UL)
#define SMIF0_STATUS        (SMIF0_BASE + 0x04UL)
#define SMIF0_TX_CMD_STAT   (SMIF0_BASE + 0x44UL)
#define SMIF0_TX_CMD_FIFO   (SMIF0_BASE + 0x50UL)
#define SMIF0_TX_DATA_STAT  (SMIF0_BASE + 0x84UL)
#define SMIF0_TX_DATA_WR1   (SMIF0_BASE + 0x90UL)
#define SMIF0_TX_DATA_WR2   (SMIF0_BASE + 0x94UL)
#define SMIF0_TX_DATA_WR4   (SMIF0_BASE + 0x98UL)
#define SMIF0_RX_DATA_STAT  (SMIF0_BASE + 0xC4UL)
#define SMIF0_RX_DATA_RD1   (SMIF0_BASE + 0xD0UL)
#define SMIF0_RX_DATA_RD2   (SMIF0_BASE + 0xD4UL)
#define SMIF0_RX_DATA_RD4   (SMIF0_BASE + 0xD8UL)
#define SMIF0_DEV0_CTL      (SMIF0_BASE + 0x800UL)
#define SMIF0_DEV0_ADDR     (SMIF0_BASE + 0x808UL)
#define SMIF0_DEV0_MASK     (SMIF0_BASE + 0x80CUL)
#define SMIF0_DEV0_ADDR_CTL (SMIF0_BASE + 0x820UL)

/* CY8CKIT-062 SMIF pins P11[2..7]: SS0, D3, D2, D1, D0, SCK (HSIOM = 17). */
#define HSIOM_P11_SEL0      0x403100B0UL
#define HSIOM_P11_SEL1      0x403100B4UL
#define GPIO_P11_CFG        0x403205A8UL

/* CM4 SCB SCTLR (D-cache disable in Init). */
#define SCB_SCTLR           0xE000ED88UL

/* SMIF interface clock: SRSS CLK_HF_ROOT_SELECT[2] = CLKPATH0 / 2
 * (0x80000010: ENABLE + ROOT_MUX=0(CLKPATH0) + ROOT_DIV=1(divide-by-2)).
 * 50 MHz when CLKPATH0 = FLL@100 MHz, 4 MHz at the reset IMO@8 MHz. */
#define SRSS_CLK_ROOT_SELECT2  0x40260388UL

#define SMIF_FLASH_BASE     0x18000000UL
#define FLASH_SECTOR        0x00040000UL  /* 256 KB (0xDC) */
#define FLASH_PAGE          0x00000200UL  /* 256 B  (0x12) */

/* TX_CMD_FIFO_WR field positions (MXSMIF v1). */
#define CMD_MODE_TXBYTE     0u
#define CMD_MODE_TXCOUNT    1u
#define CMD_MODE_RXCOUNT    2u

/* ---------------------------------------------------------------------- */

static void smif_wait_cmd_room(void)
{
    unsigned long d;
    for (d = 0; d < 1000000u; d++)
    {
        if (((*(vu32 *)SMIF0_TX_CMD_STAT) & 0x7u) < 7u)
        {
            return;
        }
    }
    DBG("[ALGO] TX_CMD fifo full!\r\n");
}

/* Push one byte (opcode or address) to the TX command FIFO. last=1 deasserts
 * the slave-select after this byte (command-only transfers).
 * SS0 = CY_SMIF_SLAVE_SELECT_0 = 1 -> CMD_FIFO_WR_SS bits [11:8] = 0x100. */
static void smif_cmd_byte(unsigned char b, unsigned int last)
{
    smif_wait_cmd_room();
    *(vu32 *)SMIF0_TX_CMD_FIFO =
        0x100u | (unsigned long)b | (last ? 0x8000u : 0u);
    DBG("[ALGO] C %02X%s\r\n", b, last ? " L" : "");
}

/* Start a TX data phase: transmit `n` bytes from the TX data FIFO. */
static void smif_tx_count(unsigned long n)
{
    smif_wait_cmd_room();
    *(vu32 *)SMIF0_TX_CMD_FIFO =
        (CMD_MODE_TXCOUNT << 18) | ((n - 1UL) & 0xFFFFu);
    DBG("[ALGO] TXN %lu\r\n", n);
}

/* Start an RX data phase: receive `n` bytes into the RX data FIFO. */
static void smif_rx_count(unsigned long n)
{
    smif_wait_cmd_room();
    *(vu32 *)SMIF0_TX_CMD_FIFO =
        (CMD_MODE_RXCOUNT << 18) | ((n - 1UL) & 0xFFFFu);
    DBG("[ALGO] RXN %lu\r\n", n);
}

/* Wait until the SMIF finishes the current transfer (STATUS.BUSY = 0). */
static void smif_wait_idle(void)
{
    unsigned long d;
    for (d = 0; d < 2000000u; d++)
    {
        if (!(*(vu32 *)SMIF0_STATUS & 0x80000000UL))
        {
            return;
        }
    }
    DBG("[ALGO] NOT IDLE! STATUS=0x%08lX\r\n", *(vu32 *)SMIF0_STATUS);
}

/* Push payload bytes to the TX data FIFO (8-byte FIFO, drain as we go). */
static void smif_tx_bytes(const unsigned char *buf, unsigned long n)
{
    unsigned long i = 0;
    while (i < n)
    {
        unsigned long left = n - i;
        if (left >= 4u)
        {
            while (((*(vu32 *)SMIF0_TX_DATA_STAT) & 0xFu) > 4u) {}
            *(vu32 *)SMIF0_TX_DATA_WR4 =
                (unsigned long)buf[i]
                | ((unsigned long)buf[i + 1u] << 8)
                | ((unsigned long)buf[i + 2u] << 16)
                | ((unsigned long)buf[i + 3u] << 24);
            i += 4u;
        }
        else if (left >= 2u)
        {
            while (((*(vu32 *)SMIF0_TX_DATA_STAT) & 0xFu) > 6u) {}
            *(vu32 *)SMIF0_TX_DATA_WR2 =
                (unsigned long)buf[i] | ((unsigned long)buf[i + 1u] << 8);
            i += 2u;
        }
        else
        {
            while (((*(vu32 *)SMIF0_TX_DATA_STAT) & 0xFu) > 7u) {}
            *(vu32 *)SMIF0_TX_DATA_WR1 = (unsigned long)buf[i];
            i += 1u;
        }
    }
}

/* Read payload bytes from the RX data FIFO (8-byte FIFO). */
static void smif_rx_bytes(unsigned char *buf, unsigned long n)
{
    unsigned long i = 0;
    while (i < n)
    {
        unsigned long left = n - i;
        unsigned long d;
        if (left >= 4u)
        {
            unsigned long v;
            for (d = 0; d < 1000000u; d++)
            {
                if (((*(vu32 *)SMIF0_RX_DATA_STAT) & 0xFu) >= 4u)
                {
                    break;
                }
            }
            if (d >= 1000000u)
            {
                DBG("[ALGO] RX stuck! want4 got=%lu\r\n",
                    (*(vu32 *)SMIF0_RX_DATA_STAT) & 0xFu);
                return;
            }
            v = *(vu32 *)SMIF0_RX_DATA_RD4;
            buf[i]      = (unsigned char)(v & 0xFFu);
            buf[i + 1u] = (unsigned char)((v >> 8) & 0xFFu);
            buf[i + 2u] = (unsigned char)((v >> 16) & 0xFFu);
            buf[i + 3u] = (unsigned char)((v >> 24) & 0xFFu);
            i += 4u;
        }
        else if (left == 3u)
        {
            unsigned long v;
            for (d = 0; d < 1000000u; d++)
            {
                if (((*(vu32 *)SMIF0_RX_DATA_STAT) & 0xFu) >= 2u)
                {
                    break;
                }
            }
            if (d >= 1000000u)
            {
                DBG("[ALGO] RX stuck! want2 got=%lu\r\n",
                    (*(vu32 *)SMIF0_RX_DATA_STAT) & 0xFu);
                return;
            }
            v = *(vu32 *)SMIF0_RX_DATA_RD2;
            buf[i]      = (unsigned char)(v & 0xFFu);
            buf[i + 1u] = (unsigned char)((v >> 8) & 0xFFu);
            for (d = 0; d < 1000000u; d++)
            {
                if (((*(vu32 *)SMIF0_RX_DATA_STAT) & 0xFu) >= 1u)
                {
                    break;
                }
            }
            if (d >= 1000000u)
            {
                DBG("[ALGO] RX stuck! want1 got=%lu\r\n",
                    (*(vu32 *)SMIF0_RX_DATA_STAT) & 0xFu);
                return;
            }
            buf[i + 2u] = (unsigned char)(*(vu32 *)SMIF0_RX_DATA_RD1);
            i += 3u;
        }
        else if (left == 2u)
        {
            unsigned long v;
            for (d = 0; d < 1000000u; d++)
            {
                if (((*(vu32 *)SMIF0_RX_DATA_STAT) & 0xFu) >= 2u)
                {
                    break;
                }
            }
            if (d >= 1000000u)
            {
                DBG("[ALGO] RX stuck! want2 got=%lu\r\n",
                    (*(vu32 *)SMIF0_RX_DATA_STAT) & 0xFu);
                return;
            }
            v = *(vu32 *)SMIF0_RX_DATA_RD2;
            buf[i]      = (unsigned char)(v & 0xFFu);
            buf[i + 1u] = (unsigned char)((v >> 8) & 0xFFu);
            i += 2u;
        }
        else
        {
            for (d = 0; d < 1000000u; d++)
            {
                if (((*(vu32 *)SMIF0_RX_DATA_STAT) & 0xFu) >= 1u)
                {
                    break;
                }
            }
            if (d >= 1000000u)
            {
                DBG("[ALGO] RX stuck! want1 got=%lu\r\n",
                    (*(vu32 *)SMIF0_RX_DATA_STAT) & 0xFu);
                return;
            }
            buf[i] = (unsigned char)(*(vu32 *)SMIF0_RX_DATA_RD1);
            i += 1u;
        }
    }
}

/* ---------------------------------------------------------------------- */
/* S25FL512S command sequences */

static void s25fl_write_enable(void)
{
    smif_cmd_byte(0x06, 1);
    smif_wait_idle();
}

static int s25fl_read_status(unsigned char *sr)
{
    smif_cmd_byte(0x05, 0);
    smif_rx_count(1);
    smif_wait_idle();
    smif_rx_bytes(sr, 1);
    return 0;
}

static int s25fl_poll_wip(unsigned long timeout)
{
    unsigned long i;
    for (i = 0; i < timeout; i++)
    {
        unsigned char sr;
        s25fl_read_status(&sr);
        if (!(sr & 0x01u))
        {
            return 0;
        }
    }
    return -1;
}

static void s25fl_send_4b_addr(unsigned long off, unsigned int last)
{
    smif_cmd_byte((unsigned char)(off >> 24), 0);
    smif_cmd_byte((unsigned char)(off >> 16), 0);
    smif_cmd_byte((unsigned char)(off >> 8), 0);
    smif_cmd_byte((unsigned char)off, last);
}

static void s25fl_reset(void)
{
    /* Reset the device (0x66/0x99) in case a previous app left it in a
     * continuous-read / QPI / 4-byte-mode state. */
    smif_cmd_byte(0x66, 1);
    smif_wait_idle();
    smif_cmd_byte(0x99, 1);
    smif_wait_idle();
    {
        volatile unsigned long d;
        for (d = 0; d < 50000u; d++) {}
    }
}

static unsigned long s25fl_read_id(void)
{
    unsigned char b[3];
    smif_cmd_byte(0x9F, 0);
    smif_rx_count(3);
    smif_wait_idle();
    smif_rx_bytes(b, 3);
    return ((unsigned long)b[0] << 16) | ((unsigned long)b[1] << 8) | b[2];
}

static int s25fl_erase_sector(unsigned long off)
{
    s25fl_write_enable();
    smif_cmd_byte(0xDC, 0);
    s25fl_send_4b_addr(off, 1);
    smif_wait_idle();
    return s25fl_poll_wip(20000000u);
}

static int s25fl_program(unsigned long off, unsigned long sz,
                         const unsigned char *buf)
{
    while (sz > 0u)
    {
        unsigned long chunk = (sz > FLASH_PAGE) ? FLASH_PAGE : sz;
        s25fl_write_enable();
        smif_cmd_byte(0x12, 0);
        s25fl_send_4b_addr(off, 0);
        smif_tx_count(chunk);
        smif_tx_bytes(buf, chunk);
        smif_wait_idle();
        if (s25fl_poll_wip(5000000u) != 0)
        {
            return -1;
        }
        off += chunk;
        buf += chunk;
        sz -= chunk;
    }
    return 0;
}

static int s25fl_read(unsigned long off, unsigned long sz, unsigned char *buf)
{
    smif_cmd_byte(0x13, 0);
    s25fl_send_4b_addr(off, 0);
    smif_rx_count(sz);
    smif_wait_idle();
    smif_rx_bytes(buf, sz);
    return 0;
}

/* ---------------------------------------------------------------------- */
/* probe-rs flash algorithm entry points (return 0 on success) */

int Init(unsigned long adr, unsigned long clk, unsigned long fnc)
{
    unsigned long id;
    (void)adr;
    (void)clk;
    (void)fnc;

    /* probe-rs writes the payload into SRAM via the debugger; the CM4
     * D-cache must not serve stale lines when the CPU reads it. */
    *(vu32 *)SCB_SCTLR &= ~(1u << 2);
    __asm volatile ("dsb");
    __asm volatile ("isb");

    /* SMIF pins P11[2..7] -> HSIOM 17 (SMIF function).
     * P11[2] SS0, P11[3] D3, P11[4] D2, P11[5] D1, P11[6] D0, P11[7] SCK.
     * HSIOM_SEL is a 5-bit field per pin (17 = 0b10001). */
    *(vu32 *)HSIOM_P11_SEL0 =
        (*(vu32 *)HSIOM_P11_SEL0 & ~0x1F1F0000u) | 0x11110000u;   /* pins 2,3 */
    *(vu32 *)HSIOM_P11_SEL1 =
        (*(vu32 *)HSIOM_P11_SEL1 & ~0x1F1F1F1Fu) | 0x11111111u;   /* pins 4-7 */
    *(vu32 *)GPIO_P11_CFG = 0xEEEEE600u;
    /* Match the HAL's GPIO output latch for the SMIF pins (pins 2-7). */
    *(vu32 *)0x40320580u = 0x000000FCu;   /* GPIO_PRT11 OUT */

    /* Enable the SMIF interface clock (CLK_HF[2] = CLKPATH0 / 2). Without it
     * the SMIF never consumes the command FIFO and stays BUSY forever. */
    *(vu32 *)SRSS_CLK_ROOT_SELECT2 = 0x80000010u;

    /* SMIF device + command mode (values captured from the working HAL app). */
    DBG("[ALGO] HSIOM0=0x%08lX HSIOM1=0x%08lX\r\n",
        *(vu32 *)HSIOM_P11_SEL0, *(vu32 *)HSIOM_P11_SEL1);
    *(vu32 *)SMIF0_DEV0_CTL      = 0x80000001u;
    *(vu32 *)SMIF0_DEV0_ADDR     = 0x18000000u;
    *(vu32 *)SMIF0_DEV0_MASK     = 0xFC000000u;
    *(vu32 *)SMIF0_DEV0_ADDR_CTL = 0x03u;
    *(vu32 *)SMIF0_CTL           = 0x80071000u;
    DBG("[ALGO] SMIF0_CTL=0x%08lX\r\n", *(vu32 *)SMIF0_CTL);

    DBG("[ALGO] reset...\r\n");
    s25fl_reset();
    DBG("[ALGO] read id...\r\n");
    id = s25fl_read_id();
    DBG("[ALGO] id=0x%06lX\r\n", id);
    if (id == 0x010220u)   /* S25FL512S (Cypress/Infineon, 512 Mbit) */
    {
        return 0;
    }
    return 1;
}

int UnInit(unsigned long fnc)
{
    (void)fnc;
    return 0;
}

int EraseSector(unsigned long adr)
{
    return s25fl_erase_sector(adr - SMIF_FLASH_BASE);
}

int EraseChip(void)
{
    s25fl_write_enable();
    smif_cmd_byte(0x60, 1);
    smif_wait_idle();
    return s25fl_poll_wip(40000000u);
}

int ProgramPage(unsigned long adr, unsigned long sz, unsigned char *buf)
{
    return s25fl_program(adr - SMIF_FLASH_BASE, sz, buf);
}

int Verify(unsigned long adr, unsigned long sz, unsigned char *buf)
{
    unsigned char tmp[FLASH_PAGE];
    unsigned long off = 0;
    while (sz > 0u)
    {
        unsigned long chunk = (sz > FLASH_PAGE) ? FLASH_PAGE : sz;
        unsigned long i;
        if (s25fl_read(adr - SMIF_FLASH_BASE + off, chunk, tmp) != 0)
        {
            return -1;
        }
        for (i = 0; i < chunk; i++)
        {
            if (tmp[i] != buf[off + i])
            {
                return -2;
            }
        }
        off += chunk;
        sz -= chunk;
    }
    return 0;
}
