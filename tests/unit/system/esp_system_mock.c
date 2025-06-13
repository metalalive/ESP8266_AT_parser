#include "system/esp_system_mock.h"
#include "esp/esp.h"
#include "system/esp_sys.h"
#include <stddef.h> // For size_t

// --- Global Mock Control Variables ---
// These variables are declared in the header and defined here.
ut_sys_mock_returns_t ut_sys_mock_returns = {
    .eESPsysInit_ret = espOK,
    .eESPsysDeInit_ret = espOK,
    .eESPlowLvlDevInit_ret = espOK,
    .eESPlowLvlDevDeInit_ret = espOK,
    .eESPlowLvlSendFn_ret = espOK,
    .eESPlowLvlRecvStartFn_ret = espOK,
    .eESPlowLvlRstFn_ret = espOK,
    .uESPsysCurrTime_ret = 0,
    .eESPsysProtect_ret = espOK,
    .eESPsysUnprotect_ret = espOK,
    .xESPsysMtxCreate_ret = NULL,
    .eESPsysMtxLock_ret = espOK,
    .eESPsysMtxUnlock_ret = espOK,
    .xESPsysSemCreate_ret = NULL,
    .eESPsysSemWait_ret = espOK,
    .eESPsysSemRelease_ret = espOK,
    .xESPsysMboxCreate_ret = NULL,
    .eESPsysMboxPut_ret = espOK,
    .eESPsysMboxGet_ret = espOK,
    .eESPsysMboxGet_msg = NULL,
    .eESPsysMboxPutISR_ret = espOK,
    .eESPsysThreadCreate_ret = espOK,
    .eESPsysThreadDelete_ret = espOK,
    .eESPsysGetTskSchedulerState_ret = ESP_SYS_TASK_SCHEDULER_NOT_STARTED,
    .eESPsysTskSchedulerStart_ret = espOK,
    .eESPsysTskSchedulerStop_ret = espOK,
    .eESPsysThreadYield_ret = espOK,
};

// --- Mock Implementations ---

espRes_t eESPsysInit(void) { return ut_sys_mock_returns.eESPsysInit_ret; }

espRes_t eESPsysDeInit(void) { return ut_sys_mock_returns.eESPsysDeInit_ret; }

espRes_t eESPlowLvlDevInit(void *params) {
    (void)params;
    return ut_sys_mock_returns.eESPlowLvlDevInit_ret;
}

espRes_t eESPlowLvlDevDeInit(void *params) {
    (void)params;
    return ut_sys_mock_returns.eESPlowLvlDevDeInit_ret;
}

espRes_t eESPlowLvlSendFn(void *data, size_t len, uint32_t timeout) {
    (void)data;
    (void)len;
    (void)timeout;
    return ut_sys_mock_returns.eESPlowLvlSendFn_ret;
}

espRes_t eESPlowLvlRecvStartFn(void) { return ut_sys_mock_returns.eESPlowLvlRecvStartFn_ret; }

void vESPlowLvlRecvStopFn(void) {}

espRes_t eESPlowLvlRstFn(uint8_t state) {
    (void)state;
    return ut_sys_mock_returns.eESPlowLvlRstFn_ret;
}

uint32_t uESPsysCurrTime(void) { return ut_sys_mock_returns.uESPsysCurrTime_ret; }

espRes_t eESPsysProtect(void) { return ut_sys_mock_returns.eESPsysProtect_ret; }

espRes_t eESPsysUnprotect(void) { return ut_sys_mock_returns.eESPsysUnprotect_ret; }

espSysMtx_t xESPsysMtxCreate(void) { return ut_sys_mock_returns.xESPsysMtxCreate_ret; }

void vESPsysMtxDelete(espSysMtx_t *mtx) { (void)mtx; }

espRes_t eESPsysMtxLock(espSysMtx_t *mtx) {
    (void)mtx;
    return ut_sys_mock_returns.eESPsysMtxLock_ret;
}

espRes_t eESPsysMtxUnlock(espSysMtx_t *mtx) {
    (void)mtx;
    return ut_sys_mock_returns.eESPsysMtxUnlock_ret;
}

espSysSem_t xESPsysSemCreate(void) { return ut_sys_mock_returns.xESPsysSemCreate_ret; }

void vESPsysSemDelete(espSysSem_t sem) { (void)sem; }

espRes_t eESPsysSemWait(espSysSem_t sem, uint32_t block_time) {
    (void)sem;
    (void)block_time;
    return ut_sys_mock_returns.eESPsysSemWait_ret;
}

espRes_t eESPsysSemRelease(espSysSem_t sem) {
    (void)sem;
    return ut_sys_mock_returns.eESPsysSemRelease_ret;
}

espSysMbox_t xESPsysMboxCreate(size_t length) {
    (void)length;
    return ut_sys_mock_returns.xESPsysMboxCreate_ret;
}

void vESPsysMboxDelete(espSysMbox_t *mb) { (void)mb; }

espRes_t eESPsysMboxPut(espSysMbox_t mb, void *msg, uint32_t block_time) {
    (void)mb;
    (void)msg;
    (void)block_time;
    return ut_sys_mock_returns.eESPsysMboxPut_ret;
}

espRes_t eESPsysMboxGet(espSysMbox_t mb, void **msg, uint32_t block_time) {
    (void)mb;
    (void)block_time;
    *msg = ut_sys_mock_returns.eESPsysMboxGet_msg;
    return ut_sys_mock_returns.eESPsysMboxGet_ret;
}

espRes_t eESPsysMboxPutISR(espSysMbox_t mb, void *msg) {
    (void)mb;
    (void)msg;
    return ut_sys_mock_returns.eESPsysMboxPutISR_ret;
}

espRes_t eESPsysThreadCreate(
    espSysThread_t *t, const char *name, espSysThreFunc thread_fn, void *const arg,
    size_t stack_size, espSysThreadPrio_t prio, uint8_t isPrivileged
) {
    (void)t;
    (void)name;
    (void)thread_fn;
    (void)arg;
    (void)stack_size;
    (void)prio;
    (void)isPrivileged;
    return ut_sys_mock_returns.eESPsysThreadCreate_ret;
}

espRes_t eESPsysThreadDelete(espSysThread_t *t) {
    (void)t;
    return ut_sys_mock_returns.eESPsysThreadDelete_ret;
}

espTskSchrState_t eESPsysGetTskSchedulerState(void) {
    return ut_sys_mock_returns.eESPsysGetTskSchedulerState_ret;
}

espRes_t eESPsysTskSchedulerStart(void) { return ut_sys_mock_returns.eESPsysTskSchedulerStart_ret; }

espRes_t eESPsysTskSchedulerStop(void) { return ut_sys_mock_returns.eESPsysTskSchedulerStop_ret; }

espRes_t eESPsysThreadYield(void) { return ut_sys_mock_returns.eESPsysThreadYield_ret; }

void vESPsysDelay(const uint32_t ms) { (void)ms; }
