// for C unit test framework Unity
#include "unity_fixture.h"
// for FreeRTOS
#include "FreeRTOS.h"

#define TEST_STACK_SIZE  0x40
#define TEST_FLOAT_POINT_ENABLE   1
#define TEST_FLOAT_POINT_DISABLE  0

// the fake current task TCB below is used only in unit test
// in this test case, we only mock stack pointer of the current TCB since
// we only check registers after restoring content from a task's stack memory 
typedef struct {
    StackType_t    *pxTopOfStack;
    xMPU_SETTINGS   xMPUSettings;
    BaseType_t      stack_alignment_words;
    StackType_t    *pxStack;
}
TCB_test_t;

// the data structure below is only for collecting current content of all GPR
// registers, xPSR, and CONTROL register, and then compare those with expected
// value.
typedef struct {
    UBaseType_t GPRs[13]; // r0 to r12
    portFLOAT   FPRs[32]; // for 32-bit floating point registers s0-s31 
    UBaseType_t msp;
    UBaseType_t psp;
    UBaseType_t lr;
    UBaseType_t control;
    UBaseType_t xPSR;
    #if ( portUSING_MPU_WRAPPERS == 1 )
        xMPU_SETTINGS  xMPUSettings;
    #endif
} RegsSet_chk_t;

// the structure is internally used to pass argument to the task 2
typedef struct {
    TCB_test_t *pxTCB;
    unsigned portSHORT    uFPen;
} mockTask2ParamStruct;


// bit 9 of stacked xPSR indicates 1-word or 2-words stack alignment
extern BaseType_t stack_alignment_words;
// the 2 tasks' TCB & stack memory for this unit test
extern volatile void *pxCurrentTCB;
// collecting actual data from registers & check at the end of this test
static RegsSet_chk_t  *expected_value = NULL;
static RegsSet_chk_t  *actual_value = NULL;

static UBaseType_t     mockReadyTaskList[2] ;
static UBaseType_t     numContextSwitches;




__attribute__((always_inline)) __INLINE void vMockTaskYield( void )
{
    __asm volatile(
        // To avoid our GCC compiler from automatically choosing GPR r0, r4-r12 as operands of
        // subsequent instructions, we set PendSV bit in the inline assembly .
         "ldr  r3,=%0     \n"
         "mov  r2, #1     \n"
         "lsl  r2, r2, %1 \n"
         "str  r2, [r3, #4] \n" // SCB->ICSR = 1 << SCB_ICSR_PENDSVSET_Pos
         "dsb  \n" 
         "isb  \n"
        ::"i"(SCB_BASE), "i"(SCB_ICSR_PENDSVSET_Pos)
        : "memory" 
    );
} //// end of vMockTaskYield



__attribute__((always_inline)) __INLINE void vSwitchFromPSP2MSP( void )
{
    __asm volatile (
        "mrs  r8 , control  \n"
        "bic  r8 , r8, #0x2 \n"
        "msr  control, r8   \n"
    );
} //// end of vSwitchFromPSP2MSP



__attribute__((always_inline)) __INLINE void vSwitchFromMSP2PSP( void )
{
    __asm volatile (
        "mrs  r8 , control  \n"
        "orr  r8 , r8, #0x2 \n"
        "msr  control, r8   \n"
    );
} //// end of vSwitchFromMSP2PSP



// --------- disable floating-point storage in exception stack frame ---------
// it seems that CONTROL.FPCA will be automatically set when a routine contains
// a floating-point instruction, but FPCA bit won't be cleaned automatically,
// therefore we manually do this at the end of each test case.
// [reference]
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0298a/BCGHEEFD.html
__attribute__((always_inline)) __INLINE void vDisableSaveFPcontext( void )
{
    __asm volatile (
        "mrs  r8 , control  \n"
        "bic  r8 , r8, #0x4 \n"
        "msr  control, r8   \n"
    );
} //// end of vDisableSaveFPcontext



