#include "tests/integration/ESP_AT_parser/mqtt_client.h"

#define  TASK_MIN_STACK_SIZE             (( unsigned portSHORT ) 0x9e)
#define  MQTT_CONN_TX_BUF_SIZE           0x140
#define  MQTT_CONN_RX_BUF_SIZE           0x140
#define  MQTT_CMD_TIMEOUT_MS             1000


static mqttCtx_t      *m_client ;
static espNetConnPtr   clientnetconn;

// application speciific data in this test
static const byte  app_json_header[] = "{ 'soil': [";
static const byte  app_json_footer[] = "] }";
static const uint8_t  app_json_header_len = 11;
static const uint8_t  app_json_footer_len = 3 ;

static const byte     json_obj_header = '{';
static const byte     json_obj_footer = '}';
static const byte     json_token_between_items = ',';
static const byte     json_moisture[]     = " 'mois': ";
static const byte     json_pH[]           = " 'pH': ";
static const byte     json_fertility[]    = " 'fert': ";
static const uint8_t  json_moisture_len   = 9;
static const uint8_t  json_pH_len         = 7;
static const uint8_t  json_fertility_len  = 9;

static uint8_t  json_moisture_history[4]  = { 31, 9, 47, 58 };
static uint8_t  json_pH_history[4]        = { 6, 7, 6, 5 };
static uint8_t  json_fertility_history[4] = { 32, 30, 29, 28 };




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



static espRes_t eESPtestMqttClientCallBack( espEvt_t*  evt )
{
    espRes_t  response = espOK;
    espConn_t  *conn   = NULL; 
    switch( evt->type )
    {
        case ESP_EVT_CONN_RECV:
            conn   = evt->body.connDataRecv.conn ;
            // put pointer of the new IPD data to message-box of the server,
            response = eESPnetconnRecvPkt( clientnetconn, conn->pbuf );
            break;
        case ESP_EVT_CONN_SEND:
            break;
        default:
            break;
    } // end of switch statement
    return  response;
} // end of eESPtestMqttClientCallBack






// the generated JSON string in following function will look like :
//
// { 
//     'soil': [
//         {
//            'mois': <number> ,
//            'pH':   <number> ,
//            'fert': <number> ,
//         }
//     ]
// }
//
static  void  vESPtestGenJSONmsg( byte *buf, word32 buff_len,  word32 *app_data_len, uint8_t select_data_idx )
{
    word32  copied_len = 0;
    uint8_t chosen_moisture   = json_moisture_history[select_data_idx] ;
    uint8_t chosen_pH         = json_pH_history[select_data_idx] ;
    uint8_t chosen_fertility  = json_fertility_history[select_data_idx] ;
    uint8_t num2str_buf[8]    = {0};
    uint8_t num_chr_append    =  0;

    ESP_MEMCPY( buf, &app_json_header, app_json_header_len );
    buf += app_json_header_len;
    copied_len += app_json_header_len;
   
    *buf++ = json_obj_header ;
    copied_len += 1;

    ESP_MEMCPY( buf, &json_moisture, json_moisture_len );
    buf += json_moisture_len;
    copied_len += json_moisture_len;

    num_chr_append = uiESPcvtNumToStr( &num2str_buf, (int)chosen_moisture, ESP_DIGIT_BASE_DECIMAL );
    ESP_MEMCPY( buf, &num2str_buf, num_chr_append );
    buf += num_chr_append;
    copied_len += num_chr_append;

    *buf++ = json_token_between_items ;
    copied_len += 1;

    ESP_MEMCPY( buf, &json_pH, json_pH_len );
    buf += json_pH_len;
    copied_len += json_pH_len;

    num_chr_append = uiESPcvtNumToStr( &num2str_buf, (int)chosen_pH, ESP_DIGIT_BASE_DECIMAL );
    ESP_MEMCPY( buf, &num2str_buf, num_chr_append );
    buf += num_chr_append;
    copied_len += num_chr_append;

    *buf++ = json_token_between_items ;
    copied_len += 1;

    ESP_MEMCPY( buf, &json_fertility, json_fertility_len );
    buf += json_fertility_len;
    copied_len += json_fertility_len;

    num_chr_append = uiESPcvtNumToStr( &num2str_buf, (int)chosen_fertility, ESP_DIGIT_BASE_DECIMAL );
    ESP_MEMCPY( buf, &num2str_buf, num_chr_append );
    buf += num_chr_append;
    copied_len += num_chr_append;

    *buf++ = json_token_between_items ;
    copied_len += 1;

    *buf++ = json_obj_footer ;
    copied_len += 1;
 
    ESP_MEMCPY( buf, &app_json_footer, app_json_footer_len );
    buf += app_json_footer_len;
    copied_len += app_json_footer_len;

    *app_data_len = copied_len;
} // end of vESPtestGenJSONmsg 





