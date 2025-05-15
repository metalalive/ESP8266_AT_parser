#include "esp/esp.h"
#include "esp/esp_private.h"

extern espGlbl_t espGlobal;

// internal structure storing essential information of TCP/UDP connections
typedef struct espNetConn {
    // list of received raw string from ESP device.  (should be IPD,
    // incoming-packet-data)
    espSysMbox_t     mbox_recv;
    uint32_t         conn_timeout_s; // connection timeout in seconds (only for server mode)
    espNetConnType_t type;           // TCP ? UDP ? SSL ?
    espConn_t       *conn;           // connection details e.g. IP, MAC, used when this struct
                                     // is created as client
    espPort_t listen_port;           // listening port for server connection
} espNetConn_t;

espRes_t eESPsetWifiMode(
    espMode_t mode, uint8_t saveDef, const espApiCmdCbFn cb, void *const cb_arg,
    const uint8_t blocking
) {
    espMsg_t *msg = NULL;
    espRes_t  response = espERR;
    msg = pxESPmsgCreate(ESP_CMD_WIFI_CWMODE, cb, cb_arg, blocking);
    if (msg == NULL) {
        return response;
    }
    if (blocking == ESP_AT_CMD_BLOCKING) {
        msg->block_time = 100;
    }
    msg->body.wifi_mode.mode = mode;
    msg->body.wifi_mode.def = saveDef;
    response = eESPsendReqToMbox(msg, eESPinitATcmd);
    return response;
} // end of eESPsetWifiMode

espRes_t eESPsetTransmitMode(
    espTransMode_t mode, const espApiCmdCbFn cb, void *const cb_arg, const uint8_t blocking
) {
    espMsg_t *msg = NULL;
    espRes_t  response = espERR;
    msg = pxESPmsgCreate(ESP_CMD_TCPIP_CIPMODE, cb, cb_arg, blocking);
    if (msg == NULL) {
        return response;
    }
    if (blocking == ESP_AT_CMD_BLOCKING) {
        msg->block_time = 100;
    }
    msg->body.tcpip_attri.trans_mode = mode;
    response = eESPsendReqToMbox(msg, eESPinitATcmd);
    return response;
} // end of eESPsetTransmitMode

espRes_t eESPsetMultiTCPconn(
    espMultiConn_t mux_conn, const espApiCmdCbFn cb, void *const cb_arg, const uint8_t blocking
) {
    espMsg_t *msg = NULL;
    espRes_t  response = espERR;
    msg = pxESPmsgCreate(ESP_CMD_TCPIP_CIPMUX, cb, cb_arg, blocking);
    if (msg == NULL) {
        return response;
    }
    if (blocking == ESP_AT_CMD_BLOCKING) {
        msg->block_time = 100;
    }
    msg->body.tcpip_attri.mux = mux_conn;
    response = eESPsendReqToMbox(msg, eESPinitATcmd);
    return response;
} // end of eESPsetMultiTCPconn

espRes_t eESPgetConnStatus(const uint32_t blocking) {
    espMsg_t *msg = NULL;
    espRes_t  response = espERR;
    msg = pxESPmsgCreate(ESP_CMD_TCPIP_CIPSTATUS, NULL, NULL, blocking);
    if (msg == NULL) {
        return response;
    }
    if (blocking == ESP_AT_CMD_BLOCKING) {
        msg->block_time = 100;
    }
    response = eESPsendReqToMbox(msg, eESPinitATcmd);
    return response;
} // end of eESPgetConnStatus

// TODO: test this API function with up-to-date AT firmware
espRes_t eESPgetLocalIPmac(
    espIp_t *sta_ip, espMac_t *sta_mac, espIp_t *ap_ip, espMac_t *ap_mac, const espApiCmdCbFn cb,
    void *const cb_arg, const uint8_t blocking
) {
    espMsg_t *msg = NULL;
    espRes_t  response = espERR;
    if ((sta_ip == NULL) || (sta_mac == NULL) || (ap_ip == NULL) || (ap_mac == NULL)) {
        return espERRARGS;
    }
    msg = pxESPmsgCreate(ESP_CMD_TCPIP_CIFSR, cb, cb_arg, blocking);
    if (msg == NULL) {
        return response;
    }
    if (blocking == ESP_AT_CMD_BLOCKING) {
        msg->block_time = 3000;
    }
    msg->body.local_ip_mac.sta_ip = sta_ip;
    msg->body.local_ip_mac.sta_mac = sta_mac;
    msg->body.local_ip_mac.ap_ip = ap_ip;
    msg->body.local_ip_mac.ap_mac = ap_mac;
    response = eESPsendReqToMbox(msg, eESPinitATcmd);
    return response;
} // end of eESPgetLocalIPmac

