/*
 * capsense_test (app_ext) - CAPSENSE buttons + 5-segment slider on
 * CY8CKIT-062-BLE, CM4 running from the external S25FL512S NOR via SMIF XIP
 * (0x18000000).
 *
 * Port of app/capsense_test (Infineon mtb-example-psoc6-capsense-buttons-slider)
 * to the app_ext XIP flow: the CM0+ stub inits the SMIF and releases this core,
 * this app re-clocks the CPU to 150 MHz (PLL on CLKPATH1, SMIF stays on
 * CLKPATH0) and drives the CAPSENSE slider + buttons + LED PWM through the
 * vendored capsense middleware (v10.0.0) and the cyhal stack compiled from
 * app/mtb_shared.
 *
 * The generated device config (cycfg_*) is copied into this app; only the
 * peripheral/pin/clock init is applied (init_cycfg_system is skipped so the
 * stub's clock/SMIF setup is not disturbed).
 *
 * Button0 (CSX) turns the user LED (P1_5) on, Button1 turns it off, and the
 * 5-segment LinearSlider0 controls the LED brightness. A CAPSENSE Tuner EzI2C
 * bridge (SCB3, P6_1 SDA / P6_0 SCL, addr 8) is set up for the Tuner GUI.
 *
 * CAPSENSE pins (CY8CKIT-062-BLE): Cmod P7_7, CintA P7_1, CintB P7_2,
 * Button0 RX0 P8_1, Button1 RX0 P8_2, TX P1_0 (shared), slider Sns0..4 P8_3..P8_7.
 */

#include "cyhal.h"
#include "cycfg.h"
#include "cycfg_capsense.h"
#include "cy_syslib.h"
#include "cy_wdt.h"
#include "cy_sysclk.h"
#include "system_psoc6.h"
#include "led.h"
#include <stdio.h>
#include <stdbool.h>

extern void uart_init(void);

#define CAPSENSE_INTR_PRIORITY      (7u)
#define EZI2C_INTR_PRIORITY         (6u) /* EZI2C interrupt priority must be
                                          * higher than CapSense interrupt */

static uint32_t initialize_capsense(void);
static void process_touch(void);
static void initialize_capsense_tuner(void);
static void capsense_isr(void);
static void capsense_callback(cy_stc_active_scan_sns_t *ptrActiveScan);
void handle_error(void);

/* Raise the CPU to 150 MHz (PLL on CLKPATH1) while keeping CLKPATH0 (the
 * FLL, from the CM0+ stub) as the SMIF interface clock source. XIP data
 * reads are reliable at 150 MHz and marginal at 100 MHz on this board. */
static void clock_init_150mhz_pll(void)
{
    Cy_SysClk_ClkPathSetSource(1u, CY_SYSCLK_CLKPATH_IN_IMO);
    {
        static const cy_stc_pll_config_t pllCfg = {
            .inputFreq  = 8000000u,
            .outputFreq = 150000000u,
            .outputMode = CY_SYSCLK_FLLPLL_OUTPUT_OUTPUT,
        };
        (void)Cy_SysClk_PllConfigure(1u, &pllCfg);
    }
    (void)Cy_SysClk_PllEnable(1u, 100000u);
    Cy_SysLib_SetWaitStates(false, 150UL);
    Cy_SysClk_ClkHfSetSource(0u, CY_SYSCLK_CLKHF_IN_CLKPATH1);
    SystemCoreClockUpdate();
}

cy_stc_scb_ezi2c_context_t ezi2c_context;
cyhal_ezi2c_t sEzI2C;
cyhal_ezi2c_slave_cfg_t sEzI2C_sub_cfg;
cyhal_ezi2c_cfg_t sEzI2C_cfg;
volatile bool capsense_scan_complete = false;

void handle_error(void)
{
    __disable_irq();
    CY_ASSERT(0);
}

int main(void)
{
    cy_rslt_t result;

    Cy_WDT_Unlock();
    Cy_WDT_Disable();
    __enable_irq();  /* cyhal / CAPSENSE ISRs */

    /* CPU to 150 MHz (PLL, CLKPATH1). SMIF XIP stays on CLKPATH0 (stub FLL). */
    clock_init_150mhz_pll();

    /* Device config: peripheral dividers (incl. the CSD clock), routing,
     * peripherals and pins. init_cycfg_system is deliberately NOT called -
     * the CM0+ stub already set the clocks/SMIF. */
    init_cycfg_clocks();
    init_cycfg_routing();
    init_cycfg_peripherals();
    init_cycfg_pins();

    uart_init();  /* SCB5 P5_1/P5_0 @ 115200 (replaces cy_retarget_io) */

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("**********************************************************\r\n");
    printf("PSoC 6 MCU: CAPSENSE buttons and slider\r\n");
    printf("**********************************************************\r\n");

    initialize_led();
    initialize_capsense_tuner();
    result = initialize_capsense();
    if (CYRET_SUCCESS != result)
    {
        handle_error();
    }

    /* Initiate first scan */
    Cy_CapSense_ScanAllWidgets(&cy_capsense_context);

    for (;;)
    {
        if (capsense_scan_complete)
        {
            Cy_CapSense_ProcessAllWidgets(&cy_capsense_context);
            process_touch();
            Cy_CapSense_RunTuner(&cy_capsense_context);
            Cy_CapSense_ScanAllWidgets(&cy_capsense_context);
            capsense_scan_complete = false;
        }
    }
}

