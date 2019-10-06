#include "esp/esp.h"
#include "tests/integration/ESP_AT_parser/mqtt/mqtt_include.h"



static  const mqttDataType mqttQueryPropDataType [] = 
{
    MQTT_DATA_TYPE_NONE, // MQTT_PROP_NONE = 0x00,
    MQTT_DATA_TYPE_BYTE, // MQTT_PROP_PKT_FMT_INDICATOR = 0x01,
    MQTT_DATA_TYPE_INT , // MQTT_PROP_MSG_EXPIRY_INTVL  = 0x02,
    MQTT_DATA_TYPE_STRING, // MQTT_PROP_CONTENT_TYPE      = 0x03,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_STRING,  // MQTT_PROP_RESP_TOPIC        = 0x08,
    MQTT_DATA_TYPE_BINARY,  // MQTT_PROP_CORRELATION_DATA  = 0x09,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_VAR_INT, // MQTT_PROP_SUBSCRIBE_ID      = 0x0b,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_INT ,   // MQTT_PROP_SESSION_EXPIRY_INTVL = 0x11,
    MQTT_DATA_TYPE_STRING, // MQTT_PROP_ASSIGNED_CLIENT_ID   = 0x12,
    MQTT_DATA_TYPE_SHORT,  // MQTT_PROP_SERVER_KEEP_ALIVE    = 0x13,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_STRING, // MQTT_PROP_AUTH_METHOD       = 0x15,
    MQTT_DATA_TYPE_BINARY, // MQTT_PROP_AUTH_DATA         = 0x16,
    MQTT_DATA_TYPE_BYTE,   // MQTT_PROP_REQ_PROBLEM_INFO  = 0x17,
    MQTT_DATA_TYPE_INT ,   // MQTT_PROP_WILL_DELAY_INTVL  = 0x18,
    MQTT_DATA_TYPE_BYTE,   // MQTT_PROP_REQ_RESP_INFO     = 0x19,
    MQTT_DATA_TYPE_STRING, // MQTT_PROP_RESP_INFO         = 0x1a,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_STRING, // MQTT_PROP_SERVER_REF        = 0x1c,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_STRING, // MQTT_PROP_REASON_STR        = 0x1f,
    MQTT_DATA_TYPE_NONE,
    MQTT_DATA_TYPE_SHORT,  // MQTT_PROP_RECV_MAX          = 0x21,
    MQTT_DATA_TYPE_SHORT,  // MQTT_PROP_TOPIC_ALIAS_MAX   = 0x22,
    MQTT_DATA_TYPE_SHORT,  // MQTT_PROP_TOPIC_ALIAS       = 0x23,
    MQTT_DATA_TYPE_BYTE,   // MQTT_PROP_MAX_QOS           = 0x24,
    MQTT_DATA_TYPE_BYTE,   // MQTT_PROP_RETAIN_AVAILABLE  = 0x25,
    MQTT_DATA_TYPE_STRING_PAIR, // MQTT_PROP_USER_PROPERTY     = 0x26,
    MQTT_DATA_TYPE_INT ,   // MQTT_PROP_MAX_PKT_SIZE      = 0x27,
    MQTT_DATA_TYPE_BYTE,   // MQTT_PROP_WILDCARD_SUBS_AVAIL = 0x28,
    MQTT_DATA_TYPE_BYTE,   // MQTT_PROP_SUBSCRIBE_ID_AVAIL  = 0x29,
    MQTT_DATA_TYPE_BYTE,   // MQTT_PROP_SHARE_SUBSCRIBE_AVAIL = 0x2a,
}; // end of mqttGetPropLength



word32 mqttEncodeVarBytes( byte *buf, word32  value )
{
    word32  len = 0;
    byte    enc_val = 0; // encoded part of value
    const   byte    continuation_bit = 0x80;
    do {
        enc_val   = value & 0x7f;
        value   >>= 7;
        if( value > 0 ){
            enc_val |= continuation_bit;
        }
        if(buf != NULL) {
            buf[len] = enc_val;
        }
        len++;
    } while( value > 0 ) ; // end of while-loop
    return  len;
} // end of mqttEncodeVarBytes




word32 mqttDecodeVarBytes(const byte *buf, word32 *value )
{
    word32  val = 0;
    word16  idx = 0;
    byte    enc_val = 0; 
    const   byte  continuation_bit = 0x80;
    do {
        enc_val  = buf[idx];
        val     |= (enc_val & 0x7f) << (idx * 7);
        idx++;
    } while((enc_val & continuation_bit) != 0x0);
    *value = val;
    return  idx;
} // end of mqttDecodeVarBytes




word32 mqttEncodeWord16( byte *buf , word16 value )
{
    if(buf != NULL){
        buf[0] = value >> 8; 
        buf[1] = value &  0xff; 
    }
    // return number of bytes used to store the encoded value
    return  (word32)2; 
} // end of mqttEncodeWord16




word32 mqttDecodeWord16( byte *buf , word16 *value )
{
    if((buf != NULL) && (value != NULL)) {
        *value  =  buf[1]; 
        *value |=  buf[0] << 8 ;
    }
    return  (word32)2; 
} // end of mqttDecodeWord16




