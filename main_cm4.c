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
#include <stdio.h>

/*******************************************************************************
*        Function Prototypes
*******************************************************************************/
void handle_error(void);

/*******************************************************************************
*            Constants
*******************************************************************************/
#define LED_ON               (0x00u)
#define LED_OFF              (!LED_ON)
#define HZ 1000
/* UART receive ring buffer size */
#define RING_BUFFER_SIZE     (32u)

/* The number of characters to be received from terminal before echoing */
#define CHARACTER_BUNCH_SIZE (10u)

/*******************************************************************************
*            Global variables
*******************************************************************************/

/***************************************************************************//**
* Function Name: handle_error
********************************************************************************
*
* This function processes unrecoverable errors such as any component 
* initialization errors etc. In case of such error the system will switch on 
* ERROR_RED_LED and stay in the infinite loop of this function.
*
*******************************************************************************/
void handle_error(void)
{   
     /* Disable all interrupts */
    __disable_irq();
    
    /* Switch on error LED */
//    Cy_GPIO_Write(ERROR_RED_LED_0_PORT, ERROR_RED_LED_0_NUM, LED_ON);
    while(1u) {}
}

volatile static uint32_t g_Ticks;
//SysTick的ISR
void SysTick_Handler(void)
{
  g_Ticks++;
}

int _write(int fd, const void *buffer, size_t count)
{
	uint32_t tmpCnt = count;

    for(uint32_t i=0; i<count; ++i)
    {
	    Cy_SCB_UART_Put(UART_1_HW, *((char*)buffer+i));
    }
    
	return tmpCnt;
}

int main(void)
{
    cy_en_scb_uart_status_t uart_status;
    cy_en_sysint_status_t sysint_status;
    uint8_t uartBuffer[RING_BUFFER_SIZE+1];
    
    /* Configure SysTick */
    SysTick_Config(SystemCoreClock/HZ);

 /* Initialize the UART operation */
    uart_status = Cy_SCB_UART_Init(UART_1_HW, &UART_1_config, &UART_1_context);
    if(uart_status != CY_SCB_UART_SUCCESS)
    {
        handle_error();
    }
    
    /* Start the UART receive ring buffer */
    Cy_SCB_UART_StartRingBuffer(UART_1_HW, uartBuffer, RING_BUFFER_SIZE, &UART_1_context);
    
    /* Enable the UART */
    Cy_SCB_UART_Enable(UART_1_HW);
        
    __enable_irq(); /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    /* Transmit header to the terminal. */
    Cy_SCB_UART_PutString(UART_1_HW, "GitHub is home to over 20 million developers working together to host \r\n");

    printf("PSOC6 CM4 @ %u Hz\n", SystemCoreClock);
    
    /* Wait for completion of header transmit */
    while (Cy_SCB_UART_IsTxComplete(UART_1_HW) == false);
    
    for(;;)
    {
        /* Place your application code here. */
        Cy_GPIO_Inv(LED3_0_PORT, LED3_0_NUM); /* toggle the pin */
        
        Cy_GPIO_Inv(LED4_0_PORT, LED4_0_NUM); /* toggle the pin */        
        
        Cy_GPIO_Inv(LED5_0_PORT, LED5_0_NUM); /* toggle the pin */       
        
        Cy_SysLib_Delay(500/*msec*/);
        Cy_SysLib_Delay(500/*msec*/);
        Cy_SysLib_Delay(500/*msec*/);
    }
}

/* [] END OF FILE */
