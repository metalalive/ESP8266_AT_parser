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


// --------------------------------------------------------------
// Implementation of functions defined in portable.h and portmacro.h
// , the implemenetation in this file are for integration with STM32
// Hardware Abstraction Layer (HAL) and any STM32 board that 
// includes ARM Cortex-M4 microcontroller.
// --------------------------------------------------------------

// ----------------- defined macros, included headers -----------------
// by defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prior to the 2 
// C header files below, we can prevent task.h from redefining all
// the API functions to use the MPU wrappers. This should be done only
// when task.h is included from an application file.
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "FreeRTOS.h"
#include "task.h"
// check if ARM VFP unit is in use
#ifndef __VFP_FP__
    #error This port can only be used when the project options are \
     configured to enable hardware floating point support.
#endif // end of  __VFP_FP__
#undef  MPU_WRAPPERS_INCLUDED_FROM_API_FILE


#if( configASSERT_DEFINED == 1 )
// To call FreeRTOS API functions in ISR, applicaiton writers must
// use the FreeRTOS function whose the name ends with "FromISR". 
// The priority value of interrupt must be numerically greater than
// the defined macro configMAX_SYSCALL_INTERRUPT_PRIORITY.
// ucMaxInNvicIPRx represents maximum valid value for interrupts, which
// attempts to call FreeRTOS API function.
PRIVILEGED_DATA static unsigned portCHAR   ucMaxInNvicIPRx ;
// in Cortex-M4, every 8-bit NVIC_IPRx can be split to "priority group"
// and "sub-priority" , one can also choose not to implement sub-priority
// by setting AIRCR.PRIGROUP < 0x4 in System Control Block.
// For simplicity, FreeRTOS doesn't need sub-priority, so AIRCR.PRIGROUP
// should be less than 0x4 for the interrupts which calls interrupt-safe 
// version of FreeRTOS API function.
PRIVILEGED_DATA static unsigned portSHORT  ucMaxPriGroupInAIRCR;
#endif // end of configASSERT_DEFINED

// This Cortex-M4 port will access first 2 members of current task's TCB,
// that are top of stack pointer & the pointer to MPU settings.
extern void * volatile pxCurrentTCB;

// once a task entered a critical section before it leaves, it can reenter
// the critical section many times, it also has to leave the same section
// the same times to let other tasks go in.
PRIVILEGED_DATA static UBaseType_t uxCriticalNesting = 0;




StackType_t  *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxTaskStartFunc, void *pvParams, BaseType_t xRunPrivileged)
{
    // simulate the stack frame for exception return & context switch,
    // kernel will pop the stack frame when it returns from SVC exception
    // (or PendSV exception) after context switch.

    // the stack frame layout must meets requirement on Cortex-M4.
    // first dataword of the stack frame is reserved. 
    pxTopOfStack--;
    // xPSR.T = 1 means the Cortex-M4 should be in thumb mode, everything
    // else can be zero .
    *pxTopOfStack = xPSR_T_Msk;
    pxTopOfStack--;
    // write start address of task function to following place ,
    // then it will be written to PC at exception return ( after
    // context switch)
    *pxTopOfStack = ((StackType_t) pxTaskStartFunc) & portCPU_ADDRESS_MASK;
    pxTopOfStack--;
    // set LR = 0, when task completes its work, it should delete itself
    // by calling API function xTaskDelete() instead of return to address
    // 0x0
    *pxTopOfStack = 0x0;
    pxTopOfStack--;
    // skip registers  r12, r3, r2, r1
    pxTopOfStack -= 4;
    // store argument pointer for the task function at here,
    // on exception return it will be written to register r0
    *pxTopOfStack = (StackType_t)pvParams;
    pxTopOfStack--;
    // on exception return, program jumps to this special address,then
    // CPU decodes this address, switch (from MSP) to PSP. 
    *pxTopOfStack = portEXPTN_RETURN_TO_TASK;
    pxTopOfStack--;
    // keep space for R4-R11, for consistency 
    pxTopOfStack -= 8;
    // modify privilege bit of CONtrol register before exception return
    //, with MPU enabled, each task has its own privilege attribute
    if( xRunPrivileged == pdTRUE ) {
        *pxTopOfStack = CONTROL_SPSEL_Msk  ;
    }
    else {
        *pxTopOfStack = CONTROL_SPSEL_Msk | CONTROL_nPRIV_Msk ;
    }
    
    return pxTopOfStack;
} //// end of pxPortInitialiseStack



void vPortYield()
{
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __asm volatile( "dsb" ::: "memory" );
    __asm volatile( "isb" );
} // end of vPortYield



void  vPortSetBASEPRI( UBaseType_t ulNewMaskValue )
{
    __set_BASEPRI( ulNewMaskValue );
} // end of vPortSetBASEPRI


void  vPortRaiseBASEPRI( void )
{ // let compiler decide which registers to use
    __set_BASEPRI( configMAX_SYSCALL_INTERRUPT_PRIORITY );
    __asm volatile(
        "isb               \n"
        "dsb               \n"
    );
} // end of vPortRaiseBASEPRI



