#include "esp/esp.h"
#include "tests/integration/ESP_AT_parser/mqtt/mqtt_include.h"


static mqttProp_t clientPropStack[MQTT_MAX_NUM_PROPS] = {0};


// initialize the  mqttCtx_t  structure
int   mqttClientInit( mqttCtx_t **mctx, int cmd_timeout_ms, word32 tx_buf_len, word32 rx_buf_len )
{
    // clear static data
    ESP_MEMSET( &clientPropStack, 0x00, sizeof(mqttProp_t) * MQTT_MAX_NUM_PROPS );   
    // create global structure mqttCtx_t object
    mqttCtx_t *c = NULL;
    c  =  ESP_MALLOC( sizeof(mqttCtx_t) );
    if( c == NULL ){
        return MQTT_RETURN_ERROR_MEMORY;
    }
    ESP_MEMSET( c, 0x00, sizeof(mqttCtx_t) );
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
    c->max_qos         = MQTT_QOS_2;
    c->retain_avail    = 1; 
    *mctx  =  c;
    // TODO: create semaphores from packet send/receive operations in multithreading case.
    return  MQTT_RETURN_SUCCESS;
} // end of mqttClientInit



int  mqttClientDeinit( mqttCtx_t *c )
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




int  mqttClientWaitPkt( mqttCtx_t *mctx, mqttCtrlPktType cmdtype, void* p_decode )
{
    int         status ;
    byte       *rx_buf;
    word32      rx_buf_len;
    word32      pkt_total_len = 0;

    rx_buf     = mctx->rx_buf;
    rx_buf_len = mctx->rx_buf_len;
    // wait until we receive incoming packet.
    status = mqttPktRead( mctx, rx_buf, rx_buf_len, &pkt_total_len );
    if( status != MQTT_RETURN_SUCCESS ) {
        return status;
    }
    // start decoding the received packet
    status = mqttDecodePkt( mctx, rx_buf, pkt_total_len, p_decode ); 
    return  status;
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




int   mqttSendConnect( mqttCtx_t *mctx )
{
    byte       *tx_buf;
    word32      tx_buf_len;
    word32      pkt_total_len;
    int         status ;
    if( mctx == NULL ){ 
        return MQTT_RETURN_ERROR_BAD_ARG;
    }
    tx_buf     = mctx->tx_buf;
    tx_buf_len = mctx->tx_buf_len;
    pkt_total_len  =  mqttEncodePktConnect( tx_buf, tx_buf_len, (mqttConn_t *)&mctx->pkt.conn );
    if(pkt_total_len <= 0) {
        return  MQTT_RETURN_ERROR_MALFORMED_DATA;
    }
    else if(pkt_total_len > tx_buf_len) {
        return  MQTT_RETURN_ERROR_OUT_OF_BUFFER;
    }
    status = mqttPktWrite( mctx, tx_buf, pkt_total_len );
    // the return value must be equal to length of Tx buffer, otherwise something
    // in the underlying system must be wrong.
    if( status != MQTT_RETURN_SUCCESS ) {
        return status;
    }
    status = mqttClientWaitPkt( mctx, MQTT_PACKET_TYPE_CONNACK, (void *)&mctx->pkt.recv_connack );
    return  status;
} // end of mqttSendConnect




int   mqttSendDisconnect( mqttCtx_t *mctx )
{
    int         status ;
    byte       *tx_buf;
    word32      tx_buf_len;
    word32      pkt_total_len;
    if( mctx == NULL ){ 
        return MQTT_RETURN_ERROR_BAD_ARG;
    }
    tx_buf     = mctx->tx_buf;
    tx_buf_len = mctx->tx_buf_len;
    pkt_total_len  =  mqttEncodePktDisconn( tx_buf, tx_buf_len, (void *)&mctx->pkt.disconn );
    if(pkt_total_len <= 0) {
        return  MQTT_RETURN_ERROR_MALFORMED_DATA;
    }
    status = mqttPktWrite( mctx, tx_buf, pkt_total_len );
    return  status;
} // end of  mqttSendDisconnect






