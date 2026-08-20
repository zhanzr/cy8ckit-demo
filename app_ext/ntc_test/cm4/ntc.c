/*
 * ntc.c - NTC thermistor + internal DTS temperature reading.
 *
 * Circuit (CY8CKIT-028-EPD):
 *   A0 (P10.0) -- 10k -- A1/A2 (P10.1/P10.2) -- NTC -- A3 (P10.3)
 *
 * The 0R strap resistors that would tie A0 to VIO_REF and A3 to GND are NOT
 * populated on this shield, so the GPIO drives the divider: A0 is driven high
 * (VDDD ~3.3 V supply) and A3 low (0 V return). Only A1/A2 (the divider
 * output) are sampled by the SAR ADC - the driven A0/A3 pins cannot be read
 * cleanly through the SARMUX.
 *
 *   OUT      = (A1+A2)/2            (in mV)
 *   rNTC     = R_ref * OUT / (VDD - OUT)
 *   T        = B / ln(rNTC / R_infinity) - 273.15
 *
 * The ADC follows Infineon mtb-example-hal-adc-basic: VDDA reference, VSSA
 * VNEG, 12-bit, continuous scanning disabled, 1 us acquisition. The internal
 * DTS is read once at startup via the low-level SAR (VREF = VDDA, rescaled to
 * the 1.2 V bandgap basis before the factory calibration).
 */

#include "cy_device.h"
#include "cy_device_headers.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_syslib.h"
#include "cy_sysclk.h"
#include "cy_sysanalog.h"
#include "cy_sar.h"
#include "system_psoc6.h"
#include "ntc.h"
#include <math.h>

/* ---- DTS (DieTemp) SAR config - 1.2 V bandgap reference (blink_hello) ---- */
#define CHAN_DTS                (0u)
#define CHAN_DTS_CONFIG         (CY_SAR_CHAN_SINGLE_ENDED \
                                | CY_SAR_CHAN_SAMPLE_TIME_0 \
                                | CY_SAR_POS_PORT_ADDR_SARMUX_VIRT \
                                | CY_SAR_CHAN_POS_PIN_ADDR_0 \
                                | CY_SAR_CHAN_AVG_ENABLE)
#define DTS_SAMPLE_CTRL         (CY_SAR_SINGLE_ENDED_SIGNED \
                                | CY_SAR_AVG_CNT_32 \
                                | CY_SAR_AVG_MODE_SEQUENTIAL_FIXED)
#define DTS_CTRL                (CY_SAR_VREF_SEL_BGR \
                                | CY_SAR_BYPASS_CAP_ENABLE \
                                | CY_SAR_NEG_SEL_VSSA_KELVIN)
#define DTS_SAMPLE_TIME01       ((18u << CY_SAR_SAMPLE_TIME0_SHIFT) \
                                | (4u << CY_SAR_SAMPLE_TIME1_SHIFT))
#define DTS_SAMPLE_TIME23       ((4u << CY_SAR_SAMPLE_TIME2_SHIFT) \
                                | (4u << CY_SAR_SAMPLE_TIME3_SHIFT))
#define DTS_MUX_SWITCH0         (CY_SAR_MUX_FW_VSSA_VMINUS \
                                | CY_SAR_MUX_FW_TEMP_VPLUS)
#define DTS_MUX_SWITCH_SQ_CTRL  (CY_SAR_MUX_SQ_CTRL_VSSA \
                                | CY_SAR_MUX_SQ_CTRL_TEMP)

static const cy_stc_sar_config_t dts_sar_config =
{
    .ctrl               = (uint32_t)DTS_CTRL,
    .sampleCtrl         = (uint32_t)DTS_SAMPLE_CTRL,
    .sampleTime01       = DTS_SAMPLE_TIME01,
    .sampleTime23       = DTS_SAMPLE_TIME23,
    .rangeThres         = CY_SAR_DEINIT,
    .rangeCond          = CY_SAR_RANGE_COND_BELOW,
    .chanEn             = (1u << CHAN_DTS),
    .chanConfig         = {(uint32_t)CHAN_DTS_CONFIG, 0u, 0u, 0u,
                           0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u},
    .intrMask           = CY_SAR_DEINIT,
    .satIntrMask        = CY_SAR_DEINIT,
    .rangeIntrMask      = CY_SAR_DEINIT,
    .muxSwitch          = DTS_MUX_SWITCH0,
    .muxSwitchSqCtrl    = DTS_MUX_SWITCH_SQ_CTRL,
    .configRouting      = true,
    .vrefMvValue        = 1200u,
};

