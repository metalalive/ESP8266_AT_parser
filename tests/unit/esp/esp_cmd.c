#include "esp/esp.h"         // For espRes_t, espLLvlSendFn, espLLvlRstFn, etc.
#include "esp/esp_private.h" // For espMsg_t, espGlbl_t, espCmd_t, etc.
#include "unity.h"
#include "unity_fixture.h"
#include <string.h> // For memset, memcpy

// --- Mock variables for low-level functions and system delays ---
static uint8_t  mock_send_buffer[ESP_CFG_MAX_AT_CMD_SIZE + 1]; // +1 for null terminator
static size_t   mock_send_len = 0;
static uint32_t mock_send_timeout = 0;
static uint8_t  mock_reset_assert_called = 0;
static uint8_t  mock_reset_deassert_called = 0;

// --- Mock implementations ---

// Mock implementation for espLLvlSendFn
espRes_t mock_ll_send_fn(void *data, size_t len, uint32_t timeout) {
    TEST_ASSERT_LESS_OR_EQUAL_UINT32(ESP_CFG_MAX_AT_CMD_SIZE, len); // Ensure buffer won't overflow
    memcpy(mock_send_buffer, data, len);
    mock_send_buffer[len] = '\0'; // Null-terminate for string comparison
    mock_send_len = len;
    mock_send_timeout = timeout;
    return espOK;
}

// Mock implementation for espLLvlRstFn
espRes_t mock_ll_reset_fn(uint8_t state) {
    if (state == ESP_HW_RST_ASSERT) {
        mock_reset_assert_called = 1;
    } else if (state == ESP_HW_RST_DEASSERT) {
        mock_reset_deassert_called = 1;
    }
    return espOK;
}

TEST_GROUP(EspATcmd);

TEST_SETUP(EspATcmd) {
    // Reset mock variables before each test
    mock_send_len = 0;
    mock_send_timeout = 0;
    mock_reset_assert_called = 0;
    mock_reset_deassert_called = 0;
    memset(mock_send_buffer, 0, sizeof(mock_send_buffer));
}

TEST_TEAR_DOWN(EspATcmd) {}

// --- Test cases ---

// Test case for ESP_CMD_RESET when hardware reset is enabled (gbl->ll.reset_fn is not NULL)
TEST(EspATcmd, HardwareReset) {
    espMsg_t  msg = {0}; // Initialize with {0}
    espGlbl_t gbl = {0}; // Initialize with {0}

    msg.cmd = ESP_CMD_RESET;
    msg.is_blocking = ESP_AT_CMD_BLOCKING;
    msg.block_time = 1000;      // Example block time
    msg.body.reset.delay = 500; // Delay for hardware reset

    gbl.ll.send_fn = mock_ll_send_fn;
    gbl.ll.reset_fn = mock_ll_reset_fn;

    espRes_t result = eESPinitATcmd(&msg, &gbl);

    // Expect espOKNOCMDREQ because hardware reset handles it
    TEST_ASSERT_EQUAL(espOKNOCMDREQ, result);
    TEST_ASSERT_EQUAL(espOKNOCMDREQ, msg.res); // Message result should also be updated

    // Verify hardware reset functions were called
    TEST_ASSERT_EQUAL(1, mock_reset_assert_called);
    TEST_ASSERT_EQUAL(1, mock_reset_deassert_called);

    // Verify no AT command was sent via send_fn in this path
    TEST_ASSERT_EQUAL(0, mock_send_len);
}

// Test case for ESP_CMD_RESET when hardware reset is NOT enabled (gbl->ll.reset_fn is NULL)
TEST(EspATcmd, ATReset) {
    espMsg_t  msg = {0}; // Initialize with {0}
    espGlbl_t gbl = {0}; // Initialize with {0}

    msg.cmd = ESP_CMD_RESET;
    msg.is_blocking = ESP_AT_CMD_BLOCKING;
    msg.block_time = 1000;    // Example block time
    msg.body.reset.delay = 0; // Not relevant for AT reset path

    gbl.ll.send_fn = mock_ll_send_fn;
    gbl.ll.reset_fn = NULL; // Simulate no hardware reset function

    espRes_t result = eESPinitATcmd(&msg, &gbl);

    TEST_ASSERT_EQUAL(espOK, result);  // Expect espOK because command is sent
    TEST_ASSERT_EQUAL(espOK, msg.res); // Message result should also be updated

    // Verify hardware reset functions were NOT called
    TEST_ASSERT_EQUAL(0, mock_reset_assert_called);
    TEST_ASSERT_EQUAL(0, mock_reset_deassert_called);

    // Verify AT command was sent
    TEST_ASSERT_EQUAL_STRING("AT+RST\r\n", (char *)mock_send_buffer);
    TEST_ASSERT_EQUAL(8, mock_send_len);                  // "AT+RST\r\n" is 8 characters
    TEST_ASSERT_EQUAL(msg.block_time, mock_send_timeout); // Should be msg.block_time (1000)
}

