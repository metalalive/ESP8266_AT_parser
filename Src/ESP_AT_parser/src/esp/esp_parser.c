#include "esp/esp.h"
#include "esp/esp_private.h"


extern espGlbl_t espGlobal;




static espRes_t  eESPparseVersion( uint8_t **curr_chr_pp, espFwVer_t *ver )
{
    ver->major = iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL ); 
    ver->minor = iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL ); 
    ver->patch = iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL ); 
    // TODO: minimum version check
    return  espOK;
} // end of eESPparseVersion




static void   vESPparseIPfromStr( uint8_t **curr_chr_pp, espIp_t* ip )
{
    ip->ip[0] = iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL ); 
    ip->ip[1] = iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL ); 
    ip->ip[2] = iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL ); 
    ip->ip[3] = iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL ); 
} // end of vESPparseIPfromStr





static void  vESPparseMACfromStr( uint8_t **curr_chr_pp, espMac_t* mac )
{
    mac->mac[0] = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_HEX ); 
    mac->mac[1] = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_HEX ); 
    mac->mac[2] = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_HEX ); 
    mac->mac[3] = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_HEX ); 
    mac->mac[4] = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_HEX ); 
    mac->mac[5] = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_HEX ); 
} // end of vESPparseMACfromStr






// TODO: verify this function after ESP device connected to AP & get IP address
static espRes_t   eESPparseCIPstatus( uint8_t **curr_chr_pp, espDev_t *dev )
{
    espRes_t response = espOK;
    uint8_t conn_id ;
    if (strncmp(*curr_chr_pp, "+CIPSTATUS", 10) == 0) {
        *curr_chr_pp += 10;
        conn_id  = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL );
        dev->active_conns |= (1 << conn_id); 
        *curr_chr_pp += 3; // skip the type ",TCP" or ".UDP"
        vESPparseIPfromStr( curr_chr_pp, &(dev->conns[conn_id].remote_ip) );
        dev->conns[conn_id].remote_port      = (espPort_t) iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL );
        dev->conns[conn_id].local_port       = (espPort_t) iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL );
        dev->conns[conn_id].status.flg.client  = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL );
    }
    else if (strncmp(*curr_chr_pp, "STATUS:", 7) == 0) {
        // backup previous active connections
        dev->active_conns_last = dev->active_conns; 
        dev->active_conns      = 0; 
    }
    return  response;
} // end of eESPparseCIPstatus




static void  vESPparseAPstaIP( uint8_t **curr_chr_pp, espMsg_t* msg )
{
    if(strncmp(*curr_chr_pp, "ip:", 3) == 0) {
        *curr_chr_pp += 3;
        vESPparseIPfromStr( curr_chr_pp, msg->body.sta_ap_getip.ip );
    }
    else if(strncmp(*curr_chr_pp, "gateway:", 8) == 0) {
        *curr_chr_pp += 8;
        vESPparseIPfromStr( curr_chr_pp, msg->body.sta_ap_getip.gw );
    }
    else if(strncmp(*curr_chr_pp, "netmask:", 8) == 0) {
        *curr_chr_pp += 8;
        vESPparseIPfromStr( curr_chr_pp, msg->body.sta_ap_getip.nm );
    }
} // end of vESPparseAPstaIP



static void vESPparseCIFSR( uint8_t **curr_chr_pp, espMsg_t* msg )
{
    if(strncmp(*curr_chr_pp, "STAIP", 5) == 0) {
        *curr_chr_pp += 5;
        vESPparseIPfromStr( curr_chr_pp, msg->body.local_ip_mac.sta_ip );
    }
    else if(strncmp(*curr_chr_pp, "STAMAC", 6) == 0) {
        *curr_chr_pp += 6;
        vESPparseMACfromStr( curr_chr_pp, msg->body.local_ip_mac.sta_mac );
    }
    else if(strncmp(*curr_chr_pp, "APIP", 4) == 0) {
        *curr_chr_pp += 4;
        vESPparseIPfromStr( curr_chr_pp, msg->body.local_ip_mac.ap_ip );
    }
    else if(strncmp(*curr_chr_pp, "APMAC", 5) == 0) {
        *curr_chr_pp += 5;
        vESPparseMACfromStr( curr_chr_pp, msg->body.local_ip_mac.ap_mac );
    }
} // end of vESPparseCIFSR