static void process_touch(void)
{
    uint32_t button0_status;
    uint32_t button1_status;
    cy_stc_capsense_touch_t *slider_touch_info;
    uint16_t slider_pos;
    uint8_t slider_touch_status;
    bool led_update_req = false;

    static uint32_t button0_status_prev;
    static uint32_t button1_status_prev;
    static uint16_t slider_pos_prev;
    static led_data_t led_data = {LED_ON, LED_MAX_BRIGHTNESS};

    button0_status = Cy_CapSense_IsSensorActive(
        CY_CAPSENSE_BUTTON0_WDGT_ID, CY_CAPSENSE_BUTTON0_SNS0_ID, &cy_capsense_context);
    button1_status = Cy_CapSense_IsSensorActive(
        CY_CAPSENSE_BUTTON1_WDGT_ID, CY_CAPSENSE_BUTTON1_SNS0_ID, &cy_capsense_context);

    slider_touch_info = Cy_CapSense_GetTouchInfo(
        CY_CAPSENSE_LINEARSLIDER0_WDGT_ID, &cy_capsense_context);
    slider_touch_status = slider_touch_info->numPosition;
    slider_pos = slider_touch_info->ptrPosition->x;

    if ((0u != button0_status) && (0u == button0_status_prev))
    {
        led_data.state = LED_ON;
        led_update_req = true;
    }
    if ((0u != button1_status) && (0u == button1_status_prev))
    {
        led_data.state = LED_OFF;
        led_update_req = true;
    }
    if ((0 != slider_touch_status) && (slider_pos != slider_pos_prev))
    {
        led_data.brightness = (slider_pos * 100)
                / cy_capsense_context.ptrWdConfig[CY_CAPSENSE_LINEARSLIDER0_WDGT_ID].xResolution;
        led_update_req = true;
    }

    if (led_update_req)
    {
        update_led_state(&led_data);
    }

    button0_status_prev = button0_status;
    button1_status_prev = button1_status;
    slider_pos_prev = slider_pos;
}

static uint32_t initialize_capsense(void)
{
    uint32_t status = CYRET_SUCCESS;

    static const cy_stc_sysint_t capSense_intr_config =
    {
        .intrSrc = csd_interrupt_IRQn,
        .intrPriority = CAPSENSE_INTR_PRIORITY,
    };

    status = Cy_CapSense_Init(&cy_capsense_context);
    if (CYRET_SUCCESS != status)
    {
        return status;
    }

    cyhal_system_set_isr(csd_interrupt_IRQn, csd_interrupt_IRQn, CAPSENSE_INTR_PRIORITY, &capsense_isr);
    NVIC_ClearPendingIRQ(capSense_intr_config.intrSrc);
    NVIC_EnableIRQ(capSense_intr_config.intrSrc);

    status = Cy_CapSense_Enable(&cy_capsense_context);
    if (CYRET_SUCCESS != status)
    {
        return status;
    }

    status = Cy_CapSense_RegisterCallback(CY_CAPSENSE_END_OF_SCAN_E,
            capsense_callback, &cy_capsense_context);
    return status;
}

static void capsense_isr(void)
{
    Cy_CapSense_InterruptHandler(CYBSP_CSD_HW, &cy_capsense_context);
}

void capsense_callback(cy_stc_active_scan_sns_t *ptrActiveScan)
{
    (void)ptrActiveScan;
    capsense_scan_complete = true;
}

static void initialize_capsense_tuner(void)
{
    cy_rslt_t result;

    sEzI2C_sub_cfg.buf = (uint8_t *)&cy_capsense_tuner;
    sEzI2C_sub_cfg.buf_rw_boundary = sizeof(cy_capsense_tuner);
    sEzI2C_sub_cfg.buf_size = sizeof(cy_capsense_tuner);
    sEzI2C_sub_cfg.slave_address = 8U;

    sEzI2C_cfg.data_rate = CYHAL_EZI2C_DATA_RATE_400KHZ;
    sEzI2C_cfg.enable_wake_from_sleep = false;
    sEzI2C_cfg.slave1_cfg = sEzI2C_sub_cfg;
    sEzI2C_cfg.sub_address_size = CYHAL_EZI2C_SUB_ADDR16_BITS;
    sEzI2C_cfg.two_addresses = false;

    result = cyhal_ezi2c_init(&sEzI2C, CYBSP_I2C_SDA, CYBSP_I2C_SCL, NULL, &sEzI2C_cfg);
    if (result != CY_RSLT_SUCCESS)
    {
        handle_error();
    }
}
