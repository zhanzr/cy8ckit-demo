/*
 * CM0+ XIP stub for app_ext.
 *
 * Runs from the internal flash (0x10000000). Jobs:
 *   1. Run both cores at 100 MHz (FLL) so the SMIF XIP reads run at the
 *      proven 50 MHz interface clock.
 *   2. Initialize the SMIF in XIP/memory mode so the external S25FL512S is
 *      memory-mapped at 0x18000000 (the CM4's flash).
 *   3. Release the CM4 with its vector table at 0x18000000 (it runs from
 *      the external NOR via XIP).
 *   4. Enter an interruptible IDLE (WFI) loop and stay out of the way.
 *
 * Per the app_ext brick-risk assessment the CM0+ keeps a dedicated SRAM
 * region (0x08000000..0x08008000, see common/cm0p_xip_stub/linker) and keeps
 * interrupts enabled, so it can always wake, respond and be debugged.
 *
 * NOTE: this board's ROM does not auto-boot the CM0+ image (boot-ROM hold).
 * Boot it manually, e.g. with tools/flash_and_boot.tcl, or set SP/PC.
 */

#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_sysclk.h"
#include "cy_syslib.h"
#include "cy_wdt.h"
#include "system_psoc6.h"

/* SMIF / clock / GPIO registers (values captured from the working HAL app). */
#define SMIF0_CTL            0x40420000UL
#define SMIF0_DEV0_CTL       0x40420800UL
#define SMIF0_DEV0_ADDR      0x40420808UL
#define SMIF0_DEV0_MASK      0x4042080CUL
#define SMIF0_DEV0_ADDR_CTL  0x40420820UL
#define SMIF0_DEV0_RD_CMD    0x40420840UL
#define SMIF0_DEV0_RD_ADDR   0x40420844UL
#define SMIF0_DEV0_RD_MODE   0x40420848UL
#define SMIF0_DEV0_RD_DUMMY  0x4042084CUL
#define SMIF0_DEV0_RD_DATA   0x40420850UL
#define SMIF0_DEV0_WR_CMD    0x40420860UL
#define SMIF0_DEV0_WR_ADDR   0x40420864UL
#define SMIF0_DEV0_WR_MODE   0x40420868UL
#define SMIF0_DEV0_WR_DUMMY  0x4042086CUL
#define SMIF0_DEV0_WR_DATA   0x40420870UL

#define SRSS_CLK_ROOT_SEL2   0x40260388UL   /* CLK_HF[2] = SMIF interface clock */
#define HSIOM_P11_SEL0       0x403100B0UL
#define HSIOM_P11_SEL1       0x403100B4UL
#define GPIO_P11_CFG         0x403205A8UL
#define GPIO_P11_OUT         0x40320580UL

#define CM4_APPL_ADDR        0x18000000UL   /* CM4 image in the external NOR (XIP) */

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

/* Configure the SMIF in XIP/memory mode so 0x18000000..0x1BFFFFFF reads the
 * external S25FL512S. Register values are the ones captured from the working
 * cy_serial_flash_qspi app (quad read 0xEC, 4-byte address, WR 0x34). */
static void smif_xip_init(void)
{
    /* SMIF pins P11[2..7] -> HSIOM 17 (5-bit fields). */
    *(volatile uint32_t *)HSIOM_P11_SEL0 =
        (*(volatile uint32_t *)HSIOM_P11_SEL0 & ~0x1F1F0000u) | 0x11110000u;
    *(volatile uint32_t *)HSIOM_P11_SEL1 =
        (*(volatile uint32_t *)HSIOM_P11_SEL1 & ~0x1F1F1F1Fu) | 0x11111111u;
    *(volatile uint32_t *)GPIO_P11_CFG = 0xEEEEE600u;
    *(volatile uint32_t *)GPIO_P11_OUT = 0x000000FCu;

    /* SMIF interface clock: CLK_HF[2] = CLKPATH0 / 4 = 25 MHz @ FLL 100
     * (extra margin for XIP reads). */
    *(volatile uint32_t *)SRSS_CLK_ROOT_SEL2 = 0x80000030u;

    /* SMIF device 0 + XIP command config. Values captured from the working
     * cy_serial_flash_qspi app (quad read 0xEC, 4-byte address, WR 0x34). */
    *(volatile uint32_t *)SMIF0_DEV0_CTL     = 0x80000001u;  /* WR_EN + ENABLED */
    *(volatile uint32_t *)SMIF0_DEV0_ADDR    = 0x18000000u;
    *(volatile uint32_t *)SMIF0_DEV0_MASK    = 0xFC000000u;
    *(volatile uint32_t *)SMIF0_DEV0_ADDR_CTL = 0x03u;       /* 4-byte address */
    *(volatile uint32_t *)SMIF0_DEV0_RD_CMD  = 0x800000ECu;  /* quad read 0xEC */
    *(volatile uint32_t *)SMIF0_DEV0_RD_ADDR = 0x00020000u;  /* quad address */
    *(volatile uint32_t *)SMIF0_DEV0_RD_MODE = 0x80020001u;  /* mode byte, quad */
    *(volatile uint32_t *)SMIF0_DEV0_RD_DUMMY = 0x80000003u;
    *(volatile uint32_t *)SMIF0_DEV0_RD_DATA = 0x00020000u;  /* quad data */
    *(volatile uint32_t *)SMIF0_DEV0_WR_CMD  = 0x80000034u;  /* quad program 0x34 */
    *(volatile uint32_t *)SMIF0_DEV0_WR_ADDR = 0x00000000u;
    *(volatile uint32_t *)SMIF0_DEV0_WR_MODE = 0x00000000u;
    *(volatile uint32_t *)SMIF0_DEV0_WR_DUMMY = 0x00000000u;
    *(volatile uint32_t *)SMIF0_DEV0_WR_DATA = 0x00020000u;  /* quad data */

    /* Command mode, then XIP (memory) mode. */
    *(volatile uint32_t *)SMIF0_CTL = 0x80071000u;
    *(volatile uint32_t *)SMIF0_CTL = 0x80071001u;
}

int main(void)
{
    Cy_WDT_Unlock();
    Cy_WDT_Disable();

    clock_init_100mhz();
    smif_xip_init();

    /* Release the CM4: vector table at the external NOR (XIP). */
    Cy_SysEnableCM4(CM4_APPL_ADDR);

    /* Interruptible IDLE: keep interrupts enabled (PRIMASK clear) so the
     * core can always wake; dedicated SRAM is reserved above 0x08008000
     * for the CM4, this core owns 0x08000000..0x08007FFF. */
    __enable_irq();
    for (;;)
    {
        __WFI();
    }
}