UBaseType_t ulPortRaiseBASEPRI( void )
{ // let compiler decide which registers to use
    UBaseType_t ulOriginBasepri = __get_BASEPRI();
    __set_BASEPRI( configMAX_SYSCALL_INTERRUPT_PRIORITY );
    __asm volatile(
        "isb    \n"
        "dsb    \n"
    );
    return  ulOriginBasepri;
} // end of ulPortRaiseBASEPRI




UBaseType_t  prvMPUregionSizeEncode(UBaseType_t ulSizeInBytes)
{
    UBaseType_t  MPU_RASR_size  = 4; // 2**(4+1)
    UBaseType_t  ulSize         = (ulSizeInBytes - 1) >> 5;
    // start from smallest allowed size of RASR.SIZE : 32 Bytes
    while(ulSize > 0)
    {
        ulSize = ulSize >> 1;
        MPU_RASR_size++;
    }
    return MPU_RASR_size;
}




// [Note]
// carefully implement this function & recheck disassembly code,
// this function switches from msp to psp, any stacking operation
// at here is likely to corrupt program execution.
// To avoid GCC compiler from dynamically generating push/pop
// instructions to perform stacking operations at here, it's better
// to check C coding style in disassembly code.
void prvRestoreContextOfFirstTask( void )
{
    // stack pointer should be reset only when task scheduler starts
    // working & picking up first task to run
    StackType_t  init_sp  = *(StackType_t *)(SCB->VTOR);
    __asm volatile (
        "msr msp, %0    \n"
        ::"r"(init_sp) :
    );

    // restore MPU region base address & attributes from task TCB to MPU
    volatile xMPU_SETTINGS *next_task_mpu_setup = 
           (volatile xMPU_SETTINGS *)((UBaseType_t *)pxCurrentTCB + 1);
    // [Reminder] 
    // that task TCB stores upto 4 MPU regions, these are numbered
    // from #4 to #7, the first 4 regions (#0 to #3) are reserved for
    // RTOS kernel. Here we copy the 4 regions' settings to MPU_RBAR,
    // MPU_RASR, and make use of the 3 pairs of alias registers
    // (MPU_RBAR_A1, MPU_RASR_A1, MPU_RBAR_A2 ....)
    __asm volatile (
        // For region #4 : r5 = MPU_RBAR,  r6  = MPU_RASR
        // For region #5 : r7 = MPU_RBAR,  r8  = MPU_RASR
        // For region #6 : r9 = MPU_RBAR,  r10 = MPU_RASR
        // For region #7 : r11 = MPU_RBAR, r12 = MPU_RASR
        "ldmia %0!, {r5-r12}   \n" 
        "stmia %1!, {r5-r12}   \n"
        :: "r"(next_task_mpu_setup), "r"(&(MPU->RBAR))
        :
    );

    // ensure we have one task ready to run (pxCurrentTCB in task.c) 
    // read stack pointer of pxCurrentTCB, pop the stack frames to 
    // register
    StackType_t  next_task_sp  = *(StackType_t *)pxCurrentTCB;
    __asm volatile (
        "mov   r0, %0                   \n"
        "ldmia r0!, {r3, r4-r11, lr}    \n"
        // it seems that we must modify psp before we set CONTROL.SPSEL
        // otherwise we get HardFault exception
        "msr  psp, r0                    \n"
        "msr  control, r3                \n"
        ::"r"(next_task_sp):
    );
    
    // clear interrupt mask & start running first task
    // To avoid build tool from automatically generating unnecessary stack operations (at instruction level)
    // ,we use the CMSIS macro __set_BASEPRI() at here instead of portCLEAR_INTERRUPT_MASK_FROM_ISR()
    __set_BASEPRI( 0x0 );
} //// end of prvRestoreContextOfFirstTask( void )






