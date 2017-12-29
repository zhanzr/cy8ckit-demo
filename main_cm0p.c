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


/*
 * C Converted Whetstone Double Precision Benchmark
 *		Version 1.2	22 March 1998
 *
 *	(c) Copyright 1998 Painter Engineering, Inc.
 *		All Rights Reserved.
 *
 *		Permission is granted to use, duplicate, and
 *		publish this text and program as long as it
 *		includes this entire comment block and limited
 *		rights reference.
 *
 * Converted by Rich Painter, Painter Engineering, Inc. based on the
 * www.netlib.org benchmark/whetstoned version obtained 16 March 1998.
 *
 * A novel approach was used here to keep the look and feel of the
 * FORTRAN version.  Altering the FORTRAN-based array indices,
 * starting at element 1, to start at element 0 for C, would require
 * numerous changes, including decrementing the variable indices by 1.
 * Instead, the array E1[] was declared 1 element larger in C.  This
 * allows the FORTRAN index range to function without any literal or
 * variable indices changes.  The array element E1[0] is simply never
 * used and does not alter the benchmark results.
 *
 * The major FORTRAN comment blocks were retained to minimize
 * differences between versions.  Modules N5 and N12, like in the
 * FORTRAN version, have been eliminated here.
 *
 * An optional command-line argument has been provided [-c] to
 * offer continuous repetition of the entire benchmark.
 * An optional argument for setting an alternate LOOP count is also
 * provided.  Define PRINTOUT to cause the POUT() function to print
 * outputs at various stages.  Final timing measurements should be
 * made with the PRINTOUT undefined.
 *
 * Questions and comments may be directed to the author at
 *			r.painter@ieee.org
 */
/*
C**********************************************************************
C     Benchmark #2 -- Double  Precision Whetstone (A001)
C
C     o	This is a REAL*8 version of
C	the Whetstone benchmark program.
C
C     o	DO-loop semantics are ANSI-66 compatible.
C
C     o	Final measurements are to be made with all
C	WRITE statements and FORMAT sttements removed.
C
C**********************************************************************   
*/
/* map the FORTRAN math functions, etc. to the C versions */
#define DSIN	sin
#define DCOS	cos
#define DATAN	atan
#define DLOG	log
#define DEXP	exp
#define DSQRT	sqrt
#define IF		if

/* function prototypes */
void POUT(long N, long J, long K, DOUBLE X1, DOUBLE X2, DOUBLE X3, DOUBLE X4);
void PA(DOUBLE E[]);
void P0(void);
void P3(DOUBLE X, DOUBLE Y, DOUBLE *Z);
#define USAGE	"usage: whetdc [-c] [loops]\n"

/*
	COMMON T,T1,T2,E1(4),J,K,L
*/
DOUBLE T,T1,T2,E1[5];
int J,K,L;

void SysTick_Handler(void)
{
  g_Ticks++;
}

uint32_t HAL_GetTick(void)
{
    return g_Ticks;
}


void
PA(DOUBLE E[])
{
	J = 0;

L10:
	E[1] = ( E[1] + E[2] + E[3] - E[4]) * T;
	E[2] = ( E[1] + E[2] - E[3] + E[4]) * T;
	E[3] = ( E[1] - E[2] + E[3] + E[4]) * T;
	E[4] = (-E[1] + E[2] + E[3] + E[4]) / T2;
	J += 1;

	if (J < 6)
		goto L10;
}

void
P0(void)
{
	E1[J] = E1[K];
	E1[K] = E1[L];
	E1[L] = E1[J];
}

void
P3(DOUBLE X, DOUBLE Y, DOUBLE *Z)
{
	DOUBLE X1, Y1;

	X1 = X;
	Y1 = Y;
	X1 = T * (X1 + Y1);
	Y1 = T * (X1 + Y1);
	*Z  = (X1 + Y1) / T2;
}

#ifdef PRINTOUT
void
POUT(long N, long J, long K, DOUBLE X1, DOUBLE X2, DOUBLE X3, DOUBLE X4)
{
	printf("%7ld %7ld %7ld %12.4e %12.4e %12.4e %12.4e\n",
						N, J, K, X1, X2, X3, X4);
}
#endif

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
    
    printf("PSOC6 Whetstone Demo CPUID:%08X @ %u Hz, single:%u, DOUBLE:%u\r\n",
    SCB->CPUID,
    SystemCoreClock, 
    sizeof(float), 
    sizeof(DOUBLE));    
    
