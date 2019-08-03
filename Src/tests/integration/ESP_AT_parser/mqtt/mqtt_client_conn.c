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




int  mqttClientWaitPkt( mqttCtx_t *mctx, mqttCtrlPktType wait_cmdtype, word16 wait_packet_id, void* p_decode )
{
    int         status ;
    byte       *rx_buf;
    word32      rx_buf_len;
    word32      pkt_total_len = 0;

    mqttPktFxHead_t  *recv_header ;
    mqttCtrlPktType   recv_cmdtype ;
    word16            recv_pkt_id = 0;

    rx_buf     = mctx->rx_buf;
    rx_buf_len = mctx->rx_buf_len;
    while(1) {
        // wait until we receive incoming packet.
        status = mqttPktRead( mctx, rx_buf, rx_buf_len, &pkt_total_len );
        if( status != MQTT_RETURN_SUCCESS ) {
            break; 
        }
        // start decoding the received packet
        recv_pkt_id = 0;
        status = mqttDecodePkt( mctx, rx_buf, pkt_total_len, p_decode, &recv_pkt_id );
        // check whether the received packet is what we're waiting for. 
        recv_header  = (mqttPktFxHead_t *)rx_buf;
        recv_cmdtype = MQTT_CTRL_PKT_TYPE_GET(recv_header->type_flgs);
        if(wait_cmdtype == recv_cmdtype) {
            if(wait_packet_id==0) { break; }
            if(wait_packet_id==recv_pkt_id){ break; }
        } 
    } // end of loop
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
    pkt_total_len  =  mqttEncodePktConnect( tx_buf, tx_buf_len, (mqttConn_t *)&mctx->send_pkt.conn );
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
    status = mqttClientWaitPkt( mctx, MQTT_PACKET_TYPE_CONNACK, 0, 
                                (void *)&mctx->recv_pkt.connack );
    return  status;
} // end of mqttSendConnect




int   mqttSendDisconnect( mqttCtx_t *mctx )
{
    int      status ;
    byte    *tx_buf;
    word32   tx_buf_len;
    word32   pkt_total_len;
    if( mctx == NULL ){ 
        return MQTT_RETURN_ERROR_BAD_ARG;
    }
    tx_buf     = mctx->tx_buf;
    tx_buf_len = mctx->tx_buf_len;
    pkt_total_len  =  mqttEncodePktDisconn( tx_buf, tx_buf_len, (mqttPktDisconn_t *)&mctx->send_pkt.disconn );
    if(pkt_total_len <= 0) {
        return  MQTT_RETURN_ERROR_MALFORMED_DATA;
    }
    return  mqttPktWrite( mctx, tx_buf, pkt_total_len );
} // end of  mqttSendDisconnect




int   mqttSendPublish( mqttCtx_t *mctx )
{
    int          status;
    byte        *tx_buf;
    word32       tx_buf_len;
    word32       pkt_total_len;
    mqttMsg_t   *msg = NULL;
    mqttQoS      qos;
    mqttCtrlPktType   wait_cmdtype;

    if( mctx == NULL ){ 
        return MQTT_RETURN_ERROR_BAD_ARG;
    }
    qos = mctx->send_pkt.pub_msg.qos;
    if(qos > mctx->max_qos) {
        return MQTT_RETURN_ERROR_SERVER_PROP;
    }
    else if( mctx->send_pkt.pub_msg.retain==1 && mctx->retain_avail==0 ) {
        return MQTT_RETURN_ERROR_SERVER_PROP;
    }
    msg            = &mctx->send_pkt.pub_msg;
    tx_buf         =  mctx->tx_buf;
    tx_buf_len     =  mctx->tx_buf_len;
    pkt_total_len  =  mqttEncodePktPublish( tx_buf, tx_buf_len, msg );
    if(pkt_total_len <= 0) {
        return  MQTT_RETURN_ERROR_MALFORMED_DATA;
    }

    // transfer entire packet 
    word32  appdata_len = msg->app_data_len;
    word32  inbuf_pos = 0;
    do {
        while(1) {
            appdata_len    -= (inbuf_pos == 0) ? msg->inbuf_len : pkt_total_len;
            status          = mqttPktWrite( mctx, tx_buf, pkt_total_len );
            if(appdata_len <= 0) { break; }
            inbuf_pos      += pkt_total_len;
            pkt_total_len   = ESP_MIN( appdata_len, tx_buf_len );
            ESP_MEMCPY( tx_buf, &msg->buff[inbuf_pos], pkt_total_len );
        } // end of while-loop

        if( qos > MQTT_QOS_0 ) {
            wait_cmdtype = (qos==MQTT_QOS_1) ? MQTT_PACKET_TYPE_PUBACK: MQTT_PACKET_TYPE_PUBCOMP;
            // implement qos=1 or 2 wait for response packet
            status = mqttClientWaitPkt( mctx, wait_cmdtype, msg->packet_id, (void *)&mctx->recv_pkt.pub_resp );
        } 
        // we only loop back & resend PUBLISH packet again if QoS = 1, 
        // and we didn't get PUBACK from the network after a period of time.
    } while((qos==MQTT_QOS_1) && (status!=MQTT_RETURN_SUCCESS));
    return status;
} // end of mqttSendPublish











