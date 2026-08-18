#ifndef SAR_H
#define SAR_H

#include <stdint.h>

/* Initializes the SAR ADC to sample the internal DieTemp sensor. */
void sar_temp_init(void);

/* Runs a single DieTemp scan and returns the die temperature in degrees C. */
int16_t sar_temp_read_celsius(void);

#endif /* SAR_H */