TEST(EspATcmd, GMR) {
    espMsg_t  msg = {0}; // Initialize with {0}
    espGlbl_t gbl = {0}; // Initialize with {0}

    msg.cmd = ESP_CMD_GMR;
    msg.is_blocking = ESP_AT_CMD_BLOCKING;
    msg.block_time = 5000; // Typical block time for GMR

    gbl.ll.send_fn = mock_ll_send_fn;
    gbl.ll.reset_fn = NULL; // Not relevant for GMR

    espRes_t result = eESPinitATcmd(&msg, &gbl);

    TEST_ASSERT_EQUAL(espOK, result);  // Expect espOK because command is sent
    TEST_ASSERT_EQUAL(espOK, msg.res); // Message result should also be updated

    // Verify no hardware reset functions were called
    TEST_ASSERT_EQUAL(0, mock_reset_assert_called);
    TEST_ASSERT_EQUAL(0, mock_reset_deassert_called);

    // Verify AT command was sent
    TEST_ASSERT_EQUAL_STRING("AT+GMR\r\n", (char *)mock_send_buffer);
    TEST_ASSERT_EQUAL(8, mock_send_len);                  // "AT+GMR\r\n" is 8 characters
    TEST_ASSERT_EQUAL(msg.block_time, mock_send_timeout); // Should be msg.block_time (5000)
}

TEST(EspATcmd, DeepSleep) {
    espMsg_t  msg = {0}; // Initialize with {0}
    espGlbl_t gbl = {0}; // Initialize with {0}

    msg.cmd = ESP_CMD_GSLP;
    msg.is_blocking = ESP_AT_CMD_BLOCKING;
    msg.block_time = 2000;      // Example block time for GSLP
    msg.body.deepslp.ms = 4321; // Example sleep time

    gbl.ll.send_fn = mock_ll_send_fn;
    gbl.ll.reset_fn = NULL; // Not relevant for GSLP

    espRes_t result = eESPinitATcmd(&msg, &gbl);

    TEST_ASSERT_EQUAL(espOK, result);  // Expect espOK because command is sent
    TEST_ASSERT_EQUAL(espOK, msg.res); // Message result should also be updated

    // Verify no hardware reset functions were called
    TEST_ASSERT_EQUAL(0, mock_reset_assert_called);
    TEST_ASSERT_EQUAL(0, mock_reset_deassert_called);

    // Verify AT command was sent
    TEST_ASSERT_EQUAL_STRING("AT+GSLP=4321\r\n", (char *)mock_send_buffer);
    TEST_ASSERT_EQUAL(14, mock_send_len);                 // "AT+GSLP=1000\r\n" is 14 characters
    TEST_ASSERT_EQUAL(msg.block_time, mock_send_timeout); // Should be msg.block_time (2000)
}

TEST(EspATcmd, CWMODE_Current) {
    espMsg_t  msg = {0};
    espGlbl_t gbl = {0};

    msg.cmd = ESP_CMD_WIFI_CWMODE;
    msg.is_blocking = ESP_AT_CMD_BLOCKING;
    msg.block_time = 1000;
    msg.body.wifi_mode.def = ESP_SETVALUE_NOT_SAVE; // Set for current mode
    msg.body.wifi_mode.mode = ESP_MODE_STA;         // Station mode

    gbl.ll.send_fn = mock_ll_send_fn;
    gbl.ll.reset_fn = NULL;

    espRes_t result = eESPinitATcmd(&msg, &gbl);

    TEST_ASSERT_EQUAL(espOK, result);
    TEST_ASSERT_EQUAL(espOK, msg.res);

    TEST_ASSERT_EQUAL(0, mock_reset_assert_called);
    TEST_ASSERT_EQUAL(0, mock_reset_deassert_called);

    TEST_ASSERT_EQUAL_STRING("AT+CWMODE_CUR=1\r\n", (char *)mock_send_buffer);
    TEST_ASSERT_EQUAL(17, mock_send_len);
    TEST_ASSERT_EQUAL(msg.block_time, mock_send_timeout);
}

