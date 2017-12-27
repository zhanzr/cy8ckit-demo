/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"

#define HZ 1000

volatile static uint32_t g_Ticks;
//SysTick的ISR
void SysTick_Handler(void)
{
  g_Ticks++;
}

int main(void)
{
    /* Configure SysTick */
    SysTick_Config(SystemCoreClock/HZ);
    
    __enable_irq(); /* Enable global interrupts. */
    /* Enable CM4.  CY_CORTEX_M4_APPL_ADDR must be updated if CM4 memory layout is changed. */
    Cy_SysEnableCM4(CY_CORTEX_M4_APPL_ADDR); 

    printf("PSOC6 CM0+ @ %u Hz\n", SystemCoreClock);
    volatile uint32_t testClk_Cm0 = SystemCoreClock;
    for(;;)
    {
        /* Place your application code here. */
        Cy_GPIO_Inv(LED1_0_PORT, LED1_0_NUM); /* toggle the pin */
        Cy_SysLib_Delay(1500/*msec*/);  
        Cy_GPIO_Inv(LED2_0_PORT, LED2_0_NUM); /* toggle the pin */        
        Cy_SysLib_Delay(1500/*msec*/);
    }
}

/* [] END OF FILE */
