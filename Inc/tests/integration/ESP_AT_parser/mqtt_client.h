#ifndef __INTEGRATION_ESP_AT_SW_MQTT_CLIENT_H
#define __INTEGRATION_ESP_AT_SW_MQTT_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"
#include "tests/integration/ESP_AT_parser/mqtt/mqtt_include.h"


// ----------- function declaration -----------

void  vESPtestStartMqttClientTask( void );

// referenced by common.c
espRes_t  eESPtestConnAP( uint8_t waitUntilConnected );


#ifdef __cplusplus
}
#endif
#endif // end of   __INTEGRATION_ESP_AT_SW_MQTT_CLIENT_H