word32 mqttEncodeWord32( byte *buf , word32  value )
{
    if(buf != NULL){
        buf[0] =  value >> 24; 
        buf[1] = (value >> 16) & 0xff; 
        buf[2] = (value >> 8 ) & 0xff; 
        buf[3] =  value &  0xff; 
    }
    // return number of bytes used to store the encoded value
    return  (word32)4;
} // end of mqttEncodeWord32




word32 mqttDecodeWord32( byte *buf , word32 *value )
{
    if((buf != NULL) && (value != NULL)) {
        *value  = buf[3]; 
        *value |= buf[2] << 8  ;
        *value |= buf[1] << 16 ;
        *value |= buf[0] << 24 ;
    }
    return  (word32)4; 
} // end of mqttDecodeWord32




word32 mqttEncodeStr( byte *buf, const char   *str, word16   strlen )
{
    word32  len  = 0;
    len = mqttEncodeWord16( buf, strlen );
    if((buf != NULL) && (str != NULL)){
        buf += len;
        ESP_MEMCPY( buf, str, strlen );
    }
    len += strlen;
    return  len;
} // end of mqttEncodeStr




word32 mqttDecodeStr( byte *buf, const char **pstr, word16 *pstrlen )
{
    word32  len  = 0;
    if((buf != NULL) && (pstrlen != NULL) && (pstr != NULL)){
        len    = mqttDecodeWord16( buf, pstrlen );
        *pstr  = (const char *)&buf[len] ;
        len   += *pstrlen;
    }
    return  len;
} // end of mqttDecodeStr





word32 mqttEncodeProps( byte *buf, mqttProp_t *props )
{
    mqttProp_t *curr_prop   = NULL;
    word32      total_len   = 0;
    word32      len         = 0;
    for( curr_prop = props; curr_prop != NULL ; curr_prop = curr_prop->next ) 
    {
        // get property type (ID code)
        len        = mqttEncodeVarBytes( buf, (word32)curr_prop->type );
        total_len += len;
        if(buf != NULL) { buf += len; }

        // get length (number of bytes) of each property
        switch( mqttQueryPropDataType[curr_prop->type] )
        {
            case MQTT_DATA_TYPE_BYTE         : 
                len = 1;
                if(buf != NULL){ *buf  = curr_prop->body.u8; }
                break;
            case MQTT_DATA_TYPE_SHORT        :
                len = mqttEncodeWord16( buf, curr_prop->body.u16 );
                break;
            case MQTT_DATA_TYPE_INT          : 
                len = mqttEncodeWord32( buf, curr_prop->body.u32 );
                break;
            case MQTT_DATA_TYPE_VAR_INT      :
                len = mqttEncodeVarBytes( buf, curr_prop->body.u32 );
                break;
            case MQTT_DATA_TYPE_BINARY       : 
            case MQTT_DATA_TYPE_STRING       :
                len = mqttEncodeStr( buf, (const char *)curr_prop->body.str.data,  curr_prop->body.str.len );
                break;
            case MQTT_DATA_TYPE_STRING_PAIR  :
                len  = mqttEncodeStr( buf, (const char *)curr_prop->body.strpair[0].data,  curr_prop->body.strpair[0].len );
                if(buf != NULL){ buf  += len; }
                total_len += len;
                len  = mqttEncodeStr( buf, (const char *)curr_prop->body.strpair[1].data,  curr_prop->body.strpair[1].len );
                break;
            default:
                len = 0;
                break;
        } // end of switch-case statement

        if(buf != NULL){ buf  += len; }
        total_len += len;
    } // end of for-loop

    return  total_len;
} // end of mqttEncodeProps





word32 mqttDecodeProps( byte *buf, mqttProp_t **props , word32  props_len )
{
    mqttProp_t *curr_prop   = NULL;
    word32      len ;
    word32      copied_len = 0;

    if(buf == NULL){ return copied_len; }
    while(props_len > 0) 
    {
        // create new empty node to the given property list.
        curr_prop = mqttPropertyCreate( props );
        if(curr_prop == NULL) {
            mqttPropertyDel( *props );
            break; // memory error
        }
        // first byte of each property must represent the type
        len           = mqttDecodeVarBytes((const byte *)buf, (word32 *)&curr_prop->type );
        props_len    -= len;
        copied_len   += len;
        buf          += len;
        switch( mqttQueryPropDataType[curr_prop->type] )
        {
            case MQTT_DATA_TYPE_BYTE         : 
                len  = 1;
                curr_prop->body.u8 = *buf;
                break;
            case MQTT_DATA_TYPE_SHORT        :
                len = mqttDecodeWord16( buf, &curr_prop->body.u16 );
                break;
            case MQTT_DATA_TYPE_INT          : 
                len = mqttDecodeWord32( buf, &curr_prop->body.u32 );
                break;
            case MQTT_DATA_TYPE_VAR_INT      :
                len = mqttDecodeVarBytes( (const byte *)buf, &curr_prop->body.u32 );
                break;
            case MQTT_DATA_TYPE_BINARY       : 
            case MQTT_DATA_TYPE_STRING       :
                len = mqttDecodeStr( buf, (const char **)&curr_prop->body.str.data,  &curr_prop->body.str.len );
                break;
            case MQTT_DATA_TYPE_STRING_PAIR  :
                len  = mqttDecodeStr( &buf[0]  , (const char **)&curr_prop->body.strpair[0].data,  &curr_prop->body.strpair[0].len );
                len += mqttDecodeStr( &buf[len], (const char **)&curr_prop->body.strpair[1].data,  &curr_prop->body.strpair[1].len );
                break;
            default:
                len = 0;
                break;
        } // end of switch-case statement
        props_len    -= len;
        copied_len   += len;
        buf          += len;
    } // end of loop
    return  copied_len;
} // end of mqttDecodeProps



    


