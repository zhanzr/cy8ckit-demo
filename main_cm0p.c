#include <stdio.h>
#include <errno.h>

#include "project.h"

#include "ipc_user.h"


/******************************************************************************
*            Global variables
*******************************************************************************/
/* Global shared variable is static within this file. The address of this
   variable is given to the CM4 via an IPC channel. */
static uint8_t sharedVar;

static volatile uint32_t mySysError;  /* general-purpose status/error code */

static uint32_t myArray[4]; /* for Cy_IPC_Sema_Init() */

volatile static uint32_t g_Ticks;
//SysTick的ISR
void SysTick_Handler(void)
{
  g_Ticks++;
}

uint32_t HAL_GetTick(void)
{
    return g_Ticks;
}

int main(void)
{
    IPC_STRUCT_Type *myIpcHandle; /* handle for the IPC channel being used */
    
    /* Always use semaphore set/clear functions to access the shared variable;
       do computations on a local copy of the shared variable. */
    uint8_t copy;
    
    /* Configure SysTick */
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
    
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    /* Initialize the IPC semaphore subsystem. This must done by this CPU, with definition of
       semaphore array address, before the other CPU starts using the semaphore system. */
    if (Cy_IPC_Sema_Init(CY_IPC_CHAN_SEMA, sizeof(myArray) * 8ul/*bits per byte*/, myArray) != CY_IPC_SEMA_SUCCESS)
    {
        handle_error();
    }
    /* Notify the CM4 with the shared memory address. There is no interrupt associated with this notification.
       Wait for channel to be available, then acquire the lock for it, in a single atomic operation;
       and send the address. */
    myIpcHandle = Cy_IPC_Drv_GetIpcBaseAddress(MY_IPC_CHANNEL);
    
    printf("PSOC6 IPC Demo CPUID:%08X @ %u Hz\r\n",
        SCB->CPUID,
        SystemCoreClock);    
    
    /* Enable CM4.  CY_CORTEX_M4_APPL_ADDR must be updated if CM4 memory layout is changed. */
    Cy_SysEnableCM4(CY_CORTEX_M4_APPL_ADDR); 
    
    __enable_irq(); /* Enable global interrupts. */
    
    //    while(Cy_IPC_Drv_SendMsgPtr(myIpcHandle, CY_IPC_NO_NOTIFICATION, &sharedVar) != CY_IPC_DRV_SUCCESS);
    //    /* Wait for release, which indicates other CPU has read the pointer value. */
    //    while (Cy_IPC_Drv_IsLockAcquired(myIpcHandle));   

    for(;;)
    { 
//        copy = (copy & 0xF0u) | ((copy + 1u) & 0x0Fu);
//        /* write the shared variable, with semaphore set/clear */
//        mySysError = WriteSharedVar(&sharedVar, copy);
//        if (mySysError != (uint32_t)CY_IPC_SEMA_SUCCESS) handle_error();
//
//        /* Wait for the 4 MS bits of the shared variable to equal the 4 LS bits.
//           The other CPU sets the 4 MS bits to be equal to the 4 LS bits. */
//        do
//        {
//            /* grab a copy of the shared variable, with semaphore set/clear */
//            mySysError = ReadSharedVar(&sharedVar, &copy);
//            if (mySysError != (uint32_t)CY_IPC_SEMA_SUCCESS) handle_error();
//            /* brief delay to give the other CPU a chance to acquire the semaphore */
//            CyDelayUs(1ul/*usec*/);
//        } while (((copy >> 4) & 0x0Fu) != (copy & 0x0Fu));
        
        ipc_output(SCB->CPUID, g_Ticks);
        
        /* Place your application code here. */
        Cy_GPIO_Inv(LED1_0_PORT, LED1_0_NUM); /* toggle the pin */
//        Cy_SysLib_Delay(500/*msec*/);  
        Cy_GPIO_Inv(LED2_0_PORT, LED2_0_NUM); /* toggle the pin */        
//        Cy_SysLib_Delay(500/*msec*/);        
    }
}

/* [] END OF FILE */
