// for C unit test framework Unity
#include "unity_fixture.h"
// in this test, we will put a function & few variables in privileged area
// by putting the macros PRIVILEGED_FUNCTION and PRIVILEGED_DATA ahead of
// the privileged function & data. 
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
// for FreeRTOS
#include "FreeRTOS.h"

static BaseType_t  *expected_value = NULL;
static BaseType_t  *actual_value = NULL;



void TEST_HELPER_xPortRaisePrivilege_SVCentry( void )
{
    if( expected_value != NULL ) {
        __asm volatile(
            "pop  {lr}            \n"
            "b    vPortSVCHandler \n"
        );
    }
} //// end of TEST_HELPER_xPortRaisePrivilege_SVCentry


// To declare a new test group in Unity, firstly you use the macro below
TEST_GROUP( xPortRaisePrivilege );



TEST_SETUP( xPortRaisePrivilege )
{
    expected_value = (BaseType_t *) unity_malloc( sizeof(BaseType_t) );
    actual_value   = (BaseType_t *) unity_malloc( sizeof(BaseType_t) );
    __asm volatile(
        "cpsie i  \n" 
        "cpsie f  \n" 
        "dsb      \n" 
        "isb      \n"
        :::"memory"
    );
} // end of TEST_SETUP


TEST_TEAR_DOWN( xPortRaisePrivilege )
{
    // free memory allocated to check lists 
    unity_free( (void *)expected_value ); 
    unity_free( (void *)actual_value   ); 
    expected_value = NULL; 
    actual_value   = NULL; 
    __asm volatile(
        "cpsid i  \n" 
        "cpsid f  \n" 
        "dsb      \n" 
        "isb      \n"
        :::"memory"
    );
} // end of TEST_TEAR_DOWN


TEST( xPortRaisePrivilege , regs_chk )
{
    BaseType_t  xState;
    // switch to unprivileged state
    __asm volatile (
        "mrs  r8 , control  \n"
        "orr  r8 , r8, #0x1 \n"
        "msr  control, r8   \n"
        "dsb  \n"
        "isb  \n"
    );
    *actual_value = 0x1;
    // switch back to privileged state by calling "svn 2" (clear CONTROL.nPRIV)
    xState = xPortRaisePrivilege();
    // check whether we are in privileged mode.
    __asm volatile (
        "mrs  %0 , control  \n"
        :"=r"(*actual_value)::
    );
    *expected_value = 0x0;
    *actual_value = (*actual_value) & CONTROL_nPRIV_Msk ;
    TEST_ASSERT_EQUAL_INT32( xState , pdTRUE );
    TEST_ASSERT_EQUAL_INT32( (*expected_value), (*actual_value) );
    // raise privilege again in privilege mode, this time it will not affect anything.
    xState = xPortRaisePrivilege();
    TEST_ASSERT_EQUAL_INT32( xState , pdFALSE );
} // end of test body