void vPortPendSVHandler( void )
{
    // these are the addresses to stack pointer, not its content.
    StackType_t *prev_tsk_sp_addr = (StackType_t *)pxCurrentTCB;
    StackType_t *next_tsk_sp_addr = NULL ;
    // store context to the stack of previously running task
    __asm volatile (
        // r0-r3 are automatically stacked onto current task's stack memory
        // , so they are available in this routine to copy rest of GPRs before
        // we perform context switch.
        "mrs       r0, psp     \n"
        "isb                   \n"
        // check bit 4 of exception return address to see whether to
        // save floating-point registers.
        "tst       lr, #0x10   \n"
        "it        eq          \n"
        "vstmdbeq  r0!, {s16-s31}   \n"
        // save CONTROL register, r4-r11, and lr.
        // Note every task might have different exception return address
        "mrs       r1, control      \n"
        "stmdb     r0!, {r1, r4-r11, lr} \n"
        // store current psp to previously running task
        "str       r0,  [%0]             \n"
        :"=r"(prev_tsk_sp_addr)::
    );
     
    // temporarily disable higher-priority-but-late-arriving interrupts
    // during context switch.
    // To avoid build tool from automatically generating unnecessary stack operations (at instruction level)
    // ,we use the CMSIS macro __set_BASEPRI() at here instead of portCLEAR_INTERRUPT_MASK_FROM_ISR()
    __set_BASEPRI( configMAX_SYSCALL_INTERRUPT_PRIORITY );
    __asm volatile(
        "isb               \n"
        "dsb               \n"
    );
    __asm volatile ("bl    vTaskSwitchContext  \n");
    // To avoid build tool from automatically generating unnecessary stack operations (at instruction level)
    // ,we use the CMSIS macro __set_BASEPRI() at here instead of portCLEAR_INTERRUPT_MASK_FROM_ISR()
    __set_BASEPRI( 0x0 );
    // load context to the stack of next running task
    xMPU_SETTINGS *next_task_mpu_setup = 
           (xMPU_SETTINGS *)((UBaseType_t *)pxCurrentTCB + 1);
    // restore MPU region setting of next running task, this must be done
    // before we restore the GPRs of next running task
    __asm volatile (
        // For region #4 : r5 = MPU_RBAR,  r6  = MPU_RASR
        // For region #5 : r7 = MPU_RBAR,  r8  = MPU_RASR
        // For region #6 : r9 = MPU_RBAR,  r10 = MPU_RASR
        // For region #7 : r11 = MPU_RBAR, r12 = MPU_RASR
        "ldmia %0!, {r5-r12}   \n" 
        "stmia %1!, {r5-r12}   \n"
        :: "r"(next_task_mpu_setup), "r"(&(MPU->RBAR))
    );
    next_tsk_sp_addr = (StackType_t *)pxCurrentTCB;
    __asm volatile (
        // load stack pointer of next running task  
        "ldr    r0, [%0]     \n"
        // restore CONTROL register, r4-r11, and lr.
        // Note every task might have different exception return address
        "ldmia  r0!, {r1, r4-r11, lr} \n"
        // check bit 4 of exception return address to see whether to
        // save floating-point registers.
        "tst       lr, #0x10        \n"
        "it        eq               \n"
        "vldmiaeq  r0!, {s16-s31}   \n"
        // restore current stack pointer to psp
        "msr    psp, r0   \n"
        // [experiment] modify CONTROL before we modify psp
        "msr    control, r1      \n"
        ::"r"(next_tsk_sp_addr):
    );
} //// end of vPortPendSVHandler




void vPortSetMPUregion ( xMPU_REGION_REGS *xRegion )
{ // write the given xMPUSettings to MPU_MBAR , MPU_RASR 
    MPU->RBAR  = xRegion->RBAR ;
    MPU->RASR  = xRegion->RASR ;
} //// end of vCopyMPUregionSetupToCheckList


void vPortGetMPUregion ( portSHORT regionID, xMPU_REGION_REGS *xRegion )
{
    MPU->RNR = regionID;
    xRegion->RBAR = MPU->RBAR;
    xRegion->RASR = MPU->RASR;
} //// end of vCopyMPUregionSetupToCheckList





PRIVILEGED_FUNCTION void prvSetupMPU( void ) 
{
    extern  UBaseType_t  __privileged_code_end__[];
    extern  UBaseType_t  __code_segment_start__ [];
    extern  UBaseType_t  __code_segment_end__[];
    extern  UBaseType_t  __privileged_data_start__[];
    extern  UBaseType_t  __privileged_data_end__[];
    UBaseType_t     ulRegionSizeInBytes = 0;
    const portSHORT numRegions = 4;
    portSHORT       idx = 0; 
    // specify attributes to MPU regions #0 - #3
    xMPU_REGION_REGS  xRegion[numRegions];
    // unprivileged code section (all FLASH memory)
    ulRegionSizeInBytes = (UBaseType_t)__code_segment_end__ - (UBaseType_t)__code_segment_start__;
    xRegion[0].RBAR = ((UBaseType_t) __code_segment_start__ & MPU_RBAR_ADDR_Msk)
                      | MPU_RBAR_VALID_Msk | portUNPRIVILEGED_FLASH_REGION;
    xRegion[0].RASR = portMPU_REGION_READ_ONLY |  MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                     ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                     | MPU_RASR_ENABLE_Msk ;
    // privileged code section (first few KBytes of the FLASH memory, determined by application)
    ulRegionSizeInBytes = (UBaseType_t)__privileged_code_end__  - (UBaseType_t)__code_segment_start__;
    xRegion[1].RBAR = ((UBaseType_t) __code_segment_start__ & MPU_RBAR_ADDR_Msk)
                      | MPU_RBAR_VALID_Msk | portPRIVILEGED_FLASH_REGION;
    xRegion[1].RASR = portMPU_REGION_PRIVILEGED_READ_ONLY |  MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                    ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                    | MPU_RASR_ENABLE_Msk;
    // privileged data section (first few KBytes of the SRAM memory, determined by application)
    ulRegionSizeInBytes = (UBaseType_t) __privileged_data_end__ - (UBaseType_t)__privileged_data_start__;
    xRegion[2].RBAR = ((UBaseType_t) __privileged_data_start__ & MPU_RBAR_ADDR_Msk)
                      | MPU_RBAR_VALID_Msk | portPRIVILEGED_SRAM_REGION ;
    xRegion[2].RASR = portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                     ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                     | MPU_RASR_ENABLE_Msk;
    // peripheral region should have different attributes from normal memory
    ulRegionSizeInBytes = (UBaseType_t) PERIPH_SIZE;
    xRegion[3].RBAR = ((UBaseType_t) PERIPH_BASE & MPU_RBAR_ADDR_Msk) | MPU_RBAR_VALID_Msk
                      | portGENERAL_PERIPHERALS_REGION ;
    xRegion[3].RASR = (portMPU_REGION_READ_WRITE | portMPU_REGION_EXEC_NEVER) | MPU_RASR_ENABLE_Msk |
                      ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(PERIPH_SIZE) << MPU_RASR_SIZE_Pos) );
    // write the xRegion structure above to MPU regisrers
    for (idx=0; idx<numRegions; idx++) {
        vPortSetMPUregion( &xRegion[idx] );
    }
    // turn on MemManage fault handler, which captures any memory access
    // violation after MPU is enabled, without enabling MemManage fault
    // hanlder, HardFault handler will be used for such access violations.
    SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk ;
    // turn on MPU
    MPU->CTRL  |= MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk;
} // end of prvSetupMPU






