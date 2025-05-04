#ifndef __INTEGRATION_ESP_AT_SW_MQTT_CLIENT_H
#define __INTEGRATION_ESP_AT_SW_MQTT_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"
#include "demo_apps/mqtt/mqtt_include.h"

void  vESPtestStartMqttClientTask( void );

// referenced by common.c
espRes_t  eESPtestConnAP( uint8_t waitUntilConnected );

#ifdef __cplusplus
}
#endif
#endif // end of   __INTEGRATION_ESP_AT_SW_MQTT_CLIENT_H