static espRes_t  eESPparseFoundAP( uint8_t **curr_chr_pp, espMsg_t* msg )
{
    espRes_t  response = espOK;
    uint16_t  idx ;
    uint16_t  max_num ;
    uint16_t  num_chrs_copied ;
    espAP_t  *aps  =  NULL;

    idx     = *(msg->body.ap_list.num_ap_found);
    max_num = msg->body.ap_list.apslen;
    aps     = &( msg->body.ap_list.aps[idx] );
    // get rid of useless characters at the beginning
    *curr_chr_pp  = strstr( *curr_chr_pp, "+CWLAP:");
    //// if( *curr_chr_pp == NULL ) {
    ////     response = espERR;
    ////     return  response;
    //// }
    *curr_chr_pp += 7; // skip the beginning part +CWLAP:xxx...
    // get encryption method
    aps->ecn    = (espEncrypt_t) iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL );
    *curr_chr_pp += 1; // skip dot char, 
    if(**curr_chr_pp == ESP_ASCII_DOUBLE_QUOTE) {
        *curr_chr_pp += 1; // skip double quote
        num_chrs_copied = uESPparseStrUntilToken( &(aps->ssid),  *curr_chr_pp, ESP_CFG_MAX_SSID_LEN, ESP_ASCII_DOUBLE_QUOTE );
        *curr_chr_pp += 1; // skip double quote
    }
    else{
        num_chrs_copied = uESPparseStrUntilToken( &(aps->ssid),  *curr_chr_pp, ESP_CFG_MAX_SSID_LEN, ESP_ASCII_DOT );
    }
    *curr_chr_pp += num_chrs_copied; 
    aps->rssi  = (int16_t) iESPparseFirstNumFromStr( curr_chr_pp, ESP_DIGIT_BASE_DECIMAL ); // Received signal strength indicator 

    if(**curr_chr_pp == ESP_ASCII_DOUBLE_QUOTE) {
        *curr_chr_pp += 1; // skip double quote
        vESPparseMACfromStr( curr_chr_pp, &(aps->mac) );
        *curr_chr_pp += 1; // skip double quote
    }
    else {
        vESPparseMACfromStr( curr_chr_pp, &(aps->mac) );
    }
    aps->ch      = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp , ESP_DIGIT_BASE_DECIMAL ); // WiFi channel used on access point 
    aps->offset  = (int8_t)  iESPparseFirstNumFromStr( curr_chr_pp , ESP_DIGIT_BASE_DECIMAL ); // Access point offset 
    aps->cal     = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp , ESP_DIGIT_BASE_DECIMAL ); // Calibration value 
    *curr_chr_pp  = strstr( *curr_chr_pp, ",") + 1; // skip pwc
    *curr_chr_pp  = strstr( *curr_chr_pp, ",") + 1; // skip gc
    aps->bgn     = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp , ESP_DIGIT_BASE_DECIMAL ); // Information about 802.11[b|g|n] support 
    aps->wps     = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp , ESP_DIGIT_BASE_DECIMAL ); // Status if WPS function is supported 
    idx++;
    *(msg->body.ap_list.num_ap_found) = idx;
    // if the given array msg->body.ap_list.aps[] is full, then we skip the AP we find subsequently.
    if( idx == max_num ) { response = espOKIGNOREMORE; }
    return  response;
} // end of eESPparseFoundAP





