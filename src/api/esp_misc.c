#include "esp/esp.h"
#include "esp/esp_private.h"

extern espGlbl_t espGlobal;


static void vESPenableATecho (uint8_t enable )
{
    espMsg_t *msg      = NULL;
    espCmd_t  cmd;  
    if( enable == 0 ) { cmd = ESP_CMD_ATE0; }
    else{ cmd = ESP_CMD_ATE1; }
    msg = pxESPmsgCreate( cmd, NULL, NULL, ESP_AT_CMD_BLOCKING );
    if( msg == NULL) { return; }
    msg->block_time = 100;
    eESPsendReqToMbox( msg, eESPinitATcmd );
} // end of vESPenableATecho



static  espRes_t  eESPgetCurrATversion( void )
{
    espRes_t response = espERR ;
    espMsg_t *msg = NULL;
    msg = pxESPmsgCreate( ESP_CMD_GMR, NULL, NULL, ESP_AT_CMD_BLOCKING );
    if( msg == NULL) { return espERRMEM; }
    msg->block_time = 100;
    response = eESPsendReqToMbox( msg, eESPinitATcmd );
    return response;
} // end of eESPgetCurrATversion



espRes_t    eESPrestore( const espApiCmdCbFn cb, void* const cb_arg )
{
    espMsg_t *msg = NULL;
    espRes_t response = espERR ; 
    msg = pxESPmsgCreate( ESP_CMD_RESTORE, cb, cb_arg, ESP_AT_CMD_BLOCKING );
    if( msg == NULL) { return response; }
    msg->block_time = 5000;
    response = eESPsendReqToMbox( msg, eESPinitATcmd );
    if(response != espOK) { return response; }
    response = eESPresetWithDelay( 3, cb , cb_arg );
    return response;
} // end of eESPrestore



espRes_t    eESPresetWithDelay( uint32_t dly_ms, const espApiCmdCbFn cb , void* const cb_arg )
{
    espMsg_t *msg = NULL;
    espRes_t response = espERR ;
    if(dly_ms == 0) { return espERRARGS; }
#ifdef  ESP_CFG_PLATFORM_REINIT_ON_RST
    // enable low-level UART/GPIO hardware function
    eESPlowLvlDevInit(NULL);
    // initialize (UART Rx) receiving function everytime when we'd like to reset ESP device,
    // then the ESP device / host microcontroller can receive AT-command response or IPD data from other clients.
    vESPlowLvlRecvStopFn();
    response = eESPlowLvlRecvStartFn();
    if(response != espOK){ return response; }
#endif // end of ESP_CFG_PLATFORM_REINIT_ON_RST
    // start resetting ESP device & feed default configurations
    msg = pxESPmsgCreate( ESP_CMD_RESET, cb, cb_arg, ESP_AT_CMD_BLOCKING );
    if( msg == NULL ) { return response; }
    msg->block_time = 1000;
    msg->body.reset.delay = dly_ms;
    response = eESPsendReqToMbox( msg, eESPinitATcmd );
    if( response != espOKNOCMDREQ && response != espOK ) { return response; }
    // automatically run essential AT-command sequences to initialize ESP device.
    // turn on/off AT echo function, depends on the configuration ESP_CFG_AT_ECHO 
    vESPenableATecho( ESP_CFG_AT_ECHO );
    // read / record AT-command firmware version on current ESP device
    response = eESPgetCurrATversion();
    if(response != espOK){ return response; }
    // automatically set to station mode after reset de-assertion.
    eESPsetWifiMode( ESP_MODE_STA, ESP_SETVALUE_NOT_SAVE, NULL, NULL, ESP_AT_CMD_NONBLOCKING );
    // enable multiple TCP connections
    eESPsetMultiTCPconn( ESP_TCP_MULTIPLE_CONNECTION, NULL, NULL, ESP_AT_CMD_NONBLOCKING );
    // enable to send message for extra information of network connection.
    eESPsetConnPublishExtraMsg( ESP_ENABLE );
    // enable to send message for extra information of IPD data.
    eESPsetIPDextraMsg( ESP_ENABLE );
    // read TCP/IP connection status 
    response =  eESPgetConnStatus( ESP_AT_CMD_BLOCKING );
    return response;
} // end of eESPresetWithDelay



espRes_t    eESPsetConnPublishExtraMsg(espFnEn_t en)
{
    espRes_t response = espOK ; 
    espMsg_t *msg     = NULL;
    msg = pxESPmsgCreate( ESP_CMD_SYSMSG, NULL, NULL, ESP_AT_CMD_BLOCKING );
    if( msg == NULL) { return espERRMEM; }
    msg->block_time = 50;
    msg->body.sysargs.ext_info_netconn = en;
    response = eESPsendReqToMbox( msg, eESPinitATcmd );
    return  response;
} // end of eESPsetConnPublishExtraMsg


espRes_t   eESPsetIPDextraMsg( espFnEn_t en )
{
    espRes_t response = espOK ; 
    espMsg_t *msg     = NULL;
    msg = pxESPmsgCreate( ESP_CMD_TCPIP_CIPDINFO, NULL, NULL, ESP_AT_CMD_BLOCKING );
    if( msg == NULL) { return espERRMEM; }
    msg->block_time = 50;
    msg->body.sysargs.ext_info_ipd = en;
    response = eESPsendReqToMbox( msg, eESPinitATcmd );
    return  response;
} // end of eESPsetIPDextraMsg



espRes_t   eESPenterDeepSleep( uint32_t sleep_ms, const espApiCmdCbFn api_cb, void* const api_cb_arg, const uint32_t blocking )
{ 
// in general, ESP-01 device does not support wake up from deep sleep mode.
// unless you manually solder GPIO16 (XPD_DCDC pin on the chip) to the RST pin 
#if defined(ESP_CFG_DEV_ESP01)
    return  espERR;
#else
    espRes_t response = espOK ; 
    espMsg_t *msg     = NULL;
    msg = pxESPmsgCreate( ESP_CMD_GSLP, api_cb, api_cb_arg, blocking );
    if( msg == NULL) { return espERRMEM; }
    msg->body.deepslp.ms = sleep_ms;
    msg->block_time      = sleep_ms;
    response = eESPsendReqToMbox( msg, eESPinitATcmd );
    return  response;
#endif // end of ESP_CFG_DEV_ESP01 == 1
} // end of eESPenterDeepSleep




espRes_t  eESPcloseDevice( void )
{
    espRes_t response = espOK ; 
    eESPcoreLock();
    if (espGlobal.status.flg.dev_present == 1) 
    {
        eESPcoreUnlock();
#if (ESP_CFG_MODE_STATION != 0)
        // disconnect wifi if the ESP in station mode connected another AP.
        eESPstaQuit( NULL, NULL, ESP_AT_CMD_BLOCKING );
#endif // ESP_CFG_MODE_STATION  
        // TODO: 
        // * close all the established network connections in the ESP device.
        eESPcoreLock();
        espGlobal.status.flg.dev_present = 0;
    }
    eESPcoreUnlock();
    return response;
} // end of eESPcloseDevice



espRes_t    eESPdeviceIsPresent( void )
{
    uint8_t present = 0;
    eESPcoreLock();
    present = espGlobal.status.flg.dev_present;
    eESPcoreUnlock();
    return ( present != 0 ? espOK : espERRNODEVICE );
} // end of eESPdeviceIsPresent


