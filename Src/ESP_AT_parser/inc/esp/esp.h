#ifndef __ESP_H
#define __ESP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp_includes.h"


// macro declaration
#define     xESPsetCurrATversion(v, _major, _minor, _patch)  \
            { (v)->major = (_major); (v)->minor = (_minor); (v)->patch = (_patch); }



// core API functions declaration in this ESP AT library.
espRes_t    eESPinit( espEvtCbFn cb );
espRes_t    eESPdeinit( void );

espRes_t    eESPreset( const espApiCmdCbFn cb, void* const cb_arg, const uint8_t blocking );
espRes_t    eESPresetWithDelay( uint32_t dly_ms, const espApiCmdCbFn cb , void* const cb_arg );

espRes_t    eESPrestore( const espApiCmdCbFn cb, void* const cb_arg );
espRes_t    eESPupdateSW( const espApiCmdCbFn cb, void* const cb_arg );

espRes_t    eESPenterDeepSleep( uint32_t sleep_ms, const espApiCmdCbFn api_cb, void* const api_cb_arg, const uint32_t blocking );
espRes_t    eESPsetBaud( uint32_t baud, const espApiCmdCbFn cb, void* const cb_arg, const uint8_t blocking);
espRes_t    eESPsetWifiMode( espMode_t mode, uint8_t saveDef, const espApiCmdCbFn cb, void* const cb_arg, const uint8_t blocking);
espRes_t    eESPsetTransmitMode( espTransMode_t mode,     const espApiCmdCbFn cb, void* const cb_arg, const uint8_t blocking);
espRes_t    eESPsetMultiTCPconn( espMultiConn_t mux_conn, const espApiCmdCbFn cb, void* const cb_arg, const uint8_t blocking);



// lock / unlock the entire ESP AT software & underlying system, avoid them from data corruption.
espRes_t    eESPcoreLock(void);
espRes_t    eESPcoreUnlock(void);

espRes_t    eESPevtRegister( espEvtCbFn cb );
espRes_t    eESPevtUnregister( espEvtCbFn cb );
// run event callback function(s) registered in ESP AT software.
void        vESPrunEvtCallbacks( espEvt_t *evtp );

espRes_t    eESPcloseDevice( void );
espRes_t    eESPdeviceIsPresent( void );

// the entry point to process the received data from ESP device and analyze the result of the previously sent AT command.
espRes_t    eESPprocessRecvDataFromDev( const void* data, size_t len );

// once user thread calls API functions, 
// * one thread, created by this AT software, dedicates to collecting the requests generated from these API functions, 
//   then translate to AT-format string, send it to ESP device.
void    vESPthreadATreqHandler( void* const arg );

// * another thread, created by this AT software, dedicates to receiving raw data, the response string of the AT command,
//   , then translate the response string to data structure used in this ESP AT software.
void    vESPthreadATrespHandler( void* const arg );

// service routine that should be called in UART Rx / DMA interrupt in order to transmit pieces
// of received response  data string (if very large) to response-handling thread above 
// (vESPthreadATrespHandler)
espRes_t  eESPappendRecvRespISR( uint8_t* data, uint16_t data_len );

// process the received response raw string, analyze/extract them, then feed to the body of message structure,
// for the message (the AT command request) ESP AT software is processing.
espRes_t  eESPprocessPieceRecvResp( espBuf_t *recv_buf, uint8_t *isFinalPieceResp);

// get local IP/Mac address
espRes_t  eESPgetLocalIPmac( espIp_t* sta_ip, espMac_t* sta_mac,  espIp_t* ap_ip, espMac_t* ap_mac, 
                             const espApiCmdCbFn cb, void* const cb_arg, const uint8_t blocking );

// flush items in message boxes
espRes_t    eESPflushMsgBox( espSysMbox_t mb );








// enable / disable the server on ESP device
espRes_t    eESPsetServer( espNetConnPtr serverconn, espFnEn_t en, espPort_t port, espEvtCbFn evt_cb, 
                           const espApiCmdCbFn api_cb,  void* const api_cb_arg, const uint8_t blocking );

espRes_t    eESPsetServerTimeout( espNetConnPtr serverconn, uint16_t timeout,  const espApiCmdCbFn cb, 
                                  void* const cb_arg, const uint8_t blocking );

// helper function to set up everything essential for starting / stopping a server
espRes_t     eESPstartServer( espNetConnPtr serverconn, espPort_t port, espEvtCbFn evt_cb, uint16_t timeout );

espRes_t     eESPstopServer(  espNetConnPtr serverconn );

espFnEn_t    eESPconnIsServerActive( void );

espRes_t       eESPgetConnStatus( const uint32_t blocking );

espRes_t       eESPsetConnPublishExtraMsg(espFnEn_t en);

espRes_t       eESPsetIPDextraMsg( espFnEn_t en );

void           vESPconnRunEvtCallback( espConn_t *conn, espEvtType_t evt_type );

espRes_t       eESPnetconnRecvPkt( espNetConnPtr  nc, espPbuf_t *pbuf );

espRes_t       eESPnetconnGrabNextPkt( espNetConnPtr  nc, espPbuf_t **pbuf, uint32_t block_time_ms );

espRes_t       eESPconnClientStart( espConn_t *conn_in, espConnType_t type, const char* const host, uint16_t host_len, espPort_t port, 
                                    espEvtCbFn evt_cb,  espApiCmdCbFn api_cb,  void* const api_cb_arg,  const uint32_t blocking );
espRes_t       eESPconnClientClose( espConn_t *conn_in, espApiCmdCbFn cb,  void* const cb_arg, const uint32_t blocking);

espRes_t       eESPconnClientSend( espConn_t *conn, const uint8_t *data, size_t d_size, espApiCmdCbFn cb,  
                                   void* const cb_arg, const uint32_t blocking);

uint8_t       ucESPconnGetID( espConn_t * conn );

espConn_t*    pxESPgetNxtAvailConn( void );



// used for updating connection status o ESP device.
void        vESPparseRecvATrespLine( uint8_t *data_line_buf, uint16_t buf_idx, uint8_t *isEndOfResp );
espRes_t    eESPparseNetConnStatus( uint8_t *data_line_buf );


// used for extracting IPD data (incoming packet data) from ESP device
espRes_t    eESPparseIPDsetup( uint8_t* metadata );
espRes_t    eESPparseIPDcopyData( const uint8_t* data, uint32_t data_len );
espRes_t    eESPparseIPDreset( void );


espPbuf_t*   pxESPpktBufCreate( size_t len );
espRes_t      eESPpktBufCopy( espPbuf_t *des_p, void *src_p , size_t len );
void          vESPpktBufItemDelete( espPbuf_t *buff );
void          vESPpktBufChainDelete( espPbuf_t *buff_head );










#ifdef __cplusplus
} // end of extern-C statement
#endif
#endif // end of  __ESP_INCLUDES_H 

