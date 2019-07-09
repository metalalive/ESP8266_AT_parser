#include "esp/esp.h"
#include "esp/esp_private.h"

extern espGlbl_t espGlobal;



static void  AT_APPEND_CHR( uint8_t **_des_p, const uint8_t _chr ) 
{   
    if(ESP_ISVALIDASCII(_chr)) { 
        **_des_p = _chr; 
        (*_des_p)++ ; 
    }
}


static void  AT_APPEND_STR( uint8_t **_des_p, const uint8_t *_str, uint16_t _str_len )
{ 
    uint16_t  idx;
    for( idx=0; idx<_str_len ; idx++ ) {
        AT_APPEND_CHR( _des_p, _str[idx] );
    }
}



static void  vESPappendIPtoStr( uint8_t **curr_chr_pp, espIp_t* ip )
{
    const uint8_t ip_num_cnt = 4;
    uint32_t  num_chr_append = 0;
    uint8_t   idx = 0;

    AT_APPEND_CHR( curr_chr_pp, ESP_ASCII_DOUBLE_QUOTE ) ;
    for (idx=0; idx<ip_num_cnt; idx++) {
	num_chr_append = uiESPcvtNumToStr( *curr_chr_pp, ip->ip[idx], ESP_DIGIT_BASE_DECIMAL );
        *curr_chr_pp += num_chr_append;
        if(idx < (ip_num_cnt - 1)) {
            AT_APPEND_CHR( curr_chr_pp, ESP_ASCII_DOT ) ;
        }
    }
    AT_APPEND_CHR( curr_chr_pp, ESP_ASCII_DOUBLE_QUOTE ) ;
} // end of vESPappendIPtoStr




