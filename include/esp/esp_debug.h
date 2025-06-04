#ifndef __ESP_DEBUG_H
#define __ESP_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "string.h"

// List of debug levels
#define ESP_DBG_LVL_ALL     0x00 /*!< Print all messages of all types */
#define ESP_DBG_LVL_WARNING 0x01 /*!< Print warning and upper messages */
#define ESP_DBG_LVL_DANGER  0x02 /*!< Print danger errors */
#define ESP_DBG_LVL_SEVERE  0x03 /*!< Print severe problems affecting program flow */
#define ESP_DBG_LVL_MASK    0x03 /*!< Mask for getting debug level */

// List of debug types
#define ESP_DBG_TYPE_TRACE 0x40 /*!< Debug trace messages for program flow */
#define ESP_DBG_TYPE_STATE 0x20 /*!< Debug state messages (such as state machines) */
#define ESP_DBG_TYPE_ALL   (ESP_DBG_TYPE_TRACE | ESP_DBG_TYPE_STATE) /*!< All debug types */

#ifdef __cplusplus
}
#endif
#endif // __ESP_DEBUG_H
