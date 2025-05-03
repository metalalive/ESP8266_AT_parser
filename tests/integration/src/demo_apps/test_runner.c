#include "tests/integration/ESP_AT_parser/test_runner.h"


void vCreateAllTestTasks( void )
{
    #if defined(ESP_TEST_PING)
        vESPtestStartPingTask();
    #elif defined(ESP_TEST_HTTP_SERVER)
        vESPtestStartHttpServerTask();
    #elif defined(ESP_TEST_MQTT_CLIENT)
        vESPtestStartMqttClientTask();
    #endif
} // end of vCreateAllTestTasks



void vIntegrationTestTimerISR1(void)
{
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
    }
} // end of vIntegrationTestTimerISR1



void vIntegrationTestTimerISR2(void)
{
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
    }
} // end of vIntegrationTestTimerISR2



BaseType_t vIntegrationTestRTOSMemManageHandler(void)
{
    BaseType_t alreadyHandled = pdFALSE;
    if(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
    }
    return alreadyHandled;
} // end of vIntegrationTestRTOSMemManageHandler

