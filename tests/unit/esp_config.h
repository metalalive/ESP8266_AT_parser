#ifndef __ESP_CONFIG_H
#define __ESP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_CFG_DEV_ESP12 1
#define ESP_CFG_SYS_PORT  ESP_SYS_PORT_USER
#define ESP_CFG_PING      1

// specify hardware reset pin instead of running AT+RST command
#define ESP_CFG_RST_PIN

// mock system-specific struct type
#include <assert.h>
#include <stdint.h>

typedef struct ut_mock_sys_s {
    uint32_t dummy;
} ut_mock_sys_t;

typedef ut_mock_sys_t *espSysMtx_t;
typedef ut_mock_sys_t *espSysSem_t;
typedef ut_mock_sys_t *espSysMbox_t;
typedef ut_mock_sys_t *espSysThread_t;
typedef uint32_t       espSysThreadPrio_t;

#define ESP_SYS_MAX_TIMEOUT  ((uint32_t)0xfffff)
#define ESP_APPS_THREAD_PRIO (1)
#define ESP_SYS_THREAD_PRIO  (3)

#define ESP_SYS_THREAD_STACK_SIZE (0xbe)
#define ESP_SYS_TICK_RATE_HZ      1234

#define ESP_ASSERT(cond) assert(cond)

#ifdef __cplusplus
}
#endif
#endif // end of  __ESP_CONFIG_H
