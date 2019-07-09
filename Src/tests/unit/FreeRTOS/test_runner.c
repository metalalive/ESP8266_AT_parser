// for C unit test framework Unity
#include "unity_fixture.h"
// for FreeRTOS
#include "FreeRTOS.h"
#include "task.h"

// -------------------------------------------------------------------
// this file collects all the test cases of FreeRTOS port functions



// from test_vPortYield.c
extern void  TEST_HELPER_setPendsvVisitFlag(); 
// from test_prvRestoreContextOfFirstTask.c
extern void TEST_HELPER_prvRestoreContextOfFirstTask_SVCentry(); 
// from test_vPortPendSVHandler.c
extern void TEST_HELPER_vPortPendSVHandler_PendSVentry();
// from test_xPortStartScheduler.c
extern void TEST_HELPER_xPortStartScheduler_memMgtFaultEntry( void );
// from test_xPortStartScheduler.c
extern void TEST_HELPER_xPortStartScheduler_SVC0entry( void );
// from test_vPortSVCHandler.c
extern void TEST_HELPER_vPortSVCHandler_SVCentry( void );
extern void TEST_HELPER_vPortSVCHandler_PendSVentry( void );
// from test_xPortRaisePrivilege.c
extern void TEST_HELPER_xPortRaisePrivilege_SVCentry( void );
// from test_vPortEnterCritical.c
extern void TEST_HELPER_vPortEnterCritical_SVCentry( void );
extern void TEST_HELPER_vPortEnterCritical_hardFaultEntry( void );
extern void TEST_HELPER_vPortEnterCritical_sysTickEntry( void );
//from test_vPortSysTickHandler.c
extern void TEST_HELPER_vPortSysTickHandler_SysTickEntry( void );
extern void TEST_HELPER_vPortSysTickHandler_PendSVentry( void );
// from port.c
extern void vPortGetMPUregion ( portSHORT regionID, xMPU_SETTINGS *xMPUSettings );


// the fake current task TCB below is used only in unit test
volatile void *pxCurrentTCB;
// for some functions ,the variables below can be used to recover back to
//  normal execution
StackType_t   msp_backup;
StackType_t   excpt_return_backup ;
// In Cortex-M4, 
// bit 9 of stacked xPSR describes alignment information on each exception entry,
// if the bit 9 is zero, CPU reserves 1 data word for stack alignment ;
// otherwise bit 9 is one, CPU reserves 2 data words for alignment.
BaseType_t stack_alignment_words;


// entry function for PendSV exception event
void PendSV_Handler(void)
{
    uint32_t  *pulSelectedSP         = NULL;
    __asm volatile (
        "tst     lr, #0x4  \n"
        "ite     eq        \n"
        "mrseq   %0, msp   \n"
        "mrsne   %0, psp   \n"
        :"=r"(pulSelectedSP) ::
    );
    stack_alignment_words = (pulSelectedSP[7] & 0x200) >> 9;
    __asm volatile (
        "push {lr} \n"
        "bl   TEST_HELPER_setPendsvVisitFlag               \n"
        "bl   TEST_HELPER_vPortSVCHandler_PendSVentry      \n"
        "bl   TEST_HELPER_vPortSysTickHandler_PendSVentry  \n"
        "pop  {lr} \n"
        // the function below must be placed in the final line of this function, since
        // the function mocks context switch between 2 tasks and never return back here.
        "b    TEST_HELPER_vPortPendSVHandler_PendSVentry \n"
        :::
    );
} // end of PendSV_Handler()


