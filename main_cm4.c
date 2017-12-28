#include <stdio.h>
#include <errno.h>

#include "project.h"

#include "dhry.h"

#include "ipc_user.h"

#define TEST_RUN_NUM	800000

#ifndef MSC_CLOCK
#define MSC_CLOCK

#define HZ	1000
#endif

/* UART receive ring buffer size */
#define RING_BUFFER_SIZE     (250)

/*******************************************************************************
*        Function Prototypes
*******************************************************************************/
void handle_error(void);
/*******************************************************************************
*            Global variables
*******************************************************************************/
static volatile uint32_t mySysError;  /* general-purpose status/error code */
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

uint32_t HAL_GetTick(void)
{
    return g_Ticks;
}

int _write(int fd, const void *buffer, size_t count)
{
	uint32_t tmpCnt = count;

    Cy_SCB_UART_PutArrayBlocking(UART_1_HW, buffer, count);
    
	return tmpCnt;
}


/* Global Variables: */

Rec_Pointer     Ptr_Glob,
                Next_Ptr_Glob;
int             Int_Glob;
Boolean         Bool_Glob;
char            Ch_1_Glob,
                Ch_2_Glob;
int             Arr_1_Glob [50];
int             Arr_2_Glob [50] [50];

#ifndef REG
        Boolean Reg = false;
#define REG
        /* REG becomes defined as empty */
        /* i.e. no register variables   */
#else
        Boolean Reg = true;
#endif

/* variables for time measurement: */

#ifdef TIMES
struct tms      time_info;
extern  int     times (void);
                /* see library function "times" */
#define Too_Small_Time (2*HZ)
                /* Measurements should last at least about 2 seconds */
#endif
#ifdef TIME
extern long     time(long *);
                /* see library function "time"  */
#define Too_Small_Time 2
                /* Measurements should last at least 2 seconds */
#endif
#ifdef MSC_CLOCK
//extern clock_t clock(void);
#define Too_Small_Time (2*HZ)
#endif

long            Begin_Time,
                End_Time,
                User_Time;
float           Microseconds,
                Dhrystones_Per_Second;

/* end of variables for time measurement */


void Proc_1 (Rec_Pointer Ptr_Val_Par)
/******************/
    /* executed once */
{
  REG Rec_Pointer Next_Record = Ptr_Val_Par->Ptr_Comp;
                                        /* == Ptr_Glob_Next */
  /* Local variable, initialized with Ptr_Val_Par->Ptr_Comp,    */
  /* corresponds to "rename" in Ada, "with" in Pascal           */

  structassign (*Ptr_Val_Par->Ptr_Comp, *Ptr_Glob);
  Ptr_Val_Par->variant.var_1.Int_Comp = 5;
  Next_Record->variant.var_1.Int_Comp = Ptr_Val_Par->variant.var_1.Int_Comp;
  Next_Record->Ptr_Comp = Ptr_Val_Par->Ptr_Comp;
  Proc_3 (&Next_Record->Ptr_Comp);
    /* Ptr_Val_Par->Ptr_Comp->Ptr_Comp == Ptr_Glob->Ptr_Comp */
  if (Next_Record->Discr == Ident_1)
    /* then, executed */
  {
    Next_Record->variant.var_1.Int_Comp = 6;
    Proc_6 (Ptr_Val_Par->variant.var_1.Enum_Comp,
           &Next_Record->variant.var_1.Enum_Comp);
    Next_Record->Ptr_Comp = Ptr_Glob->Ptr_Comp;
    Proc_7 (Next_Record->variant.var_1.Int_Comp, 10,
           &Next_Record->variant.var_1.Int_Comp);
  }
  else /* not executed */
    structassign (*Ptr_Val_Par, *Ptr_Val_Par->Ptr_Comp);
} /* Proc_1 */


void Proc_2 (One_Fifty *Int_Par_Ref)
/******************/
    /* executed once */
    /* *Int_Par_Ref == 1, becomes 4 */
{
  One_Fifty  Int_Loc;
  Enumeration   Enum_Loc;

  Int_Loc = *Int_Par_Ref + 10;
  do /* executed once */
    if (Ch_1_Glob == 'A')
      /* then, executed */
    {
      Int_Loc -= 1;
      *Int_Par_Ref = Int_Loc - Int_Glob;
      Enum_Loc = Ident_1;
    } /* if */
    while (Enum_Loc != Ident_1); /* true */
} /* Proc_2 */


void Proc_3 (Rec_Pointer *Ptr_Ref_Par)
/******************/
    /* executed once */
    /* Ptr_Ref_Par becomes Ptr_Glob */
{
  if (Ptr_Glob != Null)
    /* then, executed */
    *Ptr_Ref_Par = Ptr_Glob->Ptr_Comp;
  Proc_7 (10, Int_Glob, &Ptr_Glob->variant.var_1.Int_Comp);
} /* Proc_3 */


