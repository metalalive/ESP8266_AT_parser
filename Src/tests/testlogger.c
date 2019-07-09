#include <stdlib.h>
#include "tests/testlogger.h"

static volatile TestLoogerList_t  *testloggerlist;



void vInitTestLogger(void)
{
    testloggerlist = (TestLoogerList_t *) malloc(sizeof(TestLoogerList_t));
    testloggerlist->head = NULL;
    testloggerlist->tail = NULL;
    testloggerlist->uNumTestCases = 0x0;
} // end of vInitTestLogger




TestLogger_t*  xRegisterNewTestLogger( const char *filepath, 
                                       const char *description )
{   
    TestLogger_t  *logitem = NULL;
    if( testloggerlist->uNumTestCases < MAX_NUM_TEST_LOGGER_ITEMS ) 
    {
        logitem = (TestLogger_t *) malloc(sizeof(TestLogger_t));
        logitem->next = NULL;
        logitem->filepath = filepath;
        logitem->lineNumber = 0;
        logitem->description = description;
        logitem->compare_condition = TESTCMP_UNKNOWN;
        logitem->expectedValue[0] = NULL;
        logitem->expectedValue[1] = NULL;
        logitem->actualValue      = NULL;
        logitem->failFlag         = 0x0;
        if(testloggerlist->head == NULL) {
            testloggerlist->head = logitem;
        }
        if(testloggerlist->tail != NULL) {
            testloggerlist->tail->next = logitem;
        }
        testloggerlist->tail = logitem;
        testloggerlist->uNumTestCases++;
    }
    return  logitem;
} // end of xRegisterNewTestLogger




void vLogTestMismatchGeneric( TestLogger_t *logitem, unsigned int linenum, 
                       TESTLOGGER_COMPARISON_T compare_option, 
                       void *expected0, void *expected1, void *actual ) 
{
    if(logitem == NULL) return;
    logitem->lineNumber        = linenum;
    logitem->compare_condition = compare_option;
    logitem->expectedValue[0] = expected0;
    logitem->expectedValue[1] = expected1; 
    logitem->actualValue      = actual ;
    logitem->failFlag         = 0x1;
} // end of vLogTestMismatchIntGeneric



