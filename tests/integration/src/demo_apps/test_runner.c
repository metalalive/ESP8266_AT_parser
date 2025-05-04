#include "demo_apps/test_runner.h"

void vIntegrationTestTimerISR1(void) {
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {}
}

void vIntegrationTestTimerISR2(void) {
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {}
}

BaseType_t vIntegrationTestRTOSMemManageHandler(void)
{
    BaseType_t alreadyHandled = pdFALSE;
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {}
    return alreadyHandled;
} // end of vIntegrationTestRTOSMemManageHandler

