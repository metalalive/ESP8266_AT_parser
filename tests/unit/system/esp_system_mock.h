#ifndef ESP_SYSTEM_MOCK_H
#define ESP_SYSTEM_MOCK_H

#include "esp/esp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    espRes_t          eESPsysInit_ret;
    espRes_t          eESPsysDeInit_ret;
    espRes_t          eESPlowLvlDevInit_ret;
    espRes_t          eESPlowLvlDevDeInit_ret;
    espRes_t          eESPlowLvlSendFn_ret;
    espRes_t          eESPlowLvlRecvStartFn_ret;
    espRes_t          eESPlowLvlRstFn_ret;
    uint32_t          uESPsysCurrTime_ret;
    espRes_t          eESPsysProtect_ret;
    espRes_t          eESPsysUnprotect_ret;
    espSysMtx_t       xESPsysMtxCreate_ret;
    espRes_t          eESPsysMtxLock_ret;
    espRes_t          eESPsysMtxUnlock_ret;
    espSysSem_t       xESPsysSemCreate_ret;
    espRes_t          eESPsysSemRelease_ret;
    espSysMbox_t      xESPsysMboxCreate_ret;
    espRes_t          eESPsysMboxPut_ret;
    espRes_t          eESPsysMboxGet_ret;
    void             *eESPsysMboxGet_msg;
    espRes_t          eESPsysMboxPutISR_ret;
    espRes_t          eESPsysThreadCreate_ret;
    espRes_t          eESPsysThreadDelete_ret;
    espTskSchrState_t eESPsysGetTskSchedulerState_ret;
    espRes_t          eESPsysTskSchedulerStart_ret;
    espRes_t          eESPsysTskSchedulerStop_ret;
    espRes_t          eESPsysSemWait_ret;
    espRes_t          eESPsysThreadYield_ret;
} ut_sys_mock_returns_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* ESP_SYSTEM_MOCK_H */