void Proc_4 (void) /* without parameters */
/*******/
    /* executed once */
{
  Boolean Bool_Loc;

  Bool_Loc = Ch_1_Glob == 'A';
  Bool_Glob = Bool_Loc | Bool_Glob;
  Ch_2_Glob = 'B';
} /* Proc_4 */


void Proc_5 (void) /* without parameters */
/*******/
    /* executed once */
{
  Ch_1_Glob = 'A';
  Bool_Glob = false;
} /* Proc_5 */


        /* Procedure for the assignment of structures,          */
        /* if the C compiler doesn't support this feature       */
#ifdef  NOSTRUCTASSIGN
memcpy (d, s, l)
register char   *d;
register char   *s;
register int    l;
{
        while (l--) *d++ = *s++;
}
#endif

int main(void)
{
    One_Fifty       Int_1_Loc;
    REG One_Fifty   Int_2_Loc;
    One_Fifty       Int_3_Loc;
    REG char        Ch_Index;
    Enumeration     Enum_Loc;
    Str_30          Str_1_Loc;
    Str_30          Str_2_Loc;
    REG int         Run_Index;
    REG int         Number_Of_Runs;

    IPC_STRUCT_Type *myIpcHandle; /* handle for the IPC channel being used */

    /* A global shared variable is defined in the codefor the other CPU.
       Its address is given to the this CPU via an IPC channel. */
    uint8_t *sharedVar;
    
 /* Configure SysTick */
    SysTick_Config(SystemCoreClock/HZ);
    
    __enable_irq(); /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    /* Transmit header to the terminal. */
    printf("PSOC6 Dhrystone CM4 @ %u Hz\r\n", SystemCoreClock);


  Next_Ptr_Glob = (Rec_Pointer) malloc (sizeof (Rec_Type));
  Ptr_Glob = (Rec_Pointer) malloc (sizeof (Rec_Type));

  Ptr_Glob->Ptr_Comp                    = Next_Ptr_Glob;
  Ptr_Glob->Discr                       = Ident_1;
  Ptr_Glob->variant.var_1.Enum_Comp     = Ident_3;
  Ptr_Glob->variant.var_1.Int_Comp      = 40;
  strcpy (Ptr_Glob->variant.var_1.Str_Comp,
          "DHRYSTONE PROGRAM, SOME STRING");
  strcpy (Str_1_Loc, "DHRYSTONE PROGRAM, 1'ST STRING");

  Arr_2_Glob [8][7] = 10;
        /* Was missing in published program. Without this statement,    */
        /* Arr_2_Glob [8][7] would have an undefined value.             */
        /* Warning: With 16-Bit processors and Number_Of_Runs > 32000,  */
        /* overflow may occur for this array element.                   */

  printf ("\n");
  printf ("Dhrystone Benchmark, Version 2.1 (Language: C)\n");
  printf ("\n");
  if (Reg)
  {
    printf ("Program compiled with 'register' attribute\n");
    printf ("\n");
  }
  else
  {
    printf ("Program compiled without 'register' attribute\n");
    printf ("\n");
  }
  printf ("Please give the number of runs through the benchmark: ");
  {
//    int n;
//    scanf ("%d", &n);
    Number_Of_Runs = TEST_RUN_NUM;
  }
  printf ("\n");

  printf ("Execution starts, %d runs through Dhrystone\n", Number_Of_Runs);

  /***************/
  /* Start timer */
  /***************/

#ifdef TIMES
  times (&time_info);
  Begin_Time = (long) time_info.tms_utime;
#endif
#ifdef TIME
  Begin_Time = time ( (long *) 0);
#endif
#ifdef MSC_CLOCK
  Begin_Time = HAL_GetTick();
#endif

  for (Run_Index = 1; Run_Index <= Number_Of_Runs; ++Run_Index)
  {

    Proc_5();
    Proc_4();
      /* Ch_1_Glob == 'A', Ch_2_Glob == 'B', Bool_Glob == true */
    Int_1_Loc = 2;
    Int_2_Loc = 3;
    strcpy (Str_2_Loc, "DHRYSTONE PROGRAM, 2'ND STRING");
    Enum_Loc = Ident_2;
    Bool_Glob = ! Func_2 (Str_1_Loc, Str_2_Loc);
      /* Bool_Glob == 1 */
    while (Int_1_Loc < Int_2_Loc)  /* loop body executed once */
    {
      Int_3_Loc = 5 * Int_1_Loc - Int_2_Loc;
        /* Int_3_Loc == 7 */
      Proc_7 (Int_1_Loc, Int_2_Loc, &Int_3_Loc);
        /* Int_3_Loc == 7 */
      Int_1_Loc += 1;
    } /* while */
      /* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
    Proc_8 (Arr_1_Glob, Arr_2_Glob, Int_1_Loc, Int_3_Loc);
      /* Int_Glob == 5 */
    Proc_1 (Ptr_Glob);
    for (Ch_Index = 'A'; Ch_Index <= Ch_2_Glob; ++Ch_Index)
                             /* loop body executed twice */
    {
      if (Enum_Loc == Func_1 (Ch_Index, 'C'))
         /* then, not executed */
      {
        Proc_6 (Ident_1, &Enum_Loc);
        strcpy (Str_2_Loc, "DHRYSTONE PROGRAM, 3'RD STRING");
        Int_2_Loc = Run_Index;
        Int_Glob = Run_Index;
      }
    }
      /* Int_1_Loc == 3, Int_2_Loc == 3, Int_3_Loc == 7 */
    Int_2_Loc = Int_2_Loc * Int_1_Loc;
    Int_1_Loc = Int_2_Loc / Int_3_Loc;
    Int_2_Loc = 7 * (Int_2_Loc - Int_3_Loc) - Int_1_Loc;
      /* Int_1_Loc == 1, Int_2_Loc == 13, Int_3_Loc == 7 */
    Proc_2 (&Int_1_Loc);
      /* Int_1_Loc == 5 */

  } /* loop "for Run_Index" */

  /**************/
  /* Stop timer */
  /**************/

#ifdef TIMES
  times (&time_info);
  End_Time = (long) time_info.tms_utime;
#endif
#ifdef TIME
  End_Time = time ( (long *) 0);
#endif
#ifdef MSC_CLOCK
  End_Time = HAL_GetTick();
#endif

  printf ("Execution ends\n");
  printf ("\n");
  printf ("Final values of the variables used in the benchmark:\n");
  printf ("\n");
  printf ("Int_Glob:            %d\n", Int_Glob);
  printf ("        should be:   %d\n", 5);
  printf ("Bool_Glob:           %d\n", Bool_Glob);
  printf ("        should be:   %d\n", 1);
  printf ("Ch_1_Glob:           %c\n", Ch_1_Glob);
  printf ("        should be:   %c\n", 'A');
  printf ("Ch_2_Glob:           %c\n", Ch_2_Glob);
  printf ("        should be:   %c\n", 'B');
  printf ("Arr_1_Glob[8]:       %d\n", Arr_1_Glob[8]);
  printf ("        should be:   %d\n", 7);
  printf ("Arr_2_Glob[8][7]:    %d\n", Arr_2_Glob[8][7]);
  printf ("        should be:   Number_Of_Runs + 10\n");
  printf ("Ptr_Glob->\n");
  printf ("  Ptr_Comp:          %d\n", (int) Ptr_Glob->Ptr_Comp);
  printf ("        should be:   (implementation-dependent)\n");
  printf ("  Discr:             %d\n", Ptr_Glob->Discr);
  printf ("        should be:   %d\n", 0);
  printf ("  Enum_Comp:         %d\n", Ptr_Glob->variant.var_1.Enum_Comp);
  printf ("        should be:   %d\n", 2);
  printf ("  Int_Comp:          %d\n", Ptr_Glob->variant.var_1.Int_Comp);
  printf ("        should be:   %d\n", 17);
  printf ("  Str_Comp:          %s\n", Ptr_Glob->variant.var_1.Str_Comp);
  printf ("        should be:   DHRYSTONE PROGRAM, SOME STRING\n");
  printf ("Next_Ptr_Glob->\n");
  printf ("  Ptr_Comp:          %d\n", (int) Next_Ptr_Glob->Ptr_Comp);
  printf ("        should be:   (implementation-dependent), same as above\n");
  printf ("  Discr:             %d\n", Next_Ptr_Glob->Discr);
  printf ("        should be:   %d\n", 0);
  printf ("  Enum_Comp:         %d\n", Next_Ptr_Glob->variant.var_1.Enum_Comp);
  printf ("        should be:   %d\n", 1);
  printf ("  Int_Comp:          %d\n", Next_Ptr_Glob->variant.var_1.Int_Comp);
  printf ("        should be:   %d\n", 18);
  printf ("  Str_Comp:          %s\n",
                                Next_Ptr_Glob->variant.var_1.Str_Comp);
  printf ("        should be:   DHRYSTONE PROGRAM, SOME STRING\n");
  printf ("Int_1_Loc:           %d\n", Int_1_Loc);
  printf ("        should be:   %d\n", 5);
  printf ("Int_2_Loc:           %d\n", Int_2_Loc);
  printf ("        should be:   %d\n", 13);
  printf ("Int_3_Loc:           %d\n", Int_3_Loc);
  printf ("        should be:   %d\n", 7);
  printf ("Enum_Loc:            %d\n", Enum_Loc);
  printf ("        should be:   %d\n", 1);
  printf ("Str_1_Loc:           %s\n", Str_1_Loc);
  printf ("        should be:   DHRYSTONE PROGRAM, 1'ST STRING\n");
  printf ("Str_2_Loc:           %s\n", Str_2_Loc);
  printf ("        should be:   DHRYSTONE PROGRAM, 2'ND STRING\n");
  printf ("\n");

  User_Time = End_Time - Begin_Time;

      if (User_Time < Too_Small_Time)
      {
        printf ("Measured time too small to obtain meaningful results\n");
        printf ("Please increase number of runs\n");
        printf ("\n");
      }
      else
      {
    #ifdef TIME
        Microseconds = (float) User_Time * Mic_secs_Per_Second
                            / (float) Number_Of_Runs;
        Dhrystones_Per_Second = (float) Number_Of_Runs / (float) User_Time;
    #else
        Microseconds = (float) User_Time * Mic_secs_Per_Second
                            / ((float) HZ * ((float) Number_Of_Runs));
        Dhrystones_Per_Second = ((float) HZ * (float) Number_Of_Runs)
                            / (float) User_Time;
    #endif
        printf ("[CM4]uSeconds for one run through Dhrystone: ");
        printf ("%u.%02d \n",
            (uint32_t)((100*Microseconds)/100),
        ((uint32_t)(100*Microseconds)%100));
        printf ("Dhrystones per Second:                      ");
        printf ("%u.%02d \n",
            (uint32_t)(100*Dhrystones_Per_Second)/100,
            ((uint32_t)(100*Dhrystones_Per_Second)%100));
        printf ("\n");
     }
    
 /* Read the shared memory address, from the other CPU. Wait for success, which indicates that
       (1) the sending CPU acquired the lock, and (2) this CPU read the pointer. */
    myIpcHandle = Cy_IPC_Drv_GetIpcBaseAddress(MY_IPC_CHANNEL);
    while (Cy_IPC_Drv_ReadMsgPtr(myIpcHandle, (void *)&sharedVar) != CY_IPC_DRV_SUCCESS);
    /* Release the lock. This indicates to the other CPU that the read is 
       successfully complete. */
    mySysError = Cy_IPC_Drv_LockRelease(myIpcHandle, CY_IPC_NO_NOTIFICATION);
    if (mySysError != CY_IPC_DRV_SUCCESS) handle_error();

    /* Initialize the IPC semaphore subsystem. The other CPU already defined the semaphore
       array address, so this CPU just initializes the IPC channel number. */
    if (Cy_IPC_Sema_Init(CY_IPC_CHAN_SEMA, (uint32_t)NULL, (uint32_t *)NULL) != CY_IPC_SEMA_SUCCESS) handle_error();

    for(;;)
    {
        /* Always use semaphore set/clear functions to access the shared variable;
           do computations on a local copy of the shared variable. */
        uint8_t copy;

        /* Wait for the 4 MS bits of the shared variable to NOT equal the 4 LS bits.
           The other CPU increments the 4 LS bits. */
        do
        {
            /* grab a copy of the shared variable, with semaphore set/clear */
            mySysError = ReadSharedVar(sharedVar, &copy);
            if (mySysError != (uint32_t)CY_IPC_SEMA_SUCCESS) handle_error();
            /* brief delay to give the other CPU a chance to acquire the semaphore */
            CyDelayUs(1ul/*usec*/);
        } while (((copy >> 4) & 0x0Fu) == (copy & 0x0Fu));

        /* Make the 4 MS bits of the shared variable equal the 4 LS bits. */
        copy = ((copy & 0x0Fu) << 4) | (copy & 0x0Fu);
        /* write the shared variable, with semaphore set/clear */
        mySysError = WriteSharedVar(sharedVar, copy);
        if (mySysError != (uint32_t)CY_IPC_SEMA_SUCCESS) handle_error();
        
        /* Place your application code here. */
        Cy_GPIO_Inv(LED3_0_PORT, LED3_0_NUM); /* toggle the pin */
        
        Cy_GPIO_Inv(LED4_0_PORT, LED4_0_NUM); /* toggle the pin */        
        
        Cy_GPIO_Inv(LED5_0_PORT, LED5_0_NUM); /* toggle the pin */       
        
        Cy_SysLib_Delay(5000/*msec*/);        
    }
}

/* [] END OF FILE */
