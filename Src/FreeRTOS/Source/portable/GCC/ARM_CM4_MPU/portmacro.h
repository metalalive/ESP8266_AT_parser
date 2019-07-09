/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------
// the settings in this file are for integration with STM32
// Hardware Abstraction Layer (HAL) and any STM32 board that 
// includes ARM Cortex-M4 microcontroller
// --------------------------------------------------------------

// include defined macros in STM32 low-level functions.
#include "stm32f4xx_ll_cortex.h"

#define portCHAR     char
#define portSHORT    short
#define portINT      int
#define portLONG     long
#define portFLOAT    float
#define portDOIUBLE  double

#define portSTACK_TYPE  uint32_t
#define portBASE_TYPE   portINT

typedef portSTACK_TYPE          StackType_t;
typedef portBASE_TYPE           BaseType_t;
// note: unsigned int32_t is NOT allowed in GCC, but unsigned int is OK
typedef unsigned  portBASE_TYPE UBaseType_t;

// -------------- time tick datatype --------------
#if( configUSE_16_BIT_TICKS == 1 )
	typedef uint16_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffff
#else
	typedef uint32_t TickType_t;
	#define portMAX_DELAY ( TickType_t ) 0xffffffffUL
	// 32-bit tick type on a 32-bit architecture, so reads of the 
        // tick count do not need to be guarded with a critical section. 
	#define portTICK_TYPE_IS_ATOMIC 1
#endif




// -------- MPU specific constants used in RTOS kernel code -------- 
//
// enable it when your application requires to seperate tasks into "privileged tasks"
// and "unprivileged tasks". A privileged task can access all the special registers
// defiend in Cortex-M4
//
// If MPU is enabled & a task is created by a call to xTaskCreateRestricted(),
// then the task can only access limited memory areas determined when it was created.
// In most cases, a task is allowed to access its stack memory (also called
//  stack buffer in FreeRTOS), and some array variables required & declared in
// application, an application developer must explicitly pass the required array
// variables to the structure "MemoryRegion_t xRegions" inside the task structure 
// "TaskParameters_t"  on creating any task using xTaskCreateRestricted() .
#define portUSING_MPU_WRAPPERS  1

// privilege / unprivilege flag. This can be used when you'd like to create a
// privileged task 
#define portPRIVILEGE_BIT       ( 0x80000000 )

// ---- Access permission on a MPU reqion ----
// all accesses are NOT allowed & generates permission fault
#define portMPU_REGION_NO_ACCESS              LL_MPU_REGION_NO_ACCESS 
// only allow privileged accesses
#define portMPU_REGION_PRIVILEGED_READ_WRITE  LL_MPU_REGION_PRIV_RW
// only allow privileged accesses & unprivileged read,
// unprivileged writes will result in permission fault.
#define portMPU_REGION_PRIV_RW_URO            LL_MPU_REGION_PRIV_RW_URO
// full access to a region
#define portMPU_REGION_READ_WRITE             LL_MPU_REGION_FULL_ACCESS
// privileged read only (others NOT allowed)
#define portMPU_REGION_PRIVILEGED_READ_ONLY   LL_MPU_REGION_PRIV_RO
//  privileged read & unprivileged read only
#define portMPU_REGION_READ_ONLY              LL_MPU_REGION_PRIV_RO_URO

// other attributes on a MPU reqion : shareable, cacheable, bufferable
#define portMPU_REGION_SHAREABLE     LL_MPU_ACCESS_SHAREABLE
#define portMPU_REGION_CACHEABLE     LL_MPU_ACCESS_CACHEABLE 
#define portMPU_REGION_BUFFERABLE    LL_MPU_ACCESS_BUFFERABLE  
// whether to enable instruction fetch from the region ? if set, then you cannot
// put instruction code in this region. this region is only for data load
// or store operations .
#define portMPU_REGION_EXEC_NEVER    LL_MPU_INSTRUCTION_ACCESS_DISABLE 

