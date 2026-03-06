#include "demo_apps/deepsleep.h"

#define TASK_MIN_STACK_SIZE ((unsigned portSHORT)0x7e)

typedef struct {
    struct {
        espSysThread_t init;
        espSysThread_t deepsleep;
    } tsk;
    struct {
        espRes_t dsleep;
        espRes_t rst_basic_cmds;
        espRes_t ap_conn;
    } resp;
} ItestResultMonitor_t;

static espRes_t vESPdeepsleepCallBack(espEvt_t *evt) {
    espRes_t response = espOK;
    switch (evt->type) {
    case ESP_EVT_INIT_FINISH:
        printf("[INFO] vESPdeepsleepCallBack, library initialized ... OK! \r\n");
        break;

    case ESP_EVT_RESET_DETECTED:
        printf("[INFO] vESPdeepsleepCallBack, Device reset detected \r\n");
        break;

    case ESP_EVT_RESET:
        if (evt->body.reset.res == espOK) {
            printf("[INFO] vESPdeepsleepCallBack, ESP reset sequence finished with "
                   "success! \r\n");
        } else {
            printf("[INFO] vESPdeepsleepCallBack, ESP reset sequence error\r\n");
        }
        break;

    default:
        break;
    }
    return response;
}

static void vESPtestSleepResumeTask(void *params) {
    ItestResultMonitor_t *monitor = params;
#define IT_SLEEP_MS 22000
    for (;;) {
        vESPsysDelay(3000);
        monitor->resp.dsleep = eESPenterDeepSleep(0, NULL, NULL, ESP_AT_CMD_BLOCKING);
        if (monitor->resp.dsleep == espOK) {
            vESPsysDelay(IT_SLEEP_MS); // Wait for reset and re-initialization
        } else { // retry if command send fails
            continue;
        }
        monitor->resp.rst_basic_cmds = eESPresetWithDelay(2, NULL, NULL);
        if (monitor->resp.rst_basic_cmds != espOK)
            continue;
        monitor->resp.ap_conn = eESPtestConnAP(0x0);
        vESPsysDelay(10000);
    }
#undef IT_SLEEP_MS
}

static void vESPtestInitTask(void *params) {
    ItestResultMonitor_t *monitor = params;
#define isPrivileged 0x1
    espRes_t response = eESPinit(vESPdeepsleepCallBack);
    if (response == espOK) {
        response = eESPsysThreadCreate(
            &monitor->tsk.deepsleep, "SleepThenConn", vESPtestSleepResumeTask, monitor,
            (0x20 + TASK_MIN_STACK_SIZE), ESP_APPS_THREAD_PRIO, isPrivileged
        );
        ESP_ASSERT(response == espOK);
        ESP_ASSERT(monitor->tsk.deepsleep);
    } else {
        printf("[ERROR] ESP initialization failed with error: %d\r\n", (int)response);
        ESP_MEMFREE(monitor);
    }
    eESPsysThreadDelete(NULL);
#undef isPrivileged
}

void vCreateAllTestTasks(void) {
    ItestResultMonitor_t *monitor = ESP_MALLOC(sizeof(ItestResultMonitor_t));
    *monitor = (ItestResultMonitor_t){0};
    uint8_t  isPrivileged = 0x1;
    espRes_t response = eESPsysThreadCreate(
        &monitor->tsk.init, "DSleepInit", vESPtestInitTask, monitor, TASK_MIN_STACK_SIZE,
        ESP_SYS_THREAD_PRIO, isPrivileged
    );
    ESP_ASSERT(response == espOK);
    ESP_ASSERT(monitor->tsk.init);
}