/* ADC configuration per Infineon mtb-example-hal-adc-basic. */
static const cyhal_adc_config_t adc_config = {
    .continuous_scanning = false,
    .average_count       = 1,
    .vref                = CYHAL_ADC_REF_VDDA,
    .vneg                = CYHAL_ADC_VNEG_VSSA,
    .resolution          = 12u,
    .ext_vref            = NC,
    .bypass_pin          = NC,
};

static const cyhal_adc_channel_config_t channel_config = {
    .enable_averaging   = false,
    .min_acquisition_ns = 1000u,   /* 1 us acquisition (adc-basic) */
    .enabled            = true,
};

static cyhal_adc_t adc;
static cyhal_adc_channel_t ch_a1, ch_a2;
static uint16_t dts_raw_once;

static void sar_temp_init(void)
{
    Cy_SysClk_PeriphAssignDivider(PCLK_PASS_CLOCK_SAR, CY_SYSCLK_DIV_8_BIT, 1u);
    Cy_SysClk_PeriphSetDivider(CY_SYSCLK_DIV_8_BIT, 1u, 2u);
    Cy_SysClk_PeriphEnableDivider(CY_SYSCLK_DIV_8_BIT, 1u);

    /* The DieTemp sensor settles in ~1 us; a longer aperture droops the count
     * (the raw DieTemp was 469 -> 656 C at the 100 MHz app's 1.71 us aperture
     * vs 1200 -> 33 C at the 150 MHz app's 1.14 us). Size the aperture to
     * ~1.15 us from the actual SAR clock (PeriClk/3 = SystemCoreClock/9). */
    static cy_stc_sar_config_t cfg;
    cfg = dts_sar_config;
    uint32_t fsar = SystemCoreClock / 9u;
    uint32_t sample = (fsar * 115u) / 100000000u;
    if (sample < 1u) sample = 1u;
    cfg.sampleTime01 = ((sample << CY_SAR_SAMPLE_TIME0_SHIFT)
                        | (4u << CY_SAR_SAMPLE_TIME1_SHIFT));
    (void)Cy_SAR_Init(SAR, &cfg);
    Cy_SAR_Enable(SAR);
}

static uint16_t sar_read_dts(void)
{
    Cy_SAR_StartConvert(SAR, CY_SAR_START_CONVERT_SINGLE_SHOT);
    (void)Cy_SAR_IsEndConversion(SAR, CY_SAR_WAIT_FOR_RESULT);
    return (uint16_t)Cy_SAR_GetResult16(SAR, CHAN_DTS);
}
#define DIE_TEMP_OFFSET_MULT       (0x400)
#define DIE_TEMP_SAR_TEMP_SHIFT    (16u)
#define DIE_TEMP_SAR_TEMP_DIVIDER  (0x10000)
#define DIE_TEMP_HALF_OF_ONE       ((int32_t)1u << (DIE_TEMP_SAR_TEMP_SHIFT - 1u))
#define DIE_TEMP_SCALE_ADJUSTMENT  (8)
#define DIE_TEMP_DUAL_SLOPE_CORR   (0xF0000)
#define DIE_TEMP_HIGH_TEMP         (0x640000)   /* 100 in Q16.16 */
#define DIE_TEMP_LOW_TEMP          (0x280000)   /*  40 in Q16.16 */
#define DIE_TEMP_ADJ_DIVIDER       (16u)

static int32_t DieTemp_CountsTo_Celsius(int32_t adcCounts)
{
    int32_t offsetReg = (int16_t)SFLASH->SAR_TEMP_OFFSET;
    int32_t multReg   = (int16_t)SFLASH->SAR_TEMP_MULTIPLIER;

    int32_t tInitial = (adcCounts * multReg) + (offsetReg * DIE_TEMP_OFFSET_MULT);

    int32_t tAdjust;
    if (tInitial >= DIE_TEMP_DUAL_SLOPE_CORR)
    {
        tAdjust = (DIE_TEMP_SCALE_ADJUSTMENT * ((DIE_TEMP_HIGH_TEMP - tInitial)
                    / DIE_TEMP_ADJ_DIVIDER)) /
                  ((DIE_TEMP_HIGH_TEMP - DIE_TEMP_DUAL_SLOPE_CORR) / DIE_TEMP_SAR_TEMP_DIVIDER);
    }
    else
    {
        tAdjust = (DIE_TEMP_SCALE_ADJUSTMENT * ((DIE_TEMP_LOW_TEMP + tInitial)
                    / DIE_TEMP_ADJ_DIVIDER)) /
                  ((DIE_TEMP_LOW_TEMP + DIE_TEMP_DUAL_SLOPE_CORR) / DIE_TEMP_SAR_TEMP_DIVIDER);
    }

    return ((tInitial + tAdjust + DIE_TEMP_HALF_OF_ONE) / DIE_TEMP_SAR_TEMP_DIVIDER);
}

