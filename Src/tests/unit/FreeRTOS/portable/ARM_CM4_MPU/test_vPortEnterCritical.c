// for C unit test framework Unity
#include "unity_fixture.h"
// in this test, we will put a function & few variables in privileged area
// by putting the macros PRIVILEGED_FUNCTION and PRIVILEGED_DATA ahead of
// the privileged function & data. 
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
// for FreeRTOS
#include "FreeRTOS.h"

typedef struct {
    UBaseType_t  sharedCounter;
    UBaseType_t  numNest;     // number of times a task re-enters a critical section
    UBaseType_t  numSVCalls;  // number of interrupts happened in this test
} flagSet_chk_t;

static flagSet_chk_t  *expected_value = NULL;
static flagSet_chk_t  *actual_value   = NULL;


// we need to check whether this HardFault is caused under the conditions below :
// * the HardFault is caused when a task calls SVCall 
// * In FreeRTOS, when an unprivileged task enters critical section, it has to modify BASEPRI
//   by executing SVCall, temporarily switching to privileged software in the SVCall handler routine.  
//   Then set BASEPRI in the privileged software to the number as the same as SVCall exception priority. 
//   By doing so the task in critical section won't be preempted by arriving exceptions, whose priority
//   is numerically greater than / equal to SVCall.
// * when the task leaves critical section,  the first thing it has to do is to execute SVCall again
//   , temporarily switch to privileged software again to reset BASEPRI, let exceptions of all priority start working.
//   However at this moment the SVCall is temporarily disabled due to BASEPRI content, therefore Cortex-M CPU
//   generates Hardfault instead of SVCall exception.
// Based on the description above, we need to check whether the task attempts to temporarily switch to privileged 
// software on exit of critical section for resetting the privileged register BASEPRI
void TEST_HELPER_vPortEnterCritical_hardFaultEntry( UBaseType_t *pulSelectedSP )
{
    UBaseType_t  forcedHardFault;
    UBaseType_t  currBasepri;
    uint8_t        ucSVCnumber      = 0;
    uint8_t        thumbEncodeInstr = 0;
    const uint8_t  thumbEncodeSVC   = 0xdf; // the encoded value of SVC in thumb instruction

    if( expected_value != NULL ) {
        forcedHardFault  = SCB->HFSR & SCB_HFSR_FORCED_Msk;
        currBasepri      = __get_BASEPRI();
        // get pc address from exception stack frame, then 
        // go back to check whether last instrution is SVCall.
        ucSVCnumber      = ((uint8_t *) pulSelectedSP[6] )[-2];
        thumbEncodeInstr = ((uint8_t *) pulSelectedSP[6] )[-1];
        if ((forcedHardFault != pdFALSE) && (currBasepri == configMAX_SYSCALL_INTERRUPT_PRIORITY) && (thumbEncodeInstr==thumbEncodeSVC) && (ucSVCnumber==portSVC_ID_RAISE_PRIVILEGE)) 
        {
            actual_value->numSVCalls += 1;
            __asm volatile (
                "pop  {lr}               \n"
                "b    vPortSVCHandler    \n"
            );
        }
    }
} //// end of TEST_HELPER_vPortEnterCritical_hardFaultEntry



void TEST_HELPER_vPortEnterCritical_SVCentry( void )
{
    if( actual_value != NULL ) {
        actual_value->numSVCalls += 1;
        __asm volatile (
            "pop  {lr}               \n"
            "b    vPortSVCHandler    \n"
        );
    }
} //// end of TEST_HELPER_vPortEnterCritical_SVCentry





void TEST_HELPER_vPortEnterCritical_sysTickEntry ( void )
{
    if( actual_value != NULL ) {
        actual_value->sharedCounter += 0x1;
    }
} //// end of TEST_HELPER_vPortEnterCritical_sysTickEntry 