// Here are the MPU regions planning in this STM32 port, 
// some of the region below might not be used in your application. 
// Cortex-M4 supports up to 8  MPU regions, each has different attributes
// this depends on your underlying hardware. In STM32 please refer to STM32
// programming manual for recommended MPU configuration.
// 
// part of flash memory can be unprivileged region for function code / data
// every task can access.
#define portUNPRIVILEGED_FLASH_REGION    ( 0 )
// part of flash memory can be privileged region for function code / data
// only privileged tasks can access them.
#define portPRIVILEGED_FLASH_REGION      ( 1 )
// part of SRAM can be privileged region
#define portPRIVILEGED_SRAM_REGION       ( 2 )
// it's for preipheral devices that occupy range the address space, their
// memory attributes must be different  from normal memory like SRAM, flash
#define portGENERAL_PERIPHERALS_REGION   ( 3 )
// the stack memory of each application task
#define portSTACK_REGION                 ( 4 )
// There are still 3 MPU regions available & free to use in your application
// , from region #5 to region #7
#define portFIRST_CONFIGURABLE_REGION    ( 5 )
#define portLAST_CONFIGURABLE_REGION     ( 7 )
#define portNUM_CONFIGURABLE_REGIONS     (( portLAST_CONFIGURABLE_REGION - portFIRST_CONFIGURABLE_REGION ) + 1 )

// structure type to collect register content for each MPU region
typedef struct {
    UBaseType_t  RBAR; // Region Base Address Register
    UBaseType_t  RASR; // Region Attribute & Size Register
} xMPU_REGION_REGS;

// structure types to collect register content of the MPU regions
// , particularly available to an application task.
// * Reqion 0, Reqion 1, 2 are for setting privileged / unprivileged
//   code / data sections
// * Region 3 is for peripherals in the underlying hardware platform
// * Region 4 is for stack space of a task, 
// * Region 5 to 7 are available to a task & optionally used.
// So there are 4 regions that can be used in a task
typedef struct {
    xMPU_REGION_REGS xRegion[ portNUM_CONFIGURABLE_REGIONS + 1 ];
} xMPU_SETTINGS;

// calculate RASR.SIZE in MPU, the input value represents MPU region
// size in bytes, the allowable value to SIZE field are from 4
// (means 2 to power of (4+1) = 32 Bytes ) ,
//  SIZE field     MPU region size (Bytes)
// ------------   -------------------------
//     5               2^(5+1) =  64
//     6               2^(6+1) = 128
//    ...                ..........
//     31             2^(31+1) = 4GB
//
UBaseType_t  prvMPUregionSizeEncode(UBaseType_t ulSizeInBytes);

// privileged / unprivileged check, implemented in port.c
// return pdFALSE = privileged, otherwise = unprivileged
BaseType_t  xIsPrivileged( void );
// return pdTRUE if privilege state is changed (raised) in this call
BaseType_t  xPortRaisePrivilege( void );
void        vPortResetPrivilege( BaseType_t privRaised );

#define  portIS_PRIVILEGED()      xIsPrivileged()
#define  portRESET_PRIVILEGE(privRaised)    vPortResetPrivilege(privRaised)
#define  portRAISE_PRIVILEGE()    \
         __asm volatile ( \
             "svc %0 \n" : \
             :"i" ( portSVC_ID_RAISE_PRIVILEGE ) \
             :"memory" \
         ); 





// ------------------- Architectureu specific --------------------- 
// peripheral address space in Cortex-M4 is from 0x40000000 to 0x5fffffff
#define PERIPH_SIZE 0x20000000UL
// stack grouth direction indicates stack pointer moves upwards or
// downwards every time when is it used, 
// negative number means it moves downwards; while opposite number
// means it moves upwards.
#define portSTACK_GROWTH        ( -1 )
// a period of time in milliseconds between 2 tick events
#define portTICK_PERIOD_MS      (( TickType_t ) 1000 / configTICK_RATE_HZ)
// used for alignment check in stack & heap memory
#define portBYTE_ALIGNMENT       8
// In Cortex-M4, LSB of the address should be zero for alignment of
// memory accesses.
#define portCPU_ADDRESS_MASK    (( StackType_t ) 0xfffffffeUL )

// In Cortex-M4:
// * tasks running in Thread mode conventionally use PSP (process stack),
//   kernel / exception handler in Handler mode only use MSP (main stack)
// * MSP is used on reset by default
// if we want to make CPU switch to PSP and then run a task using PSP, 
// CPU can execute branch instruction, jump to special address 0xfffffffd
// at exception return. CPU will automatically switch to PSP once it
// gets the special address, also automatically update SPSEL bit of
// CONTROL register.
#define portEXPTN_RETURN_TO_TASK  0xfffffffdUL
// when saving context of last running task, bit 4 of exception return 
// address indicates whether the task has floating-point data to save,
// bit 4 = 0 means it uses floating point data, otherwise it doesn't
#define portEXPTN_RETURN_TO_TASK_FP  0xffffffedUL

