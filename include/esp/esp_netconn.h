#ifndef __ESP_NETCONN_H
#define __ESP_NETCONN_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// forward declaration, this will be declared internally in esp_conn.c file
struct espNetConn;

typedef struct espNetConn *espNetConnPtr;

// Netconn connection type
typedef enum {
    ESP_NETCONN_TYPE_TCP = ESP_CONN_TYPE_TCP, /*!< TCP connection */
    ESP_NETCONN_TYPE_SSL = ESP_CONN_TYPE_SSL, /*!< SSL connection */
    ESP_NETCONN_TYPE_UDP = ESP_CONN_TYPE_UDP, /*!< UDP connection */
} espNetConnType_t;

// create TCP-layer connection objects & acting as a server or client
espNetConnPtr pxESPnetconnCreate(espConn_t *conn_in);

espRes_t eESPnetconnDelete(espNetConnPtr nc);

espConn_t *pxESPgetConnHandleObj(espNetConnPtr nc);

#ifdef __cplusplus
}
#endif
#endif /* __ESP_NETCONN_H */
