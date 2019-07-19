#include "tests/integration/ESP_AT_parser/http_server.h"

#define  TASK_MIN_STACK_SIZE      (( unsigned portSHORT ) 0x7e)
#define  TEST_MAX_CHARS_URI       0x40

typedef enum {
    HTTP_METHOD_UNKNOWN = 0,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
} httpMthrEnum;


typedef struct {
   httpMthrEnum     method ; 
   const uint8_t    uri[TEST_MAX_CHARS_URI] ;
   uint16_t         resp_msg_len ;
   uint8_t*         resp_msg ;
} httpTestMsg_t;


static espNetConnPtr  serverconn;

static httpTestMsg_t  http_msg ;

static const  char  resp_msg_header1[] = ""
"HTTP/1.1 200 OK" ESP_CHR_CR ESP_CHR_LF
"Content-Type: text/html " ESP_CHR_CR ESP_CHR_LF 
"Content-Length: ";

static const  char  resp_msg_header2[] = ""
" " ESP_CHR_CR ESP_CHR_LF 
 ESP_CHR_CR ESP_CHR_LF 
"<html><head></head>"
"<body>"
"<p>Response from the test embedded server : </p>"
"<p>Request resource & argument from client : <b>";


static const  char  resp_msg_footer1[] = ""
"</b></p>"
"<p>The form below is used to test HTTP POST method. </p>"
"<form method='POST' name='testwebfm' id='testwebfm' action='/result'>"
"<p>"
"<input type='text' id='seek_query' name='seek_query' maxlength='16'>"
"<input type='submit' value='submit'>"
"</p>"
"</form>"
"<p>try putting random number of words in the text field above then click submit button</p>"
"<p>, it will generate HTTP POST request. </p>"
"</body></html>" ESP_CHR_CR  ESP_CHR_LF ;


static const  char  resp_msg_footer2[] = ""
"</b></p>"
"<p>POST method checked. <a href='/webfmpg'>Go back to web-form page</a></p>"
"<br><br>"
"</body></html>" ESP_CHR_CR  ESP_CHR_LF ;



static espRes_t eESPtestEvtCallBack( espEvt_t*  evt )
{
    espRes_t  response = espOK;
    switch( evt->type )
    {
        case ESP_EVT_INIT_FINISH :
            break;
        case ESP_EVT_WIFI_CONNECTED:
            break;
        case ESP_EVT_WIFI_DISCONNECTED:
            break;
        default:
            break;
    } // end of switch statement
    return  response;
} // end of eESPtestEvtCallBack



static espRes_t eESPtestiServerCallBack( espEvt_t*  evt )
{
    espRes_t  response = espOK;
    espConn_t  *conn   = NULL; 
    switch( evt->type )
    {
        case ESP_EVT_CONN_RECV:
            conn   = evt->body.connDataRecv.conn ;
            // put pointer of the new IPD data to message-box of the server,
            response = eESPnetconnRecvPkt( serverconn, conn->pbuf );
            break;
        case ESP_EVT_CONN_SEND:
            break;
        default:
            break;
    } // end of switch statement
    return  response;
} // end of eESPtestiServerCallBack




// hard-coded HTML entity for this test, we only extract http method & URI
static void  vESPtestHttpSimpleApp( espPbuf_t *pktbuf, httpTestMsg_t  *http_msg_p )
{
    espPbuf_t *curr_pbuf  =  NULL;
    uint8_t   *content_p  =  NULL;
    uint8_t   *resp_msg_p =  (uint8_t *)http_msg_p->resp_msg + sizeof(resp_msg_header1) - 1;
    uint16_t   num_bytes_uri = 0;
    uint16_t   num_bytes_html_entity = 0;
    uint16_t   num_str_cvt   = 0;
    // clean up previous application message
    http_msg_p->resp_msg_len  = sizeof(resp_msg_header1) - 1;
    http_msg_p->method = HTTP_METHOD_UNKNOWN;
    ESP_MEMSET( &http_msg_p->uri[0], 0x00, TEST_MAX_CHARS_URI) ;
    ESP_MEMSET( resp_msg_p, 0x00, sizeof(resp_msg_header2) + sizeof(resp_msg_footer1) + TEST_MAX_CHARS_URI );
    // get newly received application message
    curr_pbuf = pktbuf;
    content_p = curr_pbuf->payload;
    if(strncmp(content_p, "GET ", 4) == 0) {
        http_msg_p->method = HTTP_METHOD_GET;
        content_p += 4;
    }
    else if(strncmp(content_p, "POST ", 5) == 0) {
        http_msg_p->method = HTTP_METHOD_POST;
        content_p += 5;
    }
    if (http_msg_p->method == HTTP_METHOD_UNKNOWN) {
        return;   // this test doesn't handle other types of HTTP methods
    }
    // get URI from raw HTTP request message
    num_bytes_uri = uESPparseStrUntilToken( &http_msg_p->uri[0], content_p, TEST_MAX_CHARS_URI, ' ');
    content_p += num_bytes_uri + 1;
    // parse number of bytes in the HTTP response entity.
    uint8_t   content_length_str[4] = {0};
    num_bytes_html_entity  = num_bytes_uri + sizeof(resp_msg_header2) - 1 - 5;
    num_bytes_html_entity += (http_msg_p->method == HTTP_METHOD_POST ? sizeof(resp_msg_footer2): sizeof(resp_msg_footer1));
    num_str_cvt  =  uiESPcvtNumToStr( &content_length_str[0], num_bytes_html_entity, ESP_DIGIT_BASE_DECIMAL );
    ESP_MEMCPY( resp_msg_p, &content_length_str[0], num_str_cvt);
    http_msg_p->resp_msg_len  += num_str_cvt;
    resp_msg_p += num_str_cvt;
    ESP_MEMCPY( resp_msg_p, &resp_msg_header2[0], sizeof(resp_msg_header2));
    http_msg_p->resp_msg_len  += sizeof(resp_msg_header2) - 1;
    resp_msg_p += sizeof(resp_msg_header2) - 1;
    // copy the URI to HTTP response message (insert to hard-coded html here).
    ESP_MEMCPY( resp_msg_p, &http_msg_p->uri[0], num_bytes_uri );
    resp_msg_p += num_bytes_uri;
    http_msg_p->resp_msg_len  += num_bytes_uri;
    // parse different HTML code according to received HTTP method in this test.
    if (http_msg_p->method == HTTP_METHOD_GET) {
        ESP_MEMCPY( resp_msg_p, &resp_msg_footer1[0], sizeof(resp_msg_footer1) );
        http_msg_p->resp_msg_len  += sizeof(resp_msg_footer1);
    }
    else if (http_msg_p->method == HTTP_METHOD_POST) {
        ESP_MEMCPY( resp_msg_p, &resp_msg_footer2[0], sizeof(resp_msg_footer2) );
        http_msg_p->resp_msg_len  += sizeof(resp_msg_footer2);
    }
    // skip the rest of payload data
    curr_pbuf = curr_pbuf->next;
} // end of vESPtestHttpSimpleApp 




