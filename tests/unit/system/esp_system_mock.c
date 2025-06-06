#include "esp/esp.h"
#include <stddef.h> // For size_t

// Dummy implementation for eESPsysInit
espRes_t eESPsysInit(void) { return espOK; }

// Dummy implementation for eESPsysDeInit
espRes_t eESPsysDeInit(void) { return espOK; }

// Dummy implementation for eESPlowLvlDevInit
espRes_t eESPlowLvlDevInit(void *params) {
    (void)params; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for eESPlowLvlDevDeInit
espRes_t eESPlowLvlDevDeInit(void *params) {
    (void)params; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for eESPlowLvlSendFn
espRes_t eESPlowLvlSendFn(void *data, size_t len, uint32_t timeout) {
    (void)data;    // Suppress unused parameter warning
    (void)len;     // Suppress unused parameter warning
    (void)timeout; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for eESPlowLvlRecvStartFn
espRes_t eESPlowLvlRecvStartFn(void) { return espOK; }

// Dummy implementation for vESPlowLvlRecvStopFn
void vESPlowLvlRecvStopFn(void) {
    // No operation
}

// Dummy implementation for eESPlowLvlRstFn
espRes_t eESPlowLvlRstFn(uint8_t state) {
    (void)state; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for uESPsysCurrTime
uint32_t uESPsysCurrTime(void) {
    return 0; // Return a dummy time
}

// Dummy implementation for eESPsysProtect
espRes_t eESPsysProtect(void) { return espOK; }

// Dummy implementation for eESPsysUnprotect
espRes_t eESPsysUnprotect(void) { return espOK; }

// Dummy implementation for xESPsysMtxCreate
espSysMtx_t xESPsysMtxCreate(void) {
    return NULL; // Return a dummy mutex handle
}

// Dummy implementation for vESPsysMtxDelete
void vESPsysMtxDelete(espSysMtx_t *mtx) {
    (void)mtx; // Suppress unused parameter warning
    // No operation
}

// Dummy implementation for eESPsysMtxLock
espRes_t eESPsysMtxLock(espSysMtx_t *mtx) {
    (void)mtx; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for eESPsysMtxUnlock
espRes_t eESPsysMtxUnlock(espSysMtx_t *mtx) {
    (void)mtx; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for xESPsysSemCreate
espSysSem_t xESPsysSemCreate(void) {
    return NULL; // Return a dummy semaphore handle
}

// Dummy implementation for vESPsysSemDelete
void vESPsysSemDelete(espSysSem_t sem) {
    (void)sem; // Suppress unused parameter warning
    // No operation
}

// Dummy implementation for eESPsysSemWait
espRes_t eESPsysSemWait(espSysSem_t sem, uint32_t block_time) {
    (void)sem;        // Suppress unused parameter warning
    (void)block_time; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for eESPsysSemRelease
espRes_t eESPsysSemRelease(espSysSem_t sem) {
    (void)sem; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for xESPsysMboxCreate
espSysMbox_t xESPsysMboxCreate(size_t length) {
    (void)length; // Suppress unused parameter warning
    return NULL;  // Return a dummy mailbox handle
}

// Dummy implementation for vESPsysMboxDelete
void vESPsysMboxDelete(espSysMbox_t *mb) {
    (void)mb; // Suppress unused parameter warning
    // No operation
}

// Dummy implementation for eESPsysMboxPut
espRes_t eESPsysMboxPut(espSysMbox_t mb, void *msg, uint32_t block_time) {
    (void)mb;         // Suppress unused parameter warning
    (void)msg;        // Suppress unused parameter warning
    (void)block_time; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for eESPsysMboxGet
espRes_t eESPsysMboxGet(espSysMbox_t mb, void **msg, uint32_t block_time) {
    (void)mb;         // Suppress unused parameter warning
    (void)msg;        // Suppress unused parameter warning
    (void)block_time; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for eESPsysMboxPutISR
espRes_t eESPsysMboxPutISR(espSysMbox_t mb, void *msg) {
    (void)mb;  // Suppress unused parameter warning
    (void)msg; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for eESPsysThreadCreate
espRes_t eESPsysThreadCreate(
    espSysThread_t *t, const char *name, espSysThreFunc thread_fn, void *const arg,
    size_t stack_size, espSysThreadPrio_t prio, uint8_t isPrivileged
) {
    (void)t;            // Suppress unused parameter warning
    (void)name;         // Suppress unused parameter warning
    (void)thread_fn;    // Suppress unused parameter warning
    (void)arg;          // Suppress unused parameter warning
    (void)stack_size;   // Suppress unused parameter warning
    (void)prio;         // Suppress unused parameter warning
    (void)isPrivileged; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for eESPsysThreadDelete
espRes_t eESPsysThreadDelete(espSysThread_t *t) {
    (void)t; // Suppress unused parameter warning
    return espOK;
}

// Dummy implementation for eESPsysGetTskSchedulerState
espTskSchrState_t eESPsysGetTskSchedulerState(void) {
    return ESP_SYS_TASK_SCHEDULER_NOT_STARTED; // Return a dummy state
}

// Dummy implementation for eESPsysTskSchedulerStart
espRes_t eESPsysTskSchedulerStart(void) { return espOK; }

// Dummy implementation for eESPsysTskSchedulerStop
espRes_t eESPsysTskSchedulerStop(void) { return espOK; }

// Dummy implementation for eESPsysThreadYield
espRes_t eESPsysThreadYield(void) { return espOK; }

// Dummy implementation for vESPsysDelay
void vESPsysDelay(const uint32_t ms) {
    (void)ms; // Suppress unused parameter warning
    // No operation
}
