#ifndef __ESP_CONFIG_H
#define __ESP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_CFG_DEV_ESP01 1
// in this project we apply ESP-01, FreeRTOS port for STM32 Cortex-M4, turn off AT echo
// function
#define ESP_CFG_SYS_PORT ESP_SYS_PORT_FREERTOS
#define ESP_CFG_PING     1

// specify hardware reset pin instead of running AT+RST command
#define ESP_CFG_RST_PIN

#ifdef __cplusplus
}
#endif
#endif // end of  __ESP_CONFIG_H
