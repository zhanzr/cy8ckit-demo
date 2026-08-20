/*
 * ntc.h - NTC thermistor (NCP18XH103F03RB) + internal DTS temperature reading
 * for the CY8CKIT-028-EPD shield on CY8CKIT-062-BLE.
 *
 * The shield NTC circuit:
 *   THERM_VDD (P10.0/A0) -- 0R -- VIO_REF
 *   THERM_VDD -- 10k -- THERM_OUT (P10.1/A1 and P10.2/A2) -- NTC -- THERM_GND
 *   THERM_GND (P10.3/A3) -- 0R -- GND
 *
 * The SAR ADC measures THERM_OUT in both polarities (the ratio cancels the
 * supply/reference) on both A1 and A2, averages the computed resistances, then
 * applies the Beta equation. The internal Die Temperature sensor is read with
 * the factory-calibrated conversion (VREF = internal 1.2 V bandgap).
 *
 * NOTE: the SAR is reconfigured between the NTC read (VREF = VDDA) and the DTS
 * read (VREF = BGR), so call ntc_init()/dts_init() before each read.
 */

#ifndef NTC_H_
#define NTC_H_

#include <stdint.h>

/* Thermistor pins (CY8CKIT-028-EPD): */
#define NTC_PIN_VDD   (P10_0)   /* THERM_VDD / A0 */
#define NTC_PIN_GND   (P10_3)   /* THERM_GND / A3 */
#define NTC_PIN_OUT1  (P10_1)   /* THERM_OUT / A1 */
#define NTC_PIN_OUT2  (P10_2)   /* THERM_OUT / A2 */

/* NCP18XH103F03RB constants (see the shield pins header / datasheet): */
#define NTC_R_REF        (10000.0f)     /* fixed 10k series resistor */
#define NTC_B_CONST      (3380.0f)      /* B25/85 (Kelvin) */
#define NTC_R_INFINITY   (0.1192855f)   /* R0 * exp(-B/T0), T0 = 25 C */

/* Configure the SAR + NTC GPIO for the thermistor (A1 + A2) and the internal
 * DieTemp sensor channels (single SAR config, VREF = VDDA). */
void ntc_init(void);

/* Sample A1 + A2 (both polarities) and return the NTC temperature in C. */
float ntc_read_celsius(void);

/* Read the internal Die Temperature sensor and return degrees C. */
int16_t dts_read_celsius(void);

/* Raw SAR count of the internal DieTemp sensor (read once at startup). */
uint16_t dts_raw_value(void);

/* Raw SAR counts for A0/A1/A2/A3 (P10.0..P10.3) and the internal DTS. */
void ntc_read_raw(uint16_t *a0, uint16_t *a1, uint16_t *a2, uint16_t *a3, uint16_t *dts);

/* Pin voltages in millivolts (VDDA = 3.3 V full scale). */
void ntc_read_mv(uint16_t *a0_mv, uint16_t *a1_mv, uint16_t *a2_mv, uint16_t *a3_mv);

/* NTC temperature from the raw A0 (full-scale) / A1 / A2 / A3 counts:
 * Vsample = (A1+A2)/2 - A3, rNTC = R_ref * Vsample / (A0 - A3 - Vsample). */
float ntc_temp_from_raw(uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3);

#endif /* NTC_H_ */
