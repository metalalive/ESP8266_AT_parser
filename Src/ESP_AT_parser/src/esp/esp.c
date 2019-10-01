#include "esp/esp.h"
#include "esp/esp_private.h"


espGlbl_t espGlobal;

// default event callback function 
static espRes_t eESPdeflEvtCallback( espEvt_t*  evt )
{
    return espOK;
} // end of eESPdeflEvtCallback


static void  vESPdeleteEvtCallbacks( espEvtCbFnLstItem_t *head )
{
    espEvtCbFnLstItem_t *item = head;
    espEvtCbFnLstItem_t *next = NULL;
    // call all the registered event callback function
    while( item!=NULL ) {
        next = item->next ;
        ESP_MEMFREE( item );
        item = next;
    } // end of loop
} // end of vESPdeleteEvtCallbacks



espMsg_t* pxESPmsgCreate( espCmd_t cmd, espApiCmdCbFn  api_cb, void* cb_arg, const uint8_t blocking )
{
    espMsg_t *msg = NULL;
    msg = (espMsg_t *) ESP_MALLOC(sizeof(espMsg_t));
    msg->cmd = cmd;
    msg->api_cb = api_cb;
    msg->api_cb_arg = cb_arg;
    msg->is_blocking = blocking;
    msg->block_time = 0;
    msg->sem = NULL;
    return msg;
} // end of pxESPmsgCreate




void  vESPmsgDelete(espMsg_t** msg)
{   // TODO: figure out why exception/fault happen when freeing the allocated memory.
    if ((*msg)->sem != NULL) {
        ESP_MEMFREE( (*msg)->sem );
        (*msg)->sem = NULL;
    }
    ESP_MEMFREE(*msg);
    *msg = NULL;
} // end of vESPmsgDelete






// initialize ESP AT library, this function must be called from operating system thread.
// this function creates threads that work together to handler AT command requests generated 
// by API functions call, also a couple of data structure for internal use like mutex,
// semaphore (for thread synchronization), and message boxes (for communication between
// these threads). 
espRes_t   eESPinit( espEvtCbFn cb )
{
    espRes_t response = espOK;
    uint8_t  init_fail = 0;
    uint8_t  isThreadPrivileged = 0x1;
    // clear initialized flag
    espGlobal.status.flg.initialized = 0;
    espGlobal.status.flg.dev_present = 0;
    // clear internal system lock
    espGlobal.locked_cnt = 0;

    // build event callback function linked list
    espGlobal.evtCbLstHead = (espEvtCbFnLstItem_t *) ESP_MALLOC( sizeof(espEvtCbFnLstItem_t) );
    espGlobal.evtCbLstHead->next = NULL; 
    espGlobal.evtCbLstHead->cb   = (cb != NULL) ? cb : eESPdeflEvtCallback;
    espGlobal.evt_server = NULL;

    if( (response = eESPsysInit()) == espOK ) {
        // creating semaphores accessed between handling threads & interrupt service routine.
        espGlobal.sem_th_sync  = NULL;
        espGlobal.sem_th_sync  = xESPsysSemCreate();
        if(espGlobal.sem_th_sync == NULL) {
            init_fail++;
        }
    }
    if(init_fail == 0) {
        // create message boxes for buffering AT-command request / response between ESP device
        espGlobal.mbox_cmd_req  = xESPsysMboxCreate( ESP_CFG_AT_CMD_REQ_MBOX_SIZE ) ;       
        espGlobal.mbox_cmd_resp = xESPsysMboxCreate( ESP_CFG_AT_CMD_RESP_MBOX_SIZE ) ;       
        if( espGlobal.mbox_cmd_req == NULL) {
            init_fail++;
        }
        if( espGlobal.mbox_cmd_resp == NULL) {
            init_fail++;
        }
    }
    if(init_fail == 0) {
        // create the threads which handle AT commands & network data.
        //
        // A trick can be used at here (the detail depends on your underlying system implementation) : 
        //     the thread calling this ESP init function can take higher priority over the 2 threads below,
        //     once the 2 threads are created, that makes them (the 2 threads below) NOT preempt current
        //     running thread, so current running thread goes on to complete the initialization code.
        espGlobal.thread_cmd_req  = NULL;
        espGlobal.thread_cmd_resp = NULL;
        response = eESPsysThreadCreate( &espGlobal.thread_cmd_req, "ATcmdReq", vESPthreadATreqHandler, NULL, 
                               ESP_SYS_THREAD_STACK_SIZE, ESP_SYS_THREAD_PRIO, isThreadPrivileged );
        response = eESPsysThreadCreate( &espGlobal.thread_cmd_resp, "ATcmdResp", vESPthreadATrespHandler, NULL,
                               ESP_SYS_THREAD_STACK_SIZE, ESP_SYS_THREAD_PRIO, isThreadPrivileged );
        if((response != espOK) || (espGlobal.thread_cmd_req == NULL) || (espGlobal.thread_cmd_resp == NULL)) {
            init_fail++;
        }
    }

    // take system lock for the atomic operation
    eESPcoreLock();
    if(init_fail == 0) {
        espGlobal.ll.uart.baudrate = ESP_CFG_BAUDRATE;
        espGlobal.ll.send_fn  = eESPlowLvlSendFn; 
#if defined(ESP_CFG_RST_PIN)
        espGlobal.ll.reset_fn = eESPlowLvlRstFn ;
#else
        espGlobal.ll.reset_fn = NULL ;
#endif // end of ESP_CFG_RST_PIN
#ifndef  ESP_CFG_PLATFORM_REINIT_ON_RST
        // enable low-level UART/GPIO hardware function
        eESPlowLvlDevInit(NULL);
        // initialize (UART Rx) receiving function everytime when we'd like to reset ESP device,
        // then the ESP device / host microcontroller can receive AT-command response or IPD data from other clients.
        vESPlowLvlRecvStopFn();
        response = eESPlowLvlRecvStartFn();
        if(response != espOK){ init_fail++; }
        else
#endif // end of ESP_CFG_PLATFORM_REINIT_ON_RST
        {
            // set initialized flag
            espGlobal.status.flg.initialized = 1;
            espGlobal.evt.type = ESP_EVT_INIT_FINISH;
            vESPrunEvtCallbacks( &espGlobal.evt );
        }
    }

    if(init_fail == 0) {
        eESPcoreUnlock();
#if (ESP_CFG_RESTORE_ON_INIT != 0)
        response =  eESPrestore( NULL, NULL );
#elif (ESP_CFG_RST_ON_INIT != 0)
        // reset assertion for few milliseconds 
        response =  eESPresetWithDelay( 1, NULL, NULL );
#endif //end of ESP_CFG_RST_ON_INIT, ESP_CFG_RESTORE_ON_INIT
        eESPcoreLock();
        if(response != espOK) { init_fail++; }
    }
    // release system lock at the end
    eESPcoreUnlock();

    if(init_fail != 0) {
        // failed to initialize in operating system level
        ESP_MEMFREE( espGlobal.evtCbLstHead );
        espGlobal.evtCbLstHead = NULL;
    }
    return response;
} // end of eESPinit



