#include "esp/esp.h"
#include "esp/esp_private.h"
#if ( ESP_CFG_MODE_ACCESS_POINT != 0)

extern espGlbl_t espGlobal;


espRes_t    eESPapGetIP( espIp_t* ip, espIp_t* gw, espIp_t* nm, const espApiCmdCbFn cb, void* const cb_arg, const uint32_t blocking)
{
    espMsg_t *msg = NULL;
    espRes_t response = espERR ; 
    if((ip==NULL) || (gw==NULL) || (nm==NULL)) {
        return espERRARGS; 
    }
    msg = pxESPmsgCreate( ESP_CMD_WIFI_CIPAP_GET, cb, cb_arg, blocking );
    if( msg == NULL) { return response; }
    if( blocking == ESP_AT_CMD_BLOCKING ) {
        msg->block_time = 100;
    }
    msg->body.sta_ap_getip.ip  = ip;
    msg->body.sta_ap_getip.gw  = gw;
    msg->body.sta_ap_getip.nm  = nm;
    msg->body.sta_ap_getip.def = ESP_SETVALUE_NOT_SAVE;
    response = eESPsendReqToMbox( msg, eESPinitATcmd );
    return response;
} // end of eESPapGetIP





espRes_t    eESPapSetIP( const espIp_t* ip, const espIp_t* gw, const espIp_t* nm, uint8_t saveDef, 
                        const espApiCmdCbFn cb, void* const cb_arg, const uint32_t blocking)
{
    espMsg_t *msg = NULL;
    espRes_t response = espERR ; 
    if((ip==NULL) || (gw==NULL) || (nm==NULL)) {
        return espERRARGS; 
    }
    msg = pxESPmsgCreate( ESP_CMD_WIFI_CIPAP_SET, cb, cb_arg, blocking );
    if( msg == NULL) { return response; }
    if( blocking == ESP_AT_CMD_BLOCKING ) {
        msg->block_time = 50;
    }
    msg->body.sta_ap_setip.ip  = ip;
    msg->body.sta_ap_setip.gw  = gw;
    msg->body.sta_ap_setip.nm  = nm;
    msg->body.sta_ap_setip.def = saveDef;
    response = eESPsendReqToMbox( msg, eESPinitATcmd );
    return response;
} // end of eESPapSetIP








#endif // end of  ESP_CFG_MODE_ACCESS_POINT 

