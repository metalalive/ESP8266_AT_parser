// for C unit test framework Unity
#include "unity_fixture.h"
// for FreeRTOS
#include "FreeRTOS.h"


static volatile portSHORT  PendsvVisitFlag ;

// SVC exception handling routine will invoke this function
void  TEST_HELPER_setPendsvVisitFlag()
{
    PendsvVisitFlag = 0x1;
}


// To declare a new test group in Unity, firstly you use the macro below
TEST_GROUP( vPortYield );


// initial setup before running the test body
TEST_SETUP( vPortYield )
{
    PendsvVisitFlag  = 0x0;
    NVIC_SetPriority( PendSV_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY );
    __asm volatile( "cpsie i" );
    __asm volatile( "cpsie f" );
    __asm volatile( "dsb" ::: "memory" );
    __asm volatile( "isb" );
}


// port-processing code after test body is executed
TEST_TEAR_DOWN( vPortYield )
{
    PendsvVisitFlag  = 0x0;
    __asm volatile( "dsb" ::: "memory" );
    __asm volatile( "isb" );
    __asm volatile( "cpsid i" );
    __asm volatile( "cpsid f" );
    NVIC_SetPriority( PendSV_IRQn, 0 );
}


TEST( vPortYield, gen_pendsv_excpt )
{
    vPortYield();
    TEST_ASSERT_EQUAL_UINT16( 0x1, PendsvVisitFlag);
}


