#include "esp/esp.h"
#include "esp/esp_private.h"
#if (ESP_CFG_MODE_STATION  != 0)

extern espGlbl_t espGlobal;



espRes_t  eESPstaHasIP( void )
{
    espRes_t  response = espOK;
    eESPcoreLock();
    if(espGlobal.dev.sta.has_ip == 0) {
        response = espERRNOIP; 
    }
    eESPcoreUnlock();
    return response;
} // end of eESPstaHasIP



espRes_t  eESPstaListAP(const char* ssid, uint16_t  ssid_len, espAP_t* aps, uint16_t apslen, uint16_t* num_ap_found, 
                        const espApiCmdCbFn cb, void* const cb_arg, const uint32_t blocking)
{
    espMsg_t *msg = NULL;
    espRes_t  response = espOK;
    if((aps==NULL) || (num_ap_found==NULL)) {
        return espERRARGS; 
    }
    response = eESPdeviceIsPresent();
    if(response != espOK){ return response; }
    *num_ap_found = 0;
    msg = pxESPmsgCreate( ESP_CMD_WIFI_CWLAP, cb, cb_arg, blocking );
    if( msg == NULL) { return response; }
    if( blocking == ESP_AT_CMD_BLOCKING ) {
        msg->block_time = 30000; // need longer time to search available APs
    }
    msg->body.ap_list.ssid   = ssid;
    msg->body.ap_list.ssid_len = ssid_len;
    msg->body.ap_list.aps    = aps;
    msg->body.ap_list.apslen = apslen;
    msg->body.ap_list.num_ap_found  = num_ap_found;
    response = eESPsendReqToMbox( msg, eESPinitATcmd );
    return response;
} // end of eESPstaListAP




espRes_t  eESPstaJoin(const char* ssid, uint16_t ssid_len, const char* pass, uint16_t pass_len, const espMac_t* mac, 
                      uint8_t saveDef, const espApiCmdCbFn cb, void* const cb_arg, const uint32_t blocking)
{
    espMsg_t *msg = NULL;
    espRes_t  response = espOK;
    if((ssid==NULL) || (pass==NULL)) {
        return espERRARGS; 
    }
    response = eESPdeviceIsPresent();
    if(response != espOK){ return response; }
    msg = pxESPmsgCreate( ESP_CMD_WIFI_CWJAP, cb, cb_arg, blocking );
    if( msg == NULL) { return response; }
    if( blocking == ESP_AT_CMD_BLOCKING ) {
        msg->block_time = 30000; // TODO: find proper timeout
    }
    msg->body.sta_join.name =  ssid;
    msg->body.sta_join.pass =  pass;
    msg->body.sta_join.name_len =  ssid_len;
    msg->body.sta_join.pass_len =  pass_len;
    msg->body.sta_join.mac  =  mac;
    msg->body.sta_join.def  =  saveDef;
    msg->body.sta_join.error_num = 0;
    response = eESPsendReqToMbox( msg, eESPinitATcmd );
    return response;
} // end of eESPstaJoin




espRes_t   eESPstaQuit(const espApiCmdCbFn cb, void* const cb_arg, const uint32_t blocking)
{
    espMsg_t *msg = NULL;
    espRes_t  response = espOK;
    response = eESPdeviceIsPresent();
    if(response != espOK){ return response; }
    msg = pxESPmsgCreate( ESP_CMD_WIFI_CWQAP, cb, cb_arg, blocking );
    if( msg == NULL) { return response; }
    if( blocking == ESP_AT_CMD_BLOCKING ) {
        msg->block_time = 5000; // TODO: find proper timeout
    }
    response = eESPsendReqToMbox( msg, eESPinitATcmd );
    return response;
} // end of eESPstaQuit



// TODO: why CIPSTA does not respond in ESP-01s ?
// always retrieve IP directly from ESP device.
espRes_t    eESPstaGetIP( espIp_t* ip, espIp_t* gw, espIp_t* nm, uint8_t saveDef, 
                          const espApiCmdCbFn cb, void* const cb_arg, const uint32_t blocking )
{
    espRes_t  response = espOK;
    espMsg_t *msg = NULL;
    if((ip==NULL) || (gw==NULL) || (nm==NULL)) {
        response = espERRARGS;
        return response;
    }
    response = eESPdeviceIsPresent();
    if(response != espOK){ return response; }
    msg = pxESPmsgCreate( ESP_CMD_WIFI_CIPSTA_GET, cb, cb_arg, blocking );
    if( msg == NULL) { return response; }
    if( blocking == ESP_AT_CMD_BLOCKING ) {
        msg->block_time = 5000; 
    }
    msg->body.sta_ap_getip.ip  = ip;
    msg->body.sta_ap_getip.gw  = gw;
    msg->body.sta_ap_getip.nm  = nm;
    msg->body.sta_ap_getip.def = saveDef;
    response = eESPsendReqToMbox( msg, eESPinitATcmd );
    return response;
} // end of eESPstaGetIP





// TODO: find better way to  test this
espRes_t  eESPstaCopyIP(espIp_t* ip, espIp_t* gw, espIp_t* nm)
{
    espRes_t  response = espOK;
    espNetAttr_t *d  ;
    if((ip==NULL) || (gw==NULL) || (nm==NULL)) {
        response = espERRARGS;
        return response;
    }
    d  = &(espGlobal.dev.sta);
    // copy IP from ESP global structure, or call eESPstaGetIP() to update IP 
    if( d->has_ip == 0 ) {
        response = eESPstaGetIP( &(d->ip), &(d->gw), &(d->nm), ESP_SETVALUE_NOT_SAVE , 
                                  NULL, NULL, ESP_AT_CMD_BLOCKING );
    }
    if( response == espOK ) {
        d->has_ip = 1;
        ESP_MEMCPY( ip, &(d->ip), sizeof(espIp_t) );
        ESP_MEMCPY( gw, &(d->gw), sizeof(espIp_t) );
        ESP_MEMCPY( nm, &(d->nm), sizeof(espIp_t) );
    }
    return response;
} // end of eESPstaCopyIP





#endif // end of ESP_CFG_MODE_STATION