static void vModifyMPUregionsForTest( void )
{
    extern  UBaseType_t  __SRAM_segment_start__[];
    extern  UBaseType_t  __SRAM_segment_end__ [];
    xMPU_REGION_REGS  xRegion;
    UBaseType_t  ulRegionSizeInBytes = 0;
    // setup region #0 - #3 using exiting function
    prvSetupMPU();
    // move attributes from region #2 to region #6
    vPortGetMPUregion( portPRIVILEGED_SRAM_REGION, &xRegion );
    xRegion.RBAR &= ~(MPU_RBAR_REGION_Msk);
    xRegion.RBAR |= MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1);
    vPortSetMPUregion( &xRegion );
    // setup one more MPU region #5 only for unprivileged accesses in this test to SRAM
    ulRegionSizeInBytes = (UBaseType_t) __SRAM_segment_end__ - (UBaseType_t) __SRAM_segment_start__ ;
    xRegion.RBAR = ((UBaseType_t) __SRAM_segment_start__  & MPU_RBAR_ADDR_Msk)
                      | MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION  ;
    xRegion.RASR = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk |
                     ( MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos) )
                     | MPU_RASR_ENABLE_Msk;
    vPortSetMPUregion( &xRegion );
} // end of vModifyMPUregionsForTest



static void vResetMPUregionsForTest( void )
{
    xMPU_REGION_REGS  xRegion;
    const portSHORT   numRegions = 8;
    portSHORT         idx = 0;
    // turn off the MPU first
    SCB->SHCSR  &= ~SCB_SHCSR_MEMFAULTENA_Msk;
    MPU->CTRL   &= ~(MPU_CTRL_ENABLE_Msk | MPU_CTRL_PRIVDEFENA_Msk);
    // then clear the settings of each region
    xRegion.RASR = 0;
    for (idx=0; idx<numRegions; idx++) {
        xRegion.RBAR = MPU_RBAR_VALID_Msk | idx;
        vPortSetMPUregion( &xRegion );
    }
} // end of vResetMPUregionsForTest



// To declare a new test group in Unity, firstly you use the macro below
TEST_GROUP( vPortEnterCritical );


