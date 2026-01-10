#include "demo_apps/connect_ap_ping.h"

#define TASK_MIN_STACK_SIZE ((unsigned portSHORT)0x7e)

typedef struct {
    struct {
        espSysThread_t init;
        espSysThread_t ap_conn;
        espSysThread_t ping;
    } tsk;
    struct {
        espRes_t response;
        uint32_t resp_time;
    } ping;
    struct {
        espRes_t net_conn;
        espRes_t cmd_test;
    } device;
} ItestResultMonitor_t;

static void vESPtestConnAPtask(void *params) {
    ItestResultMonitor_t *monitor = params;
    monitor->device.cmd_test = espOK;
    for (;;) { // ESP device already reset once before starting this task
        const uint8_t waitUntilConnected = 0x0;
        if (monitor->device.cmd_test == espOK) {
            // Connect to access point.
            // Try unlimited time until access point accepts up.
            // Check for station_manager.c to define preferred access points ESP
            // should connect to
            monitor->device.net_conn = eESPtestConnAP(waitUntilConnected);
            vESPsysDelay(20000);
        }
        eESPcloseDevice(); // make device NOT present
        vESPsysDelay(4000);
        // set device to `present` state again
        monitor->device.cmd_test = eESPgetCurrATversion();
    }
}

static void vESPtestPingTask(void *params) {
    ItestResultMonitor_t *monitor = params;
#define PING_HOST "stackoverflow.com"
    for (;;) {
        monitor->ping.resp_time = 0;
        monitor->ping.response = eESPping(
            PING_HOST, sizeof(PING_HOST), &monitor->ping.resp_time, NULL, NULL, ESP_AT_CMD_BLOCKING
        );
        if (monitor->ping.response == espOK)
            ESP_ASSERT(monitor->ping.resp_time > 0);
        vESPsysDelay(1700);
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
            printf("Ping successful in %ld milliseconds!\r\n", *(evt->body.ping.resptime));
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
    ItestResultMonitor_t *monitor = params;
#define isPrivileged 0x1
    espRes_t response = eESPinit(vESPinitCallBack);
    if (response == espOK) {
        // create the 2 threads for this test
        response = eESPsysThreadCreate(
            &monitor->tsk.ping, "espTestPing", vESPtestPingTask, monitor,
            (0x20 + TASK_MIN_STACK_SIZE), ESP_APPS_THREAD_PRIO, isPrivileged
        );
        response = eESPsysThreadCreate(
            &monitor->tsk.ap_conn, "espTestConnAP", vESPtestConnAPtask, monitor,
            (0x20 + TASK_MIN_STACK_SIZE), (ESP_APPS_THREAD_PRIO + 1), isPrivileged
        );
        ESP_ASSERT(response == espOK);
        ESP_ASSERT(monitor->tsk.ping);
        ESP_ASSERT(monitor->tsk.ap_conn);
    } else {
        ESP_MEMFREE(monitor);
    }
    eESPsysThreadDelete(NULL);
#undef isPrivileged
} // end of vESPinitTask

void vCreateAllTestTasks(void) {
    ItestResultMonitor_t *monitor = ESP_MALLOC(sizeof(ItestResultMonitor_t));
    *monitor = (ItestResultMonitor_t){0};
    // the ESP initialization thread takes the smae priority as the 2 internal
    // threads working in ESP AT software.
    uint8_t  isPrivileged = 0x1;
    espRes_t response = eESPsysThreadCreate(
        &monitor->tsk.init, "espInitTask", vESPtestInitTask, monitor, TASK_MIN_STACK_SIZE,
        ESP_SYS_THREAD_PRIO, isPrivileged
    );
    ESP_ASSERT(response == espOK);
    ESP_ASSERT(monitor->tsk.init);
}
