#ifndef __INTEGRATION_ESP_AT_SW_CONN_AP_PING_H
#define __INTEGRATION_ESP_AT_SW_CONN_AP_PING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"


// ----------- function declaration -----------

void  vESPtestStartPingTask( void );

espRes_t  eESPtestConnAP( uint8_t waitUntilConnected );



#ifdef __cplusplus
}
#endif
#endif // end of  __INTEGRATION_ESP_AT_SW_CONN_AP_PING_H

