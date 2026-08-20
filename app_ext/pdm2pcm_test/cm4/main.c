/*
 * pdm2pcm_test (app_ext) - PDM/PCM microphone capture demo on CY8CKIT-062-BLE +
 * CY8CKIT-028-EPD, CM4 running from the external S25FL512S NOR via SMIF XIP
 * (0x18000000).
 *
 * Port of app/pdm2pcm_test (Infineon mtb-example-psoc6-pdm-pcm) to the app_ext
 * XIP flow: the CM0+ stub inits the SMIF and releases this core, this app
 * re-clocks the CPU to 150 MHz (PLL on CLKPATH1, SMIF stays on CLKPATH0),
 * applies the generated device config (clocks/routing/peripherals/pins - not
 * init_cycfg_system), then captures PDM mic audio (P10_5 data / P10_4 clk) and
 * reports the frame volume as a bar graph over the shared SCB5 UART.
 */

#include "cyhal.h"
#include "cybsp.h"
#include "cycfg.h"
#include "cy_syslib.h"
#include "cy_wdt.h"
#include "cy_sysclk.h"
#include "system_psoc6.h"
#include "cy_retarget_io.h"
#include <stdio.h>
#include <stdlib.h>

extern void uart_init(void);

#define FRAME_SIZE                  (1024)
#define THRESHOLD_HYSTERESIS        3u
#define VOLUME_RATIO                (4*FRAME_SIZE)
#define SAMPLE_RATE_HZ              8000u
#define DECIMATION_RATE             64u
#define AUDIO_SYS_CLOCK_HZ          24576000u
#define PDM_DATA                    P10_5
#define PDM_CLK                     P10_4

void button_isr_handler(void *arg, cyhal_gpio_event_t event);
void pdm_pcm_isr_handler(void *arg, cyhal_pdm_pcm_event_t event);
void clock_init(void);

volatile bool button_flag = false;
volatile bool pdm_pcm_flag = true;

uint32_t volume = 0;
uint32_t noise_threshold = THRESHOLD_HYSTERESIS;

cyhal_pdm_pcm_t pdm_pcm;
cyhal_clock_t   audio_clock;
cyhal_clock_t   pll_clock;

const cyhal_pdm_pcm_cfg_t pdm_pcm_cfg =
{
    .sample_rate     = SAMPLE_RATE_HZ,
    .decimation_rate = DECIMATION_RATE,
    .mode            = CYHAL_PDM_PCM_MODE_STEREO,
    .word_length     = 16,
    .left_gain       = 0,
    .right_gain      = 0,
};

cyhal_gpio_callback_data_t cb_data =
    {
        .callback = button_isr_handler,
        .callback_arg = NULL
 };

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

int main(void)
{
    cy_rslt_t result;
    int16_t  audio_frame[FRAME_SIZE] = {0};

    Cy_WDT_Unlock();
    Cy_WDT_Disable();
    __enable_irq();

    /* CPU to 150 MHz (PLL, CLKPATH1). SMIF XIP stays on CLKPATH0 (stub FLL). */
    clock_init_150mhz_pll();

    /* Device config: peripheral dividers, routing, peripherals and pins.
     * init_cycfg_system is deliberately NOT called - the CM0+ stub already
     * set the clocks/SMIF. */
    init_cycfg_clocks();
    init_cycfg_routing();
    init_cycfg_peripherals();
    init_cycfg_pins();

    /* Init the audio clocks (PLL0 for CLK_HF[1] / PDM-PCM) */
    clock_init();

    /* Initialize the User LED */
    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

    /* Initialize the User Button */
    cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);
    cyhal_gpio_enable_event(CYBSP_USER_BTN, CYHAL_GPIO_IRQ_FALL, CYHAL_ISR_PRIORITY_DEFAULT, true);
    cyhal_gpio_register_callback(CYBSP_USER_BTN, &cb_data);

    /* Initialize the PDM/PCM block */
    result = cyhal_pdm_pcm_init(&pdm_pcm, PDM_DATA, PDM_CLK, &audio_clock, &pdm_pcm_cfg);
    cyhal_pdm_pcm_register_callback(&pdm_pcm, pdm_pcm_isr_handler, NULL);
    cyhal_pdm_pcm_enable_event(&pdm_pcm, CYHAL_PDM_PCM_ASYNC_COMPLETE, CYHAL_ISR_PRIORITY_DEFAULT, true);
    cyhal_pdm_pcm_start(&pdm_pcm);

    /* UART last: the PDM setup (audio PLL / PDM clock) reconfigures the
     * peripheral dividers, so the shared uart_init must run afterwards. */
    uart_init();  /* SCB5 P5_1/P5_0 @ 115200 (replaces cy_retarget_io) */
    Cy_SysLib_DelayUs(2000u);  /* let the SCB5 TX settle after reconfiguration */

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("****************** \
    PDM/PCM Example \
    ****************** \r\n\n");

    for(;;)
    {
        if (pdm_pcm_flag)
        {
            pdm_pcm_flag = 0;

            volume = 0;
            for (uint32_t index = 0; index < FRAME_SIZE; index++)
            {
                volume += abs(audio_frame[index]);
            }

            printf("\n\r");
            for (uint32_t index = 0; index < (volume/VOLUME_RATIO); index++)
            {
                printf("-");
            }

            if ((volume/VOLUME_RATIO) > noise_threshold)
            {
                cyhal_gpio_write((cyhal_gpio_t) CYBSP_USER_LED, CYBSP_LED_STATE_ON);
            }
            else
            {
                cyhal_gpio_write((cyhal_gpio_t) CYBSP_USER_LED, CYBSP_LED_STATE_OFF);
            }

            cyhal_pdm_pcm_read_async(&pdm_pcm, audio_frame, FRAME_SIZE);
        }

        if (button_flag)
        {
            button_flag = false;
            noise_threshold = (volume/VOLUME_RATIO) + THRESHOLD_HYSTERESIS;
            printf("\n\rNoise threshold: %lu\n\r", (unsigned long) noise_threshold);
        }

        cyhal_syspm_sleep();
    }
}

void button_isr_handler(void *arg, cyhal_gpio_event_t event)
{
    (void) arg;
    (void) event;
    button_flag = true;
}

void pdm_pcm_isr_handler(void *arg, cyhal_pdm_pcm_event_t event)
{
    (void) arg;
    (void) event;
    pdm_pcm_flag = true;
}

void clock_init(void)
{
    cyhal_clock_reserve(&pll_clock, &CYHAL_CLOCK_PLL[0]);
    cyhal_clock_set_frequency(&pll_clock, AUDIO_SYS_CLOCK_HZ, NULL);
    cyhal_clock_set_enabled(&pll_clock, true, true);

    cyhal_clock_reserve(&audio_clock, &CYHAL_CLOCK_HF[1]);

    cyhal_clock_set_source(&audio_clock, &pll_clock);
    cyhal_clock_set_enabled(&audio_clock, true, true);
}