static word32 mqttEncodeFxHeader( byte *tx_buf, word32 tx_buf_len, word32 remain_len, 
                                  mqttCtrlPktType cmdtype, byte retain, byte qos, byte duplicate )
{
    word32  len = 0;
    mqttPktFxHead_t  *header = (mqttPktFxHead_t *) tx_buf;
    header->type_flgs  = 0;
    MQTT_CTRL_PKT_TYPE_SET( header->type_flgs, cmdtype );
    header->type_flgs |= (duplicate & 0x1) << 3 ;
    header->type_flgs |= (qos       & 0x3) << 1 ;
    header->type_flgs |= (retain    & 0x1) << 0 ;
    len += mqttEncodeVarBytes( &header->remain_len[0], remain_len );
    len  = len + 1; // TODO: test this part
    return  len;
} // end of  mqttEncodeFxHeader



static word32 mqttDecodeFxHeader( byte *rx_buf, word32 rx_buf_len, word32 *remain_len, 
                                  mqttCtrlPktType cmdtype, byte *retain, byte *qos, byte *duplicate )
{
    const    mqttPktFxHead_t  *header = (mqttPktFxHead_t *) rx_buf;
    word32   len = 0;
    word32   _remain_len ;

    if(MQTT_CTRL_PKT_TYPE_GET(header->type_flgs) != cmdtype) {
        return len;
    }
    if(retain != NULL) {    *retain    = (header->type_flgs >> 0) & 0x1; }
    if(qos != NULL) {       *qos       = (header->type_flgs >> 1) & 0x3; }
    if(duplicate != NULL) { *duplicate = (header->type_flgs >> 3) & 0x1; }
    len += 1;
    len += mqttDecodeVarBytes( &header->remain_len[0], &_remain_len );
    if(remain_len != NULL) { *remain_len = _remain_len; }
    return  len;
} // end of mqttDecodeFxHeader



word32  mqttEncodePktConnect( byte *tx_buf, word32 tx_buf_len, struct __mqttConn *conn )
{
    word32   fx_head_len = 0;
    word32   remain_len  = 0;
    word32   props_len   = 0;
    mqttPktHeadConnect_t  var_head = {{0, MQTT_CONN_PROTOCOL_NAME_LEN}, {'M','Q','T','T'}, 0, 0, 0}; 
    byte    *curr_buf_pos ;

    if((conn == NULL) || (tx_buf == NULL) || (tx_buf_len == 0)) { 
        return 0; 
    }
    // size of variable header in CONNECT packet, should be 10 bytes in MQTT v5.0
    remain_len += sizeof(mqttPktHeadConnect_t); 

    // size of all properties in the header of this CONNECT packet
    props_len   =  mqttEncodeProps( NULL, conn->props );
    remain_len +=  props_len;
    // number of variable bytes to store "property length"
    remain_len += mqttEncodeVarBytes(NULL, props_len);

    // size of each element in CONNECT payload section
    // TODO: implement will property, will topic, and will payload
    remain_len += MQTT_DSIZE_STR_LEN + conn->client_id.len ; 
    if(conn->username.data != NULL) {
        remain_len += MQTT_DSIZE_STR_LEN + conn->username.len ; 
    }
    if(conn->password.data != NULL) {
        remain_len += MQTT_DSIZE_STR_LEN + conn->password.len ; 
    }
    // build fixed header of CONNECT packet 
    fx_head_len = mqttEncodeFxHeader( tx_buf, tx_buf_len, remain_len, 
                                      MQTT_PACKET_TYPE_CONNECT,  0, 0, 0 );

    curr_buf_pos = &tx_buf[fx_head_len];
    // copy bytes to variable header
    var_head.protocol_lvl = conn->protocol_lvl; 
    // TODO: implement will property, will topic, and will payload
    if(conn->clean_session != 0) {
        var_head.flags |=  MQTT_CONNECT_FLG_CLEAN_START; 
    }
    if(conn->username.data != NULL) {
        var_head.flags |= MQTT_CONNECT_FLG_USERNAME ; 
    }
    if(conn->password.data != NULL) {
        var_head.flags |= MQTT_CONNECT_FLG_PASSWORD ; 
    }
    mqttEncodeWord16( (byte *)&var_head.keep_alive, conn->keep_alive_sec );
    ESP_MEMCPY( curr_buf_pos, (byte *)&var_head, sizeof(mqttPktHeadConnect_t) );
    curr_buf_pos += sizeof(mqttPktHeadConnect_t);

    // copy all properties to buffer
    curr_buf_pos += mqttEncodeVarBytes( curr_buf_pos, props_len );
    curr_buf_pos += mqttEncodeProps( curr_buf_pos, conn->props );

    // copy all elements of the payload to buffer
    curr_buf_pos += mqttEncodeStr( curr_buf_pos, (const char *)conn->client_id.data,  conn->client_id.len );
    // TODO: implement will property, will topic, and will payload
    if(conn->username.data != NULL) {
        curr_buf_pos += mqttEncodeStr( curr_buf_pos, (const char *)conn->username.data,  conn->username.len );
    }
    else {
        // [Note]
        // A server may allow a client to provide empty clientID (has length of zero byte),
        // server must assign unique ID to such CONNECT packet (with zero-byte clientID), and then
        // return CONNACK packet with the property "Assigned Client Identifier" back to client .
        curr_buf_pos += mqttEncodeWord16( curr_buf_pos, (word16)0 );
    }
    if(conn->password.data != NULL) {
        curr_buf_pos += mqttEncodeStr( curr_buf_pos, (const char *)conn->password.data,  conn->password.len );
    }

    return  (remain_len + fx_head_len);
} // end of mqttEncodePktConnect




