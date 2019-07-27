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
    while( value > 0 ) {
        enc_val   = value & 0x7f;
        value   >>= 7;
        len++;
        if( value > 0 ){
            enc_val |= continuation_bit;
        }
        if(buf != NULL) {
            buf[len] = enc_val;
        }
    } // end of while-loop
    return  len;
} // end of mqttEncodeVarBytes




word32 mqttDecodeVarBytes( byte *buf, word32 *value )
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
        *pstr  = &buf[len] ;
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
                len = mqttEncodeStr( buf, curr_prop->body.str.data,  curr_prop->body.str.len );
                break;
            case MQTT_DATA_TYPE_STRING_PAIR  :
                len  = mqttEncodeStr( buf, curr_prop->body.strpair[0].data,  curr_prop->body.strpair[0].len );
                if(buf != NULL){ buf  += len; }
                total_len += len;
                len += mqttEncodeStr( buf, curr_prop->body.strpair[1].data,  curr_prop->body.strpair[1].len );
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
        len           = mqttDecodeVarBytes( buf, (word32 *)&curr_prop->type );
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
                len = mqttDecodeVarBytes( buf, &curr_prop->body.u32 );
                break;
            case MQTT_DATA_TYPE_BINARY       : 
            case MQTT_DATA_TYPE_STRING       :
                len = mqttDecodeStr( buf, &curr_prop->body.str.data,  &curr_prop->body.str.len );
                break;
            case MQTT_DATA_TYPE_STRING_PAIR  :
                len  = mqttDecodeStr( &buf[0]  , &curr_prop->body.strpair[0].data,  &curr_prop->body.strpair[0].len );
                len += mqttDecodeStr( &buf[len], &curr_prop->body.strpair[1].data,  &curr_prop->body.strpair[1].len );
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
    MQTT_CTRL_PKT_TYPE_SET( header->type_flgs, cmdtype );
    header->type_flgs |= (duplicate & 0x1) << 3 ;
    header->type_flgs |= (qos       & 0x3) << 1 ;
    header->type_flgs |= (retain    & 0x1) << 0 ;
    len += mqttEncodeVarBytes( &header->remain_len[0], remain_len );
    len  = (len == 0 ? 2 : len+1); // TODO: test this part
    return  len;
} // end of  mqttEncodeFxHeader



static word32 mqttDecodeFxHeader( byte *rx_buf, word32 rx_buf_len, word32 *remain_len, 
                                  mqttCtrlPktType cmdtype, byte *retain, byte *qos, byte *duplicate )
{
    const    mqttPktFxHead_t  *header = (mqttPktFxHead_t *) rx_buf;
    word32   len = 0;
    word32   _remain_len ;

    if(MQTT_CTRL_PKT_TYPE_GET(header->type_flgs) != type) {
        return len;
    }
    if(retain != NULL) {    *retain    = (header->type_flgs & 0x1) >> 0; }
    if(qos != NULL) {       *qos       = (header->type_flgs & 0x3) >> 1; }
    if(duplicate != NULL) { *duplicate = (header->type_flgs & 0x1) >> 3; }
    len += 1;
    len += mqttDecodeVarBytes( &header->remain_len[0], &_remain_len );
    if(remain_len != NULL) { *remain_len = _remain_len; }
    return  len;
} // end of mqttDecodeFxHeader



word32  mqttEncodePktConnect( byte *tx_buf, word32 tx_buf_len, mqttCtx_t* mctx )
{
    if((mctx == NULL) || (tx_buf == NULL) || (tx_buf_len == 0)) { 
        return MQTT_RETURN_ERROR_BAD_ARG;
    }
    word32   fx_head_len = 0;
    word32   remain_len  = 0;
    word32   props_len   = 0;
    mqttPktHeadConnect_t  var_head = {{0, MQTT_CONN_PROTOCOL_NAME_LEN}, {'M','Q','T','T'}, 0, 0, 0, 0}; 
    byte    *curr_buf_pos ;

    // size of variable header in CONNECT packet, should be 10 bytes in MQTT v5.0
    remain_len += sizeof(mqttPktHeadConnect_t); 

    // size of all properties in the header of this CONNECT packet
    props_len   =  mqttEncodeProps( NULL, mctx->props );
    remain_len +=  props_len;

    // number of variable bytes to store "property length"
    remain_len += mqttEncodeVarBytes(NULL, props_len);

    // size of each element in CONNECT payload section
    // TODO: implement will property, will topic, and will payload
    remain_len += MQTT_DSIZE_STR_LEN + mctx->client_id.len ; 
    if(mctx->username.data != NULL) {
        remain_len += MQTT_DSIZE_STR_LEN + mctx->username.len ; 
    }
    if(mctx->password.data != NULL) {
        remain_len += MQTT_DSIZE_STR_LEN + mctx->password.len ; 
    }
    // build fixed header of CONNECT packet 
    fx_head_len = mqttEncodeFxHeader( tx_buf, tx_buf_len, remain_len, 
                                      MQTT_PACKET_TYPE_CONNECT,  0, 0, 0 );

    curr_buf_pos = &tx_buf[fx_head_len];
    // copy bytes to variable header
    var_head.protocol_lvl = mctx->protocol_lvl; 
    // TODO: implement will property, will topic, and will payload
    if(mctx->clean_session != 0) {
        var_head.flags |=  MQTT_CONNECT_FLG_CLEAN_START; 
    }
    if(mctx->username.data != NULL) {
        var_head.flags |= MQTT_CONNECT_FLG_USERNAME ; 
    }
    if(mctx->password.data != NULL) {
        var_head.flags |= MQTT_CONNECT_FLG_PASSWORD ; 
    }
    mqttEncodeWord16( (byte *)&var_head.keep_alive, mctx->keep_alive_sec );
    ESP_MEMCPY( curr_buf_pos, (byte *)&var_head, sizeof(mqttPktHeadConnect_t) );
    curr_buf_pos += sizeof(mqttPktHeadConnect_t);

    // copy all properties to buffer
    curr_buf_pos += mqttEncodeVarBytes( curr_buf_pos, props_len );
    curr_buf_pos += mqttEncodeProps( curr_buf_pos, mctx->props );

    // copy all elements of the payload to buffer
    curr_buf_pos += mqttEncodeStr( curr_buf_pos, mctx->client_id.data,  mctx->client_id.len );
    // TODO: implement will property, will topic, and will payload
    if(mctx->username.data != NULL) {
        curr_buf_pos += mqttEncodeStr( curr_buf_pos, mctx->username.data,  mctx->username.len );
    }
    else {
        // [Note]
        // A server may allow a client to provide empty clientID (has length of zero byte),
        // server must assign unique ID to such CONNECT packet (with zero-byte clientID), and then
        // return CONNACK packet with the property "Assigned Client Identifier" back to client .
        curr_buf_pos += mqttEncodeWord16( curr_buf_pos, (word16)0 );
    }
    if(mctx->password.data != NULL) {
        curr_buf_pos += mqttEncodeStr( curr_buf_pos, mctx->password.data,  mctx->password.len );
    }

    return  (remain_len + fx_head_len);
} // end of mqttEncodePktConnect