espRes_t    eESPinitATcmd( espMsg_t* msg )
{
    espRes_t response = espOK;
    uint8_t     cmd_str[0x40] = {'A', 'T', }; // automatically set 2 characters "AT" firstly.
    uint8_t    *cmd_str_p   = &cmd_str[2]; // Note: &arr[2] != (&arr) + 2 for some compilers
    uint16_t    cmd_str_len = 0;

    switch( GET_CURR_CMD(msg) ) 
    {
        case ESP_CMD_IDLE                    :  break;         
        case ESP_CMD_RESET                   :
            // clear all infomation in the structure espGlobal.dev
            ESP_MEMSET( &espGlobal.dev, 0x00, sizeof(espDev_t) );
            // perform low-level reset function to ESP device
            if( espGlobal.ll.reset_fn != NULL) {
                espGlobal.ll.reset_fn( ESP_HW_RST_ASSERT );
                vESPsysDelay( msg->body.reset.delay );
                espGlobal.ll.reset_fn( ESP_HW_RST_DEASSERT );
                // TODO: find way to determine delay after reset de-assertion (should be long enough)
                vESPsysDelay( 1000 );
                response = espOKNOCMDREQ;
                msg->res = response;
            }
            else {
                // TODO: should we set the delay after reset de-assertion if ESP device is reset by
                //       AT command.
                AT_APPEND_STR( &cmd_str_p, "+RST", 4); 
            }
            break;
            
        case ESP_CMD_ATE0                    :
                AT_APPEND_STR( &cmd_str_p, "E0", 2 ); 
                break;
        case ESP_CMD_ATE1                    :
                AT_APPEND_STR( &cmd_str_p, "E1", 2 ); 
                break;
        case ESP_CMD_GMR                     :
            AT_APPEND_STR( &cmd_str_p, "+GMR", 4 ); 
            break;  
        case ESP_CMD_GSLP                    :
            AT_APPEND_STR( &cmd_str_p, "+GSLP=", 6 ); 
            // TODO: remember to give sleeping time.
            break;            
        case ESP_CMD_RESTORE                 :
            AT_APPEND_STR( &cmd_str_p, "+RESTORE", 8 );
            break;            
        case ESP_CMD_UART                    :  break;         
        case ESP_CMD_SLEEP                   :  break;         
        case ESP_CMD_WAKEUPGPIO              :  break;         
        case ESP_CMD_RFPOWER                 :  break;         
        case ESP_CMD_RFVDD                   :  break;         
        case ESP_CMD_RFAUTOTRACE             :  break;         
        case ESP_CMD_SYSRAM                  :  break;         
        case ESP_CMD_SYSADC                  :  break;         
        case ESP_CMD_SYSIOSETCFG             :  break;         
        case ESP_CMD_SYSIOGETCFG             :  break;         
        case ESP_CMD_SYSGPIODIR              :  break;         
        case ESP_CMD_SYSGPIOWRITE            :  break;         
        case ESP_CMD_SYSGPIOREAD             :  break;         
        case ESP_CMD_SYSMSG                  :
            AT_APPEND_STR( &cmd_str_p, "+SYSMSG_CUR=", 12 );
	    AT_APPEND_CHR( &cmd_str_p, (msg->body.sysargs.ext_info_netconn==ESP_ENABLE ? '1': '0') ); 
            break; 
        case ESP_CMD_WIFI_CWLAPOPT           :  break;           

        case ESP_CMD_WIFI_CWMODE             :
	    AT_APPEND_STR( &cmd_str_p, (msg->body.wifi_mode.def==ESP_SETVALUE_SAVE ? "+CWMODE_DEF=" : "+CWMODE_CUR=") , 12 ); 
	    AT_APPEND_CHR( &cmd_str_p, ESP_NUMTOCHAR(msg->body.wifi_mode.mode) ); 
            break;            
#if (ESP_CFG_MODE_STATION != 0)
        case ESP_CMD_WIFI_CWJAP                  :
	    AT_APPEND_STR( &cmd_str_p, (msg->body.sta_join.def==ESP_SETVALUE_SAVE ? "+CWJAP_DEF=" : "+CWJAP_CUR=") , 11 );
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
	    AT_APPEND_STR( &cmd_str_p, msg->body.sta_join.name, msg->body.sta_join.name_len );
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_COMMA );
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
	    AT_APPEND_STR( &cmd_str_p, msg->body.sta_join.pass, msg->body.sta_join.pass_len );
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
            if(msg->body.sta_join.mac != NULL) {
	        AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_COMMA );
	        AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
	        AT_APPEND_STR( &cmd_str_p, msg->body.sta_join.mac , 17 ); //TODO: finish this part
	        AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
            }
            break; 
        case ESP_CMD_WIFI_CWJAP_GET              :
	    AT_APPEND_STR( &cmd_str_p, "+CWJAP_CUR?" , 11 );
            break;  
        case ESP_CMD_WIFI_CWQAP                  :
	    AT_APPEND_STR( &cmd_str_p, "+CWQAP", 6 );
            break;   
        case ESP_CMD_WIFI_CWLAP                  :
	    AT_APPEND_STR( &cmd_str_p, "+CWLAP", 6 );
            if( msg->body.ap_list.ssid != NULL ) {
	        AT_APPEND_CHR( &cmd_str_p, '=' );
	        AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
                AT_APPEND_STR( &cmd_str_p, msg->body.ap_list.ssid, msg->body.ap_list.ssid_len );
	        AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
            }
            break;   
        case ESP_CMD_WIFI_CIPSTAMAC_GET          :  break;   
        case ESP_CMD_WIFI_CIPSTAMAC_SET          :  break;   
        case ESP_CMD_WIFI_CIPSTA_GET             :
	    AT_APPEND_STR( &cmd_str_p,  "+CIPSTA_CUR?" , 12 ); 
            break;
        case ESP_CMD_WIFI_CIPSTA_SET             :
	    AT_APPEND_STR( &cmd_str_p, (msg->body.sta_ap_setip.def==ESP_SETVALUE_SAVE ? "+CIPSTA_DEF=" : "+CIPSTA_CUR=") , 12 ); 
             // TODO: finish the rest
            break;   
        case ESP_CMD_WIFI_CWAUTOCONN             :  break;   
#endif // ESP_CFG_MODE_STATION  

