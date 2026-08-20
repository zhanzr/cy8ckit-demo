/*
 * ntc.c - NTC thermistor + internal DTS temperature reading.
 *
 * Circuit (CY8CKIT-028-EPD):
 *   A0 (P10.0) -- 0R -- VDD_REF   (full-scale reference rail)
 *   A0 -- 10k -- A1/A2 (P10.1/P10.2) -- NTC -- A3 (P10.3) -- 0R -- GND
 *
 * All four pins are read as SAR ADC inputs via the cyhal ADC following the
 * Infineon mtb-example-hal-adc-basic configuration: VDDA reference, VSSA
 * VNEG, 12-bit, continuous scanning disabled, 1 us acquisition. A0/A3 are the
 * shield's power rails and must NOT be driven by the GPIO. The internal DTS is
 * read once at startup via the low-level SAR (VREF = VDDA, rescaled to the
 * 1.2 V bandgap basis before the factory calibration).
 *
 *   Vsample = (A1+A2)/2 - A3
 *   rNTC    = R_ref * Vsample / (A0 - A3 - Vsample)
 *   T       = B / ln(rNTC / R_infinity) - 273.15
 */

#include "cy_device.h"
#include "cy_device_headers.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_syslib.h"
#include "cy_sysclk.h"
#include "cy_sysanalog.h"
#include "cy_sar.h"
#include "ntc.h"
#include <math.h>

/* ---- DTS (DieTemp) SAR config - VDDA reference, rescaled to BGR basis ---- */
#define CHAN_DTS                (0u)
#define CHAN_DTS_CONFIG         (CY_SAR_CHAN_SINGLE_ENDED \
                                | CY_SAR_CHAN_SAMPLE_TIME_0 \
                                | CY_SAR_POS_PORT_ADDR_SARMUX_VIRT \
                                | CY_SAR_CHAN_POS_PIN_ADDR_0 \
                                | CY_SAR_CHAN_AVG_ENABLE)
#define DTS_SAMPLE_CTRL         (CY_SAR_SINGLE_ENDED_UNSIGNED \
                                | CY_SAR_AVG_CNT_32 \
                                | CY_SAR_AVG_MODE_SEQUENTIAL_FIXED)
#define DTS_CTRL                (CY_SAR_VREF_SEL_VDDA \
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
    .vrefMvValue        = 3300u,
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
static cyhal_adc_channel_t ch_a0, ch_a1, ch_a2, ch_a3;
static uint16_t dts_raw_once;

static void sar_temp_init(void)
{
    Cy_SysClk_PeriphAssignDivider(PCLK_PASS_CLOCK_SAR, CY_SYSCLK_DIV_8_BIT, 1u);
    Cy_SysClk_PeriphSetDivider(CY_SYSCLK_DIV_8_BIT, 1u, 2u);
    Cy_SysClk_PeriphEnableDivider(CY_SYSCLK_DIV_8_BIT, 1u);

    (void)Cy_SAR_Init(SAR, &dts_sar_config);
    Cy_SAR_Enable(SAR);
}

static uint16_t sar_read_dts(void)
{
    Cy_SAR_StartConvert(SAR, CY_SAR_START_CONVERT_SINGLE_SHOT);
    (void)Cy_SAR_IsEndConversion(SAR, CY_SAR_WAIT_FOR_RESULT);
    return (uint16_t)Cy_SAR_GetResult16(SAR, CHAN_DTS);
}

/* ---- DieTemp counts -> degrees Celsius (Q16.16 factory calibration) ---- */
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

    /* Read the internal DTS once at startup (VREF = VDDA, rescaled). */
    sar_temp_init();
    dts_raw_once = sar_read_dts();

    /* cyhal ADC owns the SAR from here on (correct SARMUX routing for P10.0-3).
     * A0/A3 are the shield's VDD_REF/GND rails - ADC input only, never driven.
     * Order follows mtb-example-hal-adc-basic: init, channels, then configure. */
    (void)cyhal_adc_init(&adc, CYBSP_A0, NULL);

    (void)cyhal_adc_channel_init_diff(&ch_a0, &adc, CYBSP_A0, CYHAL_ADC_VNEG, &channel_config);
    (void)cyhal_adc_channel_init_diff(&ch_a1, &adc, CYBSP_A1, CYHAL_ADC_VNEG, &channel_config);
    (void)cyhal_adc_channel_init_diff(&ch_a2, &adc, CYBSP_A2, CYHAL_ADC_VNEG, &channel_config);
    (void)cyhal_adc_channel_init_diff(&ch_a3, &adc, CYBSP_A3, CYHAL_ADC_VNEG, &channel_config);

    (void)cyhal_adc_configure(&adc, &adc_config);
}

/* Raw ADC counts for A0/A1/A2/A3 and the internal DTS. */
void ntc_read_raw(uint16_t *a0, uint16_t *a1, uint16_t *a2, uint16_t *a3, uint16_t *dts)
{
    *a0 = cyhal_adc_read_u16(&ch_a0);
    *a1 = cyhal_adc_read_u16(&ch_a1);
    *a2 = cyhal_adc_read_u16(&ch_a2);
    *a3 = cyhal_adc_read_u16(&ch_a3);
    *dts = dts_raw_once;
}

/* Pin voltages in millivolts. Uses the VBG-calibrated read when available;
 * falls back to the raw counts scaled at the VDDA/2 nominal full scale
 * (1650 mV) - the SAR2 "VDDA" reference - which matches a meter reading. */
void ntc_read_mv(uint16_t *a0_mv, uint16_t *a1_mv, uint16_t *a2_mv, uint16_t *a3_mv)
{
    static const uint16_t FULL_SCALE_MV = 1650u;
    int32_t uv[4] = { cyhal_adc_read_uv(&ch_a0), cyhal_adc_read_uv(&ch_a1),
                      cyhal_adc_read_uv(&ch_a2), cyhal_adc_read_uv(&ch_a3) };
    uint16_t counts[4] = { cyhal_adc_read_u16(&ch_a0), cyhal_adc_read_u16(&ch_a1),
                           cyhal_adc_read_u16(&ch_a2), cyhal_adc_read_u16(&ch_a3) };
    uint16_t *out[4] = { a0_mv, a1_mv, a2_mv, a3_mv };
    for (int i = 0; i < 4; i++)
    {
        *out[i] = (uv[i] > 0) ? (uint16_t)(uv[i] / 1000)
                              : (uint16_t)(((uint32_t)counts[i] * FULL_SCALE_MV) / 65535u);
    }
}

float ntc_temp_from_raw(uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3)
{
    float vsample = 0.5f * ((float)a1 + (float)a2) - (float)a3;
    float vscale = (float)a0 - (float)a3 - vsample;
    if (vscale <= 0.0f)
    {
        return 0.0f;
    }
    float r = NTC_R_REF * vsample / vscale;
    return (NTC_B_CONST / logf(r / NTC_R_INFINITY)) - 273.15f;
}

float ntc_read_celsius(void)
{
    uint16_t a0, a1, a2, a3;
    ntc_read_mv(&a0, &a1, &a2, &a3);
    return ntc_temp_from_raw(a0, a1, a2, a3);
}

int16_t dts_read_celsius(void)
{
    uint32_t scaled = ((uint32_t)dts_raw_once * 3300u) / 1200u;
    return (int16_t)DieTemp_CountsTo_Celsius((int16_t)scaled);
}

uint16_t dts_raw_value(void)
{
    return dts_raw_once;
}
