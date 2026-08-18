#include "cy_device.h"
#include "cy_device_headers.h"
#include "cy_syslib.h"
#include "cy_sysclk.h"
#include "cy_sysanalog.h"
#include "cy_sar.h"

/* ---- DieTemp sensor channel configuration (Infineon MTB_Die_Temp_Custom) ---- */

/* Enable only channel 0. */
#define SAR_CHAN_EN             (1u)

/* Channel 0: single-ended, aperture from Sample Time 0, DieTemp on the
 * SARMUX virtual port at pin address 0, hardware averaging enabled. */
#define SAR_CHAN0_CONFIG        (CY_SAR_CHAN_SINGLE_ENDED \
                                | CY_SAR_CHAN_SAMPLE_TIME_0 \
                                | CY_SAR_POS_PORT_ADDR_SARMUX_VIRT \
                                | CY_SAR_CHAN_POS_PIN_ADDR_0 \
                                | CY_SAR_CHAN_AVG_ENABLE)

/* Single-ended channels are signed; average 32 samples, sequential fixed. */
#define SAR_SAMPLE_CTRL         (CY_SAR_SINGLE_ENDED_SIGNED \
                                | CY_SAR_AVG_CNT_32 \
                                | CY_SAR_AVG_MODE_SEQUENTIAL_FIXED)

/* Use the internal 1.2 V bandgap reference, connect Vminus to VSSA. */
#define SAR_CTRL                (CY_SAR_VREF_SEL_BGR \
                                | CY_SAR_BYPASS_CAP_ENABLE \
                                | CY_SAR_NEG_SEL_VSSA_KELVIN)
#define SAR_VREF_MV             (1200u)

/* The DieTemp sensor needs >= 1 us settling. With a ~16.7 MHz SAR clock,
 * 18 in the register gives ~1.02 us. */
#define SAR_SAMPLE_TIME01       ((18u << CY_SAR_SAMPLE_TIME0_SHIFT) \
                                | (4u << CY_SAR_SAMPLE_TIME1_SHIFT))
#define SAR_SAMPLE_TIME23       ((4u << CY_SAR_SAMPLE_TIME2_SHIFT) \
                                | (4u << CY_SAR_SAMPLE_TIME3_SHIFT))

/* Firmware switches: DieTemp -> Vplus, VSSA -> Vminus. Closing TEMP_VPLUS
 * also powers up the sensor. */
#define SAR_MUX_SWITCH0         (CY_SAR_MUX_FW_VSSA_VMINUS \
                                | CY_SAR_MUX_FW_TEMP_VPLUS)
#define SAR_MUX_SWITCH_SQ_CTRL  (CY_SAR_MUX_SQ_CTRL_VSSA \
                                | CY_SAR_MUX_SQ_CTRL_TEMP)

static const cy_stc_sar_config_t sar_config =
{
    .ctrl               = (uint32_t)SAR_CTRL,
    .sampleCtrl         = (uint32_t)SAR_SAMPLE_CTRL,
    .sampleTime01       = SAR_SAMPLE_TIME01,
    .sampleTime23       = SAR_SAMPLE_TIME23,
    .rangeThres         = CY_SAR_DEINIT,
    .rangeCond          = CY_SAR_RANGE_COND_BELOW,
    .chanEn             = SAR_CHAN_EN,
    .chanConfig         = {(uint32_t)SAR_CHAN0_CONFIG, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
                           0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u},
    .intrMask           = CY_SAR_DEINIT,   /* polling, no interrupts */
    .satIntrMask        = CY_SAR_DEINIT,
    .rangeIntrMask      = CY_SAR_DEINIT,
    .muxSwitch          = SAR_MUX_SWITCH0,
    .muxSwitchSqCtrl    = SAR_MUX_SWITCH_SQ_CTRL,
    .configRouting      = true,
    .vrefMvValue        = SAR_VREF_MV,
};

/* ---- ADC counts -> degrees Celsius (fixed point, Q16.16) ----
 * Uses the factory calibration stored in SFLASH. See the Infineon
 * "Die Temperature" component datasheet for the algorithm. */
#define DIE_TEMP_OFFSET_MULT       (0x400)
#define DIE_TEMP_SAR_TEMP_SHIFT    (16u)
#define DIE_TEMP_SAR_TEMP_DIVIDER  (0x10000)
#define DIE_TEMP_HALF_OF_ONE       ((int32_t)1u << (DIE_TEMP_SAR_TEMP_SHIFT - 1u))
#define DIE_TEMP_SCALE_ADJUSTMENT  (8)
#define DIE_TEMP_DUAL_SLOPE_CORR   (0xF0000)
#define DIE_TEMP_HIGH_TEMP         (0x640000)   /* 100 in Q16.16 */
#define DIE_TEMP_LOW_TEMP          (0x280000)   /*  40 in Q16.16 */
#define DIE_TEMP_ADJ_DIVIDER       (16u)

static int32_t DieTemp_CountsTo_Celsius(int16_t adcCounts)
{
    int32_t offsetReg = (int16_t)SFLASH->SAR_TEMP_OFFSET;
    int32_t multReg   = (int16_t)SFLASH->SAR_TEMP_MULTIPLIER;

    /* tInitial in Q16.16 */
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

    /* Add 0.5 to round to nearest, shift off fraction bits. */
    return ((tInitial + tAdjust + DIE_TEMP_HALF_OF_ONE) / DIE_TEMP_SAR_TEMP_DIVIDER);
}

void sar_temp_init(void)
{
    /* Enable the analog reference block (AREF) used as the SAR bandgap source. */
    Cy_SysAnalog_Init(&Cy_SysAnalog_Fast_Local);
    Cy_SysAnalog_Enable();

    /* SAR clock = PeriClk / 3 (~16.7 MHz, <= 18 MHz limit). */
    Cy_SysClk_PeriphAssignDivider(PCLK_PASS_CLOCK_SAR, CY_SYSCLK_DIV_8_BIT, 1u);
    Cy_SysClk_PeriphSetDivider(CY_SYSCLK_DIV_8_BIT, 1u, 2u);
    Cy_SysClk_PeriphEnableDivider(CY_SYSCLK_DIV_8_BIT, 1u);

    (void)Cy_SAR_Init(SAR, &sar_config);
    Cy_SAR_Enable(SAR);
}

int16_t sar_temp_read_celsius(void)
{
    /* Start a single scan of all enabled channels (channel 0 = DieTemp). */
    Cy_SAR_StartConvert(SAR, CY_SAR_START_CONVERT_SINGLE_SHOT);
    (void)Cy_SAR_IsEndConversion(SAR, CY_SAR_WAIT_FOR_RESULT);

    int16_t counts = Cy_SAR_GetResult16(SAR, 0u);
    return (int16_t)DieTemp_CountsTo_Celsius(counts);
}