BaseType_t xPortStartScheduler( void )
{
    if (configMAX_SYSCALL_INTERRUPT_PRIORITY == 0) 
    { // when CPU gets here, it failed to launch task scheduler
        return pdFALSE;
    }
    // the implementation code below checks number of bits which are 
    // actually used as priority group value in NVIC_IPRx.
    #if( configASSERT_DEFINED == 1 )
    {
        uint8_t ucOriginalPriority = 0;
        uint8_t bitMaskNVICIPRx = 0;
        const unsigned portSHORT maxBitsPriGroupAIRCR = 0x7;
        ucOriginalPriority = NVIC->IP[0];
        // get bitmask of NVIC_IPRx, the bitmask should be 1's in most
        // significant part, and 0's in least significant part.
        NVIC->IP[0] = 0xff;
        bitMaskNVICIPRx = NVIC->IP[0];
        ucMaxInNvicIPRx = configMAX_SYSCALL_INTERRUPT_PRIORITY & bitMaskNVICIPRx ;
        // get maximum possible value of SCB_AIRCR.PRIGROUP if 
        // no need to consider sub-priority bits
        ucMaxPriGroupInAIRCR = maxBitsPriGroupAIRCR;
        while ((bitMaskNVICIPRx & 0x80) == 0x80) {
            ucMaxPriGroupInAIRCR--;
            bitMaskNVICIPRx <<= 0x1;
        }
        #if defined( __NVIC_PRIO_BITS)
        if ((maxBitsPriGroupAIRCR - ucMaxPriGroupInAIRCR) != __NVIC_PRIO_BITS) 
        { // when CPU gets here, it failed to launch task scheduler
            return pdFALSE;
        }
        #endif
        ucMaxPriGroupInAIRCR <<= SCB_AIRCR_PRIGROUP_Pos;
        NVIC->IP[0] = ucOriginalPriority; // recover its original value
    }
    #endif // end of configASSERT_DEFINED
    // enable MPU
    prvSetupMPU();
    // set priority of SysTick, PendSV, SVCall exception
    NVIC_SetPriority( SysTick_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY );
    NVIC_SetPriority( PendSV_IRQn , configLIBRARY_LOWEST_INTERRUPT_PRIORITY );
    NVIC_SetPriority( SVCall_IRQn , configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
    // enable SysTick timer
    SysTick->CTRL = 0L;
    SysTick->VAL  = 0L;
    SysTick->LOAD = (configCPU_CLOCK_HZ / configTICK_RATE_HZ)- 1UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk 
                    | SysTick_CTRL_ENABLE_Msk ;
    uxCriticalNesting = 0;
    // enable FPU, configure to fully access coprocessor 10 & 11
    SCB->CPACR = SCB_CPACR_CP11_Msk | SCB_CPACR_CP10_Msk ;
    // enable to automatically save floating-point context
    FPU->FPCCR |= FPU_FPCCR_LSPEN_Msk;
    // only for cleaning FPCA & SPSEL
    __set_CONTROL(0x0);
    // enable interrupt, fault eception
    // and generate SVC exception to start scheduler
    __asm volatile(
        "cpsie i               \n" 
        "cpsie f               \n" 
        "dsb                   \n" 
        "isb                   \n"
        "svc      %0           \n"
        "nop                   \n"
        ::"i"(portSVC_ID_START_SCHEDULER)
        :"memory"
    );
    // if task scheduler is successfully launched, we shouldn't get here
    return pdTRUE;
} // end of xPortStartScheduler




PRIVILEGED_FUNCTION void prvRaisePrivilege( void )
{
    UBaseType_t control_reg;
    control_reg = __get_CONTROL();
    control_reg = control_reg & ~CONTROL_nPRIV_Msk;
    __set_CONTROL( control_reg );
} //// end of prvRaisePrivilege


PRIVILEGED_FUNCTION void vPortSVCHandler( void )
{
    uint32_t  *pulSelectedSP         = NULL;
    uint8_t    ucSVCnumber           = 0;
    // check which sp we are using
    __asm volatile (
        "tst     lr, #0x4  \n"
        "ite     eq        \n"
        "mrseq   %0, msp   \n"
        "mrsne   %0, psp   \n"
        :"=r"(pulSelectedSP) ::
    );
    // get pc address from exception stack frame, then 
    // go back to find last instrution's opcode (svn <NUMBER>)
    ucSVCnumber = ((uint8_t *) pulSelectedSP[6] )[-2];
    switch(ucSVCnumber) {
        case portSVC_ID_START_SCHEDULER:
            __asm volatile ("b prvRestoreContextOfFirstTask  \n");
            break;
        case portSVC_ID_YIELD:
            __asm volatile ("b vPortYield  \n");
            break;
        case portSVC_ID_RAISE_PRIVILEGE:
            __asm volatile ("b prvRaisePrivilege  \n");
            break;
        default : 
            break;
    }; // end of switch statement.
} // end of vPortSVCHandler



void vPortEndScheduler( void )
{
    // Not implemented in ports where there is nothing to return to.
    // it simply goes to infinite loop.
    for(;;);
}




BaseType_t  xPortRaisePrivilege( void )
{
    BaseType_t  privilegeRaised = pdFALSE;
    UBaseType_t  control_reg = __get_CONTROL();
    // if CPU is currently in unprivileged state, then we invoke SVCall
    // with argument portSVC_ID_RAISE_PRIVILEGE
    if ((control_reg & CONTROL_nPRIV_Msk) == CONTROL_nPRIV_Msk)
    {
        portRAISE_PRIVILEGE();
        privilegeRaised = pdTRUE;
    }
    return privilegeRaised;
} // end of xPortRaisePrivilege




void vPortResetPrivilege( BaseType_t privWasRaised )
{
    UBaseType_t  control_reg;
    if( privWasRaised == pdTRUE)
    {
        __asm volatile ("dsb \n");
        control_reg  = __get_CONTROL();
        control_reg  = control_reg | CONTROL_nPRIV_Msk;
        __set_CONTROL( control_reg );
    }
} // end of vPortResetPrivilege




BaseType_t  xIsPrivileged( void )
{
    UBaseType_t control_reg;
    control_reg = __get_CONTROL();
    control_reg = control_reg & CONTROL_nPRIV_Msk;
    return control_reg;
} // end of xIsPrivileged



void vPortEnterCritical( void )
{
    BaseType_t xRunningPrivileged;
    xRunningPrivileged = xPortRaisePrivilege();
    if (uxCriticalNesting == 0) {
        portDISABLE_INTERRUPTS();
    }
    uxCriticalNesting++;
    vPortResetPrivilege( xRunningPrivileged );
} // end of vPortEnterCritical



void vPortExitCritical( void )
{
    BaseType_t xRunningPrivileged = xPortRaisePrivilege();
    uxCriticalNesting--;
    if (uxCriticalNesting == 0) {
        portENABLE_INTERRUPTS();
    }
    vPortResetPrivilege( xRunningPrivileged );
} // end of vPortExitCritical



void vPortSysTickHandler( void )
{
    UBaseType_t ulOriginBasepri;
    ulOriginBasepri = portSET_INTERRUPT_MASK_FROM_ISR();
    if( xTaskIncrementTick() != pdFALSE)
    { // yielding function will take effect on exit of SysTick exception
        vPortYield();
    }
    portCLEAR_INTERRUPT_MASK_FROM_ISR( ulOriginBasepri );
} //// end of vPortSysTickHandler




///void vPortStoreTaskMPUSettings( xMPU_SETTINGS *xMPUSettings, const struct xMEMORY_REGION * const xRegions, StackType_t *pxBottomOfStack, uint32_t ulStackDepth )

void vPortStoreTaskMPUSettings( xMPU_SETTINGS *xMPUSettings, 
                       const MemoryRegion_t * const xRegions,
                       StackType_t *pxBottomOfStack,
                       uint32_t ulStackDepth )
{
    portSHORT       idx;
    UBaseType_t     ulRegionSizeInBytes ;
    // if xRegions is NOT given, we simply allow this task to access
    // all the SRAM space which is NOT assigned to privileged data
    // section.
    if( xRegions == NULL )
    {
        extern  UBaseType_t  __SRAM_segment_start__[] ;
        extern  UBaseType_t  __SRAM_segment_end__  [] ;
        extern  UBaseType_t  __privileged_data_start__[];
        extern  UBaseType_t  __privileged_data_end__[];
        ulRegionSizeInBytes = (UBaseType_t)__SRAM_segment_end__    - (UBaseType_t)__SRAM_segment_start__ ;
        xMPUSettings->xRegion[0].RBAR = ((UBaseType_t)__SRAM_segment_start__ & MPU_RBAR_ADDR_Msk)
                                        | MPU_RBAR_VALID_Msk | portSTACK_REGION;
        xMPUSettings->xRegion[0].RASR = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk
                                     | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                     |  MPU_RASR_ENABLE_Msk ;
        ulRegionSizeInBytes = (UBaseType_t)__privileged_data_end__ - (UBaseType_t)__privileged_data_start__ ;
        xMPUSettings->xRegion[1].RBAR = ((UBaseType_t)__privileged_data_start__ & MPU_RBAR_ADDR_Msk)
                                         | MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION;
        xMPUSettings->xRegion[1].RASR = portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk
                                        | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                        |  MPU_RASR_ENABLE_Msk ;
        for(idx=2; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++)
        {
            xMPUSettings->xRegion[idx].RBAR = MPU_RBAR_VALID_Msk | (portSTACK_REGION + idx);
            xMPUSettings->xRegion[idx].RASR = 0x0;
        }
    }
    else
    {
        // the task is allowed to access the allocated stack memory
        // and any other optional allocated space
        if(ulStackDepth > 0) {
            ulRegionSizeInBytes = (UBaseType_t)sizeof(StackType_t) * ulStackDepth;
            xMPUSettings->xRegion[0].RBAR = ((UBaseType_t)pxBottomOfStack & MPU_RBAR_ADDR_Msk)
                                            | MPU_RBAR_VALID_Msk | portSTACK_REGION ;
            xMPUSettings->xRegion[0].RASR = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk
                                      | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                      |  MPU_RASR_ENABLE_Msk  ;
        }
        for(idx=1; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++)
        {
            ulRegionSizeInBytes = xRegions[idx - 1].ulLengthInBytes;
            if(ulRegionSizeInBytes > 0x0)
            {
                xMPUSettings->xRegion[idx].RBAR = ((UBaseType_t)xRegions[idx-1].pvBaseAddress & MPU_RBAR_ADDR_Msk)
                                               | MPU_RBAR_VALID_Msk | (portSTACK_REGION + idx) ;
                xMPUSettings->xRegion[idx].RASR = xRegions[idx - 1].ulParameters | MPU_RASR_ENABLE_Msk
                                      | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos));
            }
            else 
            {
                xMPUSettings->xRegion[idx].RBAR = MPU_RBAR_VALID_Msk | (portSTACK_REGION + idx);
                xMPUSettings->xRegion[idx].RASR = 0x0;
            }
        } // end of for-loop
    } // end of if xRegions is NULL
} //// end of vPortStoreTaskMPUSettings




