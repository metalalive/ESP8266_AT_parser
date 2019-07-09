// for C unit test framework Unity
#include "unity_fixture.h"
// in this test, we will put a function & few variables in privileged area
// by putting the macros PRIVILEGED_FUNCTION and PRIVILEGED_DATA ahead of
// the privileged function & data. 
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
// for FreeRTOS
#include "FreeRTOS.h"
#include "task.h"

#define  MOCK_STACK_DEPTH  0x30

static xMPU_SETTINGS  *expected_value = NULL;
static xMPU_SETTINGS  *actual_value   = NULL;
static UBaseType_t    *ulOptionalVariable[5] ;
static StackType_t    *ulMockStackMemory = NULL;

// To declare a new test group in Unity, firstly you use the macro below
TEST_GROUP( vPortStoreTaskMPUSettings );


TEST_SETUP( vPortStoreTaskMPUSettings )
{
    portSHORT idx = 0;
    expected_value = (xMPU_SETTINGS *) unity_malloc( sizeof(xMPU_SETTINGS) );
    actual_value   = (xMPU_SETTINGS *) unity_malloc( sizeof(xMPU_SETTINGS) );
    for(idx=0; idx<5; idx++)
    {
        ulOptionalVariable[idx]    = (UBaseType_t *) unity_malloc( sizeof(UBaseType_t) );
        *(ulOptionalVariable[idx]) = ((idx+1) << 24) | ((idx+1) << 16) | ((idx+1) << 8) | (idx+1);
    }
    ulMockStackMemory = (StackType_t *) unity_malloc( sizeof(StackType_t) * MOCK_STACK_DEPTH );
} //// end of TEST_SETUP



TEST_TEAR_DOWN( vPortStoreTaskMPUSettings )
{
    portSHORT idx = 0;
    // free memory allocated to check lists 
    unity_free( (void *)expected_value ); 
    unity_free( (void *)actual_value   ); 
    expected_value = NULL; 
    actual_value   = NULL; 
    for(idx=0; idx<5; idx++) {
        unity_free( (void *)ulOptionalVariable[idx] ); 
        ulOptionalVariable[idx] = NULL;
    }
    unity_free( (void *)ulMockStackMemory ); 
    ulMockStackMemory = NULL; 
} //// end of TEST_TEAR_DOWN



// user applications specify regions to task TCB
TEST( vPortStoreTaskMPUSettings , with_given_region )
{
    portSHORT       idx;
    UBaseType_t     ulRegionSizeInBytes ;
    MemoryRegion_t  xMemRegion[ portNUM_CONFIGURABLE_REGIONS + 1 ];

    xMemRegion[0].pvBaseAddress   = (void *)ulOptionalVariable[0];
    xMemRegion[0].ulLengthInBytes = (UBaseType_t) sizeof(UBaseType_t) ;
    xMemRegion[0].ulParameters    =  portMPU_REGION_PRIV_RW_URO | MPU_RASR_S_Msk ;
    xMemRegion[1].pvBaseAddress   = (void *)ulOptionalVariable[2] ;
    xMemRegion[1].ulLengthInBytes = (UBaseType_t) sizeof(UBaseType_t) ;
    xMemRegion[1].ulParameters    =  portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_C_Msk | MPU_RASR_B_Msk;
    xMemRegion[2].pvBaseAddress   = (void *)ulOptionalVariable[4] ;
    xMemRegion[2].ulLengthInBytes = (UBaseType_t) sizeof(UBaseType_t) ;
    xMemRegion[2].ulParameters    =  portMPU_REGION_READ_ONLY | MPU_RASR_S_Msk | MPU_RASR_B_Msk;
    // stack memory is supposed to be fully accessed by a task.
    ulRegionSizeInBytes = (UBaseType_t)sizeof(StackType_t) * MOCK_STACK_DEPTH;
    expected_value->xRegion[0].RBAR  = ((UBaseType_t)ulMockStackMemory & MPU_RBAR_ADDR_Msk) | 
                                       MPU_RBAR_VALID_Msk | portSTACK_REGION  ;
    expected_value->xRegion[0].RASR  = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk
                                      | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                      |  MPU_RASR_ENABLE_Msk  ;
    // xRegion[1] - xRegion[3] : optional variable used for this mock task
    ulRegionSizeInBytes = (UBaseType_t)sizeof(UBaseType_t) ;
    expected_value->xRegion[1].RBAR  = ((UBaseType_t)ulOptionalVariable[0] & MPU_RBAR_ADDR_Msk)
                                       | MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION    ;
    expected_value->xRegion[1].RASR  = portMPU_REGION_PRIV_RW_URO | MPU_RASR_S_Msk
                                      | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                      |  MPU_RASR_ENABLE_Msk  ;

    expected_value->xRegion[2].RBAR  = ((UBaseType_t)ulOptionalVariable[2] & MPU_RBAR_ADDR_Msk)
                                       | MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1);
    expected_value->xRegion[2].RASR  = portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_C_Msk | MPU_RASR_B_Msk
                                      | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                      |  MPU_RASR_ENABLE_Msk  ;

    expected_value->xRegion[3].RBAR  = ((UBaseType_t)ulOptionalVariable[4] & MPU_RBAR_ADDR_Msk)
                                       | MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 2);
    expected_value->xRegion[3].RASR  = portMPU_REGION_READ_ONLY | MPU_RASR_S_Msk | MPU_RASR_B_Msk
                                      | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                      |  MPU_RASR_ENABLE_Msk  ;

    vPortStoreTaskMPUSettings(actual_value, &xMemRegion, ulMockStackMemory, MOCK_STACK_DEPTH);
    for(idx=0; idx<(portNUM_CONFIGURABLE_REGIONS + 1) ; idx++) 
    {
        TEST_ASSERT_EQUAL_UINT32( expected_value->xRegion[idx].RBAR , actual_value->xRegion[idx].RBAR );
        TEST_ASSERT_EQUAL_UINT32( expected_value->xRegion[idx].RASR , actual_value->xRegion[idx].RASR );
    }

    // ------------- modify one of the MPU region's settings then verify again. -------------
    xMemRegion[0].pvBaseAddress   = (void *)ulOptionalVariable[3];
    xMemRegion[0].ulLengthInBytes = (UBaseType_t) sizeof(UBaseType_t) ;
    xMemRegion[0].ulParameters    =  portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk ;
    xMemRegion[1].pvBaseAddress   = (void *)NULL ;
    xMemRegion[1].ulLengthInBytes = 0x0 ;
    xMemRegion[1].ulParameters    = portMPU_REGION_READ_ONLY | MPU_RASR_C_Msk | MPU_RASR_B_Msk;
    ulRegionSizeInBytes = (UBaseType_t)sizeof(UBaseType_t) ;
    expected_value->xRegion[1].RBAR  = ((UBaseType_t)ulOptionalVariable[3] & MPU_RBAR_ADDR_Msk)
                                       | MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION    ;
    expected_value->xRegion[1].RASR  = portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk
                                      | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                      |  MPU_RASR_ENABLE_Msk  ;

    expected_value->xRegion[2].RBAR  = MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1);
    expected_value->xRegion[2].RASR  = 0x0 ;

    vPortStoreTaskMPUSettings(actual_value, &xMemRegion, ulMockStackMemory, MOCK_STACK_DEPTH);

    for(idx=0; idx<(portNUM_CONFIGURABLE_REGIONS + 1) ; idx++) 
    {
        TEST_ASSERT_EQUAL_UINT32( expected_value->xRegion[idx].RBAR , actual_value->xRegion[idx].RBAR );
        TEST_ASSERT_EQUAL_UINT32( expected_value->xRegion[idx].RASR , actual_value->xRegion[idx].RASR );
    }
} //// end of test body






