#include "demo_apps/connect_ap_ping.h"

#define TASK_MIN_STACK_SIZE ((unsigned portSHORT)0x7e)

static espSysThread_t itest_tsk_init = NULL;
static espSysThread_t itest_tsk_ap_conn = NULL;
static espSysThread_t itest_tsk_ping = NULL;

static void vESPtestConnAPtask(void *params) {
    espRes_t      response = espOK;
    const uint8_t waitUntilConnected = 0x0;
    for (;;) { // ESP device already reset once before starting this task
        if (response == espOK) {
            // Connect to access point.
            // Try unlimited time until access point accepts up.
            // Check for station_manager.c to define preferred access points ESP
            // should connect to
            eESPtestConnAP(waitUntilConnected);
            vESPsysDelay(20000);
        } // Set device is NOT present
        eESPcloseDevice();
        vESPsysDelay(5000);
        // reset device for next iteration of connection test
        response = eESPresetWithDelay(1, NULL, NULL);
    } // end of outer infinite loop
}

static void vESPtestPingTask(void *params) {
#define PING_HOST "stackoverflow.com"
    espRes_t response = espOK;
    uint32_t pingresptime = 0;
    for (;;) {
        pingresptime = 0;
        response =
            eESPping(PING_HOST, sizeof(PING_HOST), &pingresptime, NULL, NULL, ESP_AT_CMD_BLOCKING);
        (void)response;
        vESPsysDelay(1500);
    }
#undef PING_HOST
}

static espRes_t vESPinitCallBack(espEvt_t *evt) {
    espRes_t response = espOK;
    switch (evt->type) {
    case ESP_EVT_INIT_FINISH:
        printf("[INFO] vESPinitCallBack, library initialized ... OK! \r\n");
        break;

    case ESP_EVT_RESET_DETECTED:
        printf("[INFO] vESPinitCallBack, Device reset detected \r\n");
        break;

    case ESP_EVT_RESET:
        if (evt->body.reset.res == espOK) {
            printf("[INFO] vESPinitCallBack, ESP reset sequence finished with "
                   "success! \r\n");
        } else {
            printf("[INFO] vESPinitCallBack, ESP reset sequence error\r\n");
        }
        break;

    case ESP_EVT_WIFI_CONNECTED:
        printf("[INFO] vESPinitCallBack, Wifi connected to access point!\r\n");
        break;

    case ESP_EVT_WIFI_DISCONNECTED:
        printf("[INFO] vESPinitCallBack, Wifi disconnected from access "
               "point!\r\n");
        break;
    case ESP_EVT_PING:
        printf("[INFO] ping response time : \r\n");
        response = evt->body.ping.res;
        if (response == espOK) {
            printf("Ping successful in %d milliseconds!\r\n", *(evt->body.ping.resptime));
        } else {
            printf("Ping error with res: %d\r\n", (int)response);
        }
        break;

    default:
        break;
    } // end of switch statement
    return response;
} // end of vESPinitCallBack

static void vESPtestInitTask(void *params) {
    uint8_t  isPrivileged = 0x1;
    espRes_t response = eESPinit(vESPinitCallBack);
    if (response == espOK) {
        // create the 2 threads for this test
        response = eESPsysThreadCreate(
            &itest_tsk_ping, "espTestPing", vESPtestPingTask, NULL, (0x20 + TASK_MIN_STACK_SIZE),
            ESP_APPS_THREAD_PRIO, isPrivileged
        );
        response = eESPsysThreadCreate(
            &itest_tsk_ap_conn, "espTestConnAP", vESPtestConnAPtask, NULL,
            (0x20 + TASK_MIN_STACK_SIZE), (ESP_APPS_THREAD_PRIO + 1), isPrivileged
        );
        ESP_ASSERT(response == espOK);
        ESP_ASSERT(itest_tsk_ap_conn);
        ESP_ASSERT(itest_tsk_ping);
    }
    eESPsysThreadDelete(NULL);
} // end of vESPinitTask

void vCreateAllTestTasks(void) {
    uint8_t isPrivileged = 0x1;
    // the ESP initialization thread takes the smae priority as the 2 internal
    // threads working in ESP AT software.
    espRes_t response = eESPsysThreadCreate(
        &itest_tsk_init, "espInitTask", vESPtestInitTask, NULL, TASK_MIN_STACK_SIZE,
        ESP_SYS_THREAD_PRIO, isPrivileged
    );
    ESP_ASSERT(response == espOK);
    ESP_ASSERT(itest_tsk_init);
}