espNetConnPtr pxESPnetconnCreate(espConn_t *conn_in) {
    espNetConn_t *nc = NULL;
    if (conn_in == NULL) {
        return (espNetConnPtr)nc;
    }
    nc = ESP_MALLOC(sizeof(espNetConn_t));
    if (nc != NULL) {
        nc->type = conn_in->type;
        nc->conn = conn_in;
        nc->mbox_recv = xESPsysMboxCreate(ESP_CFG_NETCONN_RECV_Q_LEN);
        if (nc->mbox_recv == NULL) {
            ESP_MEMFREE(nc);
            nc = NULL;
        }
    }
    return (espNetConnPtr)nc;
} // end of pxESPnetconnCreate

static void vESPfreeNetConnPktBufChain(void *p) {
    vESPpktBufChainDelete((espPbuf_t *)p);
} // end of vESPfreeNetConnPktBufChain

espRes_t eESPnetconnDelete(espNetConnPtr nc) {
    espRes_t response = espOK;
    if (nc == NULL) {
        return espERRARGS;
    }
    // release espConn_t back to connection pool of ESP library.
    if (nc->conn != NULL) {
        // there might be incomplete IPD data leaving pointed by espConn_t,
        // clean up all of them if exists.
        if (nc->conn->pbuf != NULL) {
            vESPpktBufChainDelete(nc->conn->pbuf);
        }
        ESP_MEMSET(nc->conn, 0x00, sizeof(espConn_t));
    }
    response = eESPflushMsgBox(nc->mbox_recv, vESPfreeNetConnPktBufChain);
    vESPsysMboxDelete(&(nc->mbox_recv));
    ESP_MEMFREE(nc);
    return response;
} // end of eESPnetconnDelete

espConn_t *pxESPgetConnHandleObj(espNetConnPtr nc) {
    espConn_t *out = NULL;
    if (nc != NULL) {
        out = nc->conn;
    }
    return out;
} // end of pxESPgetConnHandleObj

espRes_t eESPsetServer(
    espNetConnPtr serverconn, espFnEn_t en, espPort_t port, espEvtCbFn evt_cb,
    const espApiCmdCbFn api_cb, void *const api_cb_arg, const uint8_t blocking
) {
    espRes_t  response = espOK;
    espMsg_t *msg = NULL;
    en = (en != 0 ? 1 : 0);
    if ((en == 1) && (evt_cb == NULL)) {
        return espERRARGS;
    }
    msg = pxESPmsgCreate(ESP_CMD_TCPIP_CIPSERVER, api_cb, api_cb_arg, blocking);
    if (msg == NULL) {
        return response;
    }
    if (blocking == ESP_AT_CMD_BLOCKING) {
        msg->block_time = 1000;
    }
    // TODO: refactor the structure tcpip_server
    ((espNetConn_t *)serverconn)->listen_port = port;
    msg->body.tcpip_server.en = (en == ESP_ENABLE) ? 1 : 0;
    msg->body.tcpip_server.port = port;
    msg->body.tcpip_server.cb = (en == 0 ? NULL : evt_cb);
    response = eESPsendReqToMbox(msg, eESPinitATcmd);
    return response;
} // end of eESPsetServer

espRes_t eESPsetServerTimeout(
    espNetConnPtr serverconn, uint16_t timeout, const espApiCmdCbFn cb, void *const cb_arg,
    const uint8_t blocking
) {
    espRes_t  response = espOK;
    espMsg_t *msg = NULL;
    msg = pxESPmsgCreate(ESP_CMD_TCPIP_CIPSTO, cb, cb_arg, blocking);
    if (msg == NULL) {
        return response;
    }
    if (blocking == ESP_AT_CMD_BLOCKING) {
        msg->block_time = 100;
    }
    ((espNetConn_t *)serverconn)->conn_timeout_s = timeout;
    msg->body.tcpip_server.timeout = timeout;
    response = eESPsendReqToMbox(msg, eESPinitATcmd);
    return response;
} // end of eESPsetServerTimeout

uint8_t ucESPconnGetID(espConn_t *conn) {
    uint8_t    idx = 0;
    espConn_t *c = NULL;
    espDev_t  *devp = &espGlobal.dev;
    for (idx = 0; idx < ESP_CFG_MAX_CONNS; idx++) {
        c = &devp->conns[idx];
        if (c == conn)
            break;
    }
    return idx;
} // end of ucESPconnGetID