void ntc_init(void)
{
    Cy_SysAnalog_Init(&Cy_SysAnalog_Fast_Local);
    Cy_SysAnalog_Enable();

    /* Read the internal DTS once at startup (1.2 V bandgap reference).
     * Warm up the DieTemp sensor with a few reads first (like blink_hello's
     * repeated loop reads) so the first settled value is used. */
    sar_temp_init();
    for (int i = 0; i < 8; i++)
    {
        (void)sar_read_dts();
    }
    dts_raw_once = sar_read_dts();

    /* The 0R strap resistors to VIO_REF/GND are not populated on this shield,
     * so drive the divider: A0 = supply (high), A3 = return (low). */
    (void)cyhal_gpio_init(CYBSP_A0, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1u);
    (void)cyhal_gpio_init(CYBSP_A3, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0u);

    /* cyhal ADC owns the SAR from here on. Sample only A1/A2 (the divider
     * output) - the driven A0/A3 pins cannot be read cleanly via the SARMUX.
     * Order follows mtb-example-hal-adc-basic: init, channels, then configure. */
    (void)cyhal_adc_init(&adc, CYBSP_A1, NULL);

    (void)cyhal_adc_channel_init_diff(&ch_a1, &adc, CYBSP_A1, CYHAL_ADC_VNEG, &channel_config);
    (void)cyhal_adc_channel_init_diff(&ch_a2, &adc, CYBSP_A2, CYHAL_ADC_VNEG, &channel_config);

    (void)cyhal_adc_configure(&adc, &adc_config);
}

/* Raw ADC counts for A1/A2 (P10.1/P10.2) and the internal DTS. */
void ntc_read_raw(uint16_t *a1, uint16_t *a2, uint16_t *dts)
{
    *a1 = cyhal_adc_read_u16(&ch_a1);
    *a2 = cyhal_adc_read_u16(&ch_a2);
    *dts = dts_raw_once;
}

/* A1/A2 voltages in millivolts. Uses the VBG-calibrated read when available;
 * falls back to the raw counts scaled at the measured VDDA full scale
 * (~2015 mV on this board - the SAR2 "VDDA" reference), which reproduces the
 * VBG-calibrated result. */
void ntc_read_mv(uint16_t *a1_mv, uint16_t *a2_mv)
{
    static const uint16_t FULL_SCALE_MV = 2015u;
    int32_t uv1 = cyhal_adc_read_uv(&ch_a1);
    int32_t uv2 = cyhal_adc_read_uv(&ch_a2);
    uint16_t c1 = cyhal_adc_read_u16(&ch_a1);
    uint16_t c2 = cyhal_adc_read_u16(&ch_a2);
    *a1_mv = (uv1 > 0) ? (uint16_t)(uv1 / 1000)
                       : (uint16_t)(((uint32_t)c1 * FULL_SCALE_MV) / 65535u);
    *a2_mv = (uv2 > 0) ? (uint16_t)(uv2 / 1000)
                       : (uint16_t)(((uint32_t)c2 * FULL_SCALE_MV) / 65535u);
}

/* NTC temperature from the A1/A2 divider output (mV). A0 is driven to VDDD
 * (NTC_DRIVE_VDD_MV) and A3 to 0 V. */
float ntc_temp_from_a1a2_mv(uint16_t a1_mv, uint16_t a2_mv)
{
    float out = 0.5f * ((float)a1_mv + (float)a2_mv);
    if (out <= 0.0f)
    {
        return 0.0f;
    }
    float r = NTC_R_REF * out / (NTC_DRIVE_VDD_MV - out);
    return (NTC_B_CONST / logf(r / NTC_R_INFINITY)) - 273.15f;
}

float ntc_read_celsius(void)
{
    uint16_t a1, a2;
    ntc_read_mv(&a1, &a2);
    return ntc_temp_from_a1a2_mv(a1, a2);
}

int16_t dts_read_celsius(void)
{
    return (int16_t)DieTemp_CountsTo_Celsius((int16_t)dts_raw_once);
}

uint16_t dts_raw_value(void)
{
    return dts_raw_once;
}