TEST(EspATcmd, CWMODE_Default) {
    espMsg_t  msg = {0};
    espGlbl_t gbl = {0};

    msg.cmd = ESP_CMD_WIFI_CWMODE;
    msg.is_blocking = ESP_AT_CMD_BLOCKING;
    msg.block_time = 1000;
    msg.body.wifi_mode.def = ESP_SETVALUE_SAVE; // Set for default/save to flash
    msg.body.wifi_mode.mode = ESP_MODE_AP;      // Access Point mode

    gbl.ll.send_fn = mock_ll_send_fn;
    gbl.ll.reset_fn = NULL;

    espRes_t result = eESPinitATcmd(&msg, &gbl);

    TEST_ASSERT_EQUAL(espOK, result);
    TEST_ASSERT_EQUAL(espOK, msg.res);

    TEST_ASSERT_EQUAL(0, mock_reset_assert_called);
    TEST_ASSERT_EQUAL(0, mock_reset_deassert_called);

    TEST_ASSERT_EQUAL_STRING("AT+CWMODE_DEF=2\r\n", (char *)mock_send_buffer);
    TEST_ASSERT_EQUAL(17, mock_send_len);
    TEST_ASSERT_EQUAL(msg.block_time, mock_send_timeout);
}

TEST(EspATcmd, CWJAP_Current) {
    espMsg_t  msg = {0};
    espGlbl_t gbl = {0};

    const char *ssid = "MyTestSSID", *pass = "MyTestPassword";
    espMac_t    mac4ap = {{0x11, 0x22, 0x33, 0x44, 0x55, 0x66}};

    msg.cmd = ESP_CMD_WIFI_CWJAP;
    msg.is_blocking = ESP_AT_CMD_BLOCKING;
    msg.block_time = 10000;                        // Longer timeout for Wi-Fi connection
    msg.body.sta_join.def = ESP_SETVALUE_NOT_SAVE; // Do not save to flash
    msg.body.sta_join.name = ssid;
    msg.body.sta_join.name_len = strlen(ssid);
    msg.body.sta_join.pass = pass;
    msg.body.sta_join.pass_len = strlen(pass);
    msg.body.sta_join.mac = &mac4ap;

    gbl.ll.send_fn = mock_ll_send_fn;
    gbl.ll.reset_fn = NULL;

    espRes_t result = eESPinitATcmd(&msg, &gbl);

    TEST_ASSERT_EQUAL(espOK, result);
    TEST_ASSERT_EQUAL(espOK, msg.res);

    TEST_ASSERT_EQUAL(0, mock_reset_assert_called);
    TEST_ASSERT_EQUAL(0, mock_reset_deassert_called);

    // Expected command: AT+CWJAP_CUR="MyTestSSID","MyTestPassword","11:22:33:44:55:66"\r\n
    TEST_ASSERT_EQUAL_STRING(
        "AT+CWJAP_CUR=\"MyTestSSID\",\"MyTestPassword\",\"11:22:33:44:55:66\"\r\n",
        (char *)mock_send_buffer
    );
    TEST_ASSERT_EQUAL(64, mock_send_len); // Updated length to include MAC address
    TEST_ASSERT_EQUAL(msg.block_time, mock_send_timeout);
}

