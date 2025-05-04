#include "esp/esp.h"
#include "demo_apps/mqtt/mqtt_include.h"

// [Note]
// The low-level implementation should depend on what knid of underlying hardware / operating system
// applied to your application, this file only shows API integration with our ESP AT parser, used
// with ESP8266 wifi device. 
// For Linux / Windows user, you will need to implement your own socket application.

// In some use cases, users might call this read function multiple times only for fetching few bytes of packet data.
// so we need to reserve the packet data that hasn't been completed reading (from user applications)
static espPbuf_t   *unfinish_rd_pktbuf      = NULL;
static espPbuf_t   *unfinish_rd_pktbuf_head = NULL;


word32  mqttPktLowLvlRead( struct __mqttCtx *mctx, byte *buf, word32 buf_len )
{
    espNetConnPtr   espconn = NULL; 
    espRes_t        response ;
    byte           *curr_src_p ;
    byte           *curr_dst_p ;
    size_t          rd_ptr;
    size_t          copied_len_total = 0; // total length of copied data
    size_t          copied_len_iter  = 0; // length of copied data in each iteration
    size_t          remain_payld_len = 0;
 
    if((mctx == NULL) || (buf == NULL)) { return 0; }
    espconn = (espNetConnPtr) mctx->ext_sysobjs[0];
    if(espconn == NULL) { return 0; }

    while(buf_len > 0) 
    {
        if(unfinish_rd_pktbuf == NULL) {
            // implement non-blocking packet read function.
            response = eESPnetconnGrabNextPkt( espconn, &unfinish_rd_pktbuf,  mctx->cmd_timeout_ms );
            if( response != espOK ){
                unfinish_rd_pktbuf = NULL;
                return copied_len_total; 
            }
            unfinish_rd_pktbuf_head = unfinish_rd_pktbuf;
        }
        rd_ptr       = unfinish_rd_pktbuf->rd_ptr ;
        curr_src_p   = & unfinish_rd_pktbuf->payload[rd_ptr] ;
        curr_dst_p   = & buf[ copied_len_total ];
        remain_payld_len  = unfinish_rd_pktbuf->payload_len - rd_ptr ;
        copied_len_iter   = ESP_MIN( buf_len, remain_payld_len );
        copied_len_total += copied_len_iter;
        ESP_MEMCPY( curr_dst_p, curr_src_p, copied_len_iter );
        buf_len     -= copied_len_iter;
        rd_ptr      += copied_len_iter;
        unfinish_rd_pktbuf->rd_ptr =  rd_ptr ;
        if(rd_ptr >= unfinish_rd_pktbuf->payload_len) {
            unfinish_rd_pktbuf = unfinish_rd_pktbuf->next;
            if(unfinish_rd_pktbuf == NULL) {
                // free the allocated space to the last packet we read
                vESPpktBufChainDelete( unfinish_rd_pktbuf_head );
                unfinish_rd_pktbuf_head = NULL;
            }
        }
    } // end of while-loop

    return  copied_len_total;
} // end of mqttPktLowLvlRead


word32  mqttPktLowLvlWrite( struct __mqttCtx *mctx, byte *buf, word32 buf_len )
{
    if((mctx == NULL) || (buf == NULL)) {
        return 0 ;
    }
    espConn_t  *espconn = (espConn_t *) mctx->ext_sysobjs[1] ;
    espRes_t    response ; 
    if(espconn == NULL) { 
        return 0;
    }
    response = eESPconnClientSend( espconn,  buf,  buf_len, NULL, NULL, ESP_AT_CMD_BLOCKING );
    return  ( response == espOK ? buf_len : 0);
} // end of mqttPktLowLvlWrite