__attribute__((always_inline)) __STATIC_INLINE void vUpdateCheckListFromRegs(RegsSet_chk_t *targetChkList, portSHORT FPenabled )
{
    // we will not check r3, r2, r1 value since they are automatically used 
    // by GCC compiler for instruction execution
    __asm volatile (
        "stmia   %0,  {r0-r1}     \n"
        "add     r2,  %0, #0x10   \n" // targetChkList->GPRs[4]
        "stmia   r2,  {r4-r12}    \n"
        "mrs     r4,  msp        \n" // use r4-r8 as buffer
        "mrs     r5,  psp        \n"
        "mov     r6,  lr         \n"
        "mrs     r7,  control    \n"
        "mrs     r8,  xPSR       \n"
        "add     r2,  %0, #180   \n" // targetChkList->msp
        "stmia   r2!, {r4-r8}    \n" // now r2 becomes targetChkList->xMPUSettings
        "dsb                     \n"
        "push   {lr, r0, r1, r2, r3}  \n" // backup original lr, r0-r3 for subsequent restore
        "mov    r0,  r2        \n" // r2 = &(actual_value->xMPUSettings)
        "bl     vCopyMPUregionSetupToCheckList \n"
        "pop    {lr, r0, r1, r2, r3}           \n"

        "dsb                     \n"
        "add     r2, %0, #0x10  \n" // targetChkList->GPRs[4], recvoer r4-r8
        "ldmia   r2, {r4-r8}    \n"
        ::"r"(targetChkList):
    );
    if(FPenabled == TEST_FLOAT_POINT_ENABLE){
        __asm volatile (
            "add     r2, %0, #0x34  \n" // targetChkList->FPRs
            "vstmia  r2, {s0-s31} \n"
            ::"r"(targetChkList):
        );
    }
    __asm volatile( "dsb" ::: "memory" );
} //// end of vUpdateCheckListFromRegs




static void vUpdateCheckListFromTCB(RegsSet_chk_t *targetChkList, TCB_test_t *pxTCB, portSHORT FPenabled )
{
    portSHORT idx = 0;
    StackType_t *currSP = pxTCB->pxTopOfStack;
    targetChkList->psp     = (UBaseType_t) currSP ; // psp
    targetChkList->control = *currSP; // CONTROL register
    currSP++;
    for(idx=1; idx<9; idx++) {
        targetChkList->GPRs[3+idx] = *currSP; // r4 - r11
        currSP++;
    }
    // *(currSP + 9) stores exception return address for lr, we will
    // NOT check this value at this unit test.
    currSP++;
    if (FPenabled == TEST_FLOAT_POINT_ENABLE) {
        for(idx=0xa; idx<0x1a; idx++) {
            targetChkList->FPRs[6+idx] = *(portFLOAT *)currSP ; // s16 - s31
            currSP++;
        }
    }
    for(idx=0x1a; idx<0x1e; idx++) {
        targetChkList->GPRs[idx-0x1a] = *currSP; // r0 - r3
        currSP++;
    }
    targetChkList->GPRs[12] = *currSP; // r12
    currSP++;
    targetChkList->lr = *currSP; // lr
    currSP++;
    // skip pc value
    currSP++;
    targetChkList->xPSR = *currSP ; // xPSR
    currSP++;
    if (FPenabled == TEST_FLOAT_POINT_ENABLE) {
        for(idx=0x22; idx<0x32; idx++) {
            targetChkList->FPRs[idx-0x22] = *(portFLOAT *)currSP ; // s0 - s15
            currSP++;
        }
    }
    // copy MPU regions setting
    for(idx=0x0; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++) {
        targetChkList->xMPUSettings.xRegion[idx].RBAR = pxTCB->xMPUSettings.xRegion[idx].RBAR;
        targetChkList->xMPUSettings.xRegion[idx].RASR = pxTCB->xMPUSettings.xRegion[idx].RASR;
    }
} //// end of vUpdateCheckListFromTCB