#if( configASSERT_DEFINED == 1 )
unsigned portCHAR ucGetMaxInNvicIPRx( void )
{
    return ucMaxInNvicIPRx;
} 

unsigned portSHORT ucGetMaxPriGroupInAIRCR( void )
{
    return  ucMaxPriGroupInAIRCR;
}



void vPortValidateInterruptPriority( void )
{
    BaseType_t   currInterruptNum      = 0;
    uint8_t      currInterruptPriority = 0;
    UBaseType_t  currAIRCRprigroup     = 0;
    const  BaseType_t  firstUserISRnum = 0x10;
    // get ISR_NUMBER from IPSR,
    currInterruptNum  = __get_IPSR() ;
    currInterruptNum -= firstUserISRnum;
    // In this port we only check STM32 specific Interrupts, which means
    // the ISR_NUMBER above SysTick
    if( currInterruptNum > SysTick_IRQn ) 
    {
        // following assertion will fail if current interrupt priority is
        // above configMAX_SYSCALL_INTERRUPT_PRIORITY (numerically lesser
        // than configMAX_SYSCALL_INTERRUPT_PRIORITY) calling a FreeRTOS
        // API function.
        // Interrupt-safe FreeRTOS API functions must *only* be called
        // from interrupts that have been assigned a priority at or below
        // configMAX_SYSCALL_INTERRUPT_PRIORITY.

        // Interrupts that use the FreeRTOS API must NOT be left at their
        // default priority of	zero as that is the highest possible priority,
        // which is guaranteed to be above configMAX_SYSCALL_INTERRUPT_PRIORITY,
        // and therefore also guaranteed to be invalid.
        // 
        // FreeRTOS maintains separate thread and ISR API functions to ensure
        // interrupt entry is as fast and simple as possible.
        // 
        // The following links provide detailed information:
        // http://www.freertos.org/RTOS-Cortex-M3-M4.html
        // http://www.freertos.org/FAQHelp.html
        currInterruptPriority = NVIC->IP[ currInterruptNum ];
        configASSERT( currInterruptPriority >= ucMaxInNvicIPRx );
    }
    // Priority grouping:  The interrupt controller (NVIC) allows the bits
    // that define each interrupt's priority to be split between bits that
    // define the interrupt's pre-emption priority bits and bits that define
    // the interrupt's sub-priority.  For simplicity all bits must be defined
    // to be pre-emption priority bits.  The following assertion will fail if
    // this is not the case (if some bits represent a sub-priority).

    // If the application only uses CMSIS libraries for interrupt
    // configuration then the correct setting can be achieved on all Cortex-M
    // devices by calling NVIC_SetPriorityGrouping( 0 ); before starting the
    // scheduler.  Note however that some vendor specific peripheral libraries
    // assume a non-zero priority group setting, in which cases using a value
    // of zero will result in unpredicable behaviour. 
    currAIRCRprigroup = SCB->AIRCR  &  SCB_AIRCR_PRIGROUP_Msk ;
    configASSERT( currAIRCRprigroup <= ucMaxPriGroupInAIRCR );
} //// end of vPortValidateInterruptPriority