static void vESPtestHttpServerTask(void *params) 
{
    const uint16_t  max_req_cnt = 6;
    espRes_t        response ;
    uint8_t         devPresent ;
    espPbuf_t      *pktbuf;
    uint16_t        idx = 0;

    // setup data structure required for this simplified server
    serverconn =  pxESPnetconnCreate( ESP_NETCONN_TYPE_TCP );
    if(serverconn != NULL) { 
        response = eESPstartServer( serverconn, 80, eESPtestiServerCallBack, 20 );
        // turn on server mode in ESP device
        for(idx=0; idx<max_req_cnt; idx++)
        {   // call API function to wait for request message sent from client.
            pktbuf = NULL;
            eESPnetconnGrabNextPkt( serverconn, &pktbuf );
            // analyze rquest header at here, then generate HTTP response
            vESPtestHttpSimpleApp( pktbuf, &http_msg );
            //  send HTTP response by performing AT+CIPSEND immediately
            // , in this test, passthrough mode is supposed to be turned off.
            // Note that program shouldn't be delayed/halted by debugger in the middle of AT+CIPSEND.
            eESPconnClientSend( pktbuf->conn, http_msg.resp_msg,  http_msg.resp_msg_len,
                                NULL, NULL, ESP_AT_CMD_BLOCKING );
            // free the space allocated to packet buffer.
            vESPpktBufChainDelete( pktbuf );
        } // end of outer for-loop
        // turn off server mode
        eESPstopServer( serverconn );
        // delete server object
        eESPnetconnDelete( serverconn );
    }
    // quit from AP
    devPresent = 0x0;
    eESPdeviceSetPresent( devPresent, NULL, NULL );
    // delete current (the server) thread
    eESPsysThreadDelete( NULL );
} // end of vESPtestHttpServerTask





static void vESPtestInitTask(void *params)
{
    uint8_t   xState ;
    uint8_t   isPrivileged = 0x1;
    uint8_t   waitUntilConnected = 0x1;
    espRes_t  response =  eESPinit( eESPtestEvtCallBack );

    if( response == espOK ) {
        // turn on station mode as default, connect to preferred AP.
        eESPtestConnAP( waitUntilConnected );
        // once connected to AP, create server thread 
        response = eESPsysThreadCreate( NULL, "espTestHttpServer", vESPtestHttpServerTask, NULL ,
                                   (0x20 + TASK_MIN_STACK_SIZE), ESP_APPS_THREAD_PRIO , isPrivileged );
        ESP_ASSERT( response == espOK ); 
    }
    else {
        // failed to initialize ESP AT library
    }
    eESPsysThreadDelete( NULL );
} // end of vESPinitTask





void  vESPtestStartHttpServerTask( void )
{
    espRes_t response ;
    uint8_t isPrivileged = 0x1;
    // the ESP initialization thread takes the smae priority as the 2 internal threads working 
    // in ESP AT software.
    response = eESPsysThreadCreate( NULL, "espInitTask", vESPtestInitTask, NULL ,
                                    TASK_MIN_STACK_SIZE, ESP_SYS_THREAD_PRIO ,  isPrivileged
                                  );
    ESP_ASSERT( response == espOK );
    // allocate space for HTTP response message in this test.
    http_msg.method = HTTP_METHOD_UNKNOWN; 
    http_msg.resp_msg_len = 0;
    http_msg.resp_msg = NULL;
    http_msg.resp_msg = (uint8_t *) ESP_MALLOC(  sizeof(resp_msg_header1) + sizeof(resp_msg_header2) 
                                               + sizeof(resp_msg_footer1) +  TEST_MAX_CHARS_URI );
    ESP_ASSERT( http_msg.resp_msg != NULL );
    ESP_MEMCPY( http_msg.resp_msg, &resp_msg_header1[0], sizeof(resp_msg_header1) );
} // end of vESPtestStartHttpServerTask