// Here is the task selection optimization in Cortex-M4 port :
// FreeRTOS uses variable uxTopReadyPriority that holds priority
// number of the highest-priority ready task. The variable is updated
// every time when a task's TCB is moved to ready state list, and the
// variable is read on context switch.
// In the optimization code here, uxTopReadyPriority is NOT a number,
// instead it is considered as a 32-bit map, each bit represents 
// existence of "ready task(s)" with specific priority, 
// e.g. uxTopReadyPriority = 0x1a at a given point of time
//      , then there are at least :
//          * one ready task with priority number 4
//          * one ready task with priority number 3
//          * one ready task with priority number 1
// we can get the highest priority number among all the ready tasks
// by going through each bit, from MSB of uxTopReadyPriority, until
// we find first bit which is set to one, an instruction "clz" in
// Cortex-M4 can do the job in few CPU clock cycles.
#ifndef configUSE_PORT_OPTIMISED_TASK_SELECTION
	#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1
#endif
#if (configUSE_PORT_OPTIMISED_TASK_SELECTION == 1)
    #if ( configMAX_PRIORITIES > 0x20 )
        #error configUSE_PORT_OPTIMISED_TASK_SELECTION can only be set \
         to 1 when configMAX_PRIORITIES is less than or equal to 0x20. \
         It is very rare that a system requires  more than 0x20 different \
         priorities
    #endif
    // store/clear the priorities for all ready tasks in the bitmap
    #define portRECORD_READY_PRIORITY( uxRdyTskPrioIn, uxRdyTskPrioBitmap ) \
            uxRdyTskPrioBitmap |=  ( 0x1 << uxRdyTskPrioIn )
    #define portRESET_READY_PRIORITY( uxRdyTskPrioIn, uxRdyTskPrioBitmap ) \
            uxRdyTskPrioBitmap &= ~( 0x1 << uxRdyTskPrioIn )
    // get the highest priority number among all ready tasks
    #define portGET_HIGHEST_PRIORITY( uxRdyTskPrioOut, uxRdyTskPrioBitmap) \
            { \
                __asm volatile ( \
                    "clz %0, %1 " \
                    :"=r" (uxRdyTskPrioOut) \
                    :"r"  (uxRdyTskPrioBitmap) \
                    :"memory" \
                ); \
                uxRdyTskPrioOut = 0x1f - (UBaseType_t)uxRdyTskPrioOut; \
            }
#endif //// end of configUSE_PORT_OPTIMISED_TASK_SELECTION





// --------- SVC numbers for various privileged services ------------ 
#define portSVC_ID_START_SCHEDULER    0
#define portSVC_ID_YIELD              1
#define portSVC_ID_RAISE_PRIVILEGE    2



// ------------- for context switch -------------
void prvRestoreContextOfFirstTask( void );

void vPortPendSVHandler( void );


// ------------------ task yielding functions -----------------------
// with MPU enabled, and prilvileged / unprivileged tasks created in your
// application, few services can be placed in SVC exception handling routine, 
// it means a running task in unprivileged Thread Mode must generate exception
// event firstly so the task can switch to Handler Mode (always privileged)
// and then get the service, this is typical usage of SVC, as an entry to OS 
// system call

// the yield funciton that should be implemented in port.c
void vPortYield();
// make call to SVC handler routine with argument portSVC_ID_YIELD
// indicate SVC handler routine will call vPortYield() to perform 
// context switch
#define portYIELD()  __asm volatile(   \
                         "SVC %0 \n"    \
                         :: "i" (portSVC_ID_YIELD) \
                         : "memory");   \
    
// [TODO] 
// figure out the difference between portYIELD_WITHIN_API and portYIELD .
// In official release of FreeRTOS ARM_CM4_MPU port :
// * portYIELD generates SVC exception with parameter --> then generates
//   PendSV exception to perform context switch
// * portYIELD_WITHIN_API directly generates PendSV exception, it is likely
//   used only in FreeRTOS API functions .
// would that be problem if an unprivileged task calls a FreeRTOS API
// function that internally invokes portYIELD_WITHIN_API ??
#define portYIELD_WITHIN_API()  vPortYield() 

// yield function that can be used only in ISR
#define portEND_SWITCHING_ISR( xSwitchRequired ) if(xSwitchRequired != pdFALSE) vPortYield()
#define portYIELD_FROM_ISR( xSwitchRequired )    portEND_SWITCHING_ISR( xSwitchRequired )




