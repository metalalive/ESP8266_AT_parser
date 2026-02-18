#include "demo_apps/deepsleep.h"

#define TASK_MIN_STACK_SIZE ((unsigned portSHORT)0x7e)

typedef struct {
    struct {
        espSysThread_t init;
        espSysThread_t pwrcycle;
    } tsk;
    struct {
        espRes_t pwr_off;
        espRes_t pwr_on;
        espRes_t rst_basic_cmds;
        espRes_t ap_conn;
        espRes_t ap_disconn;
    } resp;
} ItestResultMonitor_t;

static espRes_t vESPpowercycleCallBack(espEvt_t *evt) {
    switch (evt->type) {
    case ESP_EVT_RESET:
        ESP_ASSERT(evt->body.reset.res == espOK);
        break;
    case ESP_EVT_INIT_FINISH:
    case ESP_EVT_RESET_DETECTED:
    case ESP_EVT_DEVICE_PRESENT:
    case ESP_EVT_STA_LIST_AP:
    case ESP_EVT_STA_JOIN_AP:
    case ESP_EVT_STA_INFO_AP:
    case ESP_EVT_WIFI_CONNECTED:
    case ESP_EVT_WIFI_GOT_IP:
    case ESP_EVT_WIFI_DISCONNECTED:
    case ESP_EVT_WIFI_IP_ACQUIRED:
        break;
    default:
        ESP_ASSERT(0);
    }
    return espOK;
}

static void vESPtestPowerCycleTask(void *params) {
    ItestResultMonitor_t *monitor = params;
    for (;;) {
        monitor->resp.pwr_on = eESPpowerOn(5);
        if (monitor->resp.pwr_on != espOK)
            continue;
        ESP_ASSERT(eESPgetPowerStatus() == ESP_ENABLE);
        monitor->resp.rst_basic_cmds = eESPresetWithDelay(1, NULL, NULL);
        if (monitor->resp.rst_basic_cmds != espOK)
            continue;
        monitor->resp.ap_conn = eESPtestConnAP(0x0);
        if (monitor->resp.ap_conn == espOK) {
            vESPsysDelay(20000);
        }
        monitor->resp.ap_disconn = eESPcloseDevice(); // quit from AP
        monitor->resp.pwr_off = eESPpowerOff();
        ESP_ASSERT(monitor->resp.pwr_off == espOK);
        ESP_ASSERT(eESPgetPowerStatus() == ESP_DISABLE);
        vESPsysDelay(12000); // Wait for reset and re-initialization
    }
}

static void vESPtestInitTask(void *params) {
    ItestResultMonitor_t *monitor = params;
#define isPrivileged 0x1
    espRes_t response = eESPinit(vESPpowercycleCallBack);
    if (response == espOK) {
        response = eESPsysThreadCreate(
            &monitor->tsk.pwrcycle, "PwrCycleJoin", vESPtestPowerCycleTask, monitor,
            (0x20 + TASK_MIN_STACK_SIZE), ESP_APPS_THREAD_PRIO, isPrivileged
        );
        ESP_ASSERT(monitor->tsk.pwrcycle);
    } else {
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
        &monitor->tsk.init, "PwrCycleInit", vESPtestInitTask, monitor, TASK_MIN_STACK_SIZE,
        ESP_SYS_THREAD_PRIO, isPrivileged
    );
    ESP_ASSERT(response == espOK);
    ESP_ASSERT(monitor->tsk.init);
}
