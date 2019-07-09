// for C unit test framework Unity
#include "unity_fixture.h"
// in this test, we will put a function & few variables in privileged area
// by putting the macros PRIVILEGED_FUNCTION and PRIVILEGED_DATA ahead of
// the privileged function & data. 
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
// for FreeRTOS
#include "FreeRTOS.h"

typedef struct {
    unsigned portCHAR SysTick_IP;    // systick interrupt priority
    unsigned portCHAR PendSV_IP;     // PendSV  interrupt priority
    unsigned portCHAR SVCall_IP;     // SVC exception priority
    UBaseType_t       SCB_SHCSR;     // System Handler Control & Status
    UBaseType_t       MPU_CTRL;      // MPU Control register
    UBaseType_t       SCB_CPACR;     // Coprocessor Access Control Register
    UBaseType_t       FPU_FPCCR;     // floating-point context control register
    SysTick_Type      SysTickRegs;   // all registers within SysTick 
    xMPU_SETTINGS     xMPUsetup;  // record settings of first 4 MPU regions (#0 - #3)
} RegsSet_chk_t;

// collecting actual data from registers & check at the end of this test
static RegsSet_chk_t  *expected_value = NULL;
static RegsSet_chk_t  *actual_value = NULL;
static unsigned portCHAR   unpriv_branch_fail_cnt;
PRIVILEGED_DATA static volatile UBaseType_t uMockPrivVar ;
static volatile UBaseType_t uMockTickCount ;
static unsigned portCHAR ucVisitSVC0Flag;
// from port.c
extern unsigned portCHAR   ucGetMaxInNvicIPRx( void );
extern unsigned portSHORT  ucGetMaxPriGroupInAIRCR( void );



void TEST_HELPER_xPortStartScheduler_SVC0entry( void )
{
    if( expected_value != NULL ) {
        ucVisitSVC0Flag = 1;
    }
} //// end of TEST_HELPER_xPortStartScheduler_SVC0entry




void TEST_HELPER_xPortStartScheduler_memMgtFaultEntry( void )
{
    // memory management fault status register
    uint8_t      MMFSR = 0;
    // instruction accress violation flag in MMFSR
    const  uint8_t IACCVIOL = 0x1;
    const  uint8_t DACCVIOL = 0x2;
    if( expected_value != NULL ) {
        // switch back to privileged state for Thread mode.
        __asm volatile (
            "mrs  r2 , control  \n"
            "bic  r2 , r2, #0x1 \n"
            "msr  control, r2   \n"
            "isb  \n"
        );
        // get MMFSR from CFSR
        MMFSR = SCB->CFSR & SCB_CFSR_MEMFAULTSR_Msk;
        unpriv_branch_fail_cnt++;
        __asm volatile("pop  {lr}   \n");
        // when CPU gets here, that means we capture invalid unprivilege access
        if((MMFSR & DACCVIOL)==0x2) {
        }
        // when CPU gets here, that means we capture invalid branch
        // when the unprivileged software attempts to call privileged
        // function (but results in HardFault exception here)
        else if((MMFSR & IACCVIOL)==0x1) {
            // copy lr to pc in the same exception stack frame,
            // in order to skip the unprivileged branch after returning
            // from this HardFault exception handler in this test.
            __asm volatile(
                "ldr  r2,   [sp, #0x14] \n"
                "str  r2,   [sp, #0x18] \n"
                "dsb                    \n"
            );
        } // end of IACCVIOL check
    } // end of expected_value check
} //// end of TEST_HELPER_xPortStartScheduler_memMgtFaultEntry



void TEST_HELPER_xPortStartScheduler_SysTickHandleEntry( void )
{
    if( expected_value != NULL ) {
        uMockTickCount++;
    }
} //// end of TEST_HELPER_xPortStartScheduler_SysTickHandleEntry