#endif // end of configASSERT_DEFINED









//---------------------------------------------------------------------
// Extra functions that are NOT included in FreeRTOS official release 
//---------------------------------------------------------------------
// this is private routine called only by HardFault exception handler
// in FreeRTOS Cortex-M4 port
//
// we need to check whether this HardFault is caused under the conditions
// below :
// * the HardFault is caused when a task calls SVCall 
// * In FreeRTOS, when an unprivileged task enters critical section, it
//   has to modify BASEPRI by executing SVCall, temporarily switching to 
//   privileged software in the SVCall handler routine.  
// * Then set BASEPRI in the privileged software to the number as the same
//   as SVCall exception priority.  By doing so the task in critical 
//   section won't be preempted by arriving exceptions, whose priority is
//   numerically greater than / equal to SVCall.
// * when the task leaves critical section,  the first thing it has to do
//   is to execute SVCall again, temporarily switch to privileged software
//   again to reset BASEPRI, let exceptions of all priority start working.
// * However at this moment the SVCall is temporarily disabled due to
//   BASEPRI content, therefore Cortex-M CPU generates Hardfault instead
//   of SVCall exception.
// * if the unprivileged task is in nested critical section & attempts 
//   to re-enter the inner critical section(s), it will still execute SVCall
//   , which has been blocked by BASEPRI, then end up with HardFault.
// Based on the descriptions above, we need to check whether the task 
// attempts to temporarily switch to privileged software :
//     * on exit of a critical section for resetting the privileged 
//       register BASEPRI
//     * on the entry of nested critical section
void vPortCheckHardFaultCause( UBaseType_t  *pulSelectedSP )
{
    UBaseType_t  faultDiagnosis;
    uint8_t      thumbEncodeInstr = 0;
    // the encoded value of SVC in thumb instruction
    const uint8_t  thumbEncodeSVC   = 0xdf; 
    // filter out any cause that is NOT related to forced HardFault
    faultDiagnosis  = SCB->HFSR & SCB_HFSR_FORCED_Msk;
    if(faultDiagnosis == pdFALSE) return;
    // filter out any fault caused by bus (BFSR), memory management
    // (MMFSR), or improper usage (UFSR)
    faultDiagnosis  = SCB->CFSR ;
    if(faultDiagnosis != 0x0) return;
    // further check if BASEPRI matches configMAX_SYSCALL_INTERRUPT_PRIORITY
    faultDiagnosis  = 0x0;
    faultDiagnosis  = __get_BASEPRI();
    if(faultDiagnosis != configMAX_SYSCALL_INTERRUPT_PRIORITY) return;
    // get pc address from exception stack frame, then 
    // go back to check whether last instrution is SVCall.
    thumbEncodeInstr = ((uint8_t *) pulSelectedSP[6] )[-1];
    // check if a task requests specific SVCall service at critical section.
    if (thumbEncodeInstr==thumbEncodeSVC) 
    {
        __asm volatile (
            // pop a stack item to lr, it should be exception return address
            // then calls SVC handler routine, jump back to task program
            // after that.
            "pop  {lr}             \n"
            "b    SVC_Handler      \n"
        );
    }
} //// end of vPortCheckHardFaultCause




