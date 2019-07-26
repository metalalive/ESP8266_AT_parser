#include "esp/esp.h"
#include "tests/integration/ESP_AT_parser/mqtt/mqtt_include.h"


static mqttProp_t clientPropStack[MQTT_MAX_NUM_PROPS] = {0};


// initialize the  mqttConn_t  structure
int   mqttClientInit( mqttConn_t **mconn, int cmd_timeout_ms, word32 tx_buf_len, word32 rx_buf_len )
{
    // clear static data
    ESP_MEMSET( &clientPropStack, 0x00, sizeof(mqttProp_t) * MQTT_MAX_NUM_PROPS );   
    // create global structure mqttConn_t object
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
    // start decoding the received packet
    status = mqttDecodePkt( mconn, rx_buf, pkt_total_len, p_decode ); 
    return  MQTT_RETURN_SUCCESS;
} // end of mqttClientWaitPkt



mqttProp_t*  mqttPropertyCreate( mqttProp_t **head )
{ // TODO: mutex is required in multithreading case
    mqttProp_t*  curr_node = *head;
    mqttProp_t*  prev_node = NULL;
    uint8_t      idx = 0;
    while( curr_node != NULL ) {
        prev_node = curr_node;
        curr_node = curr_node->next; 
    }
    // pick up one available node 
    for(idx=0; idx<MQTT_MAX_NUM_PROPS ; idx++) {
        if(clientPropStack[idx].type == MQTT_PROP_NONE) {
            curr_node = &clientPropStack[idx] ;
            break;
        }
    }
    if(curr_node != NULL){
        if(prev_node == NULL){
            *head = curr_node;
        }
        else{
            prev_node->next = curr_node; 
        }
    }
    return  curr_node;
} // end of mqttPropertyCreate




void   mqttPropertyDel( mqttProp_t *head )
{ // TODO: mutex is required in multithreading case
    mqttProp_t*  curr_node = head;
    mqttProp_t*  next_node = NULL;
    while( curr_node != NULL ) {
        next_node = curr_node->next; 
        ESP_MEMSET( curr_node, 0x0, sizeof(mqttProp_t) );
        curr_node = next_node;
    }
} // end of mqttPropertyDel




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