static void  vESPparseJoinedAP( uint8_t **curr_chr_pp, espMsg_t* msg )
{
    uint16_t  num_chrs_copied ;
    espStaInfoAP_t  *info;

    info = msg->body.sta_info_ap.info;
    *curr_chr_pp  = strstr( *curr_chr_pp, "+CWJAP_CUR:" ) + 11;
    if(**curr_chr_pp == ESP_ASCII_DOUBLE_QUOTE) {
        *curr_chr_pp += 1; // skip double quote
        num_chrs_copied = uESPparseStrUntilToken( &(info->ssid), *curr_chr_pp, ESP_CFG_MAX_SSID_LEN, ESP_ASCII_DOUBLE_QUOTE );
        *curr_chr_pp += 1; // skip double quote
    }
    else {
        num_chrs_copied = uESPparseStrUntilToken( &(info->ssid), *curr_chr_pp, ESP_CFG_MAX_SSID_LEN, ESP_ASCII_DOT );
    }
    *curr_chr_pp += num_chrs_copied; 
    *curr_chr_pp += 1; // skip dot char, 
    if(**curr_chr_pp == ESP_ASCII_DOUBLE_QUOTE) {
        *curr_chr_pp += 1; // skip double quote
        vESPparseMACfromStr( curr_chr_pp, &(info->mac) );
        *curr_chr_pp += 1; // skip double quote
    }
    else{
        vESPparseMACfromStr( curr_chr_pp, &(info->mac) );
    }
    info->ch    = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp , ESP_DIGIT_BASE_DECIMAL ) ;
    info->rssi  = (int16_t) iESPparseFirstNumFromStr( curr_chr_pp , ESP_DIGIT_BASE_DECIMAL ) ;
} // end of vESPparseJoinedAP





static void  vESPparseJoinStatus( espNetAttr_t *d , espStaConnStatus_t status )
{
    d->is_connected = status;
    // ip adress must be update for each time ESP device connected to some AP
    d->has_ip       = 0; 
} // end of vESPparseJoinStatus




static espRes_t   eESPparseStaJoinResult( uint8_t **curr_chr_pp, espMsg_t* msg )
{
    espRes_t  response = espERR;
    uint8_t   error_num = 0;
    if(strncmp( *curr_chr_pp, "+CWJAP_", 7 ) == 0)
    { // when CPU gets herem that means something wrong happened when connecting the AP
        *curr_chr_pp  = 11;
        error_num     = (uint8_t) iESPparseFirstNumFromStr( curr_chr_pp , ESP_DIGIT_BASE_DECIMAL ) ;
        msg->body.sta_join.error_num = error_num;
        switch(error_num)
        {
            case 1:   response = espERRCONNTIMEOUT ; break;
            case 2:   response = espERRPASS; break;
            case 3:   response = espERRNOAP; break;
            case 4:   response = espERRCONNFAIL; break;
            default:  break;
        }
        return response;
    }
    if(strncmp( *curr_chr_pp, "WIFI CONNECTED", 14 ) == 0)
    {
        response = espOK;
        vESPparseJoinStatus( &(espGlobal.dev.sta), ESP_STATION_CONNECTED );
    }
    return response;
} // end of  eESPparseStaJoinResult 





static espRes_t   eESPparseStaQuitResult( uint8_t **curr_chr_pp )
{ // TODO: figure out why ESP quits from AP without response.
    espRes_t  response = espERR;
    if(strncmp( *curr_chr_pp, "WIFI DISCONNECT", 15 ) == 0)
    {
        response = espOK;
        vESPparseJoinStatus( &(espGlobal.dev.sta), ESP_STATION_DISCONNECTED );
    }
    return response;
} // end of eESPparseStaQuitResult





static void vESPparsePing( uint8_t **curr_chr_pp, espMsg_t* msg )
{
    uint32_t resptime = 0;
    if( **curr_chr_pp == '+' ) {
        resptime = (uint32_t) iESPparseFirstNumFromStr( curr_chr_pp , ESP_DIGIT_BASE_DECIMAL ) ;
        *(msg->body.tcpip_ping.resptime) = resptime;
    }
} // end of vESPparsePing





static  void   vESPparseTCPmuxConn( espMsg_t* msg )
{
    espGlobal.status.flg.mux_conn = msg->body.tcpip_attri.mux ;
} // end of vESPparseTCPmuxConn




static void vESPparseTCPserverEn( espMsg_t* msg, uint8_t is_ok )
{
    if(is_ok == 1) {
        espGlobal.evt_server =  msg->body.tcpip_server.cb ;
    }
} // end of vESPparseTCPserverEn




static  espRes_t   eESPparseConnSend( const uint8_t *curr_chr_p, uint8_t *isEndOfATresp )
{
    espRes_t response = espINPROG;
    if(strncmp( curr_chr_p, "SEND OK" ESP_CHR_CR ESP_CHR_LF , 9) == 0) {
        response = espOK;
        *isEndOfATresp = 1;
    }
    else if(strncmp( curr_chr_p, "SEND FAIL" ESP_CHR_CR ESP_CHR_LF , 11) == 0) {
        response = espERR;
        *isEndOfATresp = 1;
    }
    return  response; 
} // end of  eESPparseConnSend





