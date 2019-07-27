#ifndef __INTEGRATION_ESP_AT_SW_MQTT_CLIENT_CONN_H
#define __INTEGRATION_ESP_AT_SW_MQTT_CLIENT_CONN_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    const char *topic_name;
    word16      topic_name_len;
    word16      packet_id;
    byte        type;
    byte        retain;
    byte        duplicate;
    mqttQoS     qos;
    // total length of the message body 
    // (not represent this entire mqttMsg_t struct)
    word32      total_len;
    word32      buff_len;
    // current read position of the buffer in this struct
    word32      buff_pos; // TODO: check if we really need this
    byte       *buff;
    mqttProp_t *props;
} mqttMsg_t;


// denote every single MQTT connection 
typedef struct {
    int          cmd_timeout_ms;
    byte         protocol_lvl; // it will be 5 for MQTT v5.0
    byte         clean_session;
    word16       keep_alive_sec;
    mqttStr_t    client_id;

    //  Optional login 
    mqttStr_t    username;
    mqttStr_t    password;

    byte       *tx_buf;
    word32      tx_buf_len;
    byte       *rx_buf;
    word32      rx_buf_len;

    union {
        // Ack data , TODO: test
        mqttPktHeadConnack_t recv_connack;
        mqttPktDisconn_t     disconn;
    } pkt;
    // extract received message to this member, TODO: test 
    mqttMsg_t    *recv_msg;

    word32       packet_sz_max;  
    mqttQoS      max_qos;        
    byte         retain_avail;
    // optional properties for this MQTT connection
    mqttProp_t  *props;

    // extended connection objects used for underlying system
    void*        ext_sysobjs[2];
} mqttCtx_t;



// ----- Application Interface for MQTT client code operations -----

// initialize the  mqttCtx_t  structure
int   mqttClientInit( mqttCtx_t **mconn, int cmd_timeout_ms,
                      word32 tx_buf_len, word32 rx_buf_len );

int   mqttClientDeinit( mqttCtx_t *mconn );

// encodes & sends MQTT CONNECT packet, and waits for CONNACK packet
// this is a blocking function 
int   mqttSendConnect( mqttCtx_t *mconn );

// encodes & sends PUBLISH packet, for QoS > 0, this function waits for
// publish response packets, 
//     If QoS level = 1 then will wait for PUBLISH_ACK.
//     If QoS level = 2 then will wait for PUBLISH_REC then send
//         PUBLISH_REL and wait for PUBLISH_COMP.
int   mqttSendPublish( mqttCtx_t *mconn, mqttMsg_t *msg );

// encodes & sends MQTT SUBSCRIBE packet, then waits for SUBACK packet
int   mqttSendSubscribe( mqttCtx_t *mconn, mqttPktSubs_t *sub );

// encodes & sends MQTT UNSUBSCRIBE packet, waits for UNSUBACK packet
int   mqttSendUnsubscribe( mqttCtx_t *mconn, mqttPktUnsubs_t *unsub );

// encodes & sends MQTT PING request packet, and waits for PING response
// packet
int   mqttSendPingReq( mqttCtx_t *mconn );

// encodes & sends MQTT AUTH packet if client enabled enhanced authentication
// by adding properties "Authentication method" / "Authentication Data" to
// CONNECT packet, then client will send this AUTH packet and waits for CONNACK
// packet with authentication success status.
int    mqttSendExtendAuth( mqttCtx_t *mconn, mqttPktAuth_t *auth );

// encodes & sends MQTT DISCONNECT packet, then client must closse the TCP
// connection (no need to wait for broker's response).
int   mqttSendDisconnect( mqttCtx_t *mconn );




// create new property node to a given list, return the added item
mqttProp_t*  mqttPropertyCreate( mqttProp_t **head );

// delete/free the allocated space to entire list, start from the given head
void         mqttPropertyDel( mqttProp_t *head );



// waits for packets (with given type) to arrive, it could be incoming
// PUBLISH message, or acknowledgement of PUBLISH / SUBSCRIBE / UNSUBSCRIBE
// packet client has sent.
int  mqttClientWaitPkt( mqttCtx_t *mconn, mqttCtrlPktType cmdtype, 
                        void* p_decode );




#ifdef __cplusplus
}
#endif
#endif // end of  __INTEGRATION_ESP_AT_SW_MQTT_CLIENT_CONN_H