#if (ESP_CFG_MODE_ACCESS_POINT != 0)
        case ESP_CMD_WIFI_CWSAP_GET              :  break; 
        case ESP_CMD_WIFI_CWSAP_SET              :  break; 
        case ESP_CMD_WIFI_CIPAPMAC_GET           :  break; 
        case ESP_CMD_WIFI_CIPAPMAC_SET           :  break; 
        case ESP_CMD_WIFI_CIPAP_GET              :
	    AT_APPEND_STR( &cmd_str_p, "+CIPAP_CUR?", 11 ); 
            break; 
        case ESP_CMD_WIFI_CIPAP_SET              :
	    AT_APPEND_STR( &cmd_str_p, (msg->body.sta_ap_setip.def==ESP_SETVALUE_SAVE ? "+CIPAP_DEF=" : "+CIPAP_CUR=") , 11 ); 
            vESPappendIPtoStr( &cmd_str_p, msg->body.sta_ap_setip.ip );
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_COMMA ); 
            vESPappendIPtoStr( &cmd_str_p, msg->body.sta_ap_setip.gw );
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_COMMA ); 
            vESPappendIPtoStr( &cmd_str_p, msg->body.sta_ap_setip.nm );
            break; 
        case ESP_CMD_WIFI_CWLIF                  :  break; 
#endif /* ESP_CFG_MODE_ACCESS_POINT */ 

#if (ESP_CFG_WPS != 0)
        case ESP_CMD_WIFI_WPS                      :  break;       
#endif /* ESP_CFG_WPS  */ 

#if (ESP_CFG_MDNS != 0)
        case ESP_CMD_WIFI_MDNS                     :  break;       
#endif /* ESP_CFG_MDNS  */ 

#if (ESP_CFG_HOSTNAME != 0)
        case ESP_CMD_WIFI_CWHOSTNAME_SET           :  break;       
        case ESP_CMD_WIFI_CWHOSTNAME_GET           :  break;       
#endif /* ESP_CFG_HOSTNAME  */ 

    /* TCP/IP related commands */ 
#if  (ESP_CFG_DNS != 0)  
        case ESP_CMD_TCPIP_CIPDOMAIN               :  break;       
        case ESP_CMD_TCPIP_CIPDNS_SET              :  break;       
        case ESP_CMD_TCPIP_CIPDNS_GET              :  break;       
#endif /* ESP_CFG_DNS  */ 

        case ESP_CMD_TCPIP_CIPSTATUS               :
            AT_APPEND_STR( &cmd_str_p, "+CIPSTATUS", 10 ); 
            break;
        case ESP_CMD_TCPIP_CIPSTART                :
            AT_APPEND_STR( &cmd_str_p, "+CIPSTART=", 10 ); 
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
            // note that built-in SSL function will no longer be used since it was proved insecure.
            AT_APPEND_STR( &cmd_str_p, (msg->body.conn_start.type == ESP_CONN_TYPE_TCP ? "TCP": "UDP"), 3 ); 
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_COMMA ); 
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
            AT_APPEND_STR( &cmd_str_p, msg->body.conn_start.host, msg->body.conn_start.host_len ); 
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_COMMA ); 
            cmd_str_p += uiESPcvtNumToStr( cmd_str_p, msg->body.conn_start.port , ESP_DIGIT_BASE_DECIMAL );
            break;
        case ESP_CMD_TCPIP_CIPCLOSE                :
            AT_APPEND_STR( &cmd_str_p, "+CIPCLOSE=", 10 ); 
            cmd_str_p += uiESPcvtNumToStr( cmd_str_p, ucESPconnGetID(msg->body.conn_close.conn) , ESP_DIGIT_BASE_DECIMAL );
            break;
        case ESP_CMD_TCPIP_CIPSEND                 :
            AT_APPEND_STR( &cmd_str_p, "+CIPSEND=", 9 ); 
            cmd_str_p += uiESPcvtNumToStr( cmd_str_p, ucESPconnGetID(msg->body.conn_send.conn) , ESP_DIGIT_BASE_DECIMAL );
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_COMMA ); 
            cmd_str_p += uiESPcvtNumToStr( cmd_str_p, msg->body.conn_send.d_size , ESP_DIGIT_BASE_DECIMAL );
            break; 
        case ESP_CMD_TCPIP_CIFSR                   :
            AT_APPEND_STR( &cmd_str_p, "+CIFSR", 6 ); 
            break;     
        case ESP_CMD_TCPIP_CIPMUX                  :
            AT_APPEND_STR( &cmd_str_p, "+CIPMUX=", 8 ); 
            AT_APPEND_CHR( &cmd_str_p, ESP_NUMTOCHAR(msg->body.tcpip_attri.mux) ); 
            break;     
        case ESP_CMD_TCPIP_CIPSERVER               :
            AT_APPEND_STR( &cmd_str_p, "+CIPSERVER=", 11 ); 
            cmd_str_p += uiESPcvtNumToStr( cmd_str_p, msg->body.tcpip_server.en, ESP_DIGIT_BASE_DECIMAL );
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_COMMA ); 
            cmd_str_p += uiESPcvtNumToStr( cmd_str_p, msg->body.tcpip_server.port, ESP_DIGIT_BASE_DECIMAL );
            break;     
        case ESP_CMD_TCPIP_CIPSTO                  :
            AT_APPEND_STR( &cmd_str_p, "+CIPSTO=", 8 ); 
            cmd_str_p += uiESPcvtNumToStr( cmd_str_p, msg->body.tcpip_server.timeout, ESP_DIGIT_BASE_DECIMAL );
            break; 
        case ESP_CMD_TCPIP_CIPMODE                 :  
            AT_APPEND_STR( &cmd_str_p, "+CIPMODE=", 9 ); 
            AT_APPEND_CHR( &cmd_str_p, ESP_NUMTOCHAR(msg->body.tcpip_attri.trans_mode) ); 
            break;     