word32  mqttDecodePktConnack( byte *rx_buf, word32 rx_buf_len,  mqttPktHeadConnack_t *connack )
{
    if((connack == NULL) || (rx_buf == NULL) || (rx_buf_len == 0)) { 
        return  0;
    }
    word32   fx_head_len = 0;
    word32   remain_len  = 0;
    word32   props_len   = 0;
    byte    *curr_buf_pos ;
    byte    *end_of_buf ;
    fx_head_len = mqttDecodeFxHeader( rx_buf, rx_buf_len, &remain_len, 
                                      MQTT_PACKET_TYPE_CONNACK, NULL, NULL, NULL );
    curr_buf_pos = &rx_buf[fx_head_len];
    end_of_buf   =  curr_buf_pos + remain_len;
    connack->flags        = *curr_buf_pos++;
    connack->reason_code  = *curr_buf_pos++;
    if(end_of_buf > curr_buf_pos) {
        // copy all properties from buffer
        curr_buf_pos += mqttDecodeVarBytes( (const byte *)curr_buf_pos, &props_len );
        curr_buf_pos += mqttDecodeProps( curr_buf_pos, &connack->props, props_len );
    }
    return  (fx_head_len + remain_len);
} // end of mqttDecodePktConnack




word32  mqttEncodePktDisconn( byte *tx_buf, word32 tx_buf_len, mqttPktDisconn_t *disconn )
{
    if((disconn == NULL) || (tx_buf == NULL) || (tx_buf_len == 0)) { 
        return  0;
    }
    word32   fx_head_len = 0;
    word32   remain_len  = 0;
    word32   props_len   = 0;
    byte    *curr_buf_pos ;
    byte     reason_code = disconn->reason_code;
    // if reason code is 0x0 (normal disconnection), then there's no need to put reason code
    // and extra properties into DISCONNECT packet, therefore hte remaining length should be
    // zero.
    if((reason_code != MQTT_REASON_NORMAL_DISCONNECTION) || (disconn->props!=NULL)) {
        // 1 byte is preserved for non-zero reason code
        remain_len +=  1;
        // size of all properties in the DISCONNECT packet
        props_len   =  mqttEncodeProps( NULL, disconn->props );
        remain_len +=  props_len;
        // number of variable bytes to store "property length"
        remain_len += mqttEncodeVarBytes(NULL, props_len);
    }
    // build fixed header of CONNECT packet 
    fx_head_len = mqttEncodeFxHeader( tx_buf, tx_buf_len, remain_len, 
                                      MQTT_PACKET_TYPE_DISCONNECT,  0, 0, 0 );
    if((reason_code != MQTT_REASON_NORMAL_DISCONNECTION) || (disconn->props!=NULL)) {
        curr_buf_pos = &tx_buf[fx_head_len];
        // 1 byte is preserved for non-zero reason code, TODO: test
        *curr_buf_pos++  = reason_code;
        // copy all properties to buffer
        curr_buf_pos +=  mqttEncodeVarBytes( curr_buf_pos, props_len );
        curr_buf_pos +=  mqttEncodeProps( curr_buf_pos, disconn->props );
    }
    return (fx_head_len + remain_len);
} // end of mqttEncodePktDisconn




word32  mqttEncodePktPublish( byte *tx_buf, word32 tx_buf_len, struct __mqttMsg  *msg )
{
    word32   fx_head_len  = 0;
    word32   var_head_len = 0;
    word32   payload_len  = 0;
    word32   props_len    = 0;
    byte    *curr_buf_pos ;

    if((msg == NULL) || (tx_buf == NULL) || (tx_buf_len == 0)) { 
        return  0;
    }
    if((msg->topic.data==NULL) || (msg->topic.len < 1)) {
        return  0; // topic is a must when publishing message
    }
    // number of bytes taken to encode topic string
    if(msg->qos > MQTT_QOS_0) {
        if(msg->packet_id == 0) {
            return  0; // when QoS > 0, packet ID must be non-zero 16-bit number
        }
        var_head_len += 2; // 2 bytes for packet ID
    }
    var_head_len += MQTT_DSIZE_STR_LEN + msg->topic.len ;
    // size of all properties in the PUBLISH packet
    props_len   =  mqttEncodeProps( NULL, msg->props );
    var_head_len +=  props_len;
    // number of variable bytes to store "property length"
    var_head_len += mqttEncodeVarBytes(NULL, props_len);
    // length of application specific data 
    payload_len = msg->app_data_len;

    // build fixed header of PUBLISH packet 
    fx_head_len = mqttEncodeFxHeader( tx_buf, tx_buf_len, (var_head_len + payload_len), 
                                      MQTT_PACKET_TYPE_PUBLISH, msg->retain,
                                      msg->qos,  msg->duplicate );

    curr_buf_pos  = &tx_buf[fx_head_len]; 
    // variable header : topic filter
    curr_buf_pos += mqttEncodeStr( curr_buf_pos, (const char *)msg->topic.data, msg->topic.len );
    // variable header : packet ID (if QoS > 0)
    if(msg->qos > MQTT_QOS_0) {
        curr_buf_pos += mqttEncodeWord16( curr_buf_pos, msg->packet_id );
    }
    // variable header : properties
    curr_buf_pos += mqttEncodeVarBytes(curr_buf_pos , props_len);
    curr_buf_pos += mqttEncodeProps(curr_buf_pos , msg->props );

    // [NOTE]
    //  *  Copy the beginning part of application specific data to the Tx buffer at here
    //     , the rest of the application data will be copied at the level of mqttSendPublish() .
    //  *  It's acceptable that the size of application data is greater than transfer
    //     buffer (the Tx buffer here), in such case the MQTT client must break the huge
    //     buffer of application data into several pieces, in order to fit each piece 
    //     into network packet.
    //  *  it's also ok if payload length is zero in PUBLISH packet.
    if(payload_len > 0) {
        payload_len = ESP_MIN( payload_len, tx_buf_len - fx_head_len - var_head_len );
        ESP_MEMCPY( curr_buf_pos, msg->buff, payload_len );
        msg->inbuf_len = payload_len;
    }
    return (fx_head_len + var_head_len + payload_len);
} // end of mqttEncodePktPublish