TEST( vPortStoreTaskMPUSettings , without_given_region )
{
    portSHORT  idx;
    UBaseType_t     ulRegionSizeInBytes ;
    extern  UBaseType_t  __SRAM_segment_start__[] ;
    extern  UBaseType_t  __SRAM_segment_end__  [] ;
    extern  UBaseType_t  __privileged_data_start__[];
    extern  UBaseType_t  __privileged_data_end__[];

    ulRegionSizeInBytes = (UBaseType_t)__SRAM_segment_end__    - (UBaseType_t)__SRAM_segment_start__ ;
    expected_value->xRegion[0].RBAR = ((UBaseType_t)__SRAM_segment_start__ & MPU_RBAR_ADDR_Msk)
                                    | MPU_RBAR_VALID_Msk | portSTACK_REGION;
    expected_value->xRegion[0].RASR = portMPU_REGION_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk
                                    | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                    |  MPU_RASR_ENABLE_Msk ;
    ulRegionSizeInBytes = (UBaseType_t)__privileged_data_end__ - (UBaseType_t)__privileged_data_start__ ;
    expected_value->xRegion[1].RBAR = ((UBaseType_t)__privileged_data_start__ & MPU_RBAR_ADDR_Msk)
                                     | MPU_RBAR_VALID_Msk | portFIRST_CONFIGURABLE_REGION;
    expected_value->xRegion[1].RASR = portMPU_REGION_PRIVILEGED_READ_WRITE | MPU_RASR_S_Msk | MPU_RASR_C_Msk | MPU_RASR_B_Msk
                                    | (MPU_RASR_SIZE_Msk & (prvMPUregionSizeEncode(ulRegionSizeInBytes) << MPU_RASR_SIZE_Pos))
                                    |  MPU_RASR_ENABLE_Msk ;

    expected_value->xRegion[2].RBAR = MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 1);
    expected_value->xRegion[2].RASR = 0x0;
    expected_value->xRegion[3].RBAR = MPU_RBAR_VALID_Msk | (portFIRST_CONFIGURABLE_REGION + 2);
    expected_value->xRegion[3].RASR = 0x0;

    vPortStoreTaskMPUSettings(actual_value, NULL, NULL, 0);

    for(idx=0; idx<(portNUM_CONFIGURABLE_REGIONS + 1) ; idx++) 
    {
        TEST_ASSERT_EQUAL_UINT32( expected_value->xRegion[idx].RBAR , actual_value->xRegion[idx].RBAR );
        TEST_ASSERT_EQUAL_UINT32( expected_value->xRegion[idx].RASR , actual_value->xRegion[idx].RASR );
    }
} //// end of test body