static void vMockTask2Func(void *params)
{
    mockTask2ParamStruct *pxParam = (mockTask2ParamStruct *)params;
    unsigned portSHORT    uFPen   = pxParam->uFPen;
    TCB_test_t           *pxTCB   = pxParam->pxTCB;
    unity_free( (void *)pxParam );
    // after the first context switch, the context of previously running
    // task should be saved at somewhere, so copy the saved context
    // & check the value later.
    vUpdateCheckListFromTCB( actual_value, pxTCB, uFPen );
    // do some random floating point-caculation, to force CONTROL.FPCA set .
    __asm volatile(
        "vadd.F32   s1 , s1 , s18  \n"
        "vadd.F32   s29, s7 , s25  \n"
        "vadd.F32   s2 , s0 , s17  \n"
        "vadd.F32   s3 , s1 , s16  \n"
        "vadd.F32   s15 , s3 , s29  \n"
        "vadd.F32   s25, s3 , s9   \n"
        "vadd.F32   s14 , s5 , s27  \n"
        "vadd.F32   s9 , s7 , s25  \n"
        "vadd.F32   s11, s9 , s23  \n"
        "vadd.F32   s27, s5 , s7   \n"
        "vadd.F32   s0 , s10 , s17  \n"
    );
    // perform the second context switch.
    vMockTaskYield();
    // collect register content to check later
    vUpdateCheckListFromRegs( actual_value , TEST_FLOAT_POINT_ENABLE );
    // perform the 4th. context switch.
    vMockTaskYield();
    // do some random floating point-caculation, to force CONTROL.FPCA set .
    __asm volatile(
        "vadd.F32   s2 , s1 , s18  \n"
        "vadd.F32   s29, s7 , s25  \n"
        "vadd.F32   s25, s3 , s9   \n"
        "vadd.F32   s6 , s1 , s16  \n"
        "vadd.F32   s1 , s5 , s27  \n"
        "vadd.F32   s9 , s7 , s25  \n"
        "vadd.F32   s11, s9 , s23  \n"
        "vadd.F32   s10 , s12 , s17  \n"
    );
    for(;;);
} // end of vMockTask2Func()






static TCB_test_t* pxInitMockTask1( void )
{
    UBaseType_t  ulRegionSizeInBytes = 0;
    StackType_t *taskStack = NULL;
    TCB_test_t  *pxTCB     = NULL;

    pxTCB      = (TCB_test_t *)  unity_malloc( sizeof(TCB_test_t) );
    taskStack  = (StackType_t *) unity_malloc( sizeof(StackType_t) * TEST_STACK_SIZE );
    if(taskStack != NULL) {
        // Initially in this unit test , the test code will save registers to this stack,
        // no need to initialize at here
    }
    if(pxTCB != NULL)
    {
        pxTCB->pxStack      = taskStack; 
        pxTCB->pxTopOfStack = taskStack + TEST_STACK_SIZE - 1 ;
        pxTCB->xMPUSettings.xRegion[ 0 ].RBAR = MPU_RBAR_VALID_Msk | portSTACK_REGION ;
        pxTCB->xMPUSettings.xRegion[ 0 ].RASR = 0x0;
        pxTCB->xMPUSettings.xRegion[ 1 ].RBAR = MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION ;
        pxTCB->xMPUSettings.xRegion[ 1 ].RASR = 0x0;
        ulRegionSizeInBytes = (UBaseType_t)sizeof(StackType_t) * TEST_STACK_SIZE;
        pxTCB->xMPUSettings.xRegion[ 2 ].RBAR = ((UBaseType_t) taskStack & MPU_RBAR_ADDR_Msk) 
                                                 | MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1);
        pxTCB->xMPUSettings.xRegion[ 2 ].RASR = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                         ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                         | MPU_RASR_ENABLE_Msk ;
        ulRegionSizeInBytes = (UBaseType_t)sizeof(RegsSet_chk_t);
        pxTCB->xMPUSettings.xRegion[ 3 ].RBAR = ((UBaseType_t) expected_value & MPU_RBAR_ADDR_Msk)
                                                 | MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 2);
        pxTCB->xMPUSettings.xRegion[ 3 ].RASR = portMPU_REGION_PRIVILEGED_READ_WRITE |  MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                          ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                          | MPU_RASR_ENABLE_Msk;
        pxTCB->stack_alignment_words = 0;
    }
    return pxTCB;
} //// end of vInitMockTask1




