#ifndef __ESP_SYS_H
#define __ESP_SYS_H

// esp_sys provides functions from underlying operating system like thread,
// semaphore, mutex, delay function ... etc.
#ifdef __cplusplus
extern "C" {
#endif

// List of available system port for ESP AT-command library
#define ESP_SYS_PORT_FREERTOS 1
#define ESP_SYS_PORT_USER     99

// default system port implementation
#ifndef ESP_CFG_SYS_PORT
#define ESP_CFG_SYS_PORT ESP_SYS_PORT_USER
#endif // end of ESP_CFG_SYS_PORT

#if (ESP_CFG_SYS_PORT == ESP_SYS_PORT_FREERTOS)
#include "system/esp_system_freertos.h"
#elif (ESP_CFG_SYS_PORT == ESP_SYS_PORT_USER)
// system-specific types / functions declaration should be provided
// elsewhere in application program
#endif

typedef enum {
    ESP_SYS_TASK_SCHEDULER_NOT_STARTED, // scheduler hasn't been started, none
                                        // of the created threads has been
                                        // running
    ESP_SYS_TASK_SCHEDULER_RUNNING,
    ESP_SYS_TASK_SCHEDULER_SUSPENDED, // scheduler already started but suspended
                                      // by OS under some confitions.
} espTskSchrState_t;

// -------------------------------------------------------------------
// List of system-level APIs that should be implemented in each port.
// -------------------------------------------------------------------

// initialization code for underlying operating system
espRes_t eESPsysInit(void);
espRes_t eESPsysDeInit(void);

// In most case, ESP device works with UART interface and few GPIO pins.
// Application developer must implement following  4 functions for tests with
// target hardware platform.
// * --- initialization code for device driver.
espRes_t eESPlowLvlDevInit(void *params);
espRes_t eESPlowLvlDevDeInit(void *params);
// * --- low-level hardware-specific functions for resetting & sending AT
// command to ESP device
// TODO: recheck if the interface works well for other OS
espRes_t eESPlowLvlSendFn(void *data, size_t len, uint32_t timeout);
espRes_t eESPlowLvlRecvStartFn(void);
void     vESPlowLvlRecvStopFn(void);
espRes_t eESPlowLvlRstFn(uint8_t state);

// get current time, the format depends on system implementation, (it can be
// ticks, ms, us... etc)
uint32_t uESPsysCurrTime(void);

// the lock for atomic access (among multiple threads) happened to the core
// function of ESP AT library
espRes_t eESPsysProtect(void);
espRes_t eESPsysUnprotect(void);

// mutex, semaphore can be generated to synchronize the threads,
// which dedicate to AT-commands operations
espSysMtx_t xESPsysMtxCreate(void);
void        vESPsysMtxDelete(espSysMtx_t *mtx);
espRes_t    eESPsysMtxLock(espSysMtx_t *mtx);
espRes_t    eESPsysMtxUnlock(espSysMtx_t *mtx);

espSysSem_t xESPsysSemCreate(void);
void        vESPsysSemDelete(espSysSem_t sem);
espRes_t    eESPsysSemWait(espSysSem_t sem, uint32_t block_time);
espRes_t    eESPsysSemRelease(espSysSem_t sem);

// we define message structure to pass information of AT command,
// and received data as response of the AT commands between the threads
// work with them
espSysMbox_t xESPsysMboxCreate(size_t length);
void         vESPsysMboxDelete(espSysMbox_t *mb);
espRes_t     eESPsysMboxPut(espSysMbox_t mb, void *msg, uint32_t block_time);
espRes_t     eESPsysMboxGet(espSysMbox_t mb, void **msg, uint32_t block_time);
espRes_t     eESPsysMboxPutISR(espSysMbox_t mb, void *msg);

// thread creation, delete, yield function.
espRes_t eESPsysThreadCreate(
    espSysThread_t *t, const char *name, espSysThreFunc thread_fn, void *const arg,
    size_t stack_size, espSysThreadPrio_t prio, uint8_t isPrivileged
);

espRes_t eESPsysThreadDelete(espSysThread_t *t);

// for some system ports, users can check whether task/thread scheduler is
// running or not
espTskSchrState_t eESPsysGetTskSchedulerState(void);

// start / stop task scheduler
espRes_t eESPsysTskSchedulerStart(void);
espRes_t eESPsysTskSchedulerStop(void);

// thread yielding function, to give up CPU control & perform OS context switch
espRes_t eESPsysThreadYield();

// timing delay should be implemented by underlying operating system functions
void vESPsysDelay(const uint32_t ms);

#ifdef __cplusplus
}
#endif
#endif // end of  __ESP_SYS_H