void   vESPparseRecvATrespLine( const uint8_t *data_line_buf, uint16_t buf_idx, uint8_t *isEndOfATresp )
{
    uint8_t   is_ok  = 0;
    uint8_t   is_err = 0;
    uint8_t   is_rdy = 0;
    uint8_t  *curr_chr_p   = data_line_buf;
    espMsg_t *msg          = espGlobal.msg;
    if(msg == NULL){ return; }

    espCmd_t  curr_cmd = GET_CURR_CMD( msg );

    // return immediately if there are only CR & LF characters in this line buffer.
    if( strncmp( data_line_buf, ESP_CHR_CR ESP_CHR_LF , buf_idx ) == 0 ) {
        return;
    }
    // check end of response line we've received for the AT command.
    is_ok  = ( strncmp( data_line_buf, "OK" ESP_CHR_CR ESP_CHR_LF , buf_idx ) == 0 ? 1 : 0 );
    if( is_ok == 0 ) {
        is_err  = ( strncmp( data_line_buf, "ERROR" ESP_CHR_CR ESP_CHR_LF , buf_idx ) == 0 ? 1 : 0 );
        if( is_err == 0 ) {
            is_rdy = ( strncmp( data_line_buf, "ready" ESP_CHR_CR ESP_CHR_LF , buf_idx ) == 0 ? 1 : 0 );
        }
        else{ // if is_err is NOT equal to zero
            switch( msg->res ) 
            { // few status below can cover espERR, they mean error reported by API function with extra information.
                case espBUSY   : 
                case espERRMEM :       
                case espERRNOIP                 : 
                case espERRNOAVAILCONN          : 
                case espERRCONNTIMEOUT          : 
                case espERRPASS                 : 
                case espERRNOAP                 : 
                case espERRCONNFAIL             : 
                case espERRWIFINOTCONNECTED     : 
                case espERRNODEVICE             : 
                    break;
                default:
                    msg->res = espERR;
                    break;
            } // end of switch-case statement
        }
    }
    else{ // if is_ok is NOT equal to zero
        // check whether response of the message should be modified.
        // few status below can cover espOK, they mean API function works OK with extra information.
        switch( msg->res ) {
            case espOKIGNOREMORE:
                break;
            default:
                msg->res = espOK;
                break;
        }
    } // end of if-statement (check ok, error, fail characters)

    if((is_ok | is_err | is_rdy) != 0 ){
        *isEndOfATresp = 1; 
    }

    // for some AT commands we can safely ignore the subsequent received string from ESP device
    // then skip this parsing function. TODO: find better position to place following line of code
    if( msg->res == espOKIGNOREMORE ){  return; }

    // check if ESP device is receiving IPD data or response string of AT commands. 
    switch(curr_cmd)
    {
        case ESP_CMD_GMR:
            // there might be 2 types of response : starts with 'AT version' ,
            // or starts with 'SDK version'
            if(strncmp(curr_chr_p, "AT version", 10) == 0) {
                curr_chr_p += 10;
                eESPparseVersion( &curr_chr_p, &espGlobal.dev.version_at );
                // we can confirm that the ESP device is present & ready ONLY when we
                // get correct response of AT+GMR command.
                espGlobal.status.flg.dev_present = 1;
            }
            else if(strncmp(curr_chr_p, "SDK version", 11) == 0) {
                curr_chr_p += 11;
                eESPparseVersion( &curr_chr_p, &espGlobal.dev.version_sdk );
            }
            break;
#if (ESP_CFG_MODE_STATION != 0)
        case ESP_CMD_WIFI_CWLAP :
            msg->res = eESPparseFoundAP( &curr_chr_p, msg );
            break;
        case ESP_CMD_WIFI_CWJAP_GET:
            vESPparseJoinedAP( &curr_chr_p, msg );
            break;
        case ESP_CMD_WIFI_CWJAP :
            msg->res = eESPparseStaJoinResult( &curr_chr_p, msg );
            *isEndOfATresp = 1; 
            break;
        case ESP_CMD_WIFI_CWQAP :
            msg->res = eESPparseStaQuitResult(&curr_chr_p);
            if(msg->res == espOK) { *isEndOfATresp = 1; }
            break;
        case ESP_CMD_WIFI_CIPSTA_GET :
            curr_chr_p += 12; // skip first few characters +CIPSTA_xxx:
            vESPparseAPstaIP( &curr_chr_p, msg );
            break;
        case ESP_CMD_TCPIP_CIPMUX : // copy the setting to mux_conn flag in ESP global structure
            vESPparseTCPmuxConn( msg );
            break;
        case ESP_CMD_TCPIP_CIPSTATUS:
            eESPparseCIPstatus( &curr_chr_p, &espGlobal.dev );
            break;
#endif // ESP_CFG_MODE_STATION
        case ESP_CMD_TCPIP_CIPSERVER:
            vESPparseTCPserverEn( msg, is_ok );
            break;
        case ESP_CMD_WIFI_CWLIF:
            break;
#if ( ESP_CFG_PING != 0 )
        case ESP_CMD_TCPIP_PING:
            vESPparsePing( &curr_chr_p, msg );
            break;
#endif // ESP_CFG_PING 
        case ESP_CMD_TCPIP_CIFSR:
            curr_chr_p += 7; // skip the first 7 characters +CIFSR:
            vESPparseCIFSR( &curr_chr_p, msg );
            break;
        case ESP_CMD_TCPIP_CIPSEND:
            if(*isEndOfATresp == 0) {
                msg->res = eESPparseConnSend( curr_chr_p, isEndOfATresp ); 
            }
            break;
#if (ESP_CFG_MODE_ACCESS_POINT  != 0)
        case ESP_CMD_WIFI_CIPAP_GET:
            curr_chr_p += 11; // skip first few characters +CIPAP_xxx:
            vESPparseAPstaIP( &curr_chr_p, msg );
            break;
#endif // ESP_CFG_MODE_ACCESS_POINT  
        default:
            break;
    } // end of switch-case-statement

} // end of vESPparseRecvATrespLine