void vPortHardFaultHandler( void )
{
    // check which sp we are using
    __asm volatile (
        "tst     lr, #0x4  \n"
        "ite     eq        \n"
        "mrseq   r0, msp   \n"
        "mrsne   r0, psp   \n"
        "push    {lr}  \n"
        // for few reasons, a HardFault can be fixed since we didn't
        // enable other types of exception (e.g. memManage fault, bus fault 
        // , SVCall blocked by BASEPRI), following sub-routine recovers
        // from such HardFault then jump back to task program code
        "bl      vPortCheckHardFaultCause  \n"
        "pop     {lr}  \n"
    );
    // any other cause to HardFault leads to program crash.
    for(;;); 
} // end of vPortHardFaultHandler





// -------------------- tickless idle --------------------
// For brief introduction, please read description in portmacro.h
// or FreeRTOS reference manual.
#if( configUSE_TICKLESS_IDLE == 1 )
__weak void  vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
{
    // number of timer counts per tick
    const UBaseType_t ulTimerCountsPerTick = configCPU_CLOCK_HZ / configTICK_RATE_HZ;
    // in this port, SysTick contains 24-bit LOAD register, it is
    // necessary to calculate maximum number of tick counts that can be
    // saved to the LOAD register.
    const UBaseType_t ulMaxSuppressTicks = 0xffffffUL / ulTimerCountsPerTick;
    // Compensate for the CPU cycles that pass while the SysTick is 
    // stopped (low power function only).
    const UBaseType_t ulStopTimerCompensation = 45;
    // calculate all timer counts based on given input ticks
    UBaseType_t  ulNewReloadCounts    = 0;
    UBaseType_t  ulSysTickCtrlReg     = 0;
    UBaseType_t  ulCountFlagAsserted  = 0;
    UBaseType_t  ulExpectedTimerCountsInSleep  = 0;
    TickType_t   ulActualTicksInSleep = 0;
    // status to indicate if CPU can go to sleep mode
    eSleepModeStatus  eSleepStatus;
    // restrict the expected idle time (ticks)
    if( xExpectedIdleTime > ulMaxSuppressTicks ) {
        xExpectedIdleTime = (TickType_t) ulMaxSuppressTicks;
    }
    // stop SysTick timer
    ulSysTickCtrlReg  = SysTick->CTRL;
    SysTick->CTRL     = ulSysTickCtrlReg &  ~SysTick_CTRL_ENABLE_Msk;
    // disable interrupt by setting PRIMASK
    __asm volatile(
        "cpsid i  \n" 
        "dsb      \n" 
        "isb      \n"
        :::"memory"
    );
    // check if it is still ok to go to sleep mode
    eSleepStatus = eTaskConfirmSleepModeStatus();
    if( eSleepStatus != eAbortSleep ) 
    {
        // calculate all timer counts based on given input ticks
        ulExpectedTimerCountsInSleep = xExpectedIdleTime * ulTimerCountsPerTick;
        ulNewReloadCounts = ulExpectedTimerCountsInSleep - ulTimerCountsPerTick +  SysTick->VAL; 
        if( ulNewReloadCounts > ulStopTimerCompensation ) {
            ulNewReloadCounts -= ulStopTimerCompensation;
        }
        // then write the estimated timer counts to SysTick LOAD register.
        SysTick->LOAD     = ulNewReloadCounts;
        SysTick->VAL      = 0;
        SysTick->CTRL     = ulSysTickCtrlReg | SysTick_CTRL_ENABLE_Msk;
        // CPU enters sleep mode,  when CPU is woken by SysTick event,
        // it won't jump into ISR because  we set PRIMASK to mask off 
        // interrupt handling routine.
        __asm volatile(
            "dsb      \n" 
            "wfi      \n" 
            "isb      \n"
            :::"memory"
        );
        // stop the timer again,
        ulSysTickCtrlReg   = SysTick->CTRL;
        SysTick->CTRL      = ulSysTickCtrlReg & ~SysTick_CTRL_ENABLE_Msk;
        // check whether the wake-up event comes exactly from SysTick ,
        // or from other external interrupt input.
        ulCountFlagAsserted = ulSysTickCtrlReg & SysTick_CTRL_COUNTFLAG_Msk;
        if(ulCountFlagAsserted != 0x0) {
            ulActualTicksInSleep = xExpectedIdleTime - 0x1;
        }
        else {
            // TODO: find effective way to verify this part
            ulActualTicksInSleep = (ulExpectedTimerCountsInSleep - SysTick->VAL) / ulTimerCountsPerTick;
        }
        // now restore  timer counts of one tick.
        vTaskStepTick( ulActualTicksInSleep );
        SysTick->VAL   = 0;
        SysTick->LOAD  = ulTimerCountsPerTick - 0x1;
        // the tick counts variable xTickCount in task.c
    } //// end of eSleepStatus != eAbortSleep
    // enable  interrupt by clearing PRIMASK
    __asm volatile("cpsie i ");
    // restart SysTick timer
    SysTick->CTRL = ulSysTickCtrlReg | SysTick_CTRL_ENABLE_Msk;
}
#endif //// end of configUSE_TICKLESS_IDLE


#if( configGENERATE_RUN_TIME_STATS == 1 )
// functions required while configGENERATE_RUN_TIME_STATS is set to 1.
__weak void vCfgTimerForRuntimeStats(void)
{
} //// end of vCfgTimerForRuntimeStats


__weak unsigned long ulGetRuntimeCounterVal(void)
{
    return 0;
} //// end of ulGetRuntimeCounterVal

#endif //// end of configGENERATE_RUN_TIME_STATS


