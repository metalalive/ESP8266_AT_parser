#ifndef __INTEGRATION_ESP_AT_SW_HTTP_SERVER_H
#define __INTEGRATION_ESP_AT_SW_HTTP_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"

// ----------- function declaration -----------

void vESPtestStartHttpServerTask(void);

espRes_t eESPtestConnAP(uint8_t waitUntilConnected);

#ifdef __cplusplus
}
#endif
#endif // end of   __INTEGRATION_ESP_AT_SW_HTTP_SERVER_H
