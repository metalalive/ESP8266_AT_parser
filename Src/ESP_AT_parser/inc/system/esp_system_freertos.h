#ifndef __ESP_SYSTEM_FREERTOS_H
#define __ESP_SYSTEM_FREERTOS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "stdint.h"
#include "stdlib.h"

#include "esp_config.h"
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#undef  MPU_WRAPPERS_INCLUDED_FROM_API_FILE


typedef SemaphoreHandle_t           espSysMtx_t;
typedef SemaphoreHandle_t           espSysSem_t;
typedef QueueHandle_t               espSysMbox_t;
typedef TaskHandle_t                espSysThread_t;
typedef UBaseType_t                 espSysThreadPrio_t;

#define ESP_SYS_MAX_TIMEOUT         ((uint32_t)portMAX_DELAY - 1)
// for application threads, the priority starts from the lowest level
#define ESP_APPS_THREAD_PRIO        (tskIDLE_PRIORITY)
// for system threads used in core function of ESP AT library, the priority
// starts above the applcation thread's priority.
#define ESP_SYS_THREAD_PRIO         (tskIDLE_PRIORITY + 2)
#define ESP_SYS_THREAD_STACK_SIZE   (0x7e)

#define ESP_ASSERT( cond )   configASSERT( cond )


#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /*  __ESP_SYSTEM_FREERTOS_H */