word32  mqttDecodePktPublish( byte *rx_buf, word32 rx_buf_len, struct __mqttMsg *msg )
{
    word32   fx_head_len  = 0;
    word32   var_head_len = 0;
    word32   payload_len  = 0;
    word32   props_len    = 0;
    word16   tmp = 0;
    byte    *curr_buf_pos ;

    if((msg == NULL) || (rx_buf == NULL) || (rx_buf_len == 0)) { 
        return  0;
    }
    fx_head_len = mqttDecodeFxHeader( rx_buf, rx_buf_len, &payload_len, 
                                      MQTT_PACKET_TYPE_PUBLISH, &msg->retain,
                                      &msg->qos, &msg->duplicate );
    curr_buf_pos  = &rx_buf[fx_head_len]; 
    // variable header : topic filter
    var_head_len  = mqttDecodeStr( curr_buf_pos, (const char **)&msg->topic.data, &msg->topic.len );
    curr_buf_pos += var_head_len;
    // variable header : check QoS & see if we have packet ID field in the received PUBLISH packet
    if(msg->qos > MQTT_QOS_0) {
        tmp            = mqttDecodeWord16( curr_buf_pos, &msg->packet_id );
        curr_buf_pos  += tmp;
        var_head_len  += tmp;
    }
    // variable header : optional properties
    tmp = mqttDecodeVarBytes( (const byte *)curr_buf_pos, &props_len );
    var_head_len += tmp;
    curr_buf_pos += tmp;
    if(props_len > 0) {
        var_head_len += props_len;
        curr_buf_pos += mqttDecodeProps( curr_buf_pos, &msg->props, props_len );
    }
    // payload, if Rx buffer is insufficient to store entire application message, then we only store
    // the beginning part (and lose part of the application message)
    payload_len -= var_head_len;
    msg->buff    = curr_buf_pos;
    msg->app_data_len = payload_len; 
    msg->inbuf_len    = ESP_MIN( payload_len, rx_buf_len - fx_head_len - var_head_len);
    msg->buff_len     = msg->inbuf_len ;
    return (fx_head_len + var_head_len + payload_len);
} // end of mqttDecodePktPublish




word32  mqttEncodePktSubscribe( byte *tx_buf, word32 tx_buf_len, mqttPktSubs_t *subs )
{
    if((subs == NULL) || (tx_buf == NULL) || (tx_buf_len == 0)) { 
        return  0;
    }
    word32        fx_head_len = 0;
    word32        remain_len  = 0;
    word32        props_len   = 0;
    word16        idx = 0;
    byte         *curr_buf_pos = NULL;
    mqttTopic_t  *curr_topic = NULL;

    // 2 bytes for packet ID
    remain_len = 2; 
    // size of all properties in the packet
    props_len   =  mqttEncodeProps( NULL, subs->props );
    remain_len +=  props_len;
    remain_len +=  mqttEncodeVarBytes( NULL, props_len );
    // loop through the topic lists , to determine payload length
    for( idx=0; idx<subs->topic_cnt; idx++ ){
        curr_topic  = &subs->topics[idx];
        if(curr_topic != NULL) {
            // preserve space for each topic, encoded as UTF-8 string
            remain_len += curr_topic->filter.len + 2; 
            remain_len += 1; // 1 byte for QoS field of each topic
        }
        else{ return 0; }
    }
    // build fixed header of SUBSCRIBE packet, 
    // TODO: figure out why it's 0010 at the reserved field 
    fx_head_len = mqttEncodeFxHeader( tx_buf, tx_buf_len, remain_len, 
                                      MQTT_PACKET_TYPE_SUBSCRIBE, 0, MQTT_QOS_1, 0 );
    curr_buf_pos  = &tx_buf[fx_head_len]; 
    // variable header, packet ID, and optional properties
    curr_buf_pos += mqttEncodeWord16( curr_buf_pos, subs->packet_id );
    curr_buf_pos += mqttEncodeVarBytes( curr_buf_pos , props_len );
    curr_buf_pos += mqttEncodeProps( curr_buf_pos , subs->props );
    // payload
    for( idx=0; idx<subs->topic_cnt; idx++ ) {
        curr_topic  = &subs->topics[idx];
        curr_buf_pos += mqttEncodeStr( curr_buf_pos, (const char *)curr_topic->filter.data, curr_topic->filter.len );
        *curr_buf_pos = (byte) curr_topic->qos; // TODO: implement all fields of the subscription options byte
        curr_buf_pos++;
    }
    return (fx_head_len + remain_len);
} // end of mqttEncodePktSubscribe




