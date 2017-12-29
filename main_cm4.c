#include <stdio.h>
#include <errno.h>

#include "project.h"

#include "ipc_user.h"

/*******************************************************************************
*        Function Prototypes
*******************************************************************************/

/*******************************************************************************
*            Global variables
*******************************************************************************/
static volatile uint32_t mySysError;  /* general-purpose status/error code */

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

    /* A global shared variable is defined in the codefor the other CPU.
       Its address is given to the this CPU via an IPC channel. */
    uint8_t *sharedVar;
    
 /* Configure SysTick */
    SysTick_Config(SystemCoreClock/HZ);

    /* Read the shared memory address, from the other CPU. Wait for success, which indicates that
       (1) the sending CPU acquired the lock, and (2) this CPU read the pointer. */
//    myIpcHandle = Cy_IPC_Drv_GetIpcBaseAddress(MY_IPC_CHANNEL);
//    while (Cy_IPC_Drv_ReadMsgPtr(myIpcHandle, (void *)&sharedVar) != CY_IPC_DRV_SUCCESS);
//    /* Release the lock. This indicates to the other CPU that the read is 
//       successfully complete. */
//    mySysError = Cy_IPC_Drv_LockRelease(myIpcHandle, CY_IPC_NO_NOTIFICATION);
//    if (mySysError != CY_IPC_DRV_SUCCESS) handle_error();

    /* Initialize the IPC semaphore subsystem. The other CPU already defined the semaphore
       array address, so this CPU just initializes the IPC channel number. */
    if (Cy_IPC_Sema_Init(CY_IPC_CHAN_SEMA, (uint32_t)NULL, (uint32_t *)NULL) != CY_IPC_SEMA_SUCCESS) handle_error();

    __enable_irq(); /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    /* Transmit header to the terminal. */
        printf("PSOC6 IPC Demo CPUID:%08X @ %u Hz\r\n",
        SCB->CPUID,
        SystemCoreClock);

    for(;;)
    {
        /* Always use semaphore set/clear functions to access the shared variable;
           do computations on a local copy of the shared variable. */
//        uint8_t copy;
//
//        /* Wait for the 4 MS bits of the shared variable to NOT equal the 4 LS bits.
//           The other CPU increments the 4 LS bits. */
//        do
//        {
//            /* grab a copy of the shared variable, with semaphore set/clear */
//            mySysError = ReadSharedVar(sharedVar, &copy);
//            if (mySysError != (uint32_t)CY_IPC_SEMA_SUCCESS) handle_error();
//            /* brief delay to give the other CPU a chance to acquire the semaphore */
//            CyDelayUs(1ul/*usec*/);
//        } while (((copy >> 4) & 0x0Fu) == (copy & 0x0Fu));
//
//        /* Make the 4 MS bits of the shared variable equal the 4 LS bits. */
//        copy = ((copy & 0x0Fu) << 4) | (copy & 0x0Fu);
//        /* write the shared variable, with semaphore set/clear */
//        mySysError = WriteSharedVar(sharedVar, copy);
//        if (mySysError != (uint32_t)CY_IPC_SEMA_SUCCESS) handle_error();
        
        ipc_output(SCB->CPUID, g_Ticks);
    
        /* Place your application code here. */
        Cy_GPIO_Inv(LED3_0_PORT, LED3_0_NUM); /* toggle the pin */
        
        Cy_GPIO_Inv(LED4_0_PORT, LED4_0_NUM); /* toggle the pin */        
        
        Cy_GPIO_Inv(LED5_0_PORT, LED5_0_NUM); /* toggle the pin */       
        
//        Cy_SysLib_Delay(1000/*msec*/);        
    }
}

/* [] END OF FILE */