// ----------------------- critical sections -----------------------
// enter / exit critical section, implemented in port.c
void vPortEnterCritical( void );
void vPortExitCritical( void );
#define portENTER_CRITICAL()    vPortEnterCritical()
#define portEXIT_CRITICAL()     vPortExitCritical()




// --------------------- interrupt masking ----------------------
__INLINE UBaseType_t ulPortRaiseBASEPRI( void );
__INLINE void        vPortRaiseBASEPRI( void );
__INLINE void        vPortSetBASEPRI( UBaseType_t ulNewMaskValue );

// In Cortex-M4 port, to create critical section in ISR, you should
// use portSET_INTERRUPT_MASK_FROM_ISR() and
// portCLEAR_INTERRUPT_MASK_FROM_ISR().
//
// ISR temporarily disable late-arriving-but-lower-priority interrupts
// by calling portSET_INTERRUPT_MASK_FROM_ISR() , it will set 
// BASEPRI register to configMAX_SYSCALL_INTERRUPT_PRIORITY and 
// return this interrupt's original priority. Your program has to
// preserve the return value until the ISR leaves from the critical
// section.
// Then ISR use the preserved value to recover the content of BASEPRI
// register, then recvoer to original priority 
#define portSET_INTERRUPT_MASK_FROM_ISR()       ulPortRaiseBASEPRI()
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)    vPortSetBASEPRI(x)
// interrupt masking functions used only in tasks.
#define portDISABLE_INTERRUPTS()                vPortRaiseBASEPRI()
#define portENABLE_INTERRUPTS()                 vPortSetBASEPRI(0)




// check if CPU runs inside ISR currently
BaseType_t  xPortIsInsideInterrupt( void );




// -------------------- assertion check --------------------
#ifdef configASSERT
// the function is to check the interrupt priority in ISR. FreeRTOS supports
// interrupt nesting and has a concept called  maximum system call interrupt
// priority : configMAX_SYSCALL_INTERRUPT_PRIORITY, in other words FreeRTOS
// restricts the port-specific interrupt priority to a number greater than
//  configMAX_SYSCALL_INTERRUPT_PRIORITY.
//
// When a FreeRTOS API function is called in an ISR, we have to check :
// * its ISR_NUMBER defined in IPSR
// * the interrupt priority, must be numerically greater than 
//   configMAX_SYSCALL_INTERRUPT_PRIORITY, if this condition is NOT satisfied,
//   then interrupt-safe FreeRTOS function running in one ISR might be 
//   preempted by higher-priority interrupt in the middle, an interrupt-safe
//   FreeRTOS function execution shouldn't be preempted.
//
// So any interrupt, that attempts to call FreeRTOS API functions, must have
// priority below configMAX_SYSCALL_INTERRUPT_PRIORITY, otherwise the
// interrupt will stop working because of assertion failure in following
// function
    void    vPortValidateInterruptPriority( void );
    #define portASSERT_IF_INTERRUPT_PRIORITY_INVALID()  vPortValidateInterruptPriority()
#endif



// -------------------- tickless idle --------------------
// For those applications that only work in short period of time & spend
// most of time sleeping, this optional (port-specific) feature saves power  
// by reducing number of SysTick exception event firing periodically.
// e.g.
// * one tick is equal to 1 ms, 
// * there is only one task in an application,
// the only task works periodically in every 4 ticks, quickly gets its
// job done in one tick, then waits until next time after 4 ticks passes.
// Without tickless idle, the task still can sleep but will be woken up
// every time when other 3 tick events occur. 
// In such case, it is no need to fire tick event 3 times to weak up the
// task then immediately go to sleep again, we can configure tick counter
// to fire only once, to make CPU remain in deep sleep mode
//  (sleep for longer time) .
//
#if( configUSE_TICKLESS_IDLE == 1 )
    #define  portSUPPRESS_TICKS_AND_SLEEP  vPortSuppressTicksAndSleep
    void  vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime );
#endif //// end of configUSE_TICKLESS_IDLE




// ----------------------------------------------------------------
// Task function macros described on the FreeRTOS.org are optional 
// to use in this port.  They are defined so the common demo files
// (which build with all the ports) will build.
#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )




#ifdef __cplusplus
}
#endif // end of extern "C" { ... }

#endif // end of PORTMACRO_H