espConn_t *pxESPgetNxtAvailConn(void) {
    uint8_t    idx = 0;
    espDev_t  *devp = &espGlobal.dev;
    espConn_t *c = NULL;
    espConn_t *out = NULL;
    for (idx = 0; idx < ESP_CFG_MAX_CONNS; idx++) {
        c = &devp->conns[idx];
        if (c->status.flg.active == ESP_CONN_CLOSED) {
            out = c;
            break;
        }
    }
    return out;
} // end of pxESPgetNxtAvailConn

espRes_t eESPconnClientStart(
    espConn_t *conn_in, espConnType_t type, const char *const host, uint16_t host_len,
    espPort_t port, espEvtCbFn evt_cb, espApiCmdCbFn api_cb, void *const api_cb_arg,
    const uint32_t blocking
) {
    espRes_t  response = espOK;
    espMsg_t *msg = NULL;
    if ((conn_in == NULL) || (host == NULL) || (port == 0) || (evt_cb == NULL)) {
        return espERRARGS;
    }
    // TODO: figure out what would happen if we connect 2 hosts (using different
    // IP addresses) with the same ID,
    //       and we don't close the first connection with respect to the ID .
    if (conn_in->status.flg.active != ESP_CONN_CLOSED) {
        // the connection ID is still in used.
        // application must close the connection (w.r.t. the same ID) first,
        // then connect other host  using the same ID
        return espERRARGS;
    }
    msg = pxESPmsgCreate(ESP_CMD_TCPIP_CIPSTART, api_cb, api_cb_arg, blocking);
    if (msg == NULL) {
        return response;
    }
    conn_in->type = type;
    // NOTE: there will be network latency while performing this AT command, so
    //       we use block time as delay time even applications call this API in
    //       non-blocking mode.
    msg->block_time = ESP_CFG_CMD_BLOCK_TIME_CIPSTART;
    msg->body.conn_start.conn = &conn_in;
    msg->body.conn_start.host = host;
    msg->body.conn_start.host_len = host_len;
    msg->body.conn_start.port = port;
    msg->body.conn_start.type = type;
    msg->body.conn_start.cb = evt_cb;
    msg->body.conn_start.num = 0;
    msg->body.conn_start.success = 0;
    response = eESPsendReqToMbox(msg, eESPinitATcmd);
    return response;
} // end of eESPconnClientStart

espRes_t eESPconnClientClose(
    espConn_t *conn_in, espApiCmdCbFn api_cb, void *const api_cb_arg, const uint32_t blocking
) {
    espRes_t  response = espOK;
    espMsg_t *msg = NULL;
    if (conn_in == NULL) {
        return espERRARGS;
    }
    msg = pxESPmsgCreate(ESP_CMD_TCPIP_CIPCLOSE, api_cb, api_cb_arg, blocking);
    if (msg == NULL) {
        return response;
    }
    // NOTE: there will be network latency while performing this AT command, so
    //       we use block time as delay time even applications call this API in
    //       non-blocking mode.
    msg->block_time = ESP_CFG_CMD_BLOCK_TIME_CIPCLOSE;
    msg->body.conn_close.conn = conn_in;
    response = eESPsendReqToMbox(msg, eESPinitATcmd);
    return response;
} // end of eESPconnClientClose

static espRes_t eESPconnClientSendLimit(
    espConn_t *conn, const uint8_t *data, size_t d_size, espApiCmdCbFn cb, void *const cb_arg,
    const uint32_t blocking
) {
    espRes_t  response = espOK;
    espMsg_t *msg = NULL;
    if ((data == NULL) || (d_size == 0)) {
        return espERRARGS;
    }
    msg = pxESPmsgCreate(ESP_CMD_TCPIP_CIPSEND, cb, cb_arg, blocking);
    if (msg == NULL) {
        return response;
    }
    // NOTE: there will be network latency while performing this AT command, so
    //       we use block time as delay time even applications call this API in
    //       non-blocking mode.
    msg->block_time = ESP_CFG_CMD_BLOCK_TIME_CIPSEND;
    msg->body.conn_send.conn = conn;
    msg->body.conn_send.data = data;
    msg->body.conn_send.d_size = d_size;
    // TODO: copy remote (client) IP address & port, in case the connection
    // corrupts in the middle of transfer.
    //       then we need to reconnect in some cases
    msg->body.conn_send.remote_ip = &(conn->remote_ip);
    msg->body.conn_send.remote_port = conn->remote_port;
    response = eESPsendReqToMbox(msg, eESPinitATcmd);
    return response;
} // end of eESPconnClientSendLimit

