// for C unit test framework Unity
#include "unity_fixture.h"
// in this test, we will put a function & few variables in privileged area
// by putting the macros PRIVILEGED_FUNCTION and PRIVILEGED_DATA ahead of
// the privileged function & data. 
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
// for FreeRTOS
#include "FreeRTOS.h"

// [TODO]
// find more efficient way to test "svc 0", start running first task
typedef struct {
    unsigned portSHORT privYieldFlag;
    unsigned portSHORT raiseToPrivFlag;
} flagSet_chk_t;

// collecting actual data from registers & check at the end of this test
static flagSet_chk_t  *expected_value = NULL;
static flagSet_chk_t  *actual_value = NULL;
// from port.c
extern PRIVILEGED_FUNCTION void prvSetupMPU( void );


void TEST_HELPER_vPortSVCHandler_SVCentry( uint8_t ucSVCnumber )
{
    if( expected_value != NULL ) {
        if( ucSVCnumber==portSVC_ID_RAISE_PRIVILEGE ) {
            actual_value->raiseToPrivFlag  = 1;
        }
        __asm volatile (
            "pop  {lr}              \n"
            "b    vPortSVCHandler    \n"
        );
    }
} //// end of TEST_HELPER_xPortStartScheduler_SVC0entry



void TEST_HELPER_vPortSVCHandler_PendSVentry( void )
{
    if( expected_value != NULL ) {
        actual_value->privYieldFlag = 1;
    }
} //// end of TEST_HELPER_vPortSVCHandler_PendSVentry



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
TEST_GROUP( vPortSVCHandler );



TEST_SETUP( vPortSVCHandler )
{
    // setup extra MPU regions used in this test
    vModifyMPUregionsForTest( );
    expected_value = (flagSet_chk_t *) unity_malloc( sizeof(flagSet_chk_t) );
    actual_value   = (flagSet_chk_t *) unity_malloc( sizeof(flagSet_chk_t) );
    expected_value->privYieldFlag    = 1;
    actual_value->privYieldFlag      = 0;
    expected_value->raiseToPrivFlag  = 1;
    actual_value->raiseToPrivFlag    = 0;
    __asm volatile(
        "cpsie i  \n" 
        "cpsie f  \n" 
        "dsb      \n" 
        "isb      \n"
        :::"memory"
    );
} // end of TEST_SETUP





TEST_TEAR_DOWN( vPortSVCHandler )
{
    // clear MPU regions used in this test
    vResetMPUregionsForTest( );
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
} // end of vPortSVCHandler





TEST( vPortSVCHandler , svc_chk )
{
    UBaseType_t  curr_control = 0;
    // switch to unprivileged state
    __asm volatile (
        "mrs  r8 , control  \n"
        "orr  r8 , r8, #0x1 \n"
        "msr  control, r8   \n"
        "dsb  \n"
        "isb  \n"
    );
    // call task yielding function placed in privileged code section.
    portYIELD();
    TEST_ASSERT_EQUAL_UINT16( expected_value->privYieldFlag, actual_value->privYieldFlag );
    // switch back to privileged state by calling "svn 2" (clear CONTROL.nPRIV)
    portRAISE_PRIVILEGE();
    // check whether we are in privileged mode.
    __asm volatile (
        "mrs  %0 , control  \n"
        :"=r"(curr_control)::
    );
    TEST_ASSERT_EQUAL_UINT32( (curr_control & CONTROL_nPRIV_Msk), 0x0 );
    TEST_ASSERT_EQUAL_UINT16( expected_value->raiseToPrivFlag, actual_value->raiseToPrivFlag );
} // end of test body