/* used in the FORTRAN version */
	long I;
	long N1, N2, N3, N4, N6, N7, N8, N9, N10, N11;
	DOUBLE X1,X2,X3,X4,X,Y,Z;
	long LOOP;
	int II, JJ;

	/* added for this version */
	long loopstart;
//	long startsec, finisec;
	uint32_t start_tick, fini_tick;
	float KIPS;
	int continuous;
	
//On my host PC
//Loops: 500000, Iterations: 1, Duration: 16 sec.
//C Converted Double Precision Whetstones: 3125.0 MIPS	
	loopstart = LOOP_NUM;		/* see the note about LOOP below */
	continuous = 0;
	II = 1;		/* start at the first arg (temp use of II here) */
    
    __enable_irq(); /* Enable global interrupts. */

LCONT:
/*
C
C	Start benchmark timing at this point.
C
*/
	start_tick = HAL_GetTick();

/*
C
C	The actual benchmark starts here.
C
*/
	T  = .499975;
	T1 = 0.50025;
	T2 = 2.0;
/*
C
C	With loopcount LOOP=10, one million Whetstone instructions
C	will be executed in EACH MAJOR LOOP..A MAJOR LOOP IS EXECUTED
C	'II' TIMES TO INCREASE WALL-CLOCK TIMING ACCURACY.
C
	LOOP = 1000;
*/
	LOOP = loopstart;
	II   = 1;

	JJ = 1;

IILOOP:
	N1  = 0;
	N2  = 12 * LOOP;
	N3  = 14 * LOOP;
	N4  = 345 * LOOP;
	N6  = 210 * LOOP;
	N7  = 32 * LOOP;
	N8  = 899 * LOOP;
	N9  = 616 * LOOP;
	N10 = 0;
	N11 = 93 * LOOP;
/*
C
C	Module 1: Simple identifiers
C
*/
	X1  =  1.0;
	X2  = -1.0;
	X3  = -1.0;
	X4  = -1.0;

	for (I = 1; I <= N1; I++) {
	    X1 = (X1 + X2 + X3 - X4) * T;
	    X2 = (X1 + X2 - X3 + X4) * T;
	    X3 = (X1 - X2 + X3 + X4) * T;
	    X4 = (-X1+ X2 + X3 + X4) * T;
	}
#ifdef PRINTOUT
	IF (JJ==II)POUT(N1,N1,N1,X1,X2,X3,X4);
#endif

/*
C
C	Module 2: Array elements
C
*/
	E1[1] =  1.0;
	E1[2] = -1.0;
	E1[3] = -1.0;
	E1[4] = -1.0;

	for (I = 1; I <= N2; I++) {
	    E1[1] = ( E1[1] + E1[2] + E1[3] - E1[4]) * T;
	    E1[2] = ( E1[1] + E1[2] - E1[3] + E1[4]) * T;
	    E1[3] = ( E1[1] - E1[2] + E1[3] + E1[4]) * T;
	    E1[4] = (-E1[1] + E1[2] + E1[3] + E1[4]) * T;
	}

#ifdef PRINTOUT
	IF (JJ==II)POUT(N2,N3,N2,E1[1],E1[2],E1[3],E1[4]);
#endif

/*
C
C	Module 3: Array as parameter
C
*/
	for (I = 1; I <= N3; I++)
		PA(E1);

#ifdef PRINTOUT
	IF (JJ==II)POUT(N3,N2,N2,E1[1],E1[2],E1[3],E1[4]);
#endif

/*
C
C	Module 4: Conditional jumps
C
*/
	J = 1;
	for (I = 1; I <= N4; I++) {
		if (J == 1)
			J = 2;
		else
			J = 3;

		if (J > 2)
			J = 0;
		else
			J = 1;

		if (J < 1)
			J = 1;
		else
			J = 0;
	}

#ifdef PRINTOUT
	IF (JJ==II)POUT(N4,J,J,X1,X2,X3,X4);
#endif

/*
C
C	Module 5: Omitted
C 	Module 6: Integer arithmetic
C
*/

	J = 1;
	K = 2;
	L = 3;

	for (I = 1; I <= N6; I++) {
	    J = J * (K-J) * (L-K);
	    K = L * K - (L-J) * K;
	    L = (L-K) * (K+J);
	    E1[L-1] = J + K + L;
	    E1[K-1] = J * K * L;
	}

