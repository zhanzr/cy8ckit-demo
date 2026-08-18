#ifndef __UTILS_H__
#define	__UTILS_H__

#include <stdint.h>

void TICK_Init(void);
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t t);

#endif