word32  mqttDecodePktConnack( byte *rx_buf, word32 rx_buf_len,  mqttPktHeadConnack_t *connack )
{
    word32   fx_head_len = 0;
    word32   remain_len  = 0;
    word32   props_len   = 0;
    byte    *curr_buf_pos ;
    fx_head_len = mqttDecodeFxHeader( rx_buf, rx_buf_len, &remain_len, 
                                      MQTT_PACKET_TYPE_CONNACK, NULL, NULL, NULL );
    curr_buf_pos = &rx_buf[fx_head_len];
    connack->flags        = *curr_buf_pos++;
    connack->reason_code  = *curr_buf_pos++;
    // copy all properties from buffer
    curr_buf_pos += mqttDecodeVarBytes( curr_buf_pos, &props_len );
    if(props_len > 0) {
        curr_buf_pos += mqttDecodeProps( curr_buf_pos, &connack->props, props_len );
    }
    return  (fx_head_len + remain_len);
} // end of mqttDecodePktConnack



word32  mqttEncodePktDisconn( byte *tx_buf, word32 tx_buf_len, mqttPktDisconn_t *disconn )
{
} // end of mqttEncodePktDisconn



int  mqttPktRead( mqttCtx_t *mctx, byte *buf, word32 buf_max_len, word32 *copied_len )
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
 
    // read the first byte.
    mqttPktLowLvlRead( mctx, buf, 0x1 );
    buf += 1;
    header_len = 1;
    // read from the 2nd byte, determined remain length encoded in variable bytes.
    for(idx=0; idx<MQTT_PKT_MAX_BYTES_REMAIN_LEN ; idx++) {
        mqttPktLowLvlRead( mctx, &buf[idx], 0x1 );
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
        // current Rx buffer cannot hold entire packet, so we return error instead.
        return  MQTT_RETURN_ERROR_OUT_OF_BUFFER;
    }
    do {  // read remaining part
        tmp = mqttPktLowLvlRead( mctx, buf, remain_len );
        *copied_len += tmp;
        remain_len  -= tmp;
        buf         += tmp;
    } // end of loop
    while(remain_len > 0); 
    
    return  MQTT_RETURN_SUCCESS;
} // end of mqttPktRead






int  mqttPktWrite( mqttCtx_t *mctx, byte *buf, word32 buf_len )
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




int mqttDecodePkt( mqttCtx_t *mctx, byte *buf, word32 buf_len, void *p_decode )
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
        case MQTT_PACKET_TYPE_PUBLISH      : mqttDecodePkt123; break; 
        case MQTT_PACKET_TYPE_PUBACK       : mqttDecodePkt123; break; 
        case MQTT_PACKET_TYPE_PUBRECV      : mqttDecodePkt123; break; 
        case MQTT_PACKET_TYPE_PUBREL       : mqttDecodePkt123; break; 
        case MQTT_PACKET_TYPE_PUBCOMP      : mqttDecodePkt123; break; 
        case MQTT_PACKET_TYPE_SUBACK       : mqttDecodePkt123; break; 
        case MQTT_PACKET_TYPE_UNSUBACK     : mqttDecodePkt123; break; 
        case MQTT_PACKET_TYPE_PINGREQ      : mqttDecodePkt123; break; 
        case MQTT_PACKET_TYPE_PINGRESP     : mqttDecodePkt123; break; 
        case MQTT_PACKET_TYPE_DISCONNECT   : mqttDecodePkt123; break; 
        case MQTT_PACKET_TYPE_AUTH         : mqttDecodePkt123; break; 
        default:
            break;
    } // end of switch-case statement
    return  MQTT_RETURN_SUCCESS;
} // end of mqttDecodePkt