static TCB_test_t* pxInitMockTask3( void )
{
    UBaseType_t  ulRegionSizeInBytes = 0;
    StackType_t *taskStack = NULL;
    TCB_test_t  *pxTCB     = NULL;

    taskStack = (StackType_t *) unity_malloc( sizeof(StackType_t) * TEST_STACK_SIZE );
    pxTCB     = (TCB_test_t *)  unity_malloc( sizeof(TCB_test_t) );

    if(taskStack != NULL) {
        // Initially in this unit test , the test code will save registers to this stack,
        // no need to initialize at here
    }
    if(pxTCB != NULL)
    {
        pxTCB->pxStack      = taskStack;
        pxTCB->pxTopOfStack = taskStack + TEST_STACK_SIZE - 1;
        ulRegionSizeInBytes = (UBaseType_t)sizeof(StackType_t) * TEST_STACK_SIZE;
        pxTCB->xMPUSettings.xRegion[ 0 ].RBAR = ((UBaseType_t)taskStack & MPU_RBAR_ADDR_Msk) 
                                                 | MPU_RBAR_VALID_Msk | portSTACK_REGION ;
        pxTCB->xMPUSettings.xRegion[ 0 ].RASR = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                         ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                         | MPU_RASR_ENABLE_Msk ;
        pxTCB->xMPUSettings.xRegion[ 1 ].RBAR = MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION ;
        pxTCB->xMPUSettings.xRegion[ 1 ].RASR = 0x0;
        pxTCB->xMPUSettings.xRegion[ 2 ].RBAR = MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1) ;
        pxTCB->xMPUSettings.xRegion[ 2 ].RASR = 0x0;
        ulRegionSizeInBytes = (UBaseType_t)sizeof(RegsSet_chk_t);
        pxTCB->xMPUSettings.xRegion[ 3 ].RBAR = ((UBaseType_t) actual_value & MPU_RBAR_ADDR_Msk)
                                                 | MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 2);
        pxTCB->xMPUSettings.xRegion[ 3 ].RASR = portMPU_REGION_PRIVILEGED_READ_WRITE |  MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                          ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                          | MPU_RASR_ENABLE_Msk;
        pxTCB->stack_alignment_words = 0;
    }
    return pxTCB;
} // end of vInitMockTask3




static TCB_test_t* pxInitMockTask2( TCB_test_t *pxTCBtoTaskFunc, unsigned portSHORT uFPen )
{
    extern UBaseType_t  __SRAM_segment_start__[];
    extern UBaseType_t  __SRAM_segment_end__[];
    extern UBaseType_t  __code_segment_start__ [];
    extern UBaseType_t  __code_segment_end__   [];
    portSHORT idx = 0;
    UBaseType_t  ulRegionSizeInBytes = 0;
    StackType_t *taskStack = NULL;
    StackType_t *currSP    = NULL;
    TCB_test_t  *pxTCB     = NULL;
    mockTask2ParamStruct *pxParam = NULL;

    pxTCB      = (TCB_test_t  *) unity_malloc( sizeof(TCB_test_t) );
    taskStack  = (StackType_t *) unity_malloc( sizeof(StackType_t) * TEST_STACK_SIZE );
   
    if(taskStack != NULL)
    {
        currSP = taskStack + TEST_STACK_SIZE - 1;
        // ------------------------------------------------------------------------
        // following data words of the stack will be restored automatically by CPU
        // ------------------------------------------------------------------------
        // at least 2 datawords are reserved in the beginning of exception stack frame
        currSP -= 2;
        // for FPSCR, it should be intialized to zero 
        *currSP = 0x0 ;
        currSP -= 1;
        // for floating-point register s0-s15
        for(idx=1; idx<=16; idx++ ) {
            *(portFLOAT *)currSP = (portFLOAT)idx * 1.5f;
             currSP -= 1;
        }
        // specify appropriate status to xPSR, here we only set Thumb mode bit
        *currSP = xPSR_T_Msk;
        currSP -= 1;
        // for pc, pointed to our helper function, the help function will generate SVC
        // event & CPU will jump back to test body again.
        *currSP = ((StackType_t)vMockTask2Func) &  portCPU_ADDRESS_MASK;
        currSP -= 1;
        // for r1-r3, r12, lr assigned with an unique value 
        for(idx=1; idx<=5; idx++ ) {
            *currSP = (idx<<24) | (idx<<16) | (idx<<8) | (idx<<1);
             currSP -= 1;
        }
        // for r0, where argument pointer is stored.
        pxParam = (mockTask2ParamStruct *) unity_malloc( sizeof(mockTask2ParamStruct) );
        pxParam->pxTCB = pxTCBtoTaskFunc ;
        pxParam->uFPen = uFPen;
        *currSP = (StackType_t)pxParam;
        currSP -= 1;
        // --------------------------------------------------------- 
        // following data words of the stack will be restored by 
        // our RTOS port function vPortPendSVHandler()
        // ---------------------------------------------------------
        // for floating-point register s16-s31, 
        // treat this part of the stack as floating-point memory space
        for(idx=0x10; idx<0x20; idx++ ) {
            *(portFLOAT *)currSP = (portFLOAT)idx * 1.1f;
            currSP -= 1;
        }
        // for lr in the exception handler routine. Assume this task has floating-point 
        // data to restore, so bit 4 of exception return address has to be zero.
        *currSP = portEXPTN_RETURN_TO_TASK_FP;
        currSP -= 1;
        // each of r4-r11 is assigned with an unique value 
        for(idx=0x1; idx<=0x8; idx++ ) {
            *currSP = (idx<<26) | (idx<<17) | (idx<<12) | (idx<<3);
            currSP -= 1;
        }
        // it will be written to CONTROL register
        *currSP = CONTROL_FPCA_Msk | CONTROL_SPSEL_Msk;
    }
    if(pxTCB != NULL)
    {
        pxTCB->pxStack      = taskStack;
        pxTCB->pxTopOfStack = currSP ;
        ulRegionSizeInBytes = (UBaseType_t)__SRAM_segment_end__ - (UBaseType_t)__SRAM_segment_start__;
        pxTCB->xMPUSettings.xRegion[ 0 ].RBAR = ((UBaseType_t) __SRAM_segment_start__  & MPU_RBAR_ADDR_Msk) 
                                                 | MPU_RBAR_VALID_Msk | portSTACK_REGION;
        pxTCB->xMPUSettings.xRegion[ 0 ].RASR = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                         ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                         | MPU_RASR_ENABLE_Msk ;
        ulRegionSizeInBytes = (UBaseType_t)__code_segment_end__ - (UBaseType_t)__code_segment_start__;
        pxTCB->xMPUSettings.xRegion[ 1 ].RBAR = ((UBaseType_t)__code_segment_start__ & MPU_RBAR_ADDR_Msk)
                                                 | MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION;
        pxTCB->xMPUSettings.xRegion[ 1 ].RASR = 
                          portMPU_REGION_READ_ONLY |  MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                          ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                          | MPU_RASR_ENABLE_Msk;
        pxTCB->xMPUSettings.xRegion[ 2 ].RBAR = MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1);
        pxTCB->xMPUSettings.xRegion[ 2 ].RASR = 0x0;
        pxTCB->xMPUSettings.xRegion[ 3 ].RBAR = MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 2);
        pxTCB->xMPUSettings.xRegion[ 3 ].RASR = 0x0;
        pxTCB->stack_alignment_words = 0;
    }
    return pxTCB;
} //// end of vInitMockTask2




