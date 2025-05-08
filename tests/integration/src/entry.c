#include "entry.h"

void hw_layer_init(void);

static BaseType_t vIntegrationTestRTOSMemManageHandler(void) {
    BaseType_t alreadyHandled = pdFALSE;
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {}
    return alreadyHandled;
}

void vIntegrationTestMemManageHandler(void) {
    BaseType_t alreadyHandled = pdFALSE;
    alreadyHandled = vIntegrationTestRTOSMemManageHandler();
    if(alreadyHandled != pdTRUE) {
        for(;;);
    }
} // end of vIntegrationTestMemManageHandler

static void TestEnd(void) {
    while(1);
}

int main(void) {
    hw_layer_init();
    vCreateAllTestTasks();
    eESPsysTskSchedulerStart();
    TestEnd();
}
