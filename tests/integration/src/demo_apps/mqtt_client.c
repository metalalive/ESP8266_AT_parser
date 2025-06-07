#include "demo_apps/mqtt_client.h"

#define TASK_MIN_STACK_SIZE   ((unsigned portSHORT)0xbe)
#define MQTT_CONN_TX_BUF_SIZE 0x100
#define MQTT_CONN_RX_BUF_SIZE 0x100
#define MQTT_CMD_TIMEOUT_MS   25000

#define TEST_NUM_MOCK_DATA          4
#define ITEST_MQ_BROKER_HOSTNAME    "123.4.56.78"
#define ITEST_MQ_BROKER_PORT        1883
#define ITEST_MQ_BROKER_CLIENT_ID   "esp_pubsub_client"
#define ITEST_MQ_BROKER_AUTH_UNAME  "your-broker-username"
#define ITEST_MQ_BROKER_AUTH_PASSWD "your-broker-password"

#define ITEST_MQ_TOPIC_PREFIX "/experiment/esp-wifi"

extern espGlbl_t espGlobal;

static mqttCtx_t    *m_client;
static espNetConnPtr clientnetconn;

// topics that will be subsribed in this test.
static mqttTopic_t subs_topics[2];

// application speciific data in this test
static const byte    app_json_header[] = "{ 'soil': [";
static const byte    app_json_footer[] = "] }";
static const uint8_t app_json_header_len = 11;
static const uint8_t app_json_footer_len = 3;

static const byte    json_obj_header = '{';
static const byte    json_obj_footer = '}';
static const byte    json_separator_comma = ',';
static const byte    json_moisture[] = " 'mois': ";
static const byte    json_pH[] = " 'pH': ";
static const byte    json_fertility[] = " 'fert': ";
static const uint8_t json_moisture_len = 9;
static const uint8_t json_pH_len = 7;
static const uint8_t json_fertility_len = 9;

static uint8_t json_moisture_history[TEST_NUM_MOCK_DATA] = {31, 9, 47, 58};
static uint8_t json_pH_history[TEST_NUM_MOCK_DATA] = {6, 7, 6, 5};
static uint8_t json_fertility_history[TEST_NUM_MOCK_DATA] = {32, 30, 29, 28};
static uint8_t mock_db_select_data_idx = 0;

static espRes_t eESPtestEvtCallBack(espEvt_t *evt) {
    espRes_t response = espOK;
    switch (evt->type) {
    case ESP_EVT_INIT_FINISH:
        break;
    case ESP_EVT_WIFI_CONNECTED:
        break;
    case ESP_EVT_WIFI_DISCONNECTED:
        break;
    default:
        break;
    } // end of switch statement
    return response;
}