static void vFreeMockTask (TCB_test_t *pxTCB)
{
    // ---- free memory space allocated to test stack -----
    unity_free( (void *)pxTCB->pxStack );
    pxTCB->pxStack = NULL;
    // ---- free memory space allocated to test TCB -----
    unity_free( (void *)pxTCB );
    pxTCB = NULL;
} // end of vFreeMockTask




static void vCheckTaskContext( BaseType_t stack_alignment, portSHORT FPenabled )
{
    portSHORT idx = 0;
    // PendSV exception could be generated with floating-point state, that is, 
    // all floating-point registers would be stacked by CPU hardware (s0-s15)  or
    //  exception handling routine (s16-s31, done by software) , if any 
    // floating-point register is used in a task.
    portSHORT   sw_stacking_len = 0x1 + 0x8 + 0x1 ;
    portSHORT   hw_stacking_len = 0x8 ;
    BaseType_t  expect_stack_frame_size = 0;
    BaseType_t  actual_stack_frame_size = 0;

    if( FPenabled==TEST_FLOAT_POINT_ENABLE ) 
    {
        sw_stacking_len += 0x10; // add s16 - s31
        hw_stacking_len += 0x11; // add s0  - s15, FPSCR
        stack_alignment += 1; // one more word for alignment, this is undocumented.
    }
    expect_stack_frame_size = sw_stacking_len + hw_stacking_len + stack_alignment ;
    actual_stack_frame_size = ((UBaseType_t) expected_value->psp) - ((UBaseType_t) actual_value->psp);
    if(actual_stack_frame_size < 0) {
        actual_stack_frame_size = actual_stack_frame_size * -1;
    }
    actual_stack_frame_size = actual_stack_frame_size >> 2;
    TEST_ASSERT_EQUAL_UINT32( expected_value->GPRs[0], actual_value->GPRs[0] );
    for(idx=4; idx<=12; idx++) {
        TEST_ASSERT_EQUAL_UINT32( expected_value->GPRs[idx], actual_value->GPRs[idx] );
    }
    // copy them if floating-point state is on
    if (FPenabled == TEST_FLOAT_POINT_ENABLE) {
        for(idx=0; idx<=31; idx++) {
            TEST_ASSERT_EQUAL_FLOAT ( expected_value->FPRs[idx], actual_value->FPRs[idx] );
        }
    }
    TEST_ASSERT_EQUAL_UINT32( expected_value->lr, actual_value->lr );
    // the 2 mock tasks in this unit test use PSP, not the MSP.
    TEST_ASSERT_EQUAL_UINT32( expect_stack_frame_size, actual_stack_frame_size );
    // check CONTROL.FPCA & CONTROL.nPRIV
    TEST_ASSERT_EQUAL_UINT32( (expected_value->control & 0x5), (actual_value->control & 0x5) );
    // check xPSR.{N,Z,C,V}
    TEST_ASSERT_EQUAL_UINT32( (expected_value->xPSR & 0xf0000000UL), (actual_value->xPSR & 0xf0000000UL) );
    // check MPU region settings
    for(idx=0x0; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++) 
    {
        TEST_ASSERT_EQUAL_UINT32( (expected_value->xMPUSettings.xRegion[idx].RBAR & ~(MPU_RBAR_VALID_Msk)) , (actual_value->xMPUSettings.xRegion[idx].RBAR & ~(MPU_RBAR_VALID_Msk)) );
        TEST_ASSERT_EQUAL_UINT32( expected_value->xMPUSettings.xRegion[idx].RASR , actual_value->xMPUSettings.xRegion[idx].RASR );
    }
} //// end of vCheckTaskContext



