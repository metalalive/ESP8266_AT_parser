// for C unit test framework Unity
#include "unity_fixture.h"
// for FreeRTOS
#include "FreeRTOS.h"

#define TEST_STACK_SIZE  0x20

// the fake current task TCB below is used only in unit test
// in this test case, we only mock stack pointer of the current TCB since
// we only check registers after restoring content from a task's stack memory 
typedef struct {
    volatile StackType_t    *pxTopOfStack;
    #if ( portUSING_MPU_WRAPPERS == 1 )
        xMPU_SETTINGS  xMPUSettings;
    #endif
}
TCB_test_t;

// the data structure below is only for collecting current content of all GPR
// registers, xPSR, and CONTROL register, and then compare those with expected
// value.
typedef struct {
    UBaseType_t GPRs[13]; // r0 to r12
    UBaseType_t msp;
    UBaseType_t psp;
    UBaseType_t lr;
    UBaseType_t control;
    UBaseType_t xPSR;
    xMPU_SETTINGS  xMPUSettings;
} RegsSet_test_t;

// the task's TCB & stack memory for this unit test
extern volatile void *pxCurrentTCB;
static volatile StackType_t  *mockTaskStack = NULL;
static PRIVILEGED_DATA RegsSet_test_t   *expected_value = NULL;
static                 RegsSet_test_t   *actual_value   = NULL;

extern void vCopyMPUregionSetupToCheckList( xMPU_SETTINGS *xMPUSettings );




static void vMockTask1Func(void *params)
{
    // we passed actual_value as parameter to this function, then copy most
    // of GPR registers  directly to actual_value
    __asm volatile(
        "stmia  %0,  {r0, r1}   \n"
        "add    r2,  %0, #0x10  \n"
        "stmia  r2!, {r4-r12}   \n"
        "dsb                    \n"
        "mrs    r4,   msp  \n"
        "mrs    r5,   psp  \n"
        "mov    r6,   lr   \n"
        "mrs    r7,   control  \n"
        "mrs    r8,   xPSR     \n"
        "stmia  r2!, {r4-r8}   \n"
        "push   {lr}           \n"
        "mov    r0,  r2        \n" // r2 = &(actual_value->xMPUSettings)
        "bl     vCopyMPUregionSetupToCheckList \n"
        "pop    {lr}           \n"
        "dsb                   \n"
        ::"r"((UBaseType_t)(RegsSet_test_t *)params):
    );
    // recvoer pc, msp, control in SVC exception
    // back to unit test sequence 
    __asm volatile("svc  #0x0f"); 
    for(;;);
} // end of vMockTask1Func()




void TEST_HELPER_prvRestoreContextOfFirstTask_SVCentry( void )
{
    if(expected_value != NULL) {
        // the target function we want to test, in this test it should
        // jump to vMockTask1Func()
        // at exception return
        prvRestoreContextOfFirstTask();
    }
} // end of TEST_HELPER_prvRestoreContextOfFirstTask_SVCentry() 



// To declare a new test group in Unity, firstly you use the macro below
TEST_GROUP( prvRestoreContextOfFirstTask );