TEST(EspATcmd, CIPAP_Set_Default) {
    espMsg_t  msg = {0};
    espGlbl_t gbl = {0};

    espIp_t ip = {{10, 0, 2, 14}};
    espIp_t gw = {{10, 0, 2, 1}};
    espIp_t nm = {{255, 255, 255, 0}};

    msg.cmd = ESP_CMD_WIFI_CIPAP_SET;
    msg.is_blocking = ESP_AT_CMD_BLOCKING;
    msg.block_time = 2000;
    msg.body.sta_ap_setip.def = ESP_SETVALUE_SAVE;
    msg.body.sta_ap_setip.ip = &ip;
    msg.body.sta_ap_setip.gw = &gw;
    msg.body.sta_ap_setip.nm = &nm;

    gbl.ll.send_fn = mock_ll_send_fn;
    gbl.ll.reset_fn = NULL;

    espRes_t result = eESPinitATcmd(&msg, &gbl);

    TEST_ASSERT_EQUAL(espOK, result);
    TEST_ASSERT_EQUAL(espOK, msg.res);

    TEST_ASSERT_EQUAL(0, mock_reset_assert_called);
    TEST_ASSERT_EQUAL(0, mock_reset_deassert_called);

    TEST_ASSERT_EQUAL_STRING(
        "AT+CIPAP_DEF=\"10.0.2.14\",\"10.0.2.1\",\"255.255.255.0\"\r\n", (char *)mock_send_buffer
    );
    TEST_ASSERT_EQUAL(53, mock_send_len);
    TEST_ASSERT_EQUAL(msg.block_time, mock_send_timeout);
}

TEST(EspATcmd, CIPSTART_TCP) {
    espMsg_t  msg = {0};
    espGlbl_t gbl = {0};

    espConn_t  *dummy_conn = pxESPgetNxtAvailConn(&gbl);
    const char *host = "www.example.com";
    espPort_t   port = 83;

    msg.cmd = ESP_CMD_TCPIP_CIPSTART;
    msg.is_blocking = ESP_AT_CMD_BLOCKING;
    msg.block_time = 5000; // Typical block time for connection
    msg.body.conn_start.conn = &dummy_conn;
    msg.body.conn_start.type = ESP_CONN_TYPE_TCP;
    msg.body.conn_start.host = host;
    msg.body.conn_start.host_len = strlen(host);
    msg.body.conn_start.port = port;
    // cb, api_cb, api_cb_arg, num, success are not used by eESPinitATcmd directly for command
    // generation

    gbl.ll.send_fn = mock_ll_send_fn;
    gbl.ll.reset_fn = NULL;

    espRes_t result = eESPinitATcmd(&msg, &gbl);

    TEST_ASSERT_EQUAL(espOK, result);
    TEST_ASSERT_EQUAL(espOK, msg.res);

    TEST_ASSERT_EQUAL(0, mock_reset_assert_called);
    TEST_ASSERT_EQUAL(0, mock_reset_deassert_called);

    // Expected command: AT+CIPSTART=0,"TCP","www.example.com",83\r\n
    TEST_ASSERT_EQUAL_STRING(
        "AT+CIPSTART=0,\"TCP\",\"www.example.com\",83\r\n", (char *)mock_send_buffer
    );
    TEST_ASSERT_EQUAL(42, mock_send_len);
    TEST_ASSERT_EQUAL(msg.block_time, mock_send_timeout);
}

TEST(EspATcmd, CIPCLOSE) {
    espMsg_t  msg = {0};
    espGlbl_t gbl = {0};

    espConn_t *dummy_conn = pxESPgetNxtAvailConn(&gbl);

    msg.cmd = ESP_CMD_TCPIP_CIPCLOSE;
    msg.is_blocking = ESP_AT_CMD_BLOCKING;
    msg.block_time = 1000; // Typical block time for closing connection
    msg.body.conn_close.conn = dummy_conn;

    gbl.ll.send_fn = mock_ll_send_fn;
    gbl.ll.reset_fn = NULL;

    espRes_t result = eESPinitATcmd(&msg, &gbl);

    TEST_ASSERT_EQUAL(espOK, result);
    TEST_ASSERT_EQUAL(espOK, msg.res);

    TEST_ASSERT_EQUAL(0, mock_reset_assert_called);
    TEST_ASSERT_EQUAL(0, mock_reset_deassert_called);

    // Expected command: AT+CIPCLOSE=0\r\n
    TEST_ASSERT_EQUAL_STRING("AT+CIPCLOSE=0\r\n", (char *)mock_send_buffer);
    TEST_ASSERT_EQUAL(15, mock_send_len);
    TEST_ASSERT_EQUAL(msg.block_time, mock_send_timeout);
}