static espRes_t eESPtestMqttClientCallBack(espEvt_t *evt) {
    espRes_t   response = espOK;
    espConn_t *conn = NULL;
    switch (evt->type) {
    case ESP_EVT_CONN_RECV:
        conn = evt->body.connDataRecv.conn;
        // put pointer of the new IPD data to message-box of the server,
        response = eESPnetconnRecvPkt(clientnetconn, conn->pbuf);
        break;
    case ESP_EVT_CONN_SEND:
        break;
    default:
        break;
    }
    return response;
}

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
static void vESPtestGenJSONmsg(byte *buf, word32 buff_len, word32 *app_data_len) {
    word32  copied_len = 0;
    uint8_t chosen_moisture = 0, chosen_pH = 0, chosen_fertility = 0;
    uint8_t num2str_buf[8] = {0};
    uint8_t num_chr_append = 0;
    uint8_t idx = 0, jdx = 0;

    ESP_MEMCPY(buf, &app_json_header, app_json_header_len);
    buf += app_json_header_len;
    copied_len += app_json_header_len;

    for (idx = 0; idx < 2; idx++) {
        jdx = (mock_db_select_data_idx + idx) % TEST_NUM_MOCK_DATA;
        chosen_moisture = json_moisture_history[jdx];
        chosen_pH = json_pH_history[jdx];
        chosen_fertility = json_fertility_history[jdx];
        json_moisture_history[jdx] = 1 + chosen_moisture;
        json_pH_history[jdx] = 1 + chosen_pH;
        json_fertility_history[jdx] = 1 + chosen_fertility;

        *buf++ = json_obj_header;
        copied_len += 1;

        ESP_MEMCPY(buf, &json_moisture, json_moisture_len);
        buf += json_moisture_len;
        copied_len += json_moisture_len;

        num_chr_append =
            uiESPcvtNumToStr(&num2str_buf[0], (int)chosen_moisture, ESP_DIGIT_BASE_DECIMAL);
        ESP_MEMCPY(buf, &num2str_buf, num_chr_append);
        buf += num_chr_append;
        copied_len += num_chr_append;

        *buf++ = json_separator_comma;
        copied_len += 1;

        ESP_MEMCPY(buf, &json_pH, json_pH_len);
        buf += json_pH_len;
        copied_len += json_pH_len;

        num_chr_append = uiESPcvtNumToStr(&num2str_buf[0], (int)chosen_pH, ESP_DIGIT_BASE_DECIMAL);
        ESP_MEMCPY(buf, &num2str_buf, num_chr_append);
        buf += num_chr_append;
        copied_len += num_chr_append;

        *buf++ = json_separator_comma;
        copied_len += 1;

        ESP_MEMCPY(buf, &json_fertility, json_fertility_len);
        buf += json_fertility_len;
        copied_len += json_fertility_len;

        num_chr_append =
            uiESPcvtNumToStr(&num2str_buf[0], (int)chosen_fertility, ESP_DIGIT_BASE_DECIMAL);
        ESP_MEMCPY(buf, &num2str_buf, num_chr_append);
        buf += num_chr_append;
        copied_len += num_chr_append;

        *buf++ = json_separator_comma;
        copied_len += 1;

        *buf++ = json_obj_footer;
        copied_len += 1;

        *buf++ = json_separator_comma;
        copied_len += 1;
    }

    ESP_MEMCPY(buf, &app_json_footer, app_json_footer_len);
    buf += app_json_footer_len;
    copied_len += app_json_footer_len;

    *app_data_len = copied_len;
    mock_db_select_data_idx = (mock_db_select_data_idx + 1) % TEST_NUM_MOCK_DATA;
} // end of vESPtestGenJSONmsg

static void vESPtestMqttInitSubsTopics(void) {
    subs_topics[0].filter.data = (byte *)&(ITEST_MQ_TOPIC_PREFIX "/control/sprayer/worksecs");
    subs_topics[0].filter.len = ESP_STRLEN((const char *)subs_topics[0].filter.data);
    subs_topics[0].qos = MQTT_QOS_1;
    subs_topics[0].reason_code = 0;
    subs_topics[0].sub_id = 0;

    subs_topics[1].filter.data = (byte *)&(ITEST_MQ_TOPIC_PREFIX "/sensor/moisture/threshold");
    subs_topics[1].filter.len = ESP_STRLEN((const char *)subs_topics[1].filter.data);
    subs_topics[1].qos = MQTT_QOS_0;
    subs_topics[1].reason_code = 0;
    subs_topics[1].sub_id = 0;
} // end of vESPtestMqttInitSubsTopics