// current ESP8266 device supports up to 5 active connections at the same time,
// also in this ESP AT software, multiple connection mode is always enabled,
// so we can simply assume that ESP device only uses recv_data_line_buf[0] to represent the link ID used by ESP devive.
espRes_t  eESPparseNetConnStatus( const uint8_t *data_line_buf )
{
    uint8_t    *curr_chr_p  = data_line_buf;
    espConn_t  *c           = NULL;
    uint8_t     link_id     = 0 ;
    uint8_t     is_connect      = (strncmp(&curr_chr_p[1], ",CONNECT", 8) == 0) ? 1: 0;
    uint8_t     is_close        = (strncmp(&curr_chr_p[1], ",CLOSED",  7) == 0) ? 1: 0;
    uint8_t     is_connect_ext  = (strncmp(&curr_chr_p[0], "+LINK_CONN:",  11) == 0) ? 1: 0;

    if( is_connect==0 && is_close==0 && is_connect_ext==0 ){ 
        return espSKIP; 
    }
    if(is_connect_ext != 0) {
        curr_chr_p += 11;
        link_id = (uint8_t) iESPparseFirstNumFromStr( &curr_chr_p , ESP_DIGIT_BASE_DECIMAL ) ;
    }
    else {
        link_id = ESP_CHARTONUM( curr_chr_p[0] );
    }
    if(link_id >= ESP_CFG_MAX_CONNS) {
        return espERR; 
    }
    c = &espGlobal.dev.conns[ link_id ];
    if(is_connect != 0) 
    {
        c->status.flg.active = ESP_CONN_ESTABLISHED ;
        // check whether the new active connection acts as client (e.g. AT+CIPSTART, AT+CIPSEND)   
        // or a server (e.g. AT+CIPSERVER) on ESP device's side, then parse callback function 
        espMsg_t *msg = espGlobal.msg;
        // TODO: test following statement
        if(msg != NULL && GET_CURR_CMD(msg) == ESP_CMD_TCPIP_CIPSTART) {
            c->status.flg.client = 1;
        }
        if(c->status.flg.client == 0) { // the new connection acts as server
            c->cb  =  espGlobal.evt_server ;
        }
        else { // the new connection acts as client,
            c->cb  = msg->body.conn_start.cb;
        }
    }
    else if(is_connect_ext != 0) {
        // TODO: implement this part
    }
    else  if(is_close != 0) {
        // reset values of previous network connection (also clear active flag)
        ESP_MEMSET( c, 0x00, sizeof(espConn_t) );
    }
    return espOK; 
} // end of  eESPparseNetConnStatus






