#include "tests/integration/ESP_AT_parser/mqtt_client.h"

#define  TASK_MIN_STACK_SIZE             (( unsigned portSHORT ) 0x9e)
#define  MQTT_CONN_TX_BUF_SIZE           0x140
#define  MQTT_CONN_RX_BUF_SIZE           0x140
#define  MQTT_CMD_TIMEOUT_MS             0x1000


static mqttCtx_t     *m_client ;
static espNetConnPtr   clientnetconn;




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




static void vESPtestMqttClientApp( espNetConnPtr netconn, espConn_t*  conn,  mqttCtx_t *m_client )
{
    uint8_t   idx = 0;
    uint8_t   max_num_publish_msg = 5;
    // --- send CONNECT packet, with username/password for basic authentication ---
    m_client->ext_sysobjs[0] = (void *) netconn;
    m_client->ext_sysobjs[1] = (void *) conn;
    m_client->pkt.conn.clean_session = 0;
    m_client->pkt.conn.keep_alive_sec = MQTT_DEFAULT_KEEPALIVE_SEC;
    m_client->pkt.conn.protocol_lvl   = MQTT_CONN_PROTOCOL_LEVEL; 
    m_client->pkt.conn.client_id.data = "esp_freertos_stm32";
    m_client->pkt.conn.username.data  = "testuser";
    m_client->pkt.conn.password.data  = "guesspasswd";
    m_client->pkt.conn.client_id.len  = ESP_STRLEN( m_client->pkt.conn.client_id.data );
    m_client->pkt.conn.username.len   = ESP_STRLEN( m_client->pkt.conn.username.data  );
    m_client->pkt.conn.password.len   = ESP_STRLEN( m_client->pkt.conn.password.data  );
    mqttSendConnect( m_client );
    
    // --- subscribe topic of interests

    // --- publish messages with specific topic, in this test, we expect
    //     another client which  can act as both of subsriber or publisher, the
    //     client expects to wait for message sent by this device, or vice versa.
    for(idx=0 ; idx<max_num_publish_msg; idx++)
    {
    } // end of loop
    // --- wait for the message this device subscribed earlier in this test
    for(idx=0 ; idx<max_num_publish_msg; idx++)
    {
    } // end of loop
    // --- send DISCONNECT packet to broker ---
    m_client->pkt.disconn.reason_code = MQTT_REASON_NORMAL_DISCONNECTION;
    m_client->pkt.disconn.props       = NULL;
    mqttSendDisconnect( m_client );
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