static void vESPtestMqttClientApp(espNetConnPtr netconn, mqttCtx_t *m_client) {
#define MQTT_APP_MSG_MAX_LEN 0x90
    int     result = MQTT_RETURN_SUCCESS;
    uint8_t idx = 0, max_num_publish_msg = 5;
    // --- send CONNECT packet, with username/password for basic authentication
    mqttConn_t *mconn = &m_client->send_pkt.conn;
    m_client->ext_sysobjs[0] = (void *)netconn;
    mconn->clean_session = 0;
    mconn->keep_alive_sec = MQTT_DEFAULT_KEEPALIVE_SEC;
    mconn->protocol_lvl = MQTT_CONN_PROTOCOL_LEVEL;
    mconn->client_id.data = (byte *)&(ITEST_MQ_BROKER_CLIENT_ID);
    mconn->username.data = (byte *)&(ITEST_MQ_BROKER_AUTH_UNAME);
    mconn->password.data = (byte *)&(ITEST_MQ_BROKER_AUTH_PASSWD);
    mconn->client_id.len = ESP_STRLEN((const char *)mconn->client_id.data);
    mconn->username.len = ESP_STRLEN((const char *)mconn->username.data);
    mconn->password.len = ESP_STRLEN((const char *)mconn->password.data);
    result = mqttSendConnect(m_client);
    ESP_ASSERT(result == MQTT_RETURN_SUCCESS);

    // --- publish messages with specific topic, in this test, we expect
    //     another client which  can act as both of subsriber or publisher, the
    //     client expects to wait for message sent by this device, or vice
    //     versa.
    mqttMsg_t *pub_msg = &m_client->send_pkt.pub_msg;
    pub_msg->topic.data = (byte *)&(ITEST_MQ_TOPIC_PREFIX "/get/soilQuality/today");
    pub_msg->topic.len = ESP_STRLEN((const char *)pub_msg->topic.data);
    pub_msg->retain = 0; // we don't consider retain message in this test.
    // if QoS = 1 and we need to send duplicate PUBLISH packet, the duplicate
    // field will be set in mqttSendPublish()
    pub_msg->duplicate = 0;
    pub_msg->inbuf_len = 0;
    pub_msg->buff_len = MQTT_APP_MSG_MAX_LEN;
    pub_msg->buff = ESP_MALLOC(sizeof(byte) * MQTT_APP_MSG_MAX_LEN);
    pub_msg->props = NULL;
    for (idx = 0; idx < max_num_publish_msg; idx++) {
        pub_msg->packet_id = idx + 1;
        // in this test, we only consider to publish message with QoS = 0 or 1.
        pub_msg->qos = 0x1 & idx;
        vESPtestGenJSONmsg(pub_msg->buff, pub_msg->buff_len, &pub_msg->app_data_len);
        result = mqttSendPublish(m_client);
        ESP_ASSERT(result == MQTT_RETURN_SUCCESS);
        vESPsysDelay(1000);
    }

    // --- subscribe topic of interests
    vESPtestMqttInitSubsTopics();
    mqttPktSubs_t *subs = &m_client->send_pkt.subs;
    subs->packet_id = 11;
    subs->topic_cnt = 2;
    subs->topics = &subs_topics[0];
    subs->props = NULL;
    result = mqttSendSubscribe(m_client);
    ESP_ASSERT(result == MQTT_RETURN_SUCCESS);

    m_client->cmd_timeout_ms = ESP_SYS_MAX_TIMEOUT;
    // --- wait for the message this device subscribed earlier in this test
    for (idx = 0; idx < max_num_publish_msg; idx++) {
        mqttMsg_t *recv_msg = &m_client->recv_pkt.pub_msg;
        ESP_MEMSET((void *)recv_msg, 0x00, sizeof(mqttMsg_t));
        result = mqttClientWaitPkt(m_client, MQTT_PACKET_TYPE_PUBLISH, 0, (void *)recv_msg);
        ESP_ASSERT(result == MQTT_RETURN_SUCCESS);
    } // end of loop
    m_client->cmd_timeout_ms = MQTT_CMD_TIMEOUT_MS;
    vESPsysDelay(1000);

    // --- unsubscribe topics
    mqttPktUnsubs_t *unsubs = &m_client->send_pkt.unsubs;
    unsubs->packet_id = 12;
    unsubs->topic_cnt = 2;
    unsubs->topics = &subs_topics[0];
    unsubs->props = NULL;
    result = mqttSendUnsubscribe(m_client);
    ESP_ASSERT(result == MQTT_RETURN_SUCCESS);
    vESPsysDelay(1000);

    // --- send DISCONNECT packet to broker ---
    m_client->send_pkt.disconn.reason_code = MQTT_REASON_NORMAL_DISCONNECTION;
    m_client->send_pkt.disconn.props = NULL;
    result = mqttSendDisconnect(m_client);
    ESP_ASSERT(result == MQTT_RETURN_SUCCESS);
#undef MQTT_APP_MSG_MAX_LEN
} // end of vESPtestMqttClientApp

