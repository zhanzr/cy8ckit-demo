#ifndef __IPC_USER_H__
#define __IPC_USER_H__
    
#include "project.h"

/* Select which IPC channel is used for this application.
   The channel is used only for initialization, where the address of the 
   shared variable is passed from one CPU to the other. */
#define MY_IPC_CHANNEL 8u

/* Select which IPC semaphore is used for this application. 
   The semaphore is used in both CPUs' do forever loops, for mutex-based access
   to the shared variable.*/
#define MY_SEMANUM 0u

/* function prototypes */
uint32_t ReadSharedVar(const uint8_t *sharedVar, uint8_t *copy);
uint32_t WriteSharedVar(uint8_t *sharedVar, uint8_t value);

#endif
/* [] END OF FILE */