espRes_t eESPconnClientSend(
    espConn_t *conn, const uint8_t *data, size_t d_size, espApiCmdCbFn cb, void *const cb_arg,
    const uint32_t blocking
) {
    espRes_t       response = espOK;
    const uint8_t *curr_data_p = NULL;
    size_t         curr_d_size = 0;
    if ((conn == NULL) || (data == NULL) || (d_size == 0)) {
        return espERRARGS;
    }
    // if the connection (with specified ID) is already closed (due to timeout,
    // API misuse ... etc.), then we must reconnect using the same ID before we
    // can actually send the data out.
    if (conn->status.flg.active != ESP_CONN_ESTABLISHED) {
        return espERRNOAVAILCONN;
    }
    // get appropriate size to transfer
    curr_data_p = data;
    while (d_size > 0) {
        // we must check whether input data needs to be split to several
        // AT+CIPSEND commands in this API call.
        curr_d_size = ESP_MIN(ESP_CFG_MAX_BYTES_PER_CIPSEND, d_size);
        response = eESPconnClientSendLimit(conn, curr_data_p, curr_d_size, cb, cb_arg, blocking);
        if ((blocking == ESP_AT_CMD_BLOCKING) && (response == espERR)) {
            // for non-blocking API call, application writers must gather
            // response of each call to AT+CIPSEND if any of transfer (in that
            // AT+CIPSEND call) failed, then the entire data transfer would fail
            // & should be transferred again, recheck the application-layer
            // protocols used in user applications, sometimes the
            // application-layer protocol implementation may include error
            // checking on the data trasfer in order to re-send entire data.
            break;
        }
        d_size -= curr_d_size;
        curr_data_p += curr_d_size;
        // due to speed limits of ESP8266, users must set time interval between
        // 2 packets, users can also adjust this parameter with respect to
        // different ESP devices they use.
        vESPsysDelay(ESP_CFG_SEND_PKT_INTERVAL_MS);
    }
    return response;
} // end of eESPconnClientSend

espFnEn_t eESPconnIsServerActive(void) {
    return (espGlobal.evt_server == NULL ? ESP_DISABLE : ESP_ENABLE);
}

espRes_t
eESPstartServer(espNetConnPtr serverconn, espPort_t port, espEvtCbFn evt_cb, uint16_t timeout) {
    espRes_t response = espOK;
    response = eESPsetServer(serverconn, ESP_ENABLE, port, evt_cb, NULL, NULL, ESP_AT_CMD_BLOCKING);
    if (response == espERRARGS) {
        return response;
    }
    eESPsetServerTimeout(serverconn, timeout, NULL, NULL, ESP_AT_CMD_BLOCKING);
    return response;
} // end of eESPstartServer

espRes_t eESPstopServer(espNetConnPtr serverconn) {
    espRes_t response = espOK;
    response = eESPsetServer(serverconn, ESP_DISABLE, 0, NULL, NULL, NULL, ESP_AT_CMD_BLOCKING);
    return response;
} // end of eESPstopServer

espRes_t eESPnetconnRecvPkt(espNetConnPtr nc, espPbuf_t *pbuf) {
    espNetConn_t *nconn = (espNetConn_t *)nc;
    if ((nc == NULL) || (pbuf == NULL)) {
        return espERRARGS;
    }
    return eESPsysMboxPut(nconn->mbox_recv, (void *)pbuf, ESP_SYS_MAX_TIMEOUT);
} // end of  eESPnetconnRecvPkt

espRes_t eESPnetconnGrabNextPkt(espNetConnPtr nc, espPbuf_t **pbuf, uint32_t block_time_ms) {
    espNetConn_t *nconn = (espNetConn_t *)nc;
    if ((nc == NULL) || (pbuf == NULL)) {
        return espERRARGS;
    }
    return eESPsysMboxGet(nconn->mbox_recv, (void **)pbuf, block_time_ms);
} // end of  eESPnetconnGrabNextPkt

void vESPconnRunEvtCallback(espConn_t *conn, espEvtType_t evt_type) {
    espEvt_t e;
    e.type = evt_type;
    switch (evt_type) {
    case ESP_EVT_CONN_RECV:
        e.body.connDataRecv.conn = conn;
        conn->num_recv_pkt++;
        if (conn->status.flg.client == 0) {
            // the connection was created for server, then runs callback
            // function to notify application that IPD data is ready
            if (conn->cb != NULL) {
                conn->cb(&e);
            } else { // run default callback function list
                vESPrunEvtCallbacks(&e);
            }
        } else { // the connection was created for client
            conn->cb(&e);
        }
        break;
    case ESP_EVT_CONN_SEND:
        conn->num_sent_pkt++;
        break;
    case ESP_EVT_CONN_ACTIVE:
        break;
    case ESP_EVT_CONN_CLOSED:
        break;
    case ESP_EVT_CONN_ERROR:
        break;
    default:
        break;
    }
} // end of vESPconnRunEvtCallback