espRes_t   eESPparseIPDsetup( const uint8_t* metadata )
{
    uint8_t       link_id = 0;
    uint32_t      len     = 0;
    espIPD_t     *ipdp    = &espGlobal.dev.ipd;
    espConn_t    *c       = NULL;

    if(ipdp->read == 1) { return espBUSY ; }
    if(espGlobal.status.flg.mux_conn == ESP_TCP_MULTIPLE_CONNECTION)
    {
        link_id = (uint8_t) iESPparseFirstNumFromStr( &metadata , ESP_DIGIT_BASE_DECIMAL ) ;
        if(link_id >= ESP_CFG_MAX_CONNS) { return espERR; }
    }
    // looking for available espConn_t object, in order to store 
    c = &espGlobal.dev.conns[ link_id ];
    c->status.flg.data_received = 1;
    // get total length of current IPD data
    len  = (uint32_t) iESPparseFirstNumFromStr( &metadata , ESP_DIGIT_BASE_DECIMAL ) ;
    // get IP address / port number of remote sender
    vESPparseIPfromStr( &metadata, &(c->remote_ip) );
    c->remote_port = (espPort_t) iESPparseFirstNumFromStr( &metadata , ESP_DIGIT_BASE_DECIMAL );
    ESP_MEMCPY( &(ipdp->ip), &(c->remote_ip), sizeof(espIp_t) );
    ipdp->port       = c->remote_port;
    ipdp->tot_len    = len;
    ipdp->rem_len    = len;
    ipdp->conn       = c;
    ipdp->pbuf_head  = NULL;
    ipdp->read       = 1;
    return espOK;
} // end of eESPparseIPDsetup




espRes_t    eESPparseIPDcopyData( const uint8_t* data, uint32_t data_len )
{
    espRes_t    response    =  espINPROG;
    espIPD_t   *ipdp        = &espGlobal.dev.ipd;
    uint32_t    remain_len  =  ipdp->rem_len;
    uint32_t    copy_len    =  0;
    uint8_t     chain_cnt   =  0;
    espPbuf_t  *prev_p    =  NULL;
    espPbuf_t  *curr_p    =  NULL;
    // loop through all packet buffer & find out the last one
    curr_p = ipdp->pbuf_head;
    while( curr_p != NULL ) {
        prev_p = curr_p ;
        curr_p = curr_p->next;
        chain_cnt += 1;
    }
    // create new packet buffer to hold IPD
    copy_len =  ESP_MIN( data_len , remain_len );
    curr_p   =  (espPbuf_t *) pxESPpktBufCreate( copy_len );
    // then copy IP address, port number, and associated connection object to this packet buffer item
    ESP_MEMCPY( &(curr_p->ip), &(ipdp->ip), sizeof(espIp_t) );
    curr_p->port = ipdp->port ;
    curr_p->conn = ipdp->conn ;
    // append the new packet buffer to the last of the chain
    if(prev_p != NULL) {
        prev_p->next = curr_p ;
    }
    else {
        ipdp->pbuf_head = curr_p;
    }
    ipdp->pbuf_head->chain_len   = chain_cnt + 1;     
    // copy IPD data to packet buffer
    eESPpktBufCopy( curr_p, (void *)data ,copy_len );
    // re-calculate rest of data that hasn't been received 
    remain_len    =  (data_len > remain_len ? 0: remain_len - data_len); 
    ipdp->rem_len =  remain_len; 
    if(remain_len == 0) {
        // copy the pointer of packet buffer clain to connection object while it's ready.
        ipdp->conn->pbuf = ipdp->pbuf_head;
        response = espOK;
    }
    return response;
} // end of eESPparseIPDcopyData





espRes_t    eESPparseIPDreset( void )
{
    espIPD_t *ipdp = &espGlobal.dev.ipd;
    ESP_MEMSET( ipdp, 0x00, sizeof(espIPD_t) );
    ////ipdp->read  = 0;
    return espOK;
} // end of  eESPparseIPDreset