#if (ESP_CFG_CONN_MANUAL_TCP_RECV  != 0)
        case ESP_CMD_TCPIP_CIPRECVMODE             :  break;    
        case ESP_CMD_TCPIP_CIPRECVDATA             :  break;     
#endif // end of ESP_CFG_CONN_MANUAL_TCP_RECV 

#if  (ESP_CFG_PING != 0)
        case ESP_CMD_TCPIP_PING                    :
            AT_APPEND_STR( &cmd_str_p, "+PING=", 6 ); 
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
	    AT_APPEND_STR( &cmd_str_p, msg->body.tcpip_ping.host, msg->body.tcpip_ping.host_len );
	    AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_DOUBLE_QUOTE );
            break;     
#endif /* ESP_CFG_PING  */

        case ESP_CMD_TCPIP_CIUPDATE                :  break;

#if  (ESP_CFG_SNTP != 0)
        case ESP_CMD_TCPIP_CIPSNTPCFG              :  break;     
        case ESP_CMD_TCPIP_CIPSNTPTIME             :  break;     
#endif /* ESP_SNT  */

        case ESP_CMD_TCPIP_CIPDINFO                :
            AT_APPEND_STR( &cmd_str_p, "+CIPDINFO=", 10 );
	    AT_APPEND_CHR( &cmd_str_p, (msg->body.sysargs.ext_info_ipd==ESP_ENABLE ? '1': '0') ); 
            break;     
        default:
           response = espERR; 
           break; // should NOT get here.
    } // end of switch statement

    if( response == espOK )
    {
        uint8_t     is_blocking  =  msg->is_blocking;
        uint32_t    block_time   = (is_blocking == ESP_AT_CMD_BLOCKING ? msg->block_time: 100); 
        AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_CR ); 
        AT_APPEND_CHR( &cmd_str_p, ESP_ASCII_LF ); 
        cmd_str_len = (size_t)cmd_str_p - (size_t)(&cmd_str) ;
        espGlobal.ll.send_fn( &cmd_str, cmd_str_len, block_time );
    }
    return response;
} // end of eESPinitATcmd





espRes_t    eESPcmdStartSendData( espMsg_t* msg )
{
    if(msg == NULL) { return espERR; }
    if(GET_CURR_CMD(msg) != ESP_CMD_TCPIP_CIPSEND){ return espERR; }

    uint8_t    *data_p   = msg->body.conn_send.data; 
    uint16_t    data_len = msg->body.conn_send.d_size;
    uint32_t    block_time  = 50; 
    espGlobal.ll.send_fn( data_p, data_len, block_time );
    return   espOK;
} // end of eESPcmdStartSendData