word32  mqttEncodePktUnsubscribe( byte *tx_buf, word32 tx_buf_len, mqttPktUnsubs_t *unsubs )
{
    if((unsubs == NULL) || (tx_buf == NULL) || (tx_buf_len == 0)) { 
        return  0;
    }
    word32   fx_head_len = 0;
    word32   remain_len  = 0;
    word32   props_len   = 0;
    word16   idx = 0;
    byte    *curr_buf_pos ;
    mqttTopic_t  *curr_topic = NULL;

    // 2 bytes for packet ID
    remain_len = 2; 
    // size of all properties in the packet
    props_len   =  mqttEncodeProps( NULL, unsubs->props );
    remain_len +=  props_len;
    remain_len +=  mqttEncodeVarBytes( NULL, props_len );
    for( idx=0; idx<unsubs->topic_cnt; idx++ ){
        curr_topic  = &unsubs->topics[idx];
        if(curr_topic != NULL) {
            // preserve space for each topic, encoded as UTF-8 string
            remain_len += curr_topic->filter.len + 2; 
        }
        else{ return 0; }
    }
    // build fixed header of UNSUBSCRIBE packet 
    fx_head_len = mqttEncodeFxHeader( tx_buf, tx_buf_len, remain_len, 
                                      MQTT_PACKET_TYPE_UNSUBSCRIBE, 0, MQTT_QOS_1, 0 );
    curr_buf_pos  = &tx_buf[fx_head_len]; 
    // variable header, packet ID, and optional properties
    curr_buf_pos += mqttEncodeWord16( curr_buf_pos, unsubs->packet_id );
    curr_buf_pos += mqttEncodeVarBytes( curr_buf_pos, props_len );
    curr_buf_pos += mqttEncodeProps( curr_buf_pos , unsubs->props );
    // payload
    for( idx=0; idx<unsubs->topic_cnt; idx++ ){
        curr_topic    = &unsubs->topics[idx];
        curr_buf_pos += mqttEncodeStr( curr_buf_pos, (const char *)curr_topic->filter.data, curr_topic->filter.len );
    }
    return (fx_head_len + remain_len);
} // end of mqttEncodePktUnsubscribe




word32  mqttEncodePktPubResp( byte *tx_buf, word32 tx_buf_len, mqttPktPubResp_t *resp, mqttCtrlPktType cmdtype )
{
    if((resp == NULL) || (tx_buf == NULL) || (tx_buf_len == 0)) { 
        return  0;
    }
    word32   fx_head_len = 0;
    word32   remain_len  = 0;
    word32   props_len   = 0;
    byte    *curr_buf_pos ;

    remain_len  = 2;  // 2 bytes for packet ID
    if((resp->reason_code != MQTT_REASON_SUCCESS) || (resp->props != NULL)) {
        remain_len += 1; // 1 byte for reason code 
        props_len   = mqttEncodeProps( NULL, resp->props );
        remain_len += props_len; 
        remain_len += mqttEncodeVarBytes( NULL, props_len );
    }
    fx_head_len  = mqttEncodeFxHeader( tx_buf, tx_buf_len, remain_len, cmdtype, 0, 0, 0 );
    curr_buf_pos  = &tx_buf[fx_head_len] ;
    curr_buf_pos += mqttEncodeWord16( curr_buf_pos, resp->packet_id );

    if((resp->reason_code != MQTT_REASON_SUCCESS) || (resp->props != NULL)) {
        *curr_buf_pos++ = resp->reason_code ;
        curr_buf_pos += mqttEncodeVarBytes( curr_buf_pos, props_len );
        curr_buf_pos += mqttEncodeProps( curr_buf_pos, resp->props );
    }
    return  (fx_head_len + remain_len);
} // end of mqttEncodePktPubResp





word32  mqttDecodePktPubResp( byte *rx_buf, word32 rx_buf_len, mqttPktPubResp_t *resp, mqttCtrlPktType cmdtype )
{
    if((resp == NULL) || (rx_buf == NULL) || (rx_buf_len == 0)) { 
        return  0;
    }
    word32   fx_head_len = 0;
    word32   remain_len  = 0;
    word32   props_len   = 0;
    byte    *curr_buf_pos ;
    byte    *end_of_buf;
    fx_head_len = mqttDecodeFxHeader( rx_buf, rx_buf_len, &remain_len, 
                                      cmdtype, NULL, NULL, NULL );
    curr_buf_pos  = &rx_buf[fx_head_len];
    end_of_buf    = curr_buf_pos + remain_len;
    // there must be packet ID when receiving publish response packet(s)
    // (QoS must be greater than 0)
    curr_buf_pos += mqttDecodeWord16( curr_buf_pos, &resp->packet_id );

    if(end_of_buf > curr_buf_pos) {
        resp->reason_code = *curr_buf_pos++; 
    }
    else {
        // Reason code might not be present in the variable header, 
        // that means success code (0x00) is used as reason code.
        resp->reason_code = MQTT_REASON_SUCCESS; 
    }
    if(end_of_buf > curr_buf_pos) {
        // copy all properties from buffer
        curr_buf_pos += mqttDecodeVarBytes( (const byte *)curr_buf_pos, &props_len );
        curr_buf_pos += mqttDecodeProps( curr_buf_pos, &resp->props, props_len );
    }
    return  (fx_head_len + remain_len);
} // end of mqttDecodePktPubResp




