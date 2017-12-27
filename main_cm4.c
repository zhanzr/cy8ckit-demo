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

int main(void)
{
    __enable_irq(); /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */

    for(;;)
    {
        /* Place your application code here. */
        Cy_GPIO_Inv(LED3_0_PORT, LED3_0_NUM); /* toggle the pin */
        Cy_SysLib_Delay(500/*msec*/);
        Cy_GPIO_Inv(LED4_0_PORT, LED4_0_NUM); /* toggle the pin */        
        Cy_SysLib_Delay(500/*msec*/);
        Cy_GPIO_Inv(LED5_0_PORT, LED5_0_NUM); /* toggle the pin */                
        Cy_SysLib_Delay(500/*msec*/);
    }
}

/* [] END OF FILE */