static void vESPtestMqttClientTask(void *params) {
    const char hostname[] = ITEST_MQ_BROKER_HOSTNAME;
    uint16_t   host_len = ESP_STRLEN(hostname);
    espPort_t  port = ITEST_MQ_BROKER_PORT;
    espConn_t *conn = pxESPgetNxtAvailConn(&espGlobal);
    ESP_ASSERT(conn != NULL);
    conn->type = ESP_CONN_TYPE_TCP;

    // setup data structure required for this MQTT client
    clientnetconn = NULL;
    clientnetconn = pxESPnetconnCreate(conn);
    if (clientnetconn != NULL) {
        // establish TCP connection
        eESPconnClientStart(
            conn, conn->type, hostname, host_len, port, eESPtestMqttClientCallBack, NULL, NULL,
            ESP_AT_CMD_BLOCKING
        );
        // start running MQTT client test
        vESPtestMqttClientApp(clientnetconn, m_client);
        // close TCP connection
        eESPconnClientClose(conn, NULL, NULL, ESP_AT_CMD_BLOCKING);
        // de-initialize network connection object used in ESP parser.
        eESPnetconnDelete(clientnetconn);
    }
    // de-initialize the MQTT connection object
    ESP_ASSERT(m_client != NULL);
    mqttClientDeinit(m_client);
    m_client = NULL;
    // quit from AP
    eESPcloseDevice();
    // delete this thread
    eESPsysThreadDelete(NULL);
} // end of vESPtestMqttClientTask

static void vESPtestInitTask(void *params) {
    uint8_t  isPrivileged = 0x1, waitUntilConnected = 0x1;
    espRes_t response = eESPinit(eESPtestEvtCallBack);
    ESP_ASSERT(response == espOK);
    // initialize  MQTT connection object for this test.
    m_client = NULL;
    mqttClientInit(&m_client, MQTT_CMD_TIMEOUT_MS, MQTT_CONN_TX_BUF_SIZE, MQTT_CONN_RX_BUF_SIZE);
    ESP_ASSERT(m_client != NULL);
    // turn on station mode as default, connect to preferred AP.
    response = eESPtestConnAP(waitUntilConnected);
    ESP_ASSERT(response == espOK);
    // once connected to AP, create server thread
    response = eESPsysThreadCreate(
        NULL, "espTestMqttClient", vESPtestMqttClientTask, NULL, TASK_MIN_STACK_SIZE,
        ESP_APPS_THREAD_PRIO, isPrivileged
    );
    ESP_ASSERT(response == espOK);
    eESPsysThreadDelete(NULL);
} // end of vESPtestInitTask

void vCreateAllTestTasks(void) {
    uint8_t isPrivileged = 0x1;
    // the ESP initialization thread takes the same priority as the 2 internal
    // threads working in ESP AT software.
    espRes_t response = eESPsysThreadCreate(
        NULL, "espInitTask", vESPtestInitTask, NULL, TASK_MIN_STACK_SIZE, ESP_SYS_THREAD_PRIO,
        isPrivileged
    );
    ESP_ASSERT(response == espOK);
}

#undef TASK_MIN_STACK_SIZE
#undef MQTT_CONN_TX_BUF_SIZE
#undef MQTT_CONN_RX_BUF_SIZE
#undef MQTT_CMD_TIMEOUT_MS
