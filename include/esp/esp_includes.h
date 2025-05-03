#ifndef __ESP_INCLUDES_H
#define __ESP_INCLUDES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_config.h"

// leave the rest unchanged
#include "esp/esp_config_default.h"
#include "esp/esp_typedef.h"
#include "esp/esp_debug.h"
#include "esp/esp_utils.h"

#include "system/esp_sys.h"

#include "esp/esp_netconn.h"

#if (ESP_CFG_MODE_STATION  != 0)
#include "esp/esp_sta.h"
#endif /* ESP_CFG_MODE_STATION */
#if (ESP_CFG_MODE_ACCESS_POINT  != 0)
#include "esp/esp_ap.h"
#endif /* ESP_CFG_MODE_ACCESS_POINT  */
#if (ESP_CFG_PING  != 0)
#include "esp/esp_ping.h"
#endif /* ESP_CFG_PING */
#if (ESP_CFG_WPS  != 0)
#include "esp/esp_wps.h"
#endif /* ESP_CFG_WPS */
#if (ESP_CFG_SNTP  != 0)
#include "esp/esp_sntp.h"
#endif /* ESP_CFG_SNTP */
#if (ESP_CFG_HOSTNAME  != 0)
#include "esp/esp_hostname.h"
#endif /* ESP_CFG_HOSTNAME */
#if (ESP_CFG_DNS  != 0)
#include "esp/esp_dns.h"
#endif /* ESP_CFG_DNS */


#ifdef __cplusplus
}
#endif
#endif // end of  __ESP_INCLUDES_H 

