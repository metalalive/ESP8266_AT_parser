#ifndef __ESP_CONFIG_H
#define __ESP_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// in this project we apply ESP-01, FreeRTOS port for STM32 Cortex-M4, turn off AT echo function
#define  ESP_CFG_ESP8266 
#define  ESP_CFG_SYS_PORT   ESP_SYS_PORT_FREERTOS
#define  ESP_CFG_PING       1
#define  ESP_CFG_NETCONN    1

// TODO: hardware reset pin is very stable on STM32F4xx board
#define  ESP_CFG_RST_PIN  

// enable global debug features, print character to terminal, and assertion checking functions.
#define ESP_CFG_DBG           1
#define ESP_CFG_DBG_ASSERT    1
#define ESP_CFG_DBG_NETCONN   1 


#ifdef __cplusplus
}
#endif
#endif // end of  __ESP_CONFIG_H 

