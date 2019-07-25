#ifndef __INTEGRATION_ESP_AT_SW_MQTT_PACKET_H
#define __INTEGRATION_ESP_AT_SW_MQTT_PACKET_H

#ifdef __cplusplus
extern "C" {
#endif

#define MQTT_CONN_PROTOCOL_NAME_LEN  4
#define MQTT_CONN_PROTOCOL_NAME      "MQTT"
#define MQTT_CONN_PROTOCOL_LEVEL_5   5 // for MQTT v5.0 
#define MQTT_CONN_PROTOCOL_LEVEL     MQTT_CONN_PROTOCOL_LEVEL_5


// denote first few bytes of fixed header in a packet.
typedef struct  __mqttPktFxHead {
    // [7:4] control type
    // [3:0] flags that dedicates to the corresponding control type.
    byte  type_flgs;
    // 2nd to 5th byte are remaining length, it's encoded in variable bytes,
    // not all of them will be used, it depends on continuation bit of each
    // byte
    byte  remain_len[ MQTT_PKT_MAX_BYTES_REMAIN_LEN ];
} mqttPktFxHead_t;



// variable-sized header in CONNECT packet
typedef struct {
    byte      protocol_len[ MQTT_DSIZE_STR_LEN ];
    char      protocol_name[ MQTT_CONN_PROTOCOL_NAME_LEN ];
    byte      protocol_lvl; // it will be 5 for MQTT v5.0
    byte      flags; // filled with  mqttConnectFlg 
    word16    keep_alive;  
} mqttPktHeadConnect_t ;


// variable-sized header in CONNACK packet
typedef struct {
    byte        flags; // filled with mqttConnackFlg
    byte        reason_code;
    mqttProp_t *props;
} mqttPktHeadConnack_t ;


// variable-sized header in publish response packet e.g. PUBACK, PUBREC, PUBREL, PUBCOMP
//
// when a client sends packet PUBLISH (step #1) with different QoS levels ...
// If QoS = 0: No response 
// If QoS = 1: Expect response packet with PUBACK 
// If QoS = 2: Expect response packet with PUBREC (step #2)
// 
// Packet ID is required if QoS is 1 or 2 
// extra steps for Qos = 2:
// step #3 : after receiving PUBREC,  client sends PUBREL with the same Packet ID
//           (as shown in PUBLISH) to broker.
// step #4 : Expect response packet with type PUBCOMP, to ensure subscriber received 
//           the published message.
typedef struct {
    word16      packet_id; 
    byte        reason_code;
    mqttProp_t *props;
} mqttPktPubResp_t ;

typedef mqttPktPubResp_t mqttPktPuback_t  ;
typedef mqttPktPubResp_t mqttPktPubrecv_t ;
typedef mqttPktPubResp_t mqttPktPubrel_t  ;
typedef mqttPktPubResp_t mqttPktPubcomp_t ;


// essential data in SUBSCRIBE packet
typedef struct {
    word16         packet_id; 
    // packet ID followed by continuous topic list with QoS to subscribe
    word16         topic_cnt; 
    mqttTopic_t   *topics;
    mqttProp_t    *props;
} mqttPktSubs_t;



typedef struct {
    word16        packet_id; 
    mqttProp_t   *props;
    // a list of reason codes for all topics we want to subscribe
    byte         *return_codes; 
} mqttPktSuback_t ;


// UNSUBSCRIBE packet format
typedef mqttPktSubs_t  mqttPktUnsubs_t;

// UNSUBACK packet format
typedef mqttPktSuback_t mqttPktUnsuback_t ; 


// DISCONNECT packet format
typedef struct {
    byte          reason_code;
    mqttProp_t   *props;
} mqttPktDisconn_t ;


// AUTH packet format
typedef mqttPktDisconn_t mqttPktAuth_t; 


// ----- Application Interface for MQTT client code operations -----
// low-level interfaces to read/write packet data from underlying system
word32  mqttPktLowLvlRead(  mqttConn_t *mconn, byte *buf, word32 buf_len);
word32  mqttPktLowLvlWrite( mqttConn_t *mconn, byte *buf, word32 buf_len);

// interface to read/write packet data
int  mqttPktRead(  mqttConn_t *mconn, byte *buf, word32 buf_max_len, word32 *copied_len );
int  mqttPktWrite( mqttConn_t *mconn, byte *buf, word32 buf_len );

// element encoders / decoders
// 16-bit number from/to consecutive given 2 bytes
word32 mqttDecodeWord16( byte *buf , word16 *value );
word32 mqttEncodeWord16( byte *buf , word16  value );

// 32-bit number from/to consecutive given 4 bytes
word32 mqttDecodeWord32( byte *buf , word32 *value );
word32 mqttEncodeWord32( byte *buf , word32  value );

// encode/decode string, with string length ahead
word32 mqttDecodeStr( byte *buf, const char **pstr, word16 *pstrlen );
word32 mqttEncodeStr( byte *buf, const char   *str, word16   strlen );

// encode/decode variable-bytes number
word32 mqttDecodeVarBytes( byte *buf, word32 *value );
word32 mqttEncodeVarBytes( byte *buf, word32  value );

// encode/decode property for certain types of packets
word32 mqttDecodeProps( byte *buf, mqttProp_t *props );
word32 mqttEncodeProps( byte *buf, mqttProp_t *props );


// encode/decode  different types of MQTT packet 
word32  mqttDecodePktConnack( byte *rx_buf, word32 rx_buf_len,  mqttPktHeadConnack_t *connack );
word32  mqttDecodePktPublish( byte *rx_buf, word32 rx_buf_len, mqttMsg_t *msg );
word32  mqttDecodePktPubResp( byte *rx_buf, word32 rx_buf_len, mqttPktPubResp_t *resp, mqttCtrlPktType cmdtype );
word32  mqttDecodePktSuback( byte *rx_buf, word32 rx_buf_len, mqttPktSuback_t *suback );
word32  mqttDecodePktUnsuback( byte *rx_buf, word32 rx_buf_len, mqttPktUnsuback_t *unsuback );
word32  mqttDecodePktPing( byte *rx_buf, word32 rx_buf_len );
word32  mqttDecodePktDisconn( byte *rx_buf, word32 rx_buf_len, mqttPktDisconn_t *disconn);
word32  mqttDecodePktAuth( byte *rx_buf, word32 rx_buf_len, mqttPktAuth_t *auth );

word32  mqttEncodePktConnect( byte *tx_buf, word32 tx_buf_len, mqttConn_t* conn );
word32  mqttEncodePktPublish( byte *tx_buf, word32 tx_buf_len, mqttMsg_t *msg );
word32  mqttEncodePktPubResp( byte *tx_buf, word32 tx_buf_len, mqttPktPubResp_t *resp, mqttCtrlPktType cmdtype );
word32  mqttEncodePktSubscribe( byte *tx_buf, word32 tx_buf_len, mqttPktSubs_t *subs );
word32  mqttEncodePktUnsubscribe( byte *tx_buf, word32 tx_buf_len, mqttPktUnsubs_t *unsubs );
word32  mqttEncodePktPing( byte *tx_buf, word32 tx_buf_len );
word32  mqttEncodePktDisconn( byte *tx_buf, word32 tx_buf_len, mqttPktDisconn_t *disconn );
word32  mqttEncodePktAuth( byte *tx_buf, word32 tx_buf_len, mqttPktAuth_t *auth );




#ifdef __cplusplus
}
#endif
#endif // end of  __INTEGRATION_ESP_AT_SW_MQTT_PACKET_H

