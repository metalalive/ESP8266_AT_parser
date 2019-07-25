#include "esp/esp.h"
#include "tests/integration/ESP_AT_parser/mqtt/mqtt_include.h"

// [Note]
// The low-level implementation should depend on what knid of underlying hardware / operating system
// applied to your application, this file only shows API integration with our ESP AT parser, used
// with ESP8266 wifi device. 
// For Linux / Windows user, you will need to implement your own socket application.



// In some use cases, users might call this read function multiple times only for fetching few bytes of packet data.
// so we need to reserve the packet data that hasn't been completed reading (from user applications)
static espPbuf_t   *unfinish_rd_pktbuf      = NULL;
static espPbuf_t   *unfinish_rd_pktbuf_head = NULL;




word32  mqttPktLowLvlRead( mqttConn_t *mconn, byte *buf, word32 buf_len )
{
    espNetConnPtr   espconn = NULL; 
    espRes_t        response ;
    byte           *curr_src_p ;
    byte           *curr_dst_p ;
    size_t          rd_ptr;
    size_t          copied_len = 0;
    size_t          remain_payld_len = 0;
 
    if((mconn == NULL) || (buf == NULL)) { return 0; }
    espconn = (espNetConnPtr) mconn->ext_sysobjs[0];
    if(espconn == NULL) { return 0; }

    while(buf_len > 0) 
    {
        if(unfinish_rd_pktbuf == NULL) {
            // implement non-blocking packet read function.
            response = eESPnetconnGrabNextPkt( espconn, &unfinish_rd_pktbuf,  mconn->cmd_timeout_ms );
            if( response != espOK ){
                unfinish_rd_pktbuf = NULL;
                return copied_len; 
            }
            unfinish_rd_pktbuf_head = unfinish_rd_pktbuf;
        }
        rd_ptr       = unfinish_rd_pktbuf->rd_ptr ;
        curr_src_p   = & unfinish_rd_pktbuf->payload[rd_ptr] ;
        curr_dst_p   = & buf[ copied_len ];
        remain_payld_len = unfinish_rd_pktbuf->payload_len - rd_ptr ;
        copied_len  += ESP_MIN( buf_len, remain_payld_len );
        ESP_MEMCPY( curr_dst_p, curr_src_p, copied_len );
        buf_len     -= copied_len;
        rd_ptr      += copied_len;
        if(rd_ptr == unfinish_rd_pktbuf->payload_len) {
            unfinish_rd_pktbuf = unfinish_rd_pktbuf->next;
            if(unfinish_rd_pktbuf == NULL) {
                // free the allocated space to the last packet we read
                vESPpktBufChainDelete( unfinish_rd_pktbuf_head );
                unfinish_rd_pktbuf_head = NULL;
            }
        }
        else{
            unfinish_rd_pktbuf->rd_ptr =  rd_ptr ;
        }
    } // end of while-loop

    return  copied_len;
} // end of mqttPktLowLvlRead





word32  mqttPktLowLvlWrite( mqttConn_t *mconn, byte *buf, word32 buf_len )
{
    if((mconn == NULL) || (buf == NULL)) { 
        return 0 ;
    }
    espConn_t  *espconn = (espConn_t *) mconn->ext_sysobjs[1] ;
    espRes_t    response ; 
    if(espconn == NULL) { 
        return 0;
    }
    response = eESPconnClientSend( espconn,  buf,  buf_len, NULL, NULL, ESP_AT_CMD_BLOCKING );
    return  ( response == espOK ? buf_len : 0);
} // end of mqttPktLowLvlWrite







