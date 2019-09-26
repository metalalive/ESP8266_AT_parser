#include "esp/esp.h"
#include "esp/esp_private.h"

// this file implements received piece of data string as response, from either AT-command
// request, or IPD data like application-layer network data.

extern   espGlbl_t  espGlobal;


#define   ESP_RESP_RECV_LINE_BUF_SIZE  0x180
// [TODO] refactor this part.
static uint8_t  prev_chr         = 0;
// [TODO] try input huge chunk of response data.
static uint8_t   recv_data_line_buf[ ESP_RESP_RECV_LINE_BUF_SIZE ];
static uint16_t  recv_data_line_buf_idx = 0;


static void vESPappendChrToRecvDataBuf(uint8_t* data_buf_p , uint16_t* buf_idx_p, uint8_t chr)
{
    uint16_t idx = *buf_idx_p;
    if(idx < ESP_RESP_RECV_LINE_BUF_SIZE) {
        *(data_buf_p + idx) = chr;
        *buf_idx_p = idx + 1;
    }
} // end of vESPappendChrToRecvDataBuf






espRes_t  eESPprocessPieceRecvResp( espBuf_t *recv_buf, uint8_t *isEndOfATresp)
{
    espRes_t    response   = espOK;
    uint8_t    *curr_p     = recv_buf->buff ;
    espIPD_t   *ipdp       = &espGlobal.dev.ipd;
    uint32_t    data_len   = (uint32_t)recv_buf->size;
    uint32_t    idx        = 0;
    uint8_t     curr_chr   = 0;
    uint8_t     clean_line_buf = 0;

    if( ipdp->read == 0x0 ) 
    {   // currently it is AT command mode.
        // divide current piece of received string by CR-LF charaters, for ease of analysis
        for( idx = 0; idx < data_len; idx++ )
        {
            curr_chr = *curr_p++;
            // TODO: for AT command, currently we only support ASCII, will support UTF-8 in future
            if(! ESP_ISVALIDASCII(curr_chr)) {  break;  }
            // currently ESP device is waiting for response of ongoing AT command.
            vESPappendChrToRecvDataBuf( &recv_data_line_buf[0] , &recv_data_line_buf_idx, curr_chr );
            if((prev_chr == ESP_ASCII_CR) && (curr_chr == ESP_ASCII_LF)) 
            {
                // that means currently the ESP device is in AT-command mode, 
                // start analyzing current line of received response string
                vESPparseRecvATrespLine( &recv_data_line_buf[0] , recv_data_line_buf_idx, isEndOfATresp );
                clean_line_buf = (*isEndOfATresp != 0) || (eESPparseNetConnStatus( &recv_data_line_buf[0] ) == espOK) ? 1: 0;
                // clear the index of received line buffer for next line
                recv_data_line_buf_idx = 0x0;
            }
            else if((curr_chr == ':') && (strncmp( (const char *)&recv_data_line_buf[0], "+IPD", 4)==0) && (recv_data_line_buf_idx>5))
            {   // setup for handling received IPD string.
                eESPparseIPDsetup( &recv_data_line_buf[0] );
                // skip the colon ':' of IPD data, payload data starts immediately after the colon mark.
                idx += 1;
                // again clear the index of received line buffer, for storing IPD data.
                recv_data_line_buf_idx = 0x0;
                break;
            }
            else if((prev_chr == '>') && (curr_chr == ' ') && (eESPcmdStartSendData( espGlobal.msg )==espOK))
            { // if current command is AT+CIPSEND & we received the characters '> '
                recv_data_line_buf_idx = 0x0;
                break;
            }
            prev_chr = curr_chr;
        } // end of end of while-loop
    }

    // check where the incoming data should go , to IPD payload ? or response line buffer of current AT command ?
    // TODO: recheck whether received IPD data & received AT-command response will come & interleave in ESP device.
    if( ipdp->read != 0x0 ) 
    {   // currently ESP device is waiting for IPD data
        uint8_t  *ipd_src = curr_p ;
        data_len -= idx; 
        response  = eESPparseIPDcopyData( ipd_src, data_len );
        if(response == espOK) { 
            // run callback function to notify user application that the IPD data is ready.
            vESPconnRunEvtCallback( ipdp->conn, ESP_EVT_CONN_RECV );
            // clean up IPD data, packet buffer chain.
            eESPparseIPDreset();
            clean_line_buf = 1;
        }
    }

    // clear entire line buffer 
    if(clean_line_buf != 0){
         ESP_MEMSET( &recv_data_line_buf , 0x00, ESP_RESP_RECV_LINE_BUF_SIZE );
         recv_data_line_buf_idx = 0x0;
    }
    return response;
} // end of eESPprocessPieceRecvResp

#undef   ESP_RESP_RECV_LINE_BUF_SIZE


