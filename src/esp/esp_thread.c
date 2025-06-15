#include "esp/esp.h"
#include "esp/esp_private.h"

espRes_t eESPthreadATreqIteration(espGlbl_t *gbl) {
    espMsg_t *msg = NULL;
    eESPcoreUnlock(gbl);
    espRes_t response = eESPsysMboxGet(gbl->mbox_cmd_req, (void **)&msg, ESP_SYS_MAX_TIMEOUT);
    eESPcoreLock(gbl);
    if (response != espOK) {
        return response;
    }
    if (msg == NULL) {
        return espERRMEM;
    }
    gbl->msg = msg;
    uint32_t block_time = msg->block_time;
    uint8_t  is_blocking = msg->is_blocking;
    if (block_time == 0) {
        block_time += 50;
    }
    // generating AT command string
    response = (msg->fn != NULL) ? msg->fn(msg, gbl) : espERR;
    // once AT command string was sent, this thread has to wait for response
    // handling thread completing AT response or received network data
    if (response == espOK) {
        eESPcoreUnlock(gbl);
        // waiting until response handling thread invokes the semaphore to
        // wake this thread
        response = eESPsysSemWait(gbl->sem_th_sync, block_time);
        eESPcoreLock(gbl);
    }
    if (response == espTIMEOUT) {
        msg->res = espTIMEOUT;
    }
    vESPapiRunEvtCallbacks(msg, &gbl->evt);
    if (is_blocking == ESP_AT_CMD_BLOCKING) {
        // for blocking AT-command request, this thread releases the semaphore
        // of the message, so the blocked API caller thread can be resumed from
        // semaphore wait function.
        response = eESPsysSemRelease(msg->sem);
    } else {
        if (msg->api_cb != NULL) {
            msg->api_cb(msg->res, msg->api_cb_arg);
        }
        // for non-blocking AT-command request, there is no semaphore between
        // this thread & the API caller, we can simply free the space allocated
        // to the message structure
        vESPmsgDelete(msg);
    }
    gbl->msg = NULL;
    return response;
} // end of eESPthreadATreqIteration

void vESPthreadATreqHandler(void *const arg) {
    eESPcoreLock(arg);
    for (;;) {
        eESPthreadATreqIteration((espGlbl_t *)arg);
    }
}

espRes_t eESPthreadATrespIteration(espGlbl_t *gbl) {
    espBuf_t *recv_buf = NULL;
    eESPcoreUnlock(gbl);
    espRes_t response = eESPsysMboxGet(gbl->mbox_cmd_resp, (void **)&recv_buf, ESP_SYS_MAX_TIMEOUT);
    eESPcoreLock(gbl);
    if (response != espOK) {
        return response;
    }
    uint8_t isEndOfATresp = (uint8_t)gbl->ops.recv_resp_proc(gbl, recv_buf);
    ESP_MEMFREE(recv_buf);
    recv_buf = NULL;
    // in case response of the AT command is split to multiple segments
    if (isEndOfATresp) {
        eESPcoreUnlock(gbl);
        eESPsysSemRelease(gbl->sem_th_sync);
        eESPsysThreadYield();
        eESPcoreLock(gbl);
    }
    return response;
}

void vESPthreadATrespHandler(void *const arg) {
    espGlbl_t *gbl = (espGlbl_t *)arg;
    eESPcoreLock(gbl);
    for (;;) {
        eESPthreadATrespIteration(gbl);
    }
}

espRes_t eESPappendRecvRespISR(uint8_t *data, uint16_t data_len) {
    extern espGlbl_t espGlobal;
    espRes_t         response = espOK;
    size_t           pbuf_size = sizeof(espBuf_t);
    espBuf_t        *recv_buf = (espBuf_t *)ESP_MALLOC(pbuf_size + data_len);
    ESP_ASSERT(recv_buf != NULL);
    uint8_t *piece_resp_ptr = ((uint8_t *)recv_buf) + pbuf_size;
    ESP_MEMCPY((void *)piece_resp_ptr, (void *)data, data_len);
    recv_buf->size = (size_t)data_len;
    recv_buf->buff = piece_resp_ptr;
    response = eESPsysMboxPutISR(espGlobal.mbox_cmd_resp, (void *)recv_buf);
    return response;
}
