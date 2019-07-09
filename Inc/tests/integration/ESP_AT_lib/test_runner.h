#ifndef __INTEGRATION_ESP_AT_LIB_TEST_RUNNER_H
#define __INTEGRATION_ESP_AT_LIB_TEST_RUNNER_H

#ifdef __cplusplus
extern "C" {
#endif

//// #define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
//// #include "FreeRTOS.h"
//// #include "task.h"
//// #undef  MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "tests/integration/ESP_AT_lib/connect_ap_ping.h"

// uncomment following  headers when we start implementing them
//// #include "tests/integration/ESP_AT_lib/http_server.h"
//// #include "tests/integration/ESP_AT_lib/mqtt_client.h"
//// #include "tests/integration/ESP_AT_lib/mqtt_tls_client.h"


// ----------- function declaration -----------
void vCreateAllTestTasks( void );

void vIntegrationTestTimerISR1(void);

void vIntegrationTestTimerISR2(void);

BaseType_t vIntegrationTestRTOSMemManageHandler(void);





#ifdef __cplusplus
}
#endif
#endif // end of  __INTEGRATION_ESP_AT_LIB_TEST_RUNNER_H

