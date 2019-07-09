// for C unit test framework Unity
#include "unity_fixture.h"
// in this test, we will put a function & few variables in privileged area
// by putting the macros PRIVILEGED_FUNCTION and PRIVILEGED_DATA ahead of
// the privileged function & data. 
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
// for FreeRTOS
#include "FreeRTOS.h"

typedef struct {
    // asserted on SysTick handler routine.
    UBaseType_t  sysTickFlag;
    // asserted on PendSV handler routine
    UBaseType_t  pendSVflag;
} flagSet_chk_t;

static flagSet_chk_t  *expected_value = NULL;
static flagSet_chk_t  *actual_value   = NULL;
static BaseType_t      xMockYieldTaskFlag ;


void TEST_HELPER_vPortSysTickHandler_PendSVentry( void )
{
    if( actual_value != NULL ) {
        actual_value->pendSVflag = 1;
    }
} //// end of TEST_HELPER_vPortSysTickHandler_PendSVentry


void TEST_HELPER_vPortSysTickHandler_SysTickEntry( void )
{
    if( actual_value != NULL )
    {
        actual_value->sysTickFlag = 1;
        __asm volatile (
            "pop  {lr}                   \n"
            "b    vPortSysTickHandler    \n"
        );
    }
} //// end of TEST_HELPER_vPortSysTickHandler_SysTickEntry




// mock FreeRTOS API function for this test
BaseType_t xTaskIncrementTick( void )
{
    return xMockYieldTaskFlag;
}



TEST_GROUP( vPortSysTickHandler );


TEST_SETUP( vPortSysTickHandler )
{
    xMockYieldTaskFlag = pdFALSE;
    expected_value = (flagSet_chk_t *) unity_malloc( sizeof(flagSet_chk_t) );
    actual_value   = (flagSet_chk_t *) unity_malloc( sizeof(flagSet_chk_t) );
    expected_value->sysTickFlag = 0;
    expected_value->pendSVflag  = 0;
    actual_value->sysTickFlag = 0;
    actual_value->pendSVflag  = 0;
    // enable SysTick timer
    SysTick->CTRL = 0L;
    SysTick->VAL  = 0L;
    SysTick->LOAD = (configCPU_CLOCK_HZ / (configTICK_RATE_HZ*20))- 1UL;
    // enable interrupt & set proper priority
    NVIC_SetPriority( SysTick_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY );
    __asm volatile(
        "cpsie i  \n" 
        "cpsie f  \n" 
        "dsb      \n" 
        "isb      \n"
        :::"memory"
    );
} //// end of TEST_SETUP



TEST_TEAR_DOWN( vPortSysTickHandler )
{
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
} //// end of TEST_TEAR_DOWN





TEST( vPortSysTickHandler, func_chk )
{
    TEST_ASSERT_EQUAL_UINT32( expected_value->sysTickFlag, actual_value->sysTickFlag );
    // turn on SysTick, the sysTickFlag will be set later in the handler.
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk ;
    while (actual_value->sysTickFlag == 0);
    expected_value->sysTickFlag = 1;
    TEST_ASSERT_EQUAL_UINT32( expected_value->sysTickFlag, actual_value->sysTickFlag );
    TEST_ASSERT_EQUAL_UINT32( expected_value->pendSVflag , actual_value->pendSVflag );
    // turn off SysTick temporarily, then turn on again
    SysTick->CTRL  = 0;
    SysTick->VAL   = 0;
    actual_value->sysTickFlag = 0;
    actual_value->pendSVflag  = 0;
    xMockYieldTaskFlag = pdTRUE;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk ;

    while (actual_value->sysTickFlag == 0);
    expected_value->sysTickFlag = 1;
    expected_value->pendSVflag  = 1;
    TEST_ASSERT_EQUAL_UINT32( expected_value->sysTickFlag, actual_value->sysTickFlag );
    TEST_ASSERT_EQUAL_UINT32( expected_value->pendSVflag , actual_value->pendSVflag );
    // turn off SysTick
    SysTick->CTRL  = 0;
} //// end of test body




