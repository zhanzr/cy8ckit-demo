/*
 * harness main - run the S25FL512S probe-rs flash algorithm as a debuggable
 * firmware on the CY8CKIT-062, replicating the probe-rs "connect under reset"
 * context (reset clock, no HAL SMIF init). Prints the algorithm's Init result
 * (JEDEC read) and an erase/program/read-back cycle over SCB5 UART, so the
 * SMIF behaviour is visible instead of cryptic probe-rs error codes.
 *
 * The exact same flash_s25fl512s_smif.c source is included verbatim and its
 * Init/EraseSector/ProgramPage/Verify entry points are called directly.
 */

#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_sysclk.h"
#include "cy_wdt.h"
#include "system_psoc6.h"
#include <stdio.h>

extern void uart_init(void);
extern void uart_puts(const char *s);
extern void uart_putc(char c);

/* The algorithm under test (position-independent, register-level SMIF). */
#include "../algo/flash_s25fl512s_smif.c"

static void clock_init_100mhz(void)
{
    static const cy_stc_fll_manual_config_t fllConfig = {
        .fllMult = 500U,                 /* 8 MHz / 20 * 500 / 2 = 100 MHz */
        .refDiv = 20U,
        .ccoRange = CY_SYSCLK_FLL_CCO_RANGE4,
        .enableOutputDiv = true,
        .lockTolerance = 10U,
        .igain = 9U,
        .pgain = 5U,
        .settlingCount = 8U,
        .outputMode = CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT,
        .cco_Freq = 355U,
    };
    Cy_SysLib_SetWaitStates(false, 100UL);
    Cy_SysClk_FllDisable();
    Cy_SysClk_ClkPathSetSource(0U, CY_SYSCLK_CLKPATH_IN_IMO);
    (void)Cy_SysClk_FllManualConfigure(&fllConfig);
    (void)Cy_SysClk_FllEnable(100000u);
    Cy_SysClk_ClkHfSetSource(0U, CY_SYSCLK_CLKHF_IN_CLKPATH0);
    Cy_SysClk_ClkFastSetDivider(0U);
    SystemCoreClockUpdate();
}

#define ALGO_DEBUG

static void delay_us(unsigned long us)
{
    volatile unsigned long d = us * 12u;   /* ~1 us per iter @ 12 MHz */
    while (d--)
    {
    }
}

/* Identical register dump produced by the app AND the harness for diffing. */
static void regdump(const char *tag)
{
    printf("\r\n--- regdump (%s) ---\r\n", tag);
    printf(" SMIF0_CTL=%08lX STATUS=%08lX TXCMD=%08lX\r\n",
           *(vu32 *)0x40420000u, *(vu32 *)0x40420004u, *(vu32 *)0x40420044u);
    printf(" SMIF0 TXDATA_CTL=%08lX TXDATA=%08lX RXDATA_CTL=%08lX RXDATA=%08lX\r\n",
           *(vu32 *)0x40420080u, *(vu32 *)0x40420084u, *(vu32 *)0x404200C0u,
           *(vu32 *)0x404200C4u);
    printf(" SMIF0 INTR=%08lX INTR_SET=%08lX INTR_MASK=%08lX\r\n",
           *(vu32 *)0x404207C0u, *(vu32 *)0x404207C4u, *(vu32 *)0x404207C8u);
    {
        unsigned long i;
        printf(" SMIF0 full:");
        for (i = 0; i < 44; i++)
        {
            printf(" %08lX", *(vu32 *)(0x40420000u + i * 4u));
        }
        printf("\r\n");
    }
    printf(" DEV0 CTL=%08lX ADDR=%08lX MASK=%08lX ADDR_CTL=%08lX\r\n",
           *(vu32 *)0x40420800u, *(vu32 *)0x40420808u, *(vu32 *)0x4042080Cu,
           *(vu32 *)0x40420820u);
    printf(" HSIOM_P11 SEL0=%08lX SEL1=%08lX\r\n",
           *(vu32 *)0x403100B0u, *(vu32 *)0x403100B4u);
    printf(" GPIO_P11 OUT=%08lX IN=%08lX CFG=%08lX CFG_IN=%08lX CFG_OUT=%08lX\r\n",
           *(vu32 *)0x40320580u, *(vu32 *)0x40320590u, *(vu32 *)0x403205A8u,
           *(vu32 *)0x403205ACu, *(vu32 *)0x403205B0u);
    printf(" SRSS ROOT[0]=%08lX [1]=%08lX [2]=%08lX [3]=%08lX [4]=%08lX\r\n",
           *(vu32 *)0x40260000u, *(vu32 *)0x40260004u, *(vu32 *)0x40260008u,
           *(vu32 *)0x4026000Cu, *(vu32 *)0x40260010u);
    printf(" PERI DIV8=%08lX %08lX %08lX %08lX\r\n",
           *(vu32 *)0x40010800u, *(vu32 *)0x40010804u, *(vu32 *)0x40010808u,
           *(vu32 *)0x4001080Cu);
    printf(" PERI DIV16_5=%08lX %08lX %08lX %08lX\r\n",
           *(vu32 *)0x40010A00u, *(vu32 *)0x40010A04u, *(vu32 *)0x40010A08u,
           *(vu32 *)0x40010A0Cu);
    printf(" PERI CLOCK_CTL[0..15]=%08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX %08lX\r\n",
           *(vu32 *)0x40010C00u, *(vu32 *)0x40010C04u, *(vu32 *)0x40010C08u,
           *(vu32 *)0x40010C0Cu, *(vu32 *)0x40010C10u, *(vu32 *)0x40010C14u,
           *(vu32 *)0x40010C18u, *(vu32 *)0x40010C1Cu, *(vu32 *)0x40010C20u,
           *(vu32 *)0x40010C24u, *(vu32 *)0x40010C28u, *(vu32 *)0x40010C2Cu,
           *(vu32 *)0x40010C30u, *(vu32 *)0x40010C34u, *(vu32 *)0x40010C38u,
           *(vu32 *)0x40010C3Cu);
}

