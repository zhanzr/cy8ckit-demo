/*
 * ntc.h - NTC thermistor (NCP18XH103F03RB) + internal DTS temperature reading
 * for the CY8CKIT-028-EPD shield on CY8CKIT-062-BLE.
 *
 * The shield NTC circuit:
 *   THERM_VDD (P10.0/A0) -- 10k -- THERM_OUT (P10.1/A1, P10.2/A2) -- NTC --
 *   THERM_GND (P10.3/A3)
 *
 * The 0R strap resistors that would tie A0 to VIO_REF and A3 to GND are NOT
 * populated on this shield, so the GPIO drives the divider: A0 high (VDDD
 * ~3.3 V) and A3 low (0 V). Only A1/A2 (the divider output) are sampled by the
 * SAR ADC; the driven A0/A3 pins cannot be read cleanly through the SARMUX.
 *
 *   OUT  = (A1+A2)/2  (mV)
 *   rNTC = R_ref * OUT / (VDD - OUT)
 *   T    = B / ln(rNTC / R_infinity) - 273.15
 */

#ifndef NTC_H_
#define NTC_H_

#include <stdint.h>

/* Thermistor pins (CY8CKIT-028-EPD): */
#define NTC_PIN_VDD   (P10_0)   /* THERM_VDD / A0 - driven high (supply) */
#define NTC_PIN_GND   (P10_3)   /* THERM_GND / A3 - driven low (return) */
#define NTC_PIN_OUT1  (P10_1)   /* THERM_OUT / A1 - sampled */
#define NTC_PIN_OUT2  (P10_2)   /* THERM_OUT / A2 - sampled */

/* NCP18XH103F03RB constants (see the shield pins header / datasheet): */
#define NTC_R_REF        (10000.0f)     /* fixed 10k series resistor */
#define NTC_B_CONST      (3380.0f)      /* B25/85 (Kelvin) */
#define NTC_R_INFINITY   (0.1192855f)   /* R0 * exp(-B/T0), T0 = 25 C */

/* A0 driven high to VDDD (millivolts) - the divider supply. */
#define NTC_DRIVE_VDD_MV (3300u)

/* Drive the divider (A0 high / A3 low), init the SAR for A1/A2, and read the
 * internal DieTemp sensor once. */
void ntc_init(void);

/* Sample A1/A2 and return the NTC temperature in C. */
float ntc_read_celsius(void);

/* Read the internal Die Temperature sensor and return degrees C. */
int16_t dts_read_celsius(void);

/* Raw SAR count of the internal DieTemp sensor (read once at startup). */
uint16_t dts_raw_value(void);

/* Raw ADC counts for A1/A2 (P10.1/P10.2) and the internal DTS. */
void ntc_read_raw(uint16_t *a1, uint16_t *a2, uint16_t *dts);

/* A1/A2 voltages in millivolts. */
void ntc_read_mv(uint16_t *a1_mv, uint16_t *a2_mv);

/* NTC temperature from the A1/A2 divider output (mV) against the 3.3 V drive. */
float ntc_temp_from_a1a2_mv(uint16_t a1_mv, uint16_t a2_mv);

#endif /* NTC_H_ */
