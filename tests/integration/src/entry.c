#include "entry.h"

void hw_layer_init(void);

static BaseType_t vIntegrationTestRTOSMemManageHandler(void) {
    BaseType_t alreadyHandled = pdFALSE;
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
    }
    return alreadyHandled;
}

void vIntegrationTestMemManageHandler(void) {
    BaseType_t alreadyHandled = vIntegrationTestRTOSMemManageHandler();
    // clang-format off
    if (alreadyHandled != pdTRUE) {
        for (;;);
    }
    // clang-format on
}

static void TestEnd(void) {
    // clang-format off
    while (1);
    // clang-format on
}

int main(void) {
    hw_layer_init();
    vCreateAllTestTasks();
    eESPsysTskSchedulerStart();
    TestEnd();
}