word32  mqttDecodePktSuback( byte *rx_buf, word32 rx_buf_len, mqttPktSuback_t *suback )
{
    if((suback == NULL) || (rx_buf == NULL) || (rx_buf_len == 0)) { 
        return  0;
    }
    word32   fx_head_len = 0;
    word32   remain_len  = 0;
    word32   props_len   = 0;
    byte    *curr_buf_pos ;
    fx_head_len = mqttDecodeFxHeader( rx_buf, rx_buf_len, &remain_len, 
                                      MQTT_PACKET_TYPE_SUBACK, NULL, NULL, NULL );
    curr_buf_pos  = &rx_buf[fx_head_len];
    curr_buf_pos += mqttDecodeWord16( curr_buf_pos, &suback->packet_id );
    curr_buf_pos += mqttDecodeVarBytes( (const byte *)curr_buf_pos, &props_len );
    curr_buf_pos += mqttDecodeProps( curr_buf_pos, &suback->props, props_len );
    // the SUBACK payload contains a list of return codes that indicate whether the topic 
    // filters are subscribed successfully on the borker side.
    suback->return_codes = curr_buf_pos; 
    return  (fx_head_len + remain_len);
} // end of mqttDecodePktSuback




word32  mqttDecodePktUnsuback( byte *rx_buf, word32 rx_buf_len, mqttPktUnsuback_t *unsuback )
{
    if((unsuback == NULL) || (rx_buf == NULL) || (rx_buf_len == 0)) { 
        return  0;
    }
    word32   fx_head_len = 0;
    word32   remain_len  = 0;
    word32   props_len   = 0;
    byte    *curr_buf_pos ;
    fx_head_len = mqttDecodeFxHeader( rx_buf, rx_buf_len, &remain_len, 
                                      MQTT_PACKET_TYPE_UNSUBACK, NULL, NULL, NULL );
    curr_buf_pos  = &rx_buf[fx_head_len];
    curr_buf_pos += mqttDecodeWord16( curr_buf_pos, &unsuback->packet_id );
    curr_buf_pos += mqttDecodeVarBytes( (const byte *)curr_buf_pos, &props_len );
    curr_buf_pos += mqttDecodeProps( curr_buf_pos, &unsuback->props, props_len );
    // the UNSUBACK payload contains a list of return codes that indicate whether the topic
    // filters are unsubscribed successfully on the borker side.
    unsuback->return_codes = curr_buf_pos; 
    return  (fx_head_len + remain_len);
} // end of mqttDecodePktUnsuback




int  mqttPktRead( struct __mqttCtx *mctx, byte *buf, word32 buf_max_len, word32 *copied_len )
{
    if((mctx == NULL) || (buf == NULL) || (copied_len == NULL)) { 
        return MQTT_RETURN_ERROR_BAD_ARG;
    }
    word32  remain_len = 0;
    word32  tmp        = 0;
    word16  header_len = 0;
    word16  idx = 0;
    const   byte  continuation_bit = 0x80;
    const   mqttPktFxHead_t  *header = (mqttPktFxHead_t *) buf;
 
    // read the first byte. TODO: handle timeout error
    tmp = mqttPktLowLvlRead( mctx, buf, 0x1 );
    if(tmp == 0x0) { return MQTT_RETURN_ERROR_TIMEOUT; }
    buf += tmp;
    header_len = tmp;
    // read from the 2nd byte, determined remain length encoded in variable bytes.
    for(idx=0; idx<MQTT_PKT_MAX_BYTES_REMAIN_LEN ; idx++) {
        tmp = mqttPktLowLvlRead( mctx, &buf[idx], 0x1 );
        if(tmp == 0x0) { return MQTT_RETURN_ERROR_TIMEOUT; }
        if((header->remain_len[idx] & continuation_bit) == 0x0) {
            break;
        }
    } // end of for-loop
    if(idx == MQTT_PKT_MAX_BYTES_REMAIN_LEN) {
        return  MQTT_RETURN_ERROR_MALFORMED_DATA; // Error (Malformed Remaining Len)
    }
    // extract remaining length first
    header_len += idx + 1;
    *copied_len = header_len;
    // the return of the function call below should be the same as idx + 1
    buf        += mqttDecodeVarBytes( &header->remain_len[0], &remain_len );
    if(buf_max_len < (remain_len + header_len)) {
        // current Rx buffer cannot hold entire packet, chances are that received application
        // message exceeds the limit of Rx buffer ,so we return error instead.
        return  MQTT_RETURN_ERROR_OUT_OF_BUFFER;
    }
    do {  // read remaining part
        tmp = mqttPktLowLvlRead( mctx, buf, remain_len );
        if(tmp == 0x0) { return MQTT_RETURN_ERROR_TIMEOUT; }
        *copied_len += tmp;
        remain_len  -= tmp;
        buf         += tmp;
    } // end of loop
    while(remain_len > 0); 
    
    return  MQTT_RETURN_SUCCESS;
} // end of mqttPktRead