PRIVILEGED_FUNCTION static void prvUpdateMPUcheckList (xMPU_SETTINGS *target)
{
    extern  UBaseType_t  __privileged_code_end__[];
    extern  UBaseType_t  __code_segment_start__ [];
    extern  UBaseType_t  __code_segment_end__[];
    extern  UBaseType_t  __privileged_data_start__[];
    extern  UBaseType_t  __privileged_data_end__[];
    UBaseType_t  ulRegionSizeInBytes = 0;

    // Note: MPU_RBAR.VALID is always read as zero, no need to check this bit.
    // unprivileged code section (all FLASH memory)
    ulRegionSizeInBytes = (UBaseType_t)__code_segment_end__ - (UBaseType_t)__code_segment_start__;
    target->xRegion[0].RBAR = ((UBaseType_t) __code_segment_start__ & MPU_RBAR_ADDR_Msk) | portUNPRIVILEGED_FLASH_REGION;
    target->xRegion[0].RASR = portMPU_REGION_READ_ONLY |  MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                              ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                              | MPU_RASR_ENABLE_Msk ;

    // privileged code section (first few KBytes of the FLASH memory, determined by application)
    ulRegionSizeInBytes = (UBaseType_t)__privileged_code_end__  - (UBaseType_t)__code_segment_start__;
    target->xRegion[1].RBAR = ((UBaseType_t) __code_segment_start__ & MPU_RBAR_ADDR_Msk) | portPRIVILEGED_FLASH_REGION ;
    target->xRegion[1].RASR = portMPU_REGION_PRIVILEGED_READ_ONLY |  MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                              ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                              | MPU_RASR_ENABLE_Msk ;

    // privileged data section (first few KBytes of the SRAM memory, determined by application)
    ulRegionSizeInBytes = (UBaseType_t) __privileged_data_end__ - (UBaseType_t)__privileged_data_start__;
    target->xRegion[2].RBAR = ((UBaseType_t) __privileged_data_start__ & MPU_RBAR_ADDR_Msk) | portPRIVILEGED_SRAM_REGION;
    target->xRegion[2].RASR = portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                             ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                             | MPU_RASR_ENABLE_Msk ;

    // peripheral region should have different attributes from normal memory
    target->xRegion[3].RBAR = ((UBaseType_t) PERIPH_BASE & MPU_RBAR_ADDR_Msk) | portGENERAL_PERIPHERALS_REGION ;
    target->xRegion[3].RASR = (portMPU_REGION_READ_WRITE | portMPU_REGION_EXEC_NEVER) | MPU_RASR_ENABLE_Msk |
                              ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(PERIPH_SIZE) << MPU_RASR_SIZE_Pos) );
} //// end of prvUpdateMPUcheckList






// for testing purpose, we copy attributes from region #2 to region #6
// , and add region #5 to allow both privileged/unprivileged accesses,
// * 
static void vModifyMPUregionsForTest( void )
{
    extern  UBaseType_t  __SRAM_segment_start__[];
    extern  UBaseType_t  __SRAM_segment_end__ [];
    xMPU_REGION_REGS  xRegion;
    UBaseType_t  ulRegionSizeInBytes = 0;
    // move attributes from region #2 to region #6
    vPortGetMPUregion( portPRIVILEGED_SRAM_REGION, &xRegion );
    xRegion.RBAR &= ~(MPU_RBAR_REGION_Msk);
    xRegion.RBAR |= MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1);
    vPortSetMPUregion( &xRegion );
    // setup one more MPU region only for unprivileged accesses in this test to SRAM
    ulRegionSizeInBytes = (UBaseType_t) __SRAM_segment_end__ - (UBaseType_t) __SRAM_segment_start__ ;
    xRegion.RBAR = ((UBaseType_t) __SRAM_segment_start__  & MPU_RBAR_ADDR_Msk)
                      | MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION  ;
    xRegion.RASR = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                     ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                     | MPU_RASR_ENABLE_Msk;
    vPortSetMPUregion( &xRegion );
} //// end of vModifyMPUregionsForTest



// To declare a new test group in Unity, firstly you use the macro below
TEST_GROUP( xPortStartScheduler );



TEST_SETUP( xPortStartScheduler )
{
    unpriv_branch_fail_cnt = 0;
    uMockPrivVar   = 0xdead;
    uMockTickCount = 0;
    ucVisitSVC0Flag = 0;
    expected_value = (RegsSet_chk_t *) unity_malloc( sizeof(RegsSet_chk_t) );
    actual_value   = (RegsSet_chk_t *) unity_malloc( sizeof(RegsSet_chk_t) );
} // end of TEST_SETUP