// initial setup before running the test body
TEST_SETUP( prvRestoreContextOfFirstTask )
{
    // ------------- prepare process stack space of the mock task -------------
    extern UBaseType_t  __SRAM_segment_start__[];
    extern UBaseType_t  __SRAM_segment_end__[];
    extern UBaseType_t __privileged_data_start__[];
    extern UBaseType_t __privileged_data_end__[];
    extern UBaseType_t __code_segment_start__ [];
    extern UBaseType_t __code_segment_end__   [];

    UBaseType_t  idx = 0;
    UBaseType_t  ulRegionSizeInBytes = 0;
    StackType_t *currSP = 0;
    TCB_test_t  *pxTCB = NULL;

    expected_value = (RegsSet_test_t *) unity_malloc( sizeof(RegsSet_test_t) ) ;
    actual_value   = (RegsSet_test_t *) unity_malloc( sizeof(RegsSet_test_t) ) ;
    mockTaskStack = (StackType_t *) unity_malloc( sizeof(StackType_t) * TEST_STACK_SIZE );
    pxCurrentTCB = unity_malloc( sizeof(TCB_test_t) );

    if (mockTaskStack != NULL) 
    {
        for(idx=1; idx<=TEST_STACK_SIZE; idx++ ) {
            *(mockTaskStack + idx - 1) = (idx << 24) | (idx << 16) | (idx << 8) | (idx << 0);
        }
        currSP   = mockTaskStack + TEST_STACK_SIZE - 1;
        // reserve 2 data words for architecture in the begining of exception stack frame.
        currSP  -= 2;
        // specify appropriate status to xPSR, here we only set Thumb mode
        *currSP  = xPSR_T_Msk;
        currSP  -= 1;
        // specify our helper function to pc (exception stack frames), the helper
        // function will generate exception event again & CPU go to hanlder mode again.
        *currSP  = ((StackType_t)vMockTask1Func)  &  portCPU_ADDRESS_MASK;
        currSP  -= 1;
        // no need to modify r1-r3, r12, lr (exception stack frames) at this test.
        currSP  -= 5;
        // to r0 (exception stack frames)
        *currSP  = (StackType_t)actual_value;
        currSP  -= 1;
        // to lr, In prvRestoreContextOfFirstTask(), it will return to the special address.
        *currSP  = portEXPTN_RETURN_TO_TASK;
        currSP  -= 1;
        // each of r4-r11 is assigned with an unique value above, no need to modify.
        currSP  -= 8;
        // it will be written to CONTROL register, we only set SPSEL bit, clear FPCA
        *currSP  = CONTROL_SPSEL_Msk;
    }
    // ------------- prepare TCB for the mock task -------------
    if (pxCurrentTCB != NULL) 
    {
        pxTCB = (TCB_test_t *)pxCurrentTCB;
        // pass pointer of mock stack space to TCB
        pxTCB->pxTopOfStack = currSP;
        // pass MPU region settings to TCB
        ulRegionSizeInBytes = (UBaseType_t)__SRAM_segment_end__ - (UBaseType_t)__SRAM_segment_start__;
        pxTCB->xMPUSettings.xRegion[ 0 ].RBAR = ((UBaseType_t) __SRAM_segment_start__  & MPU_RBAR_ADDR_Msk ) // Base address. 
                                                | MPU_RBAR_VALID_Msk | portSTACK_REGION;
        pxTCB->xMPUSettings.xRegion[ 0 ].RASR = portMPU_REGION_READ_WRITE |  MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                                               ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                                               | MPU_RASR_ENABLE_Msk ;
        ulRegionSizeInBytes = (UBaseType_t) __privileged_data_end__ - (UBaseType_t) __privileged_data_start__;
        pxTCB->xMPUSettings.xRegion[ 1 ].RBAR = ((UBaseType_t) __privileged_data_start__ & MPU_RBAR_ADDR_Msk)
                                                | MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION;
        pxTCB->xMPUSettings.xRegion[ 1 ].RASR = portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                                               ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                                                | MPU_RASR_ENABLE_Msk;
        ulRegionSizeInBytes = (UBaseType_t) __code_segment_end__ - (UBaseType_t) __code_segment_start__;
        pxTCB->xMPUSettings.xRegion[ 2 ].RBAR = ((UBaseType_t) __code_segment_start__ & MPU_RBAR_ADDR_Msk)
                                                | MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1);
        pxTCB->xMPUSettings.xRegion[ 2 ].RASR = portMPU_REGION_READ_ONLY |  MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                                               ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                                                | MPU_RASR_ENABLE_Msk;
        pxTCB->xMPUSettings.xRegion[ 3 ].RBAR =  MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 2);
        pxTCB->xMPUSettings.xRegion[ 3 ].RASR = 0;
    }

    // ------------- enable SVC exception -------------
    NVIC_SetPriority( SVCall_IRQn , configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
    __asm volatile( "cpsie i" );
    __asm volatile( "cpsie f" );
    __asm volatile( "dsb" ::: "memory" );
    __asm volatile( "isb" );

    // --------------- initialize all expected values --------------------
    if (expected_value != NULL) 
    {
        currSP   = mockTaskStack + TEST_STACK_SIZE - 1;
        currSP  -= 2;
        expected_value->xPSR     = *currSP--; // note: it's decrement after assigning the content of the pointer
        currSP  -= 1; // skip pc value
        expected_value->lr       = *currSP-- ;
        expected_value->GPRs[12] = *currSP-- ;
        expected_value->GPRs[3]  = *currSP-- ;
        expected_value->GPRs[2]  = *currSP-- ;
        expected_value->GPRs[1]  = *currSP-- ;
        expected_value->GPRs[0]  = *currSP-- ; 
        currSP  -= 1; // skip lr, we won't check exception return address
        expected_value->GPRs[11] = *currSP--;
        expected_value->GPRs[10] = *currSP--;
        expected_value->GPRs[9]  = *currSP--;
        expected_value->GPRs[8]  = *currSP--;
        expected_value->GPRs[7]  = *currSP--;
        expected_value->GPRs[6]  = *currSP--;
        expected_value->GPRs[5]  = *currSP--;
        expected_value->GPRs[4]  = *currSP--;
        expected_value->control  = *currSP;  // CONTROL_SPSEL_Msk, should select PSP at the moment
        expected_value->msp      = (UBaseType_t) *(StackType_t *)(SCB->VTOR);
        // psp should point the place immediately after xPSR for exception stack frame
        expected_value->psp      = (UBaseType_t) (mockTaskStack + TEST_STACK_SIZE - 1 - 1);
        if (pxTCB != NULL) 
        {
            for(idx=0; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++ ) {
                // MPU_RBAR.VALID is read always zero on target board, we have to mask off the bit.
                expected_value->xMPUSettings.xRegion[idx].RBAR  =  pxTCB->xMPUSettings.xRegion[idx].RBAR & ~(MPU_RBAR_VALID_Msk) ;
                expected_value->xMPUSettings.xRegion[idx].RASR  =  pxTCB->xMPUSettings.xRegion[idx].RASR ;
            }
        }
    }
} // end of TEST_SETUP





