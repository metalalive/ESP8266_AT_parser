// for C unit test framework Unity
#include "unity_fixture.h"
// in this test, we will put a function & few variables in privileged area
// by putting the macros PRIVILEGED_FUNCTION and PRIVILEGED_DATA ahead of
// the privileged function & data. 
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
// for FreeRTOS
#include "FreeRTOS.h"
#include "task.h"

static UBaseType_t      *expected_value = NULL;
static UBaseType_t      *actual_value   = NULL;
static eSleepModeStatus  eMockCurrSleepMode;
static TickType_t        xMockTickCount;
static TickType_t        xMockNextTaskUnblockTime;

// mock function to determine if there is any ready task in current FreeRTOS
// then determine whether the CPU can sleep.
eSleepModeStatus  eTaskConfirmSleepModeStatus( void )
{
    return eMockCurrSleepMode;
}


// mock function to add number of ticks directly into FreeRTOs variable xTickCount
void vTaskStepTick( const TickType_t xTicksToJump )
{
    configASSERT((xMockTickCount + xTicksToJump) <= xMockNextTaskUnblockTime);
    xMockTickCount += xTicksToJump;
    *actual_value   = xMockTickCount;
}


void TEST_HELPER_vPortSuppressTicksAndSleep_sysTickEntry( void )
{
    if( expected_value != NULL ) {
        xMockTickCount += 1;
    }
} //// end of TEST_HELPER_vPortSuppressTicksAndSleep_sysTickEntry



// To declare a new test group in Unity, firstly you use the macro below
TEST_GROUP( vPortSuppressTicksAndSleep );


TEST_SETUP( vPortSuppressTicksAndSleep )
{
    expected_value  = (UBaseType_t *) unity_malloc( sizeof(UBaseType_t) );
    actual_value    = (UBaseType_t *) unity_malloc( sizeof(UBaseType_t) );
    *expected_value = 0;
    *actual_value   = 0;
    eMockCurrSleepMode       = eNoTasksWaitingTimeout;
    xMockTickCount           = 0;
    xMockNextTaskUnblockTime = 0;
    // enable SysTick interrupt for this test
    __asm volatile(
        "cpsie i  \n" 
        "cpsie f  \n" 
        "dsb      \n" 
        "isb      \n"
        :::"memory"
    );
    SysTick->CTRL = 0;
    SysTick->VAL  = 0;
    SysTick->LOAD = (configCPU_CLOCK_HZ / configTICK_RATE_HZ) - 1UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk ;
} // end of TEST_SETUP



TEST_TEAR_DOWN( vPortSuppressTicksAndSleep )
{
    unity_free( (void *)expected_value ); 
    unity_free( (void *)actual_value   ); 
    expected_value = NULL; 
    actual_value   = NULL; 
    // disable SysTick interrupt 
    SysTick->CTRL = 0;
    SysTick->VAL  = 0;
    SysTick->LOAD = 0;
    __asm volatile(
        "cpsid i  \n" 
        "cpsid f  \n" 
        "dsb      \n" 
        "isb      \n"
        :::"memory"
    );
} // end of TEST_TEAR_DOWN




#if( configUSE_TICKLESS_IDLE == 1)
TEST( vPortSuppressTicksAndSleep, sleep_failure )
#else
IGNORE_TEST( vPortSuppressTicksAndSleep, sleep_failure )
#endif //// end of configUSE_TICKLESS_IDLE
{
    portSHORT   jdx = 0;
    TickType_t  xTicksToSleep;
    TickType_t  ulTimerCountsPerTick = SysTick->LOAD ;

    xTicksToSleep   = 13;
    *expected_value = 0 ;
    eMockCurrSleepMode  = eAbortSleep;
    for (jdx=0; jdx<ulTimerCountsPerTick; jdx++) ;
    xMockNextTaskUnblockTime = xMockTickCount + xTicksToSleep;
    vPortSuppressTicksAndSleep( xTicksToSleep );
    // At here, we check number of ticks passed during the low-power entry function
    TEST_ASSERT_UINT32_WITHIN( 0x1, *expected_value , *actual_value );
} // end of test body





#if( configUSE_TICKLESS_IDLE == 1)
TEST( vPortSuppressTicksAndSleep, sleep_k_ticks )
#else
IGNORE_TEST( vPortSuppressTicksAndSleep, sleep_k_ticks )
#endif //// end of configUSE_TICKLESS_IDLE
{
    TickType_t  xTicksToSleep;
    TickType_t  ulTimerCountsPerTick = SysTick->LOAD ;
    portSHORT   idx = 0;
    portSHORT   jdx = 0;

    for (idx=0; idx<20; idx++) 
    {
        for (jdx=0; jdx<ulTimerCountsPerTick; jdx++) ;
        xTicksToSleep = 1;
        *expected_value = xMockTickCount + xTicksToSleep;
        xMockNextTaskUnblockTime = xMockTickCount + xTicksToSleep;
        vPortSuppressTicksAndSleep( xTicksToSleep );
        // At here, we check number of ticks passed during the low-power entry function
        TEST_ASSERT_UINT32_WITHIN( 0x1, *expected_value , *actual_value );

        for (jdx=0; jdx<ulTimerCountsPerTick; jdx++) ;
        xTicksToSleep = 5;
        *expected_value = xMockTickCount + xTicksToSleep;
        xMockNextTaskUnblockTime = xMockTickCount + xTicksToSleep;
        vPortSuppressTicksAndSleep( xTicksToSleep );
        TEST_ASSERT_UINT32_WITHIN( 0x1, *expected_value , *actual_value );

        for (jdx=0; jdx<ulTimerCountsPerTick; jdx++) ;
        xTicksToSleep = 300;
        *expected_value = xMockTickCount + xTicksToSleep;
        xMockNextTaskUnblockTime = xMockTickCount + xTicksToSleep;
        vPortSuppressTicksAndSleep( xTicksToSleep );
        TEST_ASSERT_UINT32_WITHIN( 0x1, *expected_value , *actual_value );
    } // end of loop
} // end of test body