static void vESPtestMqttClientApp( espNetConnPtr netconn, espConn_t*  espconn,  mqttCtx_t *m_client )
{
    #define   MQTT_APP_MSG_MAX_LEN    0x90
    uint8_t   idx = 0;
    uint8_t   max_num_publish_msg = 5;
    // --- send CONNECT packet, with username/password for basic authentication ---
    mqttConn_t  *mconn = &m_client->send_pkt.conn ;
    m_client->ext_sysobjs[0] = (void *) netconn;
    m_client->ext_sysobjs[1] = (void *) espconn;
    mconn->clean_session  = 0;
    mconn->keep_alive_sec = MQTT_DEFAULT_KEEPALIVE_SEC;
    mconn->protocol_lvl   = MQTT_CONN_PROTOCOL_LEVEL; 
    mconn->client_id.data = "esp_freertos_stm32";
    mconn->username.data  = "testuser";
    mconn->password.data  = "guesspasswd";
    mconn->client_id.len  = ESP_STRLEN( mconn->client_id.data );
    mconn->username.len   = ESP_STRLEN( mconn->username.data  );
    mconn->password.len   = ESP_STRLEN( mconn->password.data  );
    mqttSendConnect( m_client );
    
    // --- subscribe topic of interests

    // --- publish messages with specific topic, in this test, we expect
    //     another client which  can act as both of subsriber or publisher, the
    //     client expects to wait for message sent by this device, or vice versa.
    mqttMsg_t  *pub_msg  =  &m_client->send_pkt.pub_msg;
    pub_msg->topic.data = "get/soilQuality/today"; 
    pub_msg->topic.len  = ESP_STRLEN( pub_msg->topic.data ); 
    pub_msg->retain     = 0; // we don't consider retain message in this test.
    // if QoS = 1 and we need to send duplicate PUBLISH packet, the duplicate
    // field will be set in mqttSendPublish()
    pub_msg->duplicate  = 0; 
    pub_msg->inbuf_len  = 0; 
    pub_msg->buff_len   = MQTT_APP_MSG_MAX_LEN; 
    pub_msg->buff       = ESP_MALLOC( sizeof(byte) * MQTT_APP_MSG_MAX_LEN ); 
    pub_msg->props      = NULL; 
    for(idx=0 ; idx<max_num_publish_msg; idx++)
    {
        pub_msg->packet_id  = idx + 1; 
        // in this test, we only consider to publish message with QoS = 0 or 1.
        pub_msg->qos        = 0x1 & idx; 
        vESPtestGenJSONmsg( pub_msg->buff, pub_msg->buff_len, &pub_msg->app_data_len, idx ); 
        mqttSendPublish( m_client );
    } // end of loop
    // --- wait for the message this device subscribed earlier in this test
    for(idx=0 ; idx<max_num_publish_msg; idx++)
    {
    } // end of loop
    // --- send DISCONNECT packet to broker ---
    m_client->send_pkt.disconn.reason_code = MQTT_REASON_NORMAL_DISCONNECTION;
    m_client->send_pkt.disconn.props       = NULL;
    mqttSendDisconnect( m_client );
    #undef   MQTT_APP_MSG_MAX_LEN 
} // end of vESPtestMqttClientApp