TEST_TEAR_DOWN( prvRestoreContextOfFirstTask )
{
    UBaseType_t  idx = 0;
    // ------ disable all configurable MPU regions ------
    for(idx=0; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++ ) {
        MPU->RNR  = (portSTACK_REGION + idx);
        MPU->RBAR = (portSTACK_REGION + idx) | MPU_RBAR_VALID_Msk;
        MPU->RASR = 0;
        __asm volatile( "dsb" ::: "memory" );
    }
    // ---- free memory space allocated to test TCB -----
    unity_free( pxCurrentTCB );
    unity_free( (void *)mockTaskStack );
    pxCurrentTCB  = NULL;
    mockTaskStack = NULL;    
    // ------------- disable SVC exception -------------
    __asm volatile( "dsb" ::: "memory" );
    __asm volatile( "isb" );
    __asm volatile( "cpsid i" );
    __asm volatile( "cpsid f" );
    NVIC_SetPriority( SVCall_IRQn , 0 );
    // --------------- free memory from check list --------------------
    unity_free( (void *)expected_value ) ;
    unity_free( (void *)actual_value   ) ;
    expected_value = NULL ;
    actual_value   = NULL ;
} // end of TEST_TEAR_DOWN



TEST(prvRestoreContextOfFirstTask, regs_chk)
{
    portSHORT  idx = 0;
    __asm volatile (
        "svc  %0         \n"
        ::"i"(portSVC_ID_START_SCHEDULER)
        :
    );
    // supposed to jump back here from vMockTask1Func()
    // then we start checking the register content.
    TEST_ASSERT_EQUAL_UINT32( expected_value->GPRs[0], actual_value->GPRs[0] );
    TEST_ASSERT_EQUAL_UINT32( expected_value->GPRs[1], actual_value->GPRs[1] );
    for(idx=4 ; idx<=12; idx++) {
        TEST_ASSERT_EQUAL_UINT32( expected_value->GPRs[idx], actual_value->GPRs[idx] );
    }
    TEST_ASSERT_EQUAL_UINT32( expected_value->msp, actual_value->msp );
    TEST_ASSERT_EQUAL_UINT32( expected_value->psp, actual_value->psp );
    TEST_ASSERT_EQUAL_UINT32( expected_value->lr , actual_value->lr  );
    TEST_ASSERT_EQUAL_UINT32( expected_value->control , actual_value->control  );
    TEST_ASSERT_EQUAL_UINT32((expected_value->xPSR & 0xf0000000UL), (actual_value->xPSR & 0xf0000000UL));

    for(idx=0 ; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++) {
        TEST_ASSERT_EQUAL_UINT32( expected_value->xMPUSettings.xRegion[idx].RBAR , actual_value->xMPUSettings.xRegion[idx].RBAR );
        TEST_ASSERT_EQUAL_UINT32( expected_value->xMPUSettings.xRegion[idx].RASR , actual_value->xMPUSettings.xRegion[idx].RASR );
    }
} //// end of test body