#ifdef PRINTOUT
	IF (JJ==II)POUT(N6,J,K,E1[1],E1[2],E1[3],E1[4]);
#endif

/*
C
C	Module 7: Trigonometric functions
C
*/
	X = 0.5;
	Y = 0.5;

	for (I = 1; I <= N7; I++) {
		X = T * DATAN(T2*DSIN(X)*DCOS(X)/(DCOS(X+Y)+DCOS(X-Y)-1.0));
		Y = T * DATAN(T2*DSIN(Y)*DCOS(Y)/(DCOS(X+Y)+DCOS(X-Y)-1.0));
	}

#ifdef PRINTOUT
	IF (JJ==II)POUT(N7,J,K,X,X,Y,Y);
#endif

/*
C
C	Module 8: Procedure calls
C
*/
	X = 1.0;
	Y = 1.0;
	Z = 1.0;

	for (I = 1; I <= N8; I++)
		P3(X,Y,&Z);

#ifdef PRINTOUT
	IF (JJ==II)POUT(N8,J,K,X,Y,Z,Z);
#endif

/*
C
C	Module 9: Array references
C
*/
	J = 1;
	K = 2;
	L = 3;
	E1[1] = 1.0;
	E1[2] = 2.0;
	E1[3] = 3.0;

	for (I = 1; I <= N9; I++)
		P0();

#ifdef PRINTOUT
	IF (JJ==II)POUT(N9,J,K,E1[1],E1[2],E1[3],E1[4]);
#endif

/*
C
C	Module 10: Integer arithmetic
C
*/
	J = 2;
	K = 3;

	for (I = 1; I <= N10; I++) {
	    J = J + K;
	    K = J + K;
	    J = K - J;
	    K = K - J - J;
	}

#ifdef PRINTOUT
	IF (JJ==II)POUT(N10,J,K,X1,X2,X3,X4);
#endif

/*
C
C	Module 11: Standard functions
C
*/
	X = 0.75;

	for (I = 1; I <= N11; I++)
		X = DSQRT(DEXP(DLOG(X)/T1));

#ifdef PRINTOUT
	IF (JJ==II)POUT(N11,J,K,X,X,X,X);
#endif

/*
C
C      THIS IS THE END OF THE MAJOR LOOP.
C
*/
	if (++JJ <= II)
		goto IILOOP;

/*
C
C      Stop benchmark timing at this point.
C
*/
	fini_tick = HAL_GetTick();

/*
C----------------------------------------------------------------
C      Performance in Whetstone KIP's per second is given by
C
C	(100*LOOP*II)/TIME
C
C      where TIME is in seconds.
C--------------------------------------------------------------------
*/
	printf("\n");
	if (fini_tick-start_tick <= 1000) {
		printf("Insufficient duration- Increase the LOOP count\n");
		return(1);
	}

	printf("Loops: %ld, Iterations: %d, Duration: %u/%u sec.\n",
			LOOP, II, fini_tick-start_tick, 1000);

	KIPS = (100.0*LOOP*II)/(float)((fini_tick-start_tick)/1000);
	if (KIPS >= 1000.0)
		printf("C Converted Double Precision Whetstones: %u.%u MIPS\n",
            (uint32_t)(KIPS/1000),
            ((uint32_t)KIPS)%1000);
	else
		printf("C Converted Double Precision Whetstones: %u.%u KIPS\n",
            (uint32_t)(KIPS*100)/100,
            (uint32_t)(KIPS*100)%100);
	
	if (continuous)
		goto LCONT;
    
    /* Enable CM4.  CY_CORTEX_M4_APPL_ADDR must be updated if CM4 memory layout is changed. */
    Cy_SysEnableCM4(CY_CORTEX_M4_APPL_ADDR); 
    
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
        
//        ipc_output(SCB->CPUID, g_Ticks);
        
        /* Place your application code here. */
        Cy_GPIO_Inv(LED1_0_PORT, LED1_0_NUM); /* toggle the pin */
        Cy_SysLib_Delay(500/*msec*/);  
        Cy_GPIO_Inv(LED2_0_PORT, LED2_0_NUM); /* toggle the pin */        
        Cy_SysLib_Delay(500/*msec*/);        
    }
}

/* [] END OF FILE */