// entry function for SVC exception event
// ------------------------------------------------------------------
// when calling a sub-routine in C, 
// GCC compiler usually generates push/pop stacking instruction, 
// pushes a few GPR registers, and the LR to stack.
// the stacking behaviour of compiler might differ among differnet
// versions of toolchain . It's better to dump assembly code, 
// see how the functions are compiled.
// Here we call a routines in the inline assembly code.
// ------------------------------------------------------------------
void SVC_Handler(void)
{
    uint32_t  *pulSelectedSP         = NULL;
    uint8_t    ucSVCnumber           = 0;

    __asm volatile (
        // check which sp are we using
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
            __asm volatile (
                "mrs    %0, msp     \n"
                "mov    %1, lr      \n"
                :"=r"(msp_backup), "=r"(excpt_return_backup) ::
            );
            __asm volatile (
                "push  {lr}  \n"
                "bl    TEST_HELPER_xPortStartScheduler_SVC0entry         \n"
                "pop   {lr}  \n"
                "b     TEST_HELPER_prvRestoreContextOfFirstTask_SVCentry \n"
            );
            break;

        case portSVC_ID_YIELD:
            break;

        case portSVC_ID_RAISE_PRIVILEGE:
            break;

        case 0x0f:
            // recover msp & lr, used for TEST_HELPER_prvRestoreContextOfFirstTask_SVCentry() 
            __asm volatile (
                "mov lr ,      %0    \n" // restore pc  value ... will gen HardFault
                "msr msp,      %1    \n" // restore msp value
                "mov r3 ,      #0    \n"
                "msr control,  r3    \n" // switch back to msp
                ::"ir"(excpt_return_backup), "r"(msp_backup)
            );
            break;

        default : 
            break;
    };
    __asm volatile (
        "push  {lr}      \n"
        "mov   r0,   %0                                   \n"
        "bl    TEST_HELPER_vPortSVCHandler_SVCentry       \n"
        "bl    TEST_HELPER_xPortRaisePrivilege_SVCentry   \n"
        "bl    TEST_HELPER_vPortEnterCritical_SVCentry    \n"
        "pop   {lr}      \n"
        ::"r"(ucSVCnumber):
    );
} //// end of SVC_Handler


void SysTick_Handler(void)
{
    __asm volatile (
        "push  {lr}  \n"
        "bl    TEST_HELPER_xPortStartScheduler_SysTickHandleEntry    \n"
        "bl    TEST_HELPER_vPortEnterCritical_sysTickEntry           \n"
        "bl    TEST_HELPER_vPortSysTickHandler_SysTickEntry          \n"
        "bl    TEST_HELPER_vPortSuppressTicksAndSleep_sysTickEntry   \n"
        "pop   {lr}  \n"
    );
} //// end of SysTick_Handler



void MemManage_Handler(void)
{
    __asm volatile (
        "push  {lr}  \n"
        "bl    TEST_HELPER_xPortStartScheduler_memMgtFaultEntry  \n"
        "pop   {lr}  \n"
    );
    while(1);
} //// end of MemManage_Handler



void HardFault_Handler(void)
{
    // check which sp we are using
    __asm volatile (
        "tst     lr, #0x4  \n"
        "ite     eq        \n"
        "mrseq   r0, msp   \n"
        "mrsne   r0, psp   \n"
        "push    {lr}  \n"
        "bl      TEST_HELPER_vPortEnterCritical_hardFaultEntry  \n"
        "pop     {lr}  \n"
    );
    for(;;);
} //// end of HardFault_Handler



void vCopyMPUregionSetupToCheckList( xMPU_SETTINGS *xMPUSettings )
{
    // copy MPU_MBAR , MPU_RASR for the region #4 - #7
    portSHORT  idx = 0;
    for(idx=0 ; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++) {
        vPortGetMPUregion((4 + idx), &(xMPUSettings->xRegion[idx]) );
    }
} //// end of vCopyMPUregionSetupToCheckList





TEST_GROUP_RUNNER( FreeRTOS_v10_2_port )
{
    RUN_TEST_CASE( pxPortInitialiseStack, initedStack_privileged );
    RUN_TEST_CASE( pxPortInitialiseStack, initedStack_unprivileged );
    RUN_TEST_CASE( vPortPendSVHandler, cs_with_fp );
    RUN_TEST_CASE( prvRestoreContextOfFirstTask, regs_chk );
    RUN_TEST_CASE( vPortPendSVHandler, cs_without_fp );
    RUN_TEST_CASE( vPortSysTickHandler, func_chk )
    RUN_TEST_CASE( vPortSVCHandler, svc_chk );
    RUN_TEST_CASE( xPortRaisePrivilege , regs_chk );
    RUN_TEST_CASE( vPortSuppressTicksAndSleep, sleep_failure );
    RUN_TEST_CASE( vPortStoreTaskMPUSettings , with_given_region );
    RUN_TEST_CASE( vPortStoreTaskMPUSettings , without_given_region );
    RUN_TEST_CASE( vPortSuppressTicksAndSleep, sleep_k_ticks );
    RUN_TEST_CASE( vPortEnterCritical , single_critical_section );
    RUN_TEST_CASE( vPortEnterCritical , nested_critical_section );
    RUN_TEST_CASE( xPortStartScheduler , regs_chk );
    RUN_TEST_CASE( vPortYield, gen_pendsv_excpt );
}

