#ifndef __INTEGRATION_ESP_AT_LIB_TEST_RUNNER_H
#define __INTEGRATION_ESP_AT_LIB_TEST_RUNNER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"



// ----------- function declaration -----------
void vCreateAllTestTasks( void );

void vIntegrationTestTimerISR1(void);

void vIntegrationTestTimerISR2(void);

BaseType_t vIntegrationTestRTOSMemManageHandler(void);

#if defined(ESP_TEST_PING)
void    vESPtestStartPingTask(void);
#elif defined(ESP_TEST_HTTP_SERVER)
void    vESPtestStartHttpServerTask(void);
#elif defined(ESP_TEST_MQTT_CLIENT)
void    vESPtestStartMqttClientTask(void);
#endif




#ifdef __cplusplus
}
#endif
#endif // end of  __INTEGRATION_ESP_AT_LIB_TEST_RUNNER_H