espRes_t    eESPdeinit( void )
{
    espRes_t response = espOK;
    eESPcoreLock();
    {
        vESPlowLvlRecvStopFn();
        response = eESPsysThreadDelete( &espGlobal.thread_cmd_req  );
        response = eESPsysThreadDelete( &espGlobal.thread_cmd_resp );
        vESPsysMboxDelete( &espGlobal.mbox_cmd_req  );
        vESPsysMboxDelete( &espGlobal.mbox_cmd_resp );
    }
    eESPcoreUnlock();
    vESPsysSemDelete( &espGlobal.sem_th_sync );
    espGlobal.sem_th_sync  = NULL;
    response = eESPsysDeInit();
    vESPdeleteEvtCallbacks( espGlobal.evtCbLstHead );
    espGlobal.evtCbLstHead = NULL;
    return response;
} // end of eESPdeinit



espRes_t    eESPcoreLock(void)
{
    espRes_t  response = eESPsysProtect();
    if( response == espOK ) {
        espGlobal.locked_cnt++;
    }
    return response;
} // end of eESPcoreLock




espRes_t    eESPcoreUnlock(void)
{
    espRes_t  response = espERR ;
    if( espGlobal.locked_cnt > 0 ) {
        espGlobal.locked_cnt--;
        response = eESPsysUnprotect();
    }
    return response;
} // end of eESPcoreUnlock





void  vESPrunEvtCallbacks( espEvt_t *evtp )
{
    espEvtCbFnLstItem_t *item = NULL;
    // call all the registered event callback function
    for( item = espGlobal.evtCbLstHead ; item!=NULL ; item = item->next )
    {
        item->cb( evtp );
    } // end of for-loop
} // end of vESPrunEvtCallbacks