int  mqttPktWrite( struct __mqttCtx *mctx, byte *buf, word32 buf_len )
{
    if((mctx == NULL) || (buf == NULL)) { 
        return MQTT_RETURN_ERROR_BAD_ARG;
    }
    word32  copied_len = 0;
    do {
        copied_len  =  mqttPktLowLvlWrite( mctx, buf, buf_len );
        buf        +=  copied_len;
        buf_len    -=  copied_len;
    } // end of loop
    while( buf_len > 0 ); 
    return  MQTT_RETURN_SUCCESS;
} // end of mqttPktWrite





int mqttDecodePkt( struct __mqttCtx *mctx, byte *buf, word32 buf_len, void *p_decode, word16 *recv_pkt_id  )
{
    const   mqttPktFxHead_t  *header = (mqttPktFxHead_t *) buf;
    mqttCtrlPktType  cmdtype = MQTT_CTRL_PKT_TYPE_GET(header->type_flgs);

    switch (cmdtype)
    {
        case MQTT_PACKET_TYPE_CONNACK      : 
            mqttDecodePktConnack( buf, buf_len, (mqttPktHeadConnack_t *)p_decode );
            if(p_decode != NULL) {
                //  free the properties here if we take some space on 
                //  clientPropStack[...] while decoding the packet
                mqttPropertyDel( ((mqttPktHeadConnack_t *)p_decode)->props );
            }
            break;  
        case MQTT_PACKET_TYPE_PUBLISH      :
        {
            mqttDecodePktPublish( buf, buf_len, (mqttMsg_t *)p_decode );
            mqttQoS qos =  ((mqttMsg_t *)p_decode)->qos;
            *recv_pkt_id = ((mqttMsg_t *)p_decode)->packet_id;
            if(p_decode != NULL) {
                mqttPropertyDel( ((mqttMsg_t *)p_decode)->props );
            }
            if(qos > MQTT_QOS_0) {
                cmdtype = ( qos == MQTT_QOS_1 ? MQTT_PACKET_TYPE_PUBACK: MQTT_PACKET_TYPE_PUBRECV );
                mqttPktPubResp_t *pub_resp = &mctx->send_pkt.pub_resp ;
                ESP_MEMSET( pub_resp, 0x00, sizeof(mqttPktPubResp_t) );
                pub_resp->packet_id   = *recv_pkt_id ;
                pub_resp->reason_code = MQTT_REASON_SUCCESS;
                mqttSendPubResp( mctx, cmdtype );
            }
            break; 
        }
        case MQTT_PACKET_TYPE_PUBACK   :   
        case MQTT_PACKET_TYPE_PUBRECV  :   
        case MQTT_PACKET_TYPE_PUBREL   :   
        case MQTT_PACKET_TYPE_PUBCOMP  :  
        {
            mqttDecodePktPubResp( buf, buf_len, (mqttPktPubResp_t *)p_decode, cmdtype );
            *recv_pkt_id = ((mqttPktPubResp_t *)p_decode)->packet_id ;
            if(p_decode != NULL) {
                //  free the properties here if we take some space on 
                //  clientPropStack[...] while decoding the packet
                mqttPropertyDel( ((mqttPktPubResp_t  *)p_decode)->props );
            }
            // TODO: test publish response packet when QoS = 2
            if((cmdtype==MQTT_PACKET_TYPE_PUBRECV) || (cmdtype==MQTT_PACKET_TYPE_PUBREL)) {
                cmdtype = cmdtype + 1 ;
                mqttPktPubResp_t *pub_resp = &mctx->send_pkt.pub_resp ;
                ESP_MEMSET( pub_resp, 0x00, sizeof(mqttPktPubResp_t) );
                pub_resp->packet_id   = *recv_pkt_id ;
                pub_resp->reason_code = MQTT_REASON_SUCCESS;
                mqttSendPubResp( mctx, cmdtype );
            }
            break; 
        }
        case MQTT_PACKET_TYPE_SUBACK       :  
            mqttDecodePktSuback( buf, buf_len, (mqttPktSuback_t *)p_decode );
            *recv_pkt_id = ((mqttPktSuback_t *)p_decode)->packet_id ;
            if(p_decode != NULL) {
                mqttPropertyDel( ((mqttPktSuback_t *)p_decode)->props );
            }
            break; 
        case MQTT_PACKET_TYPE_UNSUBACK     :  
            mqttDecodePktUnsuback( buf, buf_len, (mqttPktUnsuback_t *)p_decode );
            *recv_pkt_id = ((mqttPktUnsuback_t *)p_decode)->packet_id ;
            if(p_decode != NULL) {
                mqttPropertyDel( ((mqttPktUnsuback_t *)p_decode)->props );
            }
            break; 
        case MQTT_PACKET_TYPE_PINGREQ      :  break; 
        case MQTT_PACKET_TYPE_PINGRESP     :  break; 
        case MQTT_PACKET_TYPE_DISCONNECT   :  break; 
        case MQTT_PACKET_TYPE_AUTH         :  break; 
        default:
            break;
    } // end of switch-case statement
    return  MQTT_RETURN_SUCCESS;
} // end of mqttDecodePkt