int main(void)
{
    unsigned char pattern[256];
    unsigned char back[256];
    unsigned long i;
    int rc;
    int pass = 0;

    Cy_WDT_Unlock();
    Cy_WDT_Disable();

    uart_init();

    printf("\r\n*************** SMIF probe-rs algorithm harness ***************\r\n");

    /* ---- SMIF CTL variant sweep (diagnostic) ---- */
    printf("\r\n[0] SMIF CTL sweep:\r\n");
    {
        static const unsigned long ctls[] = {
            0x00000000u, 0x80000000u, 0x80000300u, 0x80071000u, 0x80071300u,
        };
        unsigned long j;
        for (j = 0; j < sizeof(ctls) / sizeof(ctls[0]); j++)
        {
            *(vu32 *)SMIF0_CTL = ctls[j];
            printf("    CTL=0x%08lX -> STATUS=0x%08lX\r\n",
                   ctls[j], *(vu32 *)SMIF0_STATUS);
        }
        *(vu32 *)SMIF0_CTL = 0x80071000u;
    }

    for (i = 0; i < 256u; i++)
    {
        pattern[i] = (unsigned char)(0x50u + i);
    }

run_test:
    printf("\r\n--- test pass @ %lu Hz ---\r\n", (unsigned long)SystemCoreClock);

#ifdef READ_ONLY
    /* Read-only mode: dump 64 bytes at device addr 0 (no erase/program). */
    printf("\r\n[1] Init ...\r\n");
    rc = Init(0x18000000u, 0u, 0u);
    if (rc != 0)
    {
        printf("    Init FAILED rc=%d\r\n", rc);
        goto next_pass;
    }
    rc = s25fl_read(0u, 64u, back);
    printf("    read rc=%d, first 64 bytes at 0x18000000:\r\n", rc);
    for (i = 0; i < 64u; i++)
    {
        printf(" %02X", back[i]);
        if ((i & 0xF) == 0xFu)
        {
            printf("\r\n");
        }
    }
    printf("\r\n    JEDEC 0x%06lX\r\n", s25fl_read_id());
    goto next_pass;
#endif

    /* ---- Init (JEDEC ID) ---- */
    printf("\r\n[1] Init ...\r\n");
    rc = Init(0x18000000u, 0u, 0u);
    regdump("harness-after-init");
    if (rc != 0)
    {
        printf("    Init FAILED rc=%d (JEDEC ID read back 0x%06lX)\r\n", rc,
               s25fl_read_id());
        goto next_pass;
    }
    printf("    Init OK (JEDEC ID 0x%06lX)\r\n", s25fl_read_id());

    /* ---- Erase sector 0 ---- */
    printf("\r\n[2] EraseSector(0x18000000) ...\r\n");
    rc = EraseSector(0x18000000u);
    printf("    rc=%d\r\n", rc);
    if (rc != 0)
    {
        goto next_pass;
    }

    /* ---- Program one page ---- */
    printf("\r\n[3] ProgramPage(0x18000000, 256) ...\r\n");
    rc = ProgramPage(0x18000000u, 256u, pattern);
    printf("    rc=%d\r\n", rc);
    if (rc != 0)
    {
        goto next_pass;
    }

    /* ---- Read back + verify ---- */
    printf("\r\n[4] Verify(0x18000000, 256) ...\r\n");
    rc = Verify(0x18000000u, 256u, pattern);
    printf("    Verify rc=%d\r\n", rc);

    rc = s25fl_read(0u, 256u, back);
    printf("    read rc=%d\r\n", rc);
    {
        unsigned long bad = 0;
        for (i = 0; i < 256u; i++)
        {
            if (back[i] != pattern[i])
            {
                bad++;
            }
        }
        printf("    mismatches: %lu\r\n", bad);
        if (bad == 0u)
        {
            printf("\r\nHARNESS PASS: erase + program + readback verified.\r\n");
            pass = 1;
        }
        else
        {
            printf("\r\nHARNESS FAIL: readback mismatch.\r\n");
        }
    }

next_pass:
    if (!pass)
    {
        printf("\r\n--- switching to 100 MHz (FLL) and retrying ---\r\n");
        clock_init_100mhz();
        uart_init();
        pass = 1;   /* do not loop forever; run the 100 MHz pass once */
        goto run_test;
    }

    printf("\r\nDone.\r\n");
    for (;;)
    {
        delay_us(100000);
    }
}
