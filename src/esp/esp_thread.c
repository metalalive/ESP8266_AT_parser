#include "esp/esp.h"
#include "esp/esp_private.h"

extern   espGlbl_t  espGlobal;




void    vESPthreadATreqHandler ( void* const arg )
{
    espRes_t    response = espOK; 
    espMsg_t*   msg;
    uint32_t    block_time = 0; 
    uint8_t     is_blocking = 0;

    eESPcoreLock();
    for(;;)
    {
        msg = NULL;
        eESPcoreUnlock();
        response = eESPsysMboxGet( espGlobal.mbox_cmd_req, (void **)&msg, ESP_SYS_MAX_TIMEOUT );
        eESPcoreLock();
        if((response != espOK) || (msg == NULL)) { continue; }
        espGlobal.msg = msg;
        is_blocking = msg->is_blocking;
        block_time  = msg->block_time ;
        if(block_time == 0) { block_time += 50; }
        // generating AT command string
        if(response == espOK) {
            response = (msg->fn != NULL) ? msg->fn(msg) : espERR ;
        }
        // once AT command string was sent, this thread has to wait for response handling thread
        // getting its job done by taking internal semaphore.
        if(response == espOK) {
            eESPcoreUnlock();
            // waiting until it's released by response handling thread.
            response = eESPsysSemWait( espGlobal.sem_th_sync, block_time ); 
            eESPcoreLock();
        }
        if(response == espTIMEOUT) {
            msg->res = espTIMEOUT;
        }
        // for few API funcion calls, we run the event callback function to notify user application
        // for important events.
        vESPapiRunEvtCallbacks( msg );
        if(is_blocking == ESP_AT_CMD_BLOCKING) {
            // for blocking AT-command request, this thread releases the semaphore of the message,
            // so the blocked API caller thread can be resumed from semaphore wait function.
            eESPsysSemRelease( msg->sem );
        }
        else{
            // run callback function for the API function (regardless of its response), this is useful when
            // users made non-blocking API call and attempts to see the result of AT command.
            if( msg->api_cb != NULL ) {
                msg->api_cb( msg->res, msg->api_cb_arg );
            }
            // for non-blocking AT-command request, there is no semaphore between this thread & the API caller,
            // we can simply free the space allocated to the message structure
            vESPmsgDelete( msg );
        }
        espGlobal.msg = NULL;
    } // end of outer infinite loop
} // end of vESPthreadATreqHandler





void    vESPthreadATrespHandler ( void* const arg )
{
    espRes_t    response = espOK; 
    espBuf_t   *recv_buf    = NULL;
    uint8_t     isEndOfATresp = 0;

    eESPcoreLock();
    for(;;)
    {
        eESPcoreUnlock();  
        response = eESPsysMboxGet( espGlobal.mbox_cmd_resp, (void **)&recv_buf, ESP_SYS_MAX_TIMEOUT );
        eESPcoreLock();
        // process the piece of response string we just received
        if( response != espOK) { continue; }
        eESPprocessPieceRecvResp(recv_buf , &isEndOfATresp);
        // complete parsing the received string, now we can free the allocated space
        ESP_MEMFREE( recv_buf );
        recv_buf = NULL;
        // wait until we receive entire response string of the AT command / IPD data. 
        if( isEndOfATresp != 0 )
        {
            eESPcoreUnlock();
            eESPsysSemRelease( espGlobal.sem_th_sync );
            // yield this thread voluntarily, let next AT-command request come in & be produced (if exists)
            eESPsysThreadYield();
            eESPcoreLock();
            isEndOfATresp = 0;
        }
    } // end of outer infinite loop
} // end of vESPthreadATrespHandler



espRes_t  eESPappendRecvRespISR(uint8_t* data, uint16_t data_len)
{
    espRes_t    response  = espOK;
    size_t      pbuf_size = sizeof(espBuf_t);
    espBuf_t   *recv_buf  = (espBuf_t *) ESP_MALLOC( pbuf_size + data_len );
    ESP_ASSERT( recv_buf != NULL);
    uint8_t    *piece_resp_ptr = ((uint8_t *)recv_buf) + pbuf_size;
    ESP_MEMCPY((void *)piece_resp_ptr, (void *)data, data_len);
    recv_buf->size = (size_t) data_len ; 
    recv_buf->buff = piece_resp_ptr ; 
    response = eESPsysMboxPutISR( espGlobal.mbox_cmd_resp, (void *)recv_buf );
    return response;
} // end of eESPappendRecvRespISR



 