static void vESPtestMqttClientTask(void *params) 
{
    const uint16_t  max_req_cnt = 6;
    espRes_t        response ;
    uint8_t         devPresent ;
    espConn_t*      conn      =  NULL;
    const char      hostname[]= "124.9.12.185";
    uint16_t        host_len  = strlen(hostname);
    espPort_t       port      = 1883;

    // setup data structure required for this MQTT client
    clientnetconn =  NULL;
    clientnetconn =  pxESPnetconnCreate( ESP_NETCONN_TYPE_TCP );
    if(clientnetconn !=  NULL) {
        // establish TCP connection
        conn =  pxESPgetNxtAvailConn();
        eESPconnClientStart( conn, ESP_CONN_TYPE_TCP, hostname, host_len, port,
                             eESPtestMqttClientCallBack,  NULL, NULL, ESP_AT_CMD_BLOCKING );
        // start running MQTT client test
        vESPtestMqttClientApp( clientnetconn, conn, m_client );
        // close TCP connection
        eESPconnClientClose( conn, NULL, NULL, ESP_AT_CMD_BLOCKING );
        // de-initialize network connection object used in ESP parser.
        eESPnetconnDelete( clientnetconn );
    }
    // de-initialize the MQTT connection object
    ESP_ASSERT( m_client != NULL );
    mqttClientDeinit( m_client );
    m_client = NULL;
    // quit from AP 
    devPresent = 0x0;
    eESPdeviceSetPresent( devPresent, NULL, NULL );
    // delete this thread
    eESPsysThreadDelete( NULL );
} // end of vESPtestMqttClientTask





static void vESPtestInitTask( void  *params )
{
    uint8_t   isPrivileged = 0x1;
    uint8_t   waitUntilConnected = 0x1;
    int       status ;
    // initialize ESP AT parser
    espRes_t  response =  eESPinit( eESPtestEvtCallBack );
    // initialize  MQTT connection object for this test.
    m_client = NULL;
    status =  mqttClientInit( &m_client, MQTT_CMD_TIMEOUT_MS,
                               MQTT_CONN_TX_BUF_SIZE, MQTT_CONN_RX_BUF_SIZE );
    ESP_ASSERT( m_client != NULL );

    if( response == espOK ) {
        // turn on station mode as default, connect to preferred AP.
        eESPtestConnAP( waitUntilConnected );
        // once connected to AP, create server thread 
        response = eESPsysThreadCreate( NULL, "espTestMqttClient", vESPtestMqttClientTask, NULL ,
                                        TASK_MIN_STACK_SIZE, ESP_APPS_THREAD_PRIO , isPrivileged );
        ESP_ASSERT( response == espOK ); 
    }
    else {
        // failed to initialize ESP AT library
    }
    eESPsysThreadDelete( NULL );
} // end of vESPtestInitTask




void  vESPtestStartMqttClientTask( void )
{
    espRes_t response ;
    uint8_t isPrivileged = 0x1;
    // the ESP initialization thread takes the same priority as the 2 internal threads working 
    // in ESP AT software.
    response = eESPsysThreadCreate( NULL, "espInitTask", vESPtestInitTask, NULL ,
                                    TASK_MIN_STACK_SIZE, ESP_SYS_THREAD_PRIO ,  isPrivileged
                                  );
    ESP_ASSERT( response == espOK );
} // end of vESPtestStartMqttClientTask


#undef  TASK_MIN_STACK_SIZE      
#undef  MQTT_CONN_TX_BUF_SIZE   
#undef  MQTT_CONN_RX_BUF_SIZE   
#undef  MQTT_CMD_TIMEOUT_MS   

