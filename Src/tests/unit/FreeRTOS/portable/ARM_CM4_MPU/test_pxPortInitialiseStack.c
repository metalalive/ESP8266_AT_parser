// for C unit test framework Unity
#include "unity_fixture.h"
// for FreeRTOS
#include "FreeRTOS.h"


// ----- macro for internal use -----
#define TEST_STACK_SIZE  0x20

static volatile StackType_t  test_stack_space[ TEST_STACK_SIZE ];

static void test_function(void* params){ for(;;){} }

// To declare a new test group in Unity, firstly you use the macro below
TEST_GROUP( pxPortInitialiseStack );


// initial setup before running the test body
TEST_SETUP( pxPortInitialiseStack )
{
    uint32_t idx = 0;
    for(idx=0; idx<TEST_STACK_SIZE; idx++ )
    {
        test_stack_space[idx] = (StackType_t) 0;
    }
}


// port-processing code after test body is executed
TEST_TEAR_DOWN( pxPortInitialiseStack )
{
}


// The test cases below  are the test body for the target function
TEST( pxPortInitialiseStack, initedStack_privileged )
{
    StackType_t *stack_pointer = (StackType_t *)&( test_stack_space[TEST_STACK_SIZE - 1] );
    uint16_t     params = 0x123;
    BaseType_t   isTaskPrivileged = pdTRUE;
    StackType_t *expected_stack_pointer = 0;
    StackType_t  expected_value         = 0;

    stack_pointer = pxPortInitialiseStack(stack_pointer, test_function, (void *)&params, isTaskPrivileged );

    // check content of the mock stack frame
    expected_value  = 0 | xPSR_T_Msk;
    TEST_ASSERT_EQUAL_UINT32( expected_value, test_stack_space[TEST_STACK_SIZE - 1 - 1] );
    expected_value  = ((StackType_t)&test_function)  &  portCPU_ADDRESS_MASK;
    TEST_ASSERT_EQUAL_UINT32( expected_value, test_stack_space[TEST_STACK_SIZE - 2 - 1] );
    expected_value  = 0 ;
    TEST_ASSERT_EQUAL_UINT32( expected_value, test_stack_space[TEST_STACK_SIZE - 3 - 1] );
    expected_value  = (StackType_t)&params ;
    TEST_ASSERT_EQUAL_UINT32( expected_value, test_stack_space[TEST_STACK_SIZE - 8 - 1] );
    expected_value  = portEXPTN_RETURN_TO_TASK;
    TEST_ASSERT_EQUAL_UINT32( expected_value, test_stack_space[TEST_STACK_SIZE - 9 - 1] );
    expected_value  = CONTROL_SPSEL_Msk;
    TEST_ASSERT_EQUAL_UINT32( expected_value, test_stack_space[TEST_STACK_SIZE - 18 - 1] );

    // finally check the actual stack pointer after target function
    expected_stack_pointer = (StackType_t *)&test_stack_space[TEST_STACK_SIZE - 18 - 1];
    TEST_ASSERT_EQUAL_PTR( expected_stack_pointer, stack_pointer );
}  //// end of TEST( pxPortInitialiseStack, initedStack_privileged )




TEST( pxPortInitialiseStack, initedStack_unprivileged )
{
    StackType_t *stack_pointer = (StackType_t *)&( test_stack_space[TEST_STACK_SIZE - 1] );
    uint16_t     params = 0x654;
    BaseType_t   isTaskPrivileged = pdFALSE;
    StackType_t *expected_stack_pointer = 0;
    StackType_t  expected_value         = 0;

    stack_pointer = pxPortInitialiseStack(stack_pointer, test_function, (void *)&params, isTaskPrivileged );

    // check content of the mock stack frame
    expected_value  = 0 | xPSR_T_Msk;
    TEST_ASSERT_EQUAL_UINT32( expected_value, test_stack_space[TEST_STACK_SIZE - 1 - 1] );
    expected_value  = ((StackType_t)&test_function)  &  portCPU_ADDRESS_MASK;
    TEST_ASSERT_EQUAL_UINT32( expected_value, test_stack_space[TEST_STACK_SIZE - 2 - 1] );
    expected_value  = 0 ;
    TEST_ASSERT_EQUAL_UINT32( expected_value, test_stack_space[TEST_STACK_SIZE - 3 - 1] );
    expected_value  = (StackType_t)&params ;
    TEST_ASSERT_EQUAL_UINT32( expected_value, test_stack_space[TEST_STACK_SIZE - 8 - 1] );
    expected_value  = portEXPTN_RETURN_TO_TASK;
    TEST_ASSERT_EQUAL_UINT32( expected_value, test_stack_space[TEST_STACK_SIZE - 9 - 1] );
    expected_value  = CONTROL_SPSEL_Msk | CONTROL_nPRIV_Msk ;
    TEST_ASSERT_EQUAL_UINT32( expected_value, test_stack_space[TEST_STACK_SIZE - 18 - 1] );

    // finally check the actual stack pointer after target function
    expected_stack_pointer = (StackType_t *)&test_stack_space[TEST_STACK_SIZE - 18 - 1];
    TEST_ASSERT_EQUAL_PTR( expected_stack_pointer, stack_pointer );
} //// end of TEST( pxPortInitialiseStack, initedStack_unprivileged )