TEST_TEAR_DOWN( xPortStartScheduler )
{
    // clear the settings of extra regions for this test 
    xMPU_REGION_REGS  xRegion;
    xRegion.RBAR = MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION  ;
    xRegion.RASR = 0;
    vPortSetMPUregion( &xRegion );
    xRegion.RBAR = MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION+1)  ;
    vPortSetMPUregion( &xRegion );
    // disable interrupt for SVC, SysTick, and Memory Management Fault
    NVIC_SetPriority( SysTick_IRQn, 0 );
    NVIC_SetPriority( PendSV_IRQn , 0 );
    NVIC_SetPriority( SVCall_IRQn , 0 );
    __asm volatile(
        "cpsid i  \n" 
        "cpsid f  \n" 
        "dsb      \n" 
        "isb      \n"
        :::"memory"
    );
    // turn off the features we set for the test
    SCB->SHCSR  &= ~SCB_SHCSR_MEMFAULTENA_Msk;
    MPU->CTRL   &= ~(MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk);
    SCB->CPACR  &= ~(SCB_CPACR_CP11_Msk | SCB_CPACR_CP10_Msk);
    FPU->FPCCR  &= ~FPU_FPCCR_LSPEN_Msk;
    SysTick->CTRL  = 0;
    SysTick->VAL   = 0;
    SysTick->LOAD  = 0;
    // free memory allocated to check lists 
    unity_free( (void *)expected_value ); 
    unity_free( (void *)actual_value   ); 
    expected_value = NULL; 
    actual_value   = NULL; 
} // end of TEST_TEAR_DOWN




