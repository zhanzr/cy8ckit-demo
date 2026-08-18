#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_sysclk.h"
#include "cy_wdt.h"
#include "system_psoc6.h"
#include <stdio.h>

#include "nor_bb.h"
#include "utils.h"

extern void uart_init(void);

/* The CM4 image is embedded into this CM0p binary by build.sh
 * (objcopy --redefine-sym into a .cy_m4_image section). */
extern const uint8_t _binary_cm4_image_start[];
extern const uint8_t _binary_cm4_image_end[];
#define CM4_IMAGE_SIZE   ((uint32_t)(_binary_cm4_image_end - _binary_cm4_image_start))

#define CM4_EXT_FLASH_ADDR   0x00000000u   /* device addr 0 == XIP 0x18000000 */
#define CM4_RAM_ADDR         0x08030000u   /* where the CM4 image is copied */

/* Shared cross-core UART mutex + readiness flag (fixed SRAM addresses). */
#define UART_LOCK_ADDR   ((volatile uint32_t *)0x08005040u)
#define UART_READY_ADDR  ((volatile uint32_t *)0x08005044u)

static void uart_lock(void)
{
    while (*(volatile uint32_t *)UART_LOCK_ADDR != 0u)
    {
    }
    *(volatile uint32_t *)UART_LOCK_ADDR = 1u;
}

static void uart_unlock(void)
{
    *(volatile uint32_t *)UART_LOCK_ADDR = 0u;
}

static void clock_init(void)
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

int main(void)
{
    Cy_WDT_Unlock();
    Cy_WDT_Disable();

    clock_init();

    *UART_LOCK_ADDR = 0u;
    uart_init();
    TICK_Init();

    nor_bb_init();   /* GPIO bit-bang SPI to the S25FL512S */

    *UART_READY_ADDR = 1u;   /* UART ready for the CM4 */

    uart_lock();
    printf("\r\n=== cm4_external_app: CM0p (boot the CM4 from external flash) ===\r\n");
    printf("  CM4 image: %lu bytes, stored at external flash 0x18000000,\r\n",
           (unsigned long)CM4_IMAGE_SIZE);
    printf("  run address: SRAM 0x%08lX\r\n", (unsigned long)CM4_RAM_ADDR);
    uart_unlock();

    /* Always program the image into the external flash (sector erase + page
     * program) so the stored content is guaranteed correct. */
    uart_lock();
    printf("  Programming CM4 image to external flash (sector erase + page program)...\r\n");
    uart_unlock();
    uint32_t t0 = HAL_GetTick();
    (void)nor_bb_program(CM4_EXT_FLASH_ADDR, _binary_cm4_image_start, CM4_IMAGE_SIZE);
    uint32_t t1 = HAL_GetTick();
    uart_lock();
    printf("  Done in %lu ms\r\n", (unsigned long)(t1 - t0));
    uart_unlock();

    /* Read the image back from external flash into internal SRAM. */
    uart_lock();
    printf("  Reading image back to SRAM 0x%08lX...\r\n", (unsigned long)CM4_RAM_ADDR);
    uart_unlock();
    (void)nor_bb_read(CM4_EXT_FLASH_ADDR, (uint8_t *)CM4_RAM_ADDR, CM4_IMAGE_SIZE);

    /* Verify the copy. */
    uint32_t got = *(volatile uint32_t *)CM4_RAM_ADDR;
    uint32_t want = *(const uint32_t *)_binary_cm4_image_start;
    uart_lock();
    printf("  verify: SRAM[0]=0x%08lX, image[0]=0x%08lX  %s\r\n",
           (unsigned long)got, (unsigned long)want, (got == want) ? "OK" : "FAIL");

    /* Full-image verification. */
    uint32_t bad = 0u;
    const uint8_t *a = _binary_cm4_image_start;
    const uint8_t *b = (const uint8_t *)CM4_RAM_ADDR;
    for (uint32_t i = 0u; i < CM4_IMAGE_SIZE; i++)
    {
        if (a[i] != b[i])
        {
            bad = i + 1u;
            break;
        }
    }
    printf("  full verify: %lu bytes %s%s\r\n", (unsigned long)CM4_IMAGE_SIZE,
           bad ? "FAIL" : "OK", "");
    uart_unlock();

    /* Boot the CM4 from SRAM (vector table at 0x08030000). A clean reset
     * first avoids the boot-ROM hold state the CM4 otherwise starts in. */
    uart_lock();
    printf("  Booting CM4 from SRAM (VTOR = 0x%08lX)...\r\n", (unsigned long)CM4_RAM_ADDR);
    uart_unlock();
    Cy_SysResetCM4();
    Cy_SysEnableCM4(CM4_RAM_ADDR);

    /* CM0p is silent from here on so the CM4's own UART banner is clean. */
    for (;;)
    {
        HAL_Delay(1000);
    }
}
