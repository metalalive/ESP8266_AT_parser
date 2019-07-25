#include "esp/esp.h"
#include "tests/integration/ESP_AT_parser/mqtt/mqtt_include.h"


// initialize the  mqttConn_t  structure
int   mqttClientInit( mqttConn_t **mconn, int cmd_timeout_ms, word32 tx_buf_len, word32 rx_buf_len )
{
    mqttConn_t *c = NULL;
    c  =  ESP_MALLOC( sizeof(mqttConn_t) );
    if( c == NULL ){
        return MQTT_RETURN_ERROR_MEMORY;
    }
    ESP_MEMSET( c, 0x00, sizeof(mqttConn_t) );
    c->tx_buf = ESP_MALLOC( sizeof(byte) * tx_buf_len );
    c->tx_buf_len = tx_buf_len;
    if( c->tx_buf == NULL ) {
        return MQTT_RETURN_ERROR_MEMORY;
    }
    c->rx_buf = ESP_MALLOC( sizeof(byte) * rx_buf_len );
    c->rx_buf_len = rx_buf_len;
    if( c->rx_buf == NULL ) {
        return MQTT_RETURN_ERROR_MEMORY;
    }
    c->cmd_timeout_ms  = cmd_timeout_ms;
    c->keep_alive_sec  = MQTT_DEFAULT_KEEPALIVE_SEC;
    c->protocol_lvl    = MQTT_CONN_PROTOCOL_LEVEL; 
    c->max_qos         = MQTT_QOS_2;
    c->retain_avail    = 1; 
    *mconn  =  c;
    // TODO: create semaphores from packet send/receive operations in multithreading case.
    return  MQTT_RETURN_SUCCESS;
} // end of mqttClientInit



int  mqttClientDeinit( mqttConn_t *c )
{
    if( c == NULL ){ 
        return MQTT_RETURN_ERROR_BAD_ARG;
    }
    ESP_MEMFREE( c->tx_buf );
    c->tx_buf = NULL;
    ESP_MEMFREE( c->rx_buf );
    c->rx_buf = NULL;
    ESP_MEMFREE( c );
    return  MQTT_RETURN_SUCCESS;
} // end of  mqttClientDeinit




int  mqttClientWaitPkt( mqttConn_t *mconn, mqttCtrlPktType cmdtype, void* p_decode )
{
    int         status ;
    byte       *rx_buf;
    word32      rx_buf_len;
    word32      pkt_total_len = 0;

    rx_buf     = mconn->rx_buf;
    rx_buf_len = mconn->rx_buf_len;
    // wait until we receive incoming packet.
    status = mqttPktRead( mconn, rx_buf, rx_buf_len, &pkt_total_len );
    if( status != MQTT_RETURN_SUCCESS ) {
        return status;
    }
    // decode the received packet

    return  MQTT_RETURN_SUCCESS;
} // end of mqttClientWaitPkt





int   mqttSendConnect( mqttConn_t *mconn )
{
    byte       *tx_buf;
    word32      tx_buf_len;
    word32      pkt_total_len;
    int         status ;
    if( mconn == NULL ){ 
        return MQTT_RETURN_ERROR_BAD_ARG;
    }
    tx_buf     = mconn->tx_buf;
    tx_buf_len = mconn->tx_buf_len;
    pkt_total_len  =  mqttEncodePktConnect( tx_buf, tx_buf_len, mconn );
    if(pkt_total_len <= 0) {
        return  MQTT_RETURN_ERROR_MALFORMED_DATA;
    }
    else if(pkt_total_len > tx_buf_len) {
        return  MQTT_RETURN_ERROR_OUT_OF_BUFFER;
    }
    status = mqttPktWrite( mconn, tx_buf, pkt_total_len );
    // the return value must be equal to length of Tx buffer, otherwise something
    // in the underlying system must be wrong.
    if( status != MQTT_RETURN_SUCCESS ) {
        return status;
    }
    status = mqttClientWaitPkt( mconn, MQTT_PACKET_TYPE_CONNACK, (void *)&mconn->recv_connack );
    return  status;
} // end of mqttSendConnect