espRes_t    eESPsendReqToMbox (espMsg_t* msg, espRes_t (*initATcmdFn)(espMsg_t *) )
{
    espRes_t response = espOK;
    // check any situation that could make this function fail to send request to message box.
    eESPcoreLock();
    if( espGlobal.status.flg.initialized == 0 ) {
        response = espERR;
    }
    if((espGlobal.locked_cnt > 1) && (msg->is_blocking == ESP_AT_CMD_BLOCKING)) {
        response = espERRBLOCKING;
    }
    eESPcoreUnlock();
    if(response != espOK) {
        // something wrong in msg, then free the space allocated to msg
        vESPmsgDelete( &msg );
        return response;
    }
    if(msg->is_blocking == ESP_AT_CMD_BLOCKING) {
        // if we transfer the AT command request in blocking mode, then a semaphore is required
        // for synchronizing the transmission between request handling thread and respense handling
        // thread, respense handling thread will give this semaphore to this thread when it 
        // receives complete response (from the ESP device) .
        msg->sem =  xESPsysSemCreate();
        if( msg->sem == NULL ) {
            vESPmsgDelete( &msg );
            response = espERRMEM;
            return response;
        }
    }
    msg->fn = initATcmdFn;
    response =  eESPsysMboxPut( espGlobal.mbox_cmd_req, (void *)msg, msg->block_time );
    if(response != espOK) {
        // failed to send request to message box, and free the space allocated to msg
        vESPmsgDelete( &msg );
        return response;
    }
    // succeed to put request to message box queue, it will be processed in another thread
    msg->res = espINPROG;
    // From here on, the request handling thread should receive request from the message box.
    if(msg->is_blocking == ESP_AT_CMD_BLOCKING) {
        // wait for respense handling thread receiving the entire response message of current AT command request
        // , timeout would happen if following semaphore wait function returns espTIMEOUT.
        eESPsysSemWait( msg->sem, ESP_SYS_MAX_TIMEOUT );
        response = msg->res;
        vESPmsgDelete( &msg );
    }
    return response;
} // end of eESPsendReqToMbox




void    vESPapiRunEvtCallbacks( espMsg_t* msg )
{
    espRes_t       response =   msg->res;
    espEvt_t      *evtp     =  &espGlobal.evt;
    espEvtType_t   evt_type =   ESP_EVT_NO_EVENT;
    if( response == espTIMEOUT) {
        evt_type = ESP_EVT_CMD_TIMEOUT;
    }
    else {
        switch( GET_CURR_CMD(msg) ) 
        {
            case ESP_CMD_WIFI_CWJAP                  :
                evt_type = (response == espOK ? ESP_EVT_WIFI_CONNECTED: ESP_EVT_STA_JOIN_AP);
                evtp->body.staJoinAP.res = response;
                break; 
            case ESP_CMD_WIFI_CWQAP                  :
                evt_type = ESP_EVT_WIFI_DISCONNECTED ;
                break;   
            case ESP_CMD_WIFI_CWLAP                  :
                evt_type = ESP_EVT_STA_LIST_AP;
                evtp->body.staListAP.res = response;
                evtp->body.staListAP.aps = msg->body.ap_list.aps ;
                evtp->body.staListAP.num_ap_found = *(msg->body.ap_list.num_ap_found);
                break;   
#if ( ESP_CFG_PING != 0 )
            case ESP_CMD_TCPIP_PING                  :
                evt_type = ESP_EVT_PING;
                evtp->body.ping.res  = response;
                evtp->body.ping.host = msg->body.tcpip_ping.host;
                evtp->body.ping.resptime = msg->body.tcpip_ping.resptime ;
                break;   
#endif // end of  ESP_CFG_PING 
            default:
                break; 
        } // end of switch-case statement
    }
    if(evt_type != ESP_EVT_NO_EVENT) {
        evtp->type = evt_type;
        vESPrunEvtCallbacks( evtp );
    }
} // end of vESPapiRunEvtCallbacks



espRes_t    eESPflushMsgBox( espSysMbox_t mb )
{
    const  uint32_t no_block_time = 0;
    espRes_t  response = espOK;
    void*     itemp    = NULL;
    if(mb == NULL) {
        response = espERRARGS;
        return response;
    }
    eESPcoreLock();
    while(1) {
        itemp    = NULL;
        response = eESPsysMboxGet( mb, (void **)&itemp, no_block_time );
        if( response == espOK ){   ESP_MEMFREE(itemp);  }
        else { break; }
    }
    eESPcoreUnlock();
    return response;
} // end of eESPflushMsgBox