TEST( xPortStartScheduler , regs_chk )
{
    portSHORT    idx = 0;
    UBaseType_t  uTickCountUpperBound = 0;

    xPortStartScheduler();
    vModifyMPUregionsForTest();
    TEST_ASSERT_NOT_EQUAL( configMAX_SYSCALL_INTERRUPT_PRIORITY , 0 );

    #if( configASSERT_DEFINED == 1 )
    {
        unsigned portCHAR   ucActualMaxNvicIPRx;
        unsigned portCHAR   ucExpectMaxNvicIPRx;
        unsigned portSHORT  ucActualMaxPriGroupInAIRCR;
        unsigned portSHORT  ucExpectMaxPriGroupInAIRCR;
        // Note: 
        // NVIC_IPRx in Cortex-M4 takes 8 bits, only upper 4 bits are used,
        // CMSIS defines __NVIC_PRIO_BITS representing number of bits usesd for
        // interrupt priority, lower 4 bits of NVIC_IPRx is NOT used.
        const uint8_t bitMaskNVICIPRx = 0xf << (8 - __NVIC_PRIO_BITS);
        ucExpectMaxNvicIPRx        = configMAX_SYSCALL_INTERRUPT_PRIORITY & bitMaskNVICIPRx ;
        ucActualMaxNvicIPRx        = ucGetMaxInNvicIPRx();
        // interrupt in FreeRTOS doexn't consider sub-priority, SCB_AIRCR.PRIGROUP
        // should be numerically less than 4, in order to treat all the 8 bits of
        // NVIC_IPRx as a priority group number.
        ucExpectMaxPriGroupInAIRCR = (4 - 1) << SCB_AIRCR_PRIGROUP_Pos;
        ucActualMaxPriGroupInAIRCR = ucGetMaxPriGroupInAIRCR();
        TEST_ASSERT_EQUAL_UINT8( ucExpectMaxNvicIPRx , ucActualMaxNvicIPRx );
        TEST_ASSERT_EQUAL_UINT16( ucExpectMaxPriGroupInAIRCR, ucActualMaxPriGroupInAIRCR );
    }
    #endif
    // check priority of SysTick, PendSV exception
    expected_value->SysTick_IP = (unsigned portCHAR) configLIBRARY_LOWEST_INTERRUPT_PRIORITY;
    actual_value->SysTick_IP   = (unsigned portCHAR) NVIC_GetPriority( SysTick_IRQn );
    TEST_ASSERT_EQUAL_UINT8( expected_value->SysTick_IP, actual_value->SysTick_IP );
    expected_value->PendSV_IP  = (unsigned portCHAR) configLIBRARY_LOWEST_INTERRUPT_PRIORITY;
    actual_value->PendSV_IP    = (unsigned portCHAR) NVIC_GetPriority( PendSV_IRQn );
    TEST_ASSERT_EQUAL_UINT8( expected_value->PendSV_IP, actual_value->PendSV_IP);
    expected_value->SVCall_IP = (unsigned portCHAR) configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY ;
    actual_value->SVCall_IP   = (unsigned portCHAR) NVIC_GetPriority( SVCall_IRQn );
    TEST_ASSERT_EQUAL_UINT8( expected_value->SVCall_IP, actual_value->SVCall_IP );
     

    // check other system registers ....
    expected_value->SCB_SHCSR = SCB_SHCSR_MEMFAULTENA_Msk; 
    actual_value->SCB_SHCSR   = SCB->SHCSR & SCB_SHCSR_MEMFAULTENA_Msk; 
    expected_value->MPU_CTRL  = MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk; 
    actual_value->MPU_CTRL    = MPU->CTRL ; 
    expected_value->SCB_CPACR = SCB_CPACR_CP11_Msk | SCB_CPACR_CP10_Msk; //  configure to fully access CP10 & CP11
    actual_value->SCB_CPACR   = SCB->CPACR;
    expected_value->FPU_FPCCR = FPU_FPCCR_LSPEN_Msk;
    actual_value->FPU_FPCCR   = FPU->FPCCR & FPU_FPCCR_LSPEN_Msk;
    expected_value->SysTickRegs.CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |  SysTick_CTRL_ENABLE_Msk ;
    actual_value->SysTickRegs.CTRL   = SysTick->CTRL;
    expected_value->SysTickRegs.LOAD = (configCPU_CLOCK_HZ / configTICK_RATE_HZ)- 1UL;
    actual_value->SysTickRegs.LOAD   = SysTick->LOAD;
    expected_value->SysTickRegs.VAL  = 0UL;
    TEST_ASSERT_EQUAL_UINT32( expected_value->SCB_SHCSR, actual_value->SCB_SHCSR );
    TEST_ASSERT_EQUAL_UINT32( expected_value->MPU_CTRL , actual_value->MPU_CTRL  );
    TEST_ASSERT_EQUAL_UINT32( expected_value->SCB_CPACR, actual_value->SCB_CPACR );
    TEST_ASSERT_EQUAL_UINT32( expected_value->FPU_FPCCR, actual_value->FPU_FPCCR );
    TEST_ASSERT_EQUAL_UINT32( expected_value->SysTickRegs.CTRL , actual_value->SysTickRegs.CTRL );
    TEST_ASSERT_EQUAL_UINT32( expected_value->SysTickRegs.LOAD , actual_value->SysTickRegs.LOAD );
    uTickCountUpperBound = uMockTickCount + 10;
    while (uMockTickCount < uTickCountUpperBound)
    {
        actual_value->SysTickRegs.VAL = SysTick->VAL;
        TEST_ASSERT_UINT32_WITHIN( ((expected_value->SysTickRegs.LOAD + 1) >> 1),
                                   ((expected_value->SysTickRegs.LOAD + 1) >> 1),
                                   actual_value->SysTickRegs.VAL );
    }
 
    // --------- check MPU regions setup  -----------
    // part 1: 
    // CPU in Thread mode switches to unprivileged state, then calls the privileged function,
    // this invalid call should result in HardFault exception.
    __asm volatile (
        "mrs  r8 , control  \n"
        "orr  r8 , r8, #0x1 \n"
        "msr  control, r8   \n"
        "dsb  \n"
        "isb  \n"
    );
    prvUpdateMPUcheckList( &expected_value->xMPUsetup );
    // when CPU jumps back here, it becomes privileged thread mode
    // (switch the state in our HardFault handler routine)
    TEST_ASSERT_EQUAL_UINT16( unpriv_branch_fail_cnt , 1 );

    // part 2:
    // CPU switches back to privileged state, then invoke privileged function again
    prvUpdateMPUcheckList( &expected_value->xMPUsetup );
     // the counter should still remain the same
    TEST_ASSERT_NOT_EQUAL( unpriv_branch_fail_cnt , 2 );

    // part 3: 
    // CPU in Thread mode switches to unprivileged state again, then accesses a privileged variable,
    // this invalid access should result in HardFault exception.
    __asm volatile (
        "mrs  r8 , control  \n"
        "orr  r8 , r8, #0x1 \n"
        "msr  control, r8   \n"
        "dsb  \n"
        "isb  \n"
    );
    // following line of code will be executed twice, first time it runs at unprivileged thread mode,
    // , then jump to HardFault exception handler routine, which switches back to privileged mode,
    // then jump back here, the same line of code, and execute again at privileged thread mode.
    uMockPrivVar   = 0xacce55ed;
    TEST_ASSERT_EQUAL_UINT32( uMockPrivVar, 0xacce55ed);
    TEST_ASSERT_EQUAL_UINT16( unpriv_branch_fail_cnt , 2 );

    // check the MPU regions settings 
    for(idx=0 ; idx<4 ; idx++)
    {
        vPortGetMPUregion(idx, &(actual_value->xMPUsetup.xRegion[idx]) );
        TEST_ASSERT_EQUAL_UINT32( expected_value->xMPUsetup.xRegion[idx].RBAR , actual_value->xMPUsetup.xRegion[idx].RBAR );
        TEST_ASSERT_EQUAL_UINT32( expected_value->xMPUsetup.xRegion[idx].RASR , actual_value->xMPUsetup.xRegion[idx].RASR );
    }

    TEST_ASSERT_EQUAL_UINT8( 1, ucVisitSVC0Flag );
} //// end of test body