void TEST_HELPER_vPortPendSVHandler_PendSVentry( void )
{
    if( expected_value != NULL ) {
        // update stack alignment information on PendSV exception entry
        ((TCB_test_t *)pxCurrentTCB)->stack_alignment_words = stack_alignment_words;
        // the target function we want to test
        __asm volatile ("b vPortPendSVHandler  \n");
    }
}//// end of TEST_HELPER_vPortPendSVHandler_PendSVentry




// mock function instead of real context switch code in FreeRTOS
void vTaskSwitchContext(void)
{
    numContextSwitches++;
    pxCurrentTCB = (void *) mockReadyTaskList[ (numContextSwitches % 2) ];
}



// To declare a new test group in Unity, firstly you use the macro below
TEST_GROUP( vPortPendSVHandler );


// initial setup before running the test body
TEST_SETUP( vPortPendSVHandler )
{
    // initialize the data structure we will put the test result into
    expected_value = (RegsSet_chk_t *) unity_malloc( sizeof(RegsSet_chk_t) ) ;
    actual_value   = (RegsSet_chk_t *) unity_malloc( sizeof(RegsSet_chk_t) ) ;
    memset((void *)expected_value, 0, sizeof(RegsSet_chk_t));
    memset((void *)actual_value  , 0, sizeof(RegsSet_chk_t));
    // ------------- enable SVC & PendSV exception -------------
    NVIC_SetPriority( SVCall_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
    NVIC_SetPriority( PendSV_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY );
    __asm volatile( "cpsie i" );
    __asm volatile( "cpsie f" );
    __asm volatile( "dsb" ::: "memory" );
    __asm volatile( "isb" );
    numContextSwitches = 0;
} //// end of TEST_SETUP


TEST_TEAR_DOWN( vPortPendSVHandler )
{
    vDisableSaveFPcontext();
    // ------------- disable SVC & PendSV exception -------------
    __asm volatile( "dsb" ::: "memory" );
    __asm volatile( "isb" );
    __asm volatile( "cpsid i" );
    __asm volatile( "cpsid f" );
    NVIC_SetPriority( SVCall_IRQn, 0 );
    NVIC_SetPriority( PendSV_IRQn, 0 );
    // ---- free memory space allocated to cehck lists -----
    unity_free( (void *)expected_value ) ;
    unity_free( (void *)actual_value   ) ;
    expected_value = NULL;
    actual_value   = NULL;
    pxCurrentTCB   = NULL;
} //// end of TEST_TEAR_DOWN






TEST( vPortPendSVHandler , cs_with_fp )
{
    TCB_test_t  *pxTask1TCB = NULL;
    TCB_test_t  *pxTask2TCB = NULL;
    const portSHORT fp_list_size = 0x20;
    portFLOAT  *test_fp_X = NULL;
    portSHORT   idx = 0;

    // ------------- initial setup in the test body -------------
    // initialize stack memory & TCB of the 2 mock tasks
    pxTask1TCB = pxInitMockTask1( );
    pxTask2TCB = pxInitMockTask2( pxTask1TCB, TEST_FLOAT_POINT_ENABLE );
    mockReadyTaskList[0] = (UBaseType_t)pxTask1TCB;
    mockReadyTaskList[1] = (UBaseType_t)pxTask2TCB;
    pxCurrentTCB = (void *) mockReadyTaskList[0];
    // copy MPU regions setting from TCB to CPU
    for(idx=0x0; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++) {
        MPU->RNR  = portSTACK_REGION + idx;
        MPU->RBAR = ((TCB_test_t *)pxCurrentTCB)->xMPUSettings.xRegion[idx].RBAR;
        MPU->RASR = ((TCB_test_t *)pxCurrentTCB)->xMPUSettings.xRegion[idx].RASR;
    }
    // now CPU is in Thread mode, we can switch to psp for pxTask1TCB by setting CONTROL.SPSEL
    __asm volatile (
        "msr  psp, %0       \n"
        ::"r"(((TCB_test_t *)pxCurrentTCB)->pxTopOfStack):
    );
    vSwitchFromMSP2PSP();
    // do some random floating point-caculation, to force CONTROL.FPCA set .
    test_fp_X  = (portFLOAT *) unity_malloc(sizeof(portFLOAT) * fp_list_size);
    *(test_fp_X + 0) = 2.7181f;
    *(test_fp_X + 1) = 5.3585f;
    for(idx=2; idx<fp_list_size; idx++) {
        *(test_fp_X + idx) = (portSHORT)idx * 1.3f ;
    }
    __asm volatile(
        "vldmia %0, {s0-s31}   \n"
        "vadd.F32   s0 , s0 , s17  \n"
        "vadd.F32   s1 , s1 , s18  \n"
        "vadd.F32   s2 , s0 , s17  \n"
        "vadd.F32   s3 , s1 , s16  \n"
        "vadd.F32   s4 , s2 , s19  \n"
        "vadd.F32   s5 , s3 , s29  \n"
        "vadd.F32   s6 , s4 , s28  \n"
        "vadd.F32   s7 , s5 , s27  \n"
        "vadd.F32   s8 , s6 , s26  \n"
        "vadd.F32   s9 , s7 , s25  \n"
        "vadd.F32   s10, s8 , s24  \n"
        "vadd.F32   s11, s9 , s23  \n"
        "vadd.F32   s12, s10, s22  \n"
        "vadd.F32   s13, s11, s31  \n"
        "vadd.F32   s14, s12, s21  \n"
        "vadd.F32   s15, s13, s22  \n"
        "vadd.F32   s24, s2 , s19  \n"
        "vadd.F32   s25, s3 , s9   \n"
        "vadd.F32   s26, s4 , s28  \n"
        "vadd.F32   s27, s5 , s7   \n"
        "vadd.F32   s28, s6 , s26  \n"
        "vadd.F32   s29, s7 , s25  \n"
        "vadd.F32   s30, s8 , s4   \n"
        "vadd.F32   s31, s9 , s3   \n"
        ::"r"(test_fp_X) :
    );
    // push/pop stack (PSP) with some items, in order to get different 
    // stack alignment information on exception entry .
    __asm volatile ("push       {r11,r9,r6}  \n");

    // ---------------- part 1 ----------------
    // record the current content of CPU registers, they are supposed to be saved to
    // pxTask1TCB after the first context switch
    vUpdateCheckListFromRegs( expected_value , TEST_FLOAT_POINT_ENABLE);
    // we launch context switch by manually setting PendSV bit of SCB_ICSR,then CPU jumps 
    // to PendSV exception handler, which handles context switch implementation.
    vMockTaskYield();
    // jump back here from vMockTask2Func()
    // now CPU is in Thread mode, we switch back to msp by resetting CONTROL.SPSEL
    vSwitchFromPSP2MSP();
    // check whether context of pxTask1TCB  is correct in CPU registers
    vCheckTaskContext( pxTask1TCB->stack_alignment_words, TEST_FLOAT_POINT_ENABLE );

    // ---------------- part 2 ----------------
    // record the context of next task pxTask2TCB, they are supposed to 
    // restore to CPU registers after the following context switch
    vUpdateCheckListFromTCB( expected_value, pxTask2TCB, TEST_FLOAT_POINT_ENABLE );
    // now CPU is in Thread mode, switch to psp again, this time we don't need to
    // modify psp from top stack pointer of current task.
    vSwitchFromMSP2PSP();
    // switch to another task again by manually setting PendSV bit,
    vMockTaskYield();
    // push/pop stack (PSP) with some items, in order to get different 
    // stack alignment information on exception entry .
    __asm volatile ("pop        {r11,r9, r6}  \n");
    // switch back to msp again
    vSwitchFromPSP2MSP();
    // check whether context of pxTask2TCB is correct in CPU registers
    vCheckTaskContext( pxTask2TCB->stack_alignment_words, TEST_FLOAT_POINT_ENABLE );

    // do some random floating point caculation, to force CONTROL.FPCA set .
    __asm volatile(
        "vstmia  %0, {s0-s31}   \n"
        ::"r"(test_fp_X):
    );
    unity_free( (void *)test_fp_X );
    test_fp_X = NULL;
    // -------------- reset MPU regions setting --------------
    for(idx=0x0; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++) {
        MPU->RNR  = portSTACK_REGION + idx;
        MPU->RBAR = MPU_RBAR_VALID_Msk | (portSTACK_REGION + idx);
        MPU->RASR = MPU_RBAR_VALID_Msk | (portSTACK_REGION + idx);
    }
    // ---- free memory space allocated to test stack & TCB -----
    vFreeMockTask( pxTask1TCB );
    vFreeMockTask( pxTask2TCB );
} // end of test body





TEST( vPortPendSVHandler , cs_without_fp )
{
    TCB_test_t  *pxTask2TCB = NULL;
    TCB_test_t  *pxTask3TCB = NULL;
    portSHORT idx = 0;
    // create memory for mock task2, task 3. In this test case, 
    // task 2 performs floating-point arithmetic therefore when exception occurs
    // on task 2 the floating-point state must also be saved. Task 3 performs
    // only integer arithmetic.
    pxTask3TCB = pxInitMockTask3( );
    pxTask2TCB = pxInitMockTask2( pxTask3TCB, TEST_FLOAT_POINT_DISABLE );
    mockReadyTaskList[0] = (UBaseType_t)pxTask3TCB;
    mockReadyTaskList[1] = (UBaseType_t)pxTask2TCB;
    pxCurrentTCB = (void *) mockReadyTaskList[0];
    // copy MPU regions setting from TCB to CPU
    for(idx=0x0; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++) {
        MPU->RNR  = portSTACK_REGION + idx;
        MPU->RBAR = ((TCB_test_t *)pxCurrentTCB)->xMPUSettings.xRegion[idx].RBAR;
        MPU->RASR = ((TCB_test_t *)pxCurrentTCB)->xMPUSettings.xRegion[idx].RASR;
    }
    __asm volatile (
        "msr  psp, %0       \n"
        ::"r"(((TCB_test_t *)pxCurrentTCB)->pxTopOfStack):
    );
    // ---------------- part 1 ----------------
    // record the current content of CPU registers, they are supposed to be saved to
    // pxTask3TCB after the first context switch
    vDisableSaveFPcontext();
    // now CPU is in Thread mode, we can switch to psp for pxTask3TCB by setting CONTROL.SPSEL
    vSwitchFromMSP2PSP();
    vUpdateCheckListFromRegs( expected_value , TEST_FLOAT_POINT_DISABLE );
    vMockTaskYield();
    vSwitchFromPSP2MSP();
    vCheckTaskContext( pxTask3TCB->stack_alignment_words, TEST_FLOAT_POINT_DISABLE );
    // ---------------- part 2 ----------------
    // record the context of next task pxTask2TCB, they are supposed to 
    // restore to CPU registers after the following context switch
    vDisableSaveFPcontext();
    // now CPU is in Thread mode, we can switch to psp for pxTask3TCB by setting CONTROL.SPSEL
    vUpdateCheckListFromTCB( expected_value, pxTask2TCB, TEST_FLOAT_POINT_ENABLE );
    vSwitchFromMSP2PSP();
    vMockTaskYield();
    vSwitchFromPSP2MSP();
    vCheckTaskContext( pxTask2TCB->stack_alignment_words, TEST_FLOAT_POINT_ENABLE );

    // -------------- reset MPU regions setting --------------
    for(idx=0x0; idx<(portNUM_CONFIGURABLE_REGIONS + 1); idx++) {
        MPU->RNR  = portSTACK_REGION + idx;
        MPU->RBAR = MPU_RBAR_VALID_Msk | (portSTACK_REGION + idx);
        MPU->RASR = MPU_RBAR_VALID_Msk | (portSTACK_REGION + idx);
    }
    // ---- free memory space allocated to test stack -----
    vFreeMockTask( pxTask2TCB );
    vFreeMockTask( pxTask3TCB );
} // end of test body