TEST(EspATcmd, CIPSEND_config) {
    espMsg_t  msg = {0};
    espGlbl_t gbl = {0};

    // assume another connection object is being used not available
    espConn_t *occupied_conn = pxESPgetNxtAvailConn(&gbl);
    occupied_conn->status.flg.active = ESP_CONN_ESTABLISHED;
    espConn_t *dummy_conn = pxESPgetNxtAvailConn(&gbl);
    uint16_t   data_len = 499; // Example data length

    msg.cmd = ESP_CMD_TCPIP_CIPSEND;
    msg.is_blocking = ESP_AT_CMD_BLOCKING;
    msg.block_time = 2000;
    msg.body.conn_send.conn = dummy_conn;
    msg.body.conn_send.d_size = data_len;

    gbl.ll.send_fn = mock_ll_send_fn;
    gbl.ll.reset_fn = NULL;

    espRes_t result = eESPinitATcmd(&msg, &gbl);

    TEST_ASSERT_EQUAL(espOK, result);
    TEST_ASSERT_EQUAL(espOK, msg.res);

    TEST_ASSERT_EQUAL(0, mock_reset_assert_called);
    TEST_ASSERT_EQUAL(0, mock_reset_deassert_called);

    // Expected command: AT+CIPSEND=1,499\r\n
    TEST_ASSERT_EQUAL_STRING("AT+CIPSEND=1,499\r\n", (char *)mock_send_buffer);
    TEST_ASSERT_EQUAL(18, mock_send_len);
    TEST_ASSERT_EQUAL(msg.block_time, mock_send_timeout);
}

TEST(EspATcmd, StartSendData) {
    espMsg_t  msg = {0};
    espGlbl_t gbl = {0};

    espConn_t *test_conn = pxESPgetNxtAvailConn(&gbl);
    TEST_ASSERT_NOT_NULL(test_conn);
    TEST_ASSERT_EQUAL_UINT16(0, test_conn->num_sent_pkt);
    test_conn->num_sent_pkt += 7; // assume 7 packets have been transmitted

    const uint8_t dummy_data[] = "Hello, ESP!";
    uint16_t      dummy_data_len = sizeof(dummy_data) - 1; // Exclude null terminator

    msg.cmd = ESP_CMD_TCPIP_CIPSEND;
    msg.body.conn_send.conn = test_conn;
    msg.body.conn_send.data = (void *)dummy_data;
    msg.body.conn_send.d_size = dummy_data_len;

    gbl.ll.send_fn = mock_ll_send_fn;
    gbl.ll.reset_fn = NULL; // Not relevant for this test

    espRes_t result = eESPcmdStartSendData(&msg, &gbl);
    TEST_ASSERT_EQUAL(espOK, result);

    // Verify gbl->ll.send_fn was called with correct arguments
    TEST_ASSERT_EQUAL_STRING((char *)dummy_data, (char *)mock_send_buffer);
    TEST_ASSERT_EQUAL(dummy_data_len, mock_send_len);
    TEST_ASSERT_EQUAL(50, mock_send_timeout); // Hardcoded block_time in eESPcmdStartSendData
    TEST_ASSERT_EQUAL_UINT16(8, test_conn->num_sent_pkt);
}

TEST_GROUP_RUNNER(EspATcmd) {
    RUN_TEST_CASE(EspATcmd, HardwareReset);
    RUN_TEST_CASE(EspATcmd, ATReset);
    RUN_TEST_CASE(EspATcmd, GMR);
    RUN_TEST_CASE(EspATcmd, DeepSleep);
    RUN_TEST_CASE(EspATcmd, CWMODE_Current);
    RUN_TEST_CASE(EspATcmd, CWMODE_Default);
    RUN_TEST_CASE(EspATcmd, CWJAP_Current);
    RUN_TEST_CASE(EspATcmd, CIPAP_Set_Default);
    RUN_TEST_CASE(EspATcmd, CIPSTART_TCP);
    RUN_TEST_CASE(EspATcmd, CIPCLOSE);
    RUN_TEST_CASE(EspATcmd, CIPSEND_config);
    RUN_TEST_CASE(EspATcmd, StartSendData);
}
