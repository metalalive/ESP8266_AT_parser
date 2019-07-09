#ifndef __TESTLOGGER_H
#define __TESTLOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

// -------------------------- defined macros --------------------------
#define MAX_NUM_TEST_LOGGER_ITEMS     100



// ------------------------- defined data types -----------------------------
typedef enum
{
    TESTCMP_WITHIN           = 0x0,
    TESTCMP_EQUAL_TO         = 0x1,
    TESTCMP_GREATER_THAN     = 0x2,
    TESTCMP_GREATER_OR_EQUAL = 0x2 + TESTCMP_EQUAL_TO ,
    TESTCMP_SMALLER_THAN     = 0x4,
    TESTCMP_SMALLER_OR_EQUAL = 0x4 + TESTCMP_EQUAL_TO ,
    TESTCMP_NOT_EQUAL_TO     = TESTCMP_GREATER_THAN | TESTCMP_SMALLER_THAN,
    TESTCMP_COND_FALSE       = 0x8,
    TESTCMP_COND_TRUE        = 0x9,
    TESTCMP_UNKNOWN           
} TESTLOGGER_COMPARISON_T;



// --------------------------------------------------------------------------
// extensive data structure for logging result/mismatches of each test case 
// , e.g.: expected value, actual value, pass/failure
// --------------------------------------------------------------------------
typedef struct TESTLOGGER_T{
    // "0" as initial state, assert to "1" when a test case failed 
    unsigned short        failFlag;
    struct TESTLOGGER_T  *next ;
    const          char  *filepath;
    unsigned       int    lineNumber;
    const          char  *description;
    // the comparison can be equal-to, greater-than, less-than ...... etc.
    TESTLOGGER_COMPARISON_T    compare_condition;
    // since the data type of the expected / actual value may vary,
    // we dynamically allocate memory space to the expected / actual value
    // pointed by following structure members.
    //
    // Note that expectedValue is an array of void pointer to data of different
    // type & size, the 2nd. item of expectedValue will be used when
    // we check whether the actualValue is in the range :
    // (*expectedValue[0], *expectedValue[1])
    // , otherwise we always use expectedValue[0]
    void                      *expectedValue[2];
    void                      *actualValue;
} TestLogger_t;



typedef struct {
    TestLogger_t  *head;
    TestLogger_t  *tail;
    unsigned int   uNumTestCases;
} TestLoogerList_t;


// ------------ declared functions --------------------
void vInitTestLogger(void);

TestLogger_t* xRegisterNewTestLogger( const char *filepath, const char *description );

void vLogTestMismatchGeneric( TestLogger_t* logitem, unsigned int linenum, 
                       TESTLOGGER_COMPARISON_T compare_option, 
                       void *expected0, void *expected1, void *actual );


#ifdef __cplusplus
}
#endif
#endif // end of  __TESTLOGGER_H