TEST_SETUP( vPortEnterCritical )
{
    // setup extra MPU regions used in this test
    vModifyMPUregionsForTest( );
    expected_value = (flagSet_chk_t *) unity_malloc( sizeof(flagSet_chk_t) );
    actual_value   = (flagSet_chk_t *) unity_malloc( sizeof(flagSet_chk_t) );
    expected_value->sharedCounter = 0;
    expected_value->numNest       = 0; 
    expected_value->numSVCalls    = 0; 
    actual_value->sharedCounter = 0;
    actual_value->numNest       = 0; 
    actual_value->numSVCalls    = 0; 
    // enable interrupt & set proper priority
    NVIC_SetPriority( SysTick_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY );
    NVIC_SetPriority( SVCall_IRQn , configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
    // enable SysTick timer
    SysTick->CTRL = 0L;
    SysTick->VAL  = 0L;
    SysTick->LOAD = (configCPU_CLOCK_HZ / (configTICK_RATE_HZ*20))- 1UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk 
                    | SysTick_CTRL_ENABLE_Msk ;
    __asm volatile(
        "cpsie i  \n" 
        "cpsie f  \n" 
        "dsb      \n" 
        "isb      \n"
        :::"memory"
    );
} // end of TEST_SETUP


TEST_TEAR_DOWN( vPortEnterCritical )
{
    // raise privilege no matter this test passed or failed.
    xPortRaisePrivilege();
    // recover BASEPRI
    portENABLE_INTERRUPTS();
    // clear MPU regions used in this test
    vResetMPUregionsForTest();
    // free memory allocated to check lists 
    unity_free( (void *)expected_value ); 
    unity_free( (void *)actual_value   ); 
    expected_value = NULL; 
    actual_value   = NULL; 
    // disable interrupt & reset priority
    __asm volatile(
        "dsb      \n" 
        "isb      \n"
        "cpsid i  \n" 
        "cpsid f  \n" 
        :::"memory"
    );
    SysTick->CTRL  = 0;
    SysTick->VAL   = 0;
    SysTick->LOAD  = 0;
    NVIC_SetPriority( SysTick_IRQn, 0 );
    NVIC_SetPriority( SVCall_IRQn , 0 );
} // end of TEST_TEAR_DOWN




// it's better to test this function by simulating an unprivileged task 
// doing something in a critical section
TEST( vPortEnterCritical , single_critical_section )
{
    BaseType_t   xState;
    UBaseType_t  sharedCounter = 0;
    UBaseType_t  countsPerTick = SysTick->LOAD;
    UBaseType_t  idx = 0;

    // force to switch to unprivileged state
    vPortResetPrivilege( pdTRUE );
    // switch to privileged state by calling "svn 2" 
    xState = xPortRaisePrivilege();
    // svc exception interrupt should work properly
    expected_value->numSVCalls += 1; 
    TEST_ASSERT_EQUAL_INT32(  xState, pdTRUE );
    TEST_ASSERT_EQUAL_UINT32( expected_value->numSVCalls, actual_value->numSVCalls );
    // switch to unprivileged state again
    vPortResetPrivilege( xState );

    // write current shared counter value
    for(idx=0; idx<countsPerTick; idx++);
    expected_value->sharedCounter = actual_value->sharedCounter;
    // entering critical section
    vPortEnterCritical();
    {
        // svc exception interrupt in vPortEnterCritical() should work properly
        expected_value->numSVCalls += 1; 
        for(idx=0; idx<(countsPerTick << 10) ; idx++)
        {
            TEST_ASSERT_EQUAL_UINT32( expected_value->numSVCalls   , actual_value->numSVCalls );
            TEST_ASSERT_EQUAL_UINT32( expected_value->sharedCounter, actual_value->sharedCounter );
            __asm volatile(
                "dsb      \n" 
                "isb      \n"
                :::"memory"
            );
        }
    }
    // leaving critical section
    vPortExitCritical(); 
    // check the sharedCounter, it shouldn't be changed that much in the critical section
    sharedCounter = actual_value->sharedCounter;
    expected_value->numSVCalls += 1; 
    TEST_ASSERT_EQUAL_UINT32( expected_value->numSVCalls, actual_value->numSVCalls );
    TEST_ASSERT_UINT32_WITHIN( 0x3, expected_value->sharedCounter, sharedCounter );
} // end of test body



TEST( vPortEnterCritical , nested_critical_section )
{
    UBaseType_t  sharedCounter = 0;
    UBaseType_t  countsPerTick = SysTick->LOAD;
    UBaseType_t  idx = 0;

    // force to switch to unprivileged state
    vPortResetPrivilege( pdTRUE );
    // write current shared counter value
    for(idx=0; idx<countsPerTick; idx++);
    expected_value->sharedCounter = actual_value->sharedCounter;
    
    // entering the first critical section
    vPortEnterCritical();
    {
        // when enter/exit the critical section, it should jump to SVCall handler routine.
        expected_value->numSVCalls += 1; 
        TEST_ASSERT_EQUAL_UINT32( expected_value->numSVCalls, actual_value->numSVCalls );
        // polling actual_value->sharedCounter to see if it is increased in this critical section
        for(idx=0; idx<(countsPerTick << 8) ; idx++) {
            TEST_ASSERT_EQUAL_UINT32( expected_value->sharedCounter, actual_value->sharedCounter );
        }
        // entering the nested critical section, this time it won't jump to SVCall handler routine
        vPortEnterCritical();
        {
            expected_value->numSVCalls += 1; 
            TEST_ASSERT_EQUAL_UINT32( expected_value->numSVCalls, actual_value->numSVCalls );
            for(idx=0; idx<(countsPerTick << 10); idx++) {
                TEST_ASSERT_EQUAL_UINT32( expected_value->sharedCounter, actual_value->sharedCounter );
            }
        } // leave the nested critical section, 
        vPortExitCritical(); 
        expected_value->numSVCalls += 1; 
        TEST_ASSERT_EQUAL_UINT32( expected_value->numSVCalls, actual_value->numSVCalls );
        for(idx=0; idx<(countsPerTick << 10) ; idx++) {
            TEST_ASSERT_EQUAL_UINT32( expected_value->sharedCounter, actual_value->sharedCounter );
        }
    } // leave the first critical section
    vPortExitCritical();
    // read shared counter again  
    sharedCounter = actual_value->sharedCounter;
    expected_value->numSVCalls += 1; 
    TEST_ASSERT_EQUAL_UINT32( expected_value->numSVCalls, actual_value->numSVCalls );
    TEST_ASSERT_UINT32_WITHIN( 0x3, expected_value->sharedCounter, sharedCounter );
} // end of test body


