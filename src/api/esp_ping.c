#include "esp/esp.h"
#include "esp/esp_private.h"
#if (ESP_CFG_PING != 0)

// TODO: test this function with newer ESP SDK firmware
espRes_t eESPping(
    const char *host, uint16_t host_len, uint32_t *resptime, const espApiCmdCbFn cb,
    void *const cb_arg, const uint32_t blocking
) {
    espRes_t  response = espOK;
    espMsg_t *msg = NULL;
    if ((host == NULL) || (resptime == NULL)) {
        return espERRARGS;
    }
    msg = pxESPmsgCreate(ESP_CMD_TCPIP_PING, cb, cb_arg, blocking);
    if (msg == NULL) {
        return response;
    }
    msg->block_time = 15000;
    msg->body.tcpip_ping.host = host;
    msg->body.tcpip_ping.host_len = host_len;
    msg->body.tcpip_ping.resptime = resptime;
    response = eESPsendReqToMbox(msg, eESPinitATcmd);
    uint32_t actual_resp_time = *msg->body.tcpip_ping.resptime;
    if (actual_resp_time == 0) {
        return espERRCONNFAIL;
    } else {
        return response;
    }
} // end of eESPping

#endif // ESP_CFG_PING
