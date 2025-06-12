#include "esp/esp.h"
#include "esp/esp_private.h"
#include "unity.h"
#include "unity_fixture.h"
#include <string.h>

TEST_GROUP(ATcmdRespParser);
TEST_GROUP(ATnetConnStatusParser);
TEST_GROUP(ATnetDataParser);

TEST_SETUP(ATcmdRespParser) {}
TEST_TEAR_DOWN(ATcmdRespParser) {}

TEST_SETUP(ATnetConnStatusParser) {}
TEST_TEAR_DOWN(ATnetConnStatusParser) {}

TEST_SETUP(ATnetDataParser) {}
TEST_TEAR_DOWN(ATnetDataParser) {}

TEST(ATcmdRespParser, GMRversion) {
    espMsg_t  test_msg = {.cmd = ESP_CMD_GMR, .res = espOK};
    espGlbl_t gbl = {.msg = &test_msg};
    uint8_t   data_line_buf[128] = {0}, isEndOfATresp = 0;
    uint16_t  buf_idx = 0;

    // Sub Case 1: Parsing AT version line
    // Expected format: "AT version:X.Y.Z(date time)"
    const char *at_version_line = "AT version:1.7.4.0(Apr 3 2020 10:50:00)" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, at_version_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0; // Ensure it's reset for this line

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL_UINT8(1, gbl.dev.version_at.major);
    TEST_ASSERT_EQUAL_UINT8(7, gbl.dev.version_at.minor);
    TEST_ASSERT_EQUAL_UINT8(4, gbl.dev.version_at.patch);
    TEST_ASSERT_EQUAL_UINT8(1, gbl.status.flg.dev_present);

    // Sub Case 2: Parsing SDK version line
    // Expected format: "SDK version:X.Y.Z(build_id)"
    const char *sdk_version_line = "SDK version:3.0.1(f872739)" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, sdk_version_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0; // Ensure it's reset for this line
    memset(&gbl.dev.version_sdk, 0, sizeof(espFwVer_t));

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL_UINT8(3, gbl.dev.version_sdk.major);
    TEST_ASSERT_EQUAL_UINT8(0, gbl.dev.version_sdk.minor);
    TEST_ASSERT_EQUAL_UINT8(1, gbl.dev.version_sdk.patch);
    // dev_present flag should remain 1 from the AT version parsing, as SDK version doesn't reset
    // it.
    TEST_ASSERT_EQUAL_UINT8(1, gbl.status.flg.dev_present);

    // Sub Case 3: Parsing "OK" response line
    const char *ok_line = "OK" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, ok_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0; // Ensure it's reset for this line

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);
    TEST_ASSERT_EQUAL_UINT8(1, isEndOfATresp); // Should be set to 1 for "OK"
    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);    // Message result should be espOK
}

TEST(ATcmdRespParser, CWLAP) {
#define NUM_AP_STORED 2
    espAP_t  aps_array[NUM_AP_STORED] = {0}; // Array to store parsed APs
    uint16_t num_ap_found = 0;

    espMsg_t test_msg = {
        .cmd = ESP_CMD_WIFI_CWLAP,
        .res = espOK, // Initial state, will be updated by parser
        .body =
            {.ap_list =
                 {.aps = aps_array,
                  .apslen = NUM_AP_STORED,
                  .num_ap_found = &num_ap_found,
                  .ssid = NULL, // Not filtering by SSID for this test
                  .ssid_len = 0}}
    };
    espGlbl_t gbl = {.msg = &test_msg};

    uint8_t  data_line_buf[128] = {0};
    uint16_t buf_idx = 0;
    uint8_t  isEndOfATresp = 0;

    // Sub Case 1: Parse a single +CWLAP response line
    // Format: (ecn,"ssid",rssi,"mac",channel,offset,cal,bgn,wps)
    const char *cwlap_line = "+CWLAP:(4,\"MyAP_SSID\",-70,\"11:22:33:44:55:66\","
                             "1,0,0,0,0,0,0)" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, cwlap_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0;

    // Call the function under test
    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res); // Should be espOK as it successfully parsed one AP
    // Should not be end of response yet (OK/ERROR not received on this line)
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp);
    TEST_ASSERT_EQUAL_UINT16(1, num_ap_found); // One AP should have been found

    // Verify the parsed AP details
    TEST_ASSERT_EQUAL(ESP_ECN_WPA_WPA2_PSK, aps_array[0].ecn);
    TEST_ASSERT_EQUAL_STRING("MyAP_SSID", aps_array[0].ssid);
    TEST_ASSERT_EQUAL_INT16(-70, aps_array[0].rssi);
    TEST_ASSERT_EQUAL_UINT8(0x11, aps_array[0].mac.mac[0]);
    TEST_ASSERT_EQUAL_UINT8(0x22, aps_array[0].mac.mac[1]);
    TEST_ASSERT_EQUAL_UINT8(0x33, aps_array[0].mac.mac[2]);
    TEST_ASSERT_EQUAL_UINT8(0x44, aps_array[0].mac.mac[3]);
    TEST_ASSERT_EQUAL_UINT8(0x55, aps_array[0].mac.mac[4]);
    TEST_ASSERT_EQUAL_UINT8(0x66, aps_array[0].mac.mac[5]);
    TEST_ASSERT_EQUAL_UINT8(1, aps_array[0].ch);
    TEST_ASSERT_EQUAL_INT8(0, aps_array[0].offset);
    TEST_ASSERT_EQUAL_UINT8(0, aps_array[0].cal);
    TEST_ASSERT_EQUAL_UINT8(0, aps_array[0].bgn);
    TEST_ASSERT_EQUAL_UINT8(0, aps_array[0].wps);

    // Sub Case 2: Send a second AP line when there is still one spot for the buffer
    // This AP should be saved, but the message result should be espOKIGNOREMORE.
    const char *cwlap_line_2 = "+CWLAP:(2,\"AnotherAP\",-84,\"aa:bb:cc:dd:ee:ff\""
                               ",5,0,1,0,0, 2,3)" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, cwlap_line_2);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0;

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    // The result should be espOKIGNOREMORE because the buffer is full
    TEST_ASSERT_EQUAL(espOKIGNOREMORE, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp); // Still not end of response
    // Should be 2, the second AP was recorded but subsequent raw text was ignored
    // due to full buffer
    TEST_ASSERT_EQUAL_UINT16(2, num_ap_found);

    // Verify that the first AP's data was not overwritten
    TEST_ASSERT_EQUAL(ESP_ECN_WPA_WPA2_PSK, aps_array[0].ecn);
    TEST_ASSERT_EQUAL_STRING("MyAP_SSID", aps_array[0].ssid);

    // Verify the parsed AP details
    TEST_ASSERT_EQUAL(ESP_ECN_WPA_PSK, aps_array[1].ecn);
    TEST_ASSERT_EQUAL_STRING("AnotherAP", aps_array[1].ssid);
    TEST_ASSERT_EQUAL_INT16(-84, aps_array[1].rssi);
    TEST_ASSERT_EQUAL_UINT8(0xaa, aps_array[1].mac.mac[0]);
    TEST_ASSERT_EQUAL_UINT8(0xbb, aps_array[1].mac.mac[1]);
    TEST_ASSERT_EQUAL_UINT8(0xcc, aps_array[1].mac.mac[2]);
    TEST_ASSERT_EQUAL_UINT8(0xdd, aps_array[1].mac.mac[3]);
    TEST_ASSERT_EQUAL_UINT8(0xee, aps_array[1].mac.mac[4]);
    TEST_ASSERT_EQUAL_UINT8(0xff, aps_array[1].mac.mac[5]);
    TEST_ASSERT_EQUAL_UINT8(5, aps_array[1].ch);
    TEST_ASSERT_EQUAL_INT8(0, aps_array[1].offset);
    TEST_ASSERT_EQUAL_UINT8(1, aps_array[1].cal);
    TEST_ASSERT_EQUAL_UINT8(2, aps_array[1].bgn);
    TEST_ASSERT_EQUAL_UINT8(3, aps_array[1].wps);

    // Sub Case 3: Send a third AP line when the buffer is already full
    // This AP should be ignored, and the message result should be espOKIGNOREMORE.
    const char *cwlap_line_3 = "+CWLAP:(3,\"NoDigFarm\",-59,\"1a:e0:27:7b:c1:08\""
                               ",6,1,0,0,0,1,0)" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, cwlap_line_3);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0;

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    // The result should be espOKIGNOREMORE because the buffer is full
    TEST_ASSERT_EQUAL(espOKIGNOREMORE, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp); // Still not end of response
    // previous records should not be modified
    TEST_ASSERT_EQUAL_STRING("MyAP_SSID", aps_array[0].ssid);
    TEST_ASSERT_EQUAL_STRING("AnotherAP", aps_array[1].ssid);
#undef NUM_AP_STORED
}

TEST(ATcmdRespParser, CWJAP_GET) {
    espStaInfoAP_t sta_info_ap = {0};

    espMsg_t test_msg = {
        .cmd = ESP_CMD_WIFI_CWJAP_GET,
        .res = espOK, // Initial state
        .body = {.sta_info_ap = {.info = &sta_info_ap}}
    };
    espGlbl_t gbl = {.msg = &test_msg};

    uint8_t  data_line_buf[128] = {0}, isEndOfATresp = 0;
    uint16_t buf_idx = 0;

    // Test Case: Parsing a +CWJAP_CUR response line
    // Format: +CWJAP_CUR:"ssid","mac",channel,rssi
    const char *cwjap_cur_line = "+CWJAP_CUR:\"MyHomeAP\",\"ac:11:22:33:44:55\""
                                 ",6,-65" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, cwjap_cur_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0;

    // Call the function under test
    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);    // Should remain espOK as parsing was successful
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp); // Not an "OK" or "ERROR" line

    // Verify the parsed station info
    TEST_ASSERT_EQUAL_STRING("MyHomeAP", sta_info_ap.ssid);
    TEST_ASSERT_EQUAL_UINT8(0xac, sta_info_ap.mac.mac[0]);
    TEST_ASSERT_EQUAL_UINT8(0x11, sta_info_ap.mac.mac[1]);
    TEST_ASSERT_EQUAL_UINT8(0x22, sta_info_ap.mac.mac[2]);
    TEST_ASSERT_EQUAL_UINT8(0x33, sta_info_ap.mac.mac[3]);
    TEST_ASSERT_EQUAL_UINT8(0x44, sta_info_ap.mac.mac[4]);
    TEST_ASSERT_EQUAL_UINT8(0x55, sta_info_ap.mac.mac[5]);
    TEST_ASSERT_EQUAL_UINT8(6, sta_info_ap.ch);
    TEST_ASSERT_EQUAL_INT16(-65, sta_info_ap.rssi);

    // Sub Case 2: Parsing "OK" response line after CWJAP_GET
    const char *ok_line = "OK" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, ok_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0; // Ensure it's reset for this line

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);
    TEST_ASSERT_EQUAL_UINT8(1, isEndOfATresp); // Should be set to 1 for "OK"
    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);    // Message result should be espOK
}

TEST(ATcmdRespParser, CWJAP_JOIN) {
    espMsg_t test_msg = {
        .cmd = ESP_CMD_WIFI_CWJAP, .res = espOK, .body = {.sta_join = {.error_num = 0}}
    };
    espGlbl_t gbl = {.msg = &test_msg};
    // Initialize station connection status to disconnected for a clean test start
    gbl.dev.sta.is_connected = ESP_STATION_DISCONNECTED;
    gbl.dev.sta.has_ip = 0;

    uint8_t  data_line_buf[128] = {0};
    uint16_t buf_idx = 0;
    uint8_t  isEndOfATresp = 0;

    // Sub Case 1: Successful connection - "WIFI CONNECTED"
    const char *wifi_connected_line = "WIFI CONNECTED" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, wifi_connected_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0;

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(1, isEndOfATresp);
    TEST_ASSERT_EQUAL(ESP_STATION_CONNECTED, gbl.dev.sta.is_connected);
    TEST_ASSERT_EQUAL_UINT8(0, gbl.dev.sta.has_ip); // has_ip should be reset to 0

    // Sub Case 2: Connection failure - "+CWJAP_CUR:1" (Timeout)
    // Reset state for the next test
    memset(data_line_buf, 0, sizeof(data_line_buf));
    buf_idx = 0;
    isEndOfATresp = 0;
    gbl.msg->res = espOK;
    gbl.dev.sta.is_connected = ESP_STATION_DISCONNECTED;
    gbl.dev.sta.has_ip = 0;
    gbl.msg->body.sta_join.error_num = 0; // Reset error num

    const char *cwjap_fail_timeout_line = "+CWJAP_CUR:1" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, cwjap_fail_timeout_line);
    buf_idx = strlen((char *)data_line_buf);

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espERRCONNTIMEOUT, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(1, isEndOfATresp);
    TEST_ASSERT_EQUAL(ESP_STATION_DISCONNECTED, gbl.dev.sta.is_connected);
    TEST_ASSERT_EQUAL_UINT8(1, gbl.msg->body.sta_join.error_num);

    // Sub Case 3: Connection failure - "+CWJAP_CUR:2" (Wrong Password)
    // Reset state for the next test
    memset(data_line_buf, 0, sizeof(data_line_buf));
    buf_idx = 0;
    isEndOfATresp = 0;
    gbl.msg->res = espOK;
    gbl.dev.sta.is_connected = ESP_STATION_DISCONNECTED;
    gbl.dev.sta.has_ip = 0;
    gbl.msg->body.sta_join.error_num = 0; // Reset error num

    const char *cwjap_fail_pass_line = "+CWJAP_CUR:2" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, cwjap_fail_pass_line);
    buf_idx = strlen((char *)data_line_buf);

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espERRPASS, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(1, isEndOfATresp);
    TEST_ASSERT_EQUAL(ESP_STATION_DISCONNECTED, gbl.dev.sta.is_connected);
    TEST_ASSERT_EQUAL_UINT8(2, gbl.msg->body.sta_join.error_num);
}

TEST(ATcmdRespParser, CIPSTATUS) {
    espMsg_t test_msg = {
        .cmd = ESP_CMD_TCPIP_CIPSTATUS,
        .res = espOK // Initial state
    };
    espGlbl_t gbl = {.msg = &test_msg};

    uint8_t  data_line_buf[128] = {0}, isEndOfATresp = 0;
    uint16_t buf_idx = 0;

    // Initialize some dummy active connections to test STATUS: line later
    gbl.dev.active_conns = (1 << 0) | (1 << 2); // Connections 0 and 2 are active

    // Sub Case 1: Parsing a +CIPSTATUS line for connection ID 0
    // Format: +CIPSTATUS:<link ID>,<type>,<remote IP>,<remote port>,<local port>,<client/server
    // status>
    const char *cipstatus_line_0 = "+CIPSTATUS:0,\"TCP\",\"192.168.1.100\""
                                   ",80,1024,1" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, cipstatus_line_0);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0;

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp); // Not an "OK" or "ERROR" line

    // Verify connection 0 details
    TEST_ASSERT_EQUAL_UINT8(192, gbl.dev.conns[0].remote_ip.ip[0]);
    TEST_ASSERT_EQUAL_UINT8(168, gbl.dev.conns[0].remote_ip.ip[1]);
    TEST_ASSERT_EQUAL_UINT8(1, gbl.dev.conns[0].remote_ip.ip[2]);
    TEST_ASSERT_EQUAL_UINT8(100, gbl.dev.conns[0].remote_ip.ip[3]);
    TEST_ASSERT_EQUAL_UINT16(80, gbl.dev.conns[0].remote_port);
    TEST_ASSERT_EQUAL_UINT16(1024, gbl.dev.conns[0].local_port);
    TEST_ASSERT_EQUAL_UINT8(1, gbl.dev.conns[0].status.flg.client);
    // Connection 0 should be set
    TEST_ASSERT_EQUAL_UINT8((1 << 0) | (1 << 2), gbl.dev.active_conns);

    // Sub Case 2: Parsing a +CIPSTATUS line for connection ID 1
    const char *cipstatus_line_1 = "+CIPSTATUS:1,\"UDP\",\"10.0.0.5\""
                                   ",1234,5678,0" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, cipstatus_line_1);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0;

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp);

    // Verify connection 1 details
    TEST_ASSERT_EQUAL_UINT8(10, gbl.dev.conns[1].remote_ip.ip[0]);
    TEST_ASSERT_EQUAL_UINT8(0, gbl.dev.conns[1].remote_ip.ip[1]);
    TEST_ASSERT_EQUAL_UINT8(0, gbl.dev.conns[1].remote_ip.ip[2]);
    TEST_ASSERT_EQUAL_UINT8(5, gbl.dev.conns[1].remote_ip.ip[3]);
    TEST_ASSERT_EQUAL_UINT16(1234, gbl.dev.conns[1].remote_port);
    TEST_ASSERT_EQUAL_UINT16(5678, gbl.dev.conns[1].local_port);
    TEST_ASSERT_EQUAL_UINT8(0, gbl.dev.conns[1].status.flg.client);
    TEST_ASSERT_EQUAL_UINT8(
        (1 << 0) | (1 << 1) | (1 << 2), gbl.dev.active_conns
    ); // Connections 0, 1, 2 should be set

    // Sub Case 3: Parsing a STATUS: line (e.g., STATUS:2 for connected)
    // This should update active_conns_last and reset active_conns
    const char *status_line = "STATUS:2" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, status_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0;

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp); // Not an "OK" or "ERROR" line

    // Verify active_conns_last holds the previous value, and active_conns is reset
    TEST_ASSERT_EQUAL_UINT8((1 << 0) | (1 << 1) | (1 << 2), gbl.dev.active_conns_last);
    TEST_ASSERT_EQUAL_UINT8(0, gbl.dev.active_conns);

    // Sub Case 4: Parsing "OK" response line
    const char *ok_line = "OK" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, ok_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0;

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);
    TEST_ASSERT_EQUAL_UINT8(1, isEndOfATresp); // Should be set to 1 for "OK"
    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);    // Message result should be espOK
}

TEST(ATcmdRespParser, PING) {
    uint32_t ping_resptime = 0;
    espMsg_t test_msg = {
        .cmd = ESP_CMD_TCPIP_PING,
        .res = espOK, // Initial state
        .body = {.tcpip_ping = {.resptime = &ping_resptime}}
    };
    espGlbl_t gbl = {.msg = &test_msg};

    uint8_t  data_line_buf[128] = {0}, isEndOfATresp = 0;
    uint16_t buf_idx = 0;

    // Sub Case 1: Parsing a +PING: response line with a valid response time
    const char *ping_response_line = "+PING:123" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, ping_response_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0;

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);       // Should remain espOK as parsing was successful
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp);    // Not an "OK" or "ERROR" line
    TEST_ASSERT_EQUAL_UINT32(123, ping_resptime); // Verify the parsed response time

    // Sub Case 2: Parsing "OK" response line after PING
    const char *ok_line = "OK" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, ok_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0; // Ensure it's reset for this line

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);
    TEST_ASSERT_EQUAL_UINT8(1, isEndOfATresp); // Should be set to 1 for "OK"
    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);    // Message result should be espOK

    // Sub Case 3: Parsing a +PING: response line with a different response time
    // Reset state for the next test
    ping_resptime = 0;
    memset(data_line_buf, 0, sizeof(data_line_buf));
    buf_idx = 0;
    isEndOfATresp = 0;
    gbl.msg->res = espOK;

    const char *ping_response_line_2 = "+PING:4567" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, ping_response_line_2);
    buf_idx = strlen((char *)data_line_buf);

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp);
    TEST_ASSERT_EQUAL_UINT32(4567, ping_resptime);
}

TEST(ATcmdRespParser, CIFSR) {
    espIp_t  mock_sta_ip = {0}, mock_ap_ip = {0};
    espMac_t mock_sta_mac = {0}, mock_ap_mac = {0};
    espMsg_t test_msg =
        {.cmd = ESP_CMD_TCPIP_CIFSR,
         .res = espOK,
         .body =
             {.local_ip_mac = {
                  .sta_ip = &mock_sta_ip,
                  .sta_mac = &mock_sta_mac,
                  .ap_ip = &mock_ap_ip,
                  .ap_mac = &mock_ap_mac,
              }}};
    espGlbl_t gbl = {.msg = &test_msg};

    uint8_t  data_line_buf[128] = {0}, isEndOfATresp = 0;
    uint16_t buf_idx = 0;

    // Sub Case 1: Parsing STAIP line
    const char *staip_line = "+CIFSR:STAIP,\"192.168.1.101\"" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, staip_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0;

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp);
    TEST_ASSERT_EQUAL_UINT8(192, mock_sta_ip.ip[0]);
    TEST_ASSERT_EQUAL_UINT8(168, mock_sta_ip.ip[1]);
    TEST_ASSERT_EQUAL_UINT8(1, mock_sta_ip.ip[2]);
    TEST_ASSERT_EQUAL_UINT8(101, mock_sta_ip.ip[3]);

    // Sub Case 2: Parsing STAMAC line
    memset(data_line_buf, 0, sizeof(data_line_buf));
    buf_idx = 0;
    isEndOfATresp = 0;
    const char *stamac_line = "+CIFSR:STAMAC,\"00:11:22:AA:BB:CC\"" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, stamac_line);
    buf_idx = strlen((char *)data_line_buf);

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp);
    TEST_ASSERT_EQUAL_UINT8(0x00, mock_sta_mac.mac[0]);
    TEST_ASSERT_EQUAL_UINT8(0x11, mock_sta_mac.mac[1]);
    TEST_ASSERT_EQUAL_UINT8(0x22, mock_sta_mac.mac[2]);
    TEST_ASSERT_EQUAL_UINT8(0xAA, mock_sta_mac.mac[3]);
    TEST_ASSERT_EQUAL_UINT8(0xBB, mock_sta_mac.mac[4]);
    TEST_ASSERT_EQUAL_UINT8(0xCC, mock_sta_mac.mac[5]);

    // Sub Case 3: Parsing APIP line
    memset(data_line_buf, 0, sizeof(data_line_buf));
    buf_idx = 0;
    isEndOfATresp = 0;
    const char *apip_line = "+CIFSR:APIP,\"192.168.14.9\"" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, apip_line);
    buf_idx = strlen((char *)data_line_buf);

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp);
    TEST_ASSERT_EQUAL_UINT8(192, mock_ap_ip.ip[0]);
    TEST_ASSERT_EQUAL_UINT8(168, mock_ap_ip.ip[1]);
    TEST_ASSERT_EQUAL_UINT8(14, mock_ap_ip.ip[2]);
    TEST_ASSERT_EQUAL_UINT8(9, mock_ap_ip.ip[3]);

    // Sub Case 4: Parsing APMAC line
    memset(data_line_buf, 0, sizeof(data_line_buf));
    buf_idx = 0;
    isEndOfATresp = 0;
    const char *apmac_line = "+CIFSR:APMAC,\"DE:AD:BE:EF:00:01\"" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, apmac_line);
    buf_idx = strlen((char *)data_line_buf);

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(0, isEndOfATresp);
    TEST_ASSERT_EQUAL_UINT8(0xDE, mock_ap_mac.mac[0]);
    TEST_ASSERT_EQUAL_UINT8(0xAD, mock_ap_mac.mac[1]);
    TEST_ASSERT_EQUAL_UINT8(0xBE, mock_ap_mac.mac[2]);
    TEST_ASSERT_EQUAL_UINT8(0xEF, mock_ap_mac.mac[3]);
    TEST_ASSERT_EQUAL_UINT8(0x00, mock_ap_mac.mac[4]);
    TEST_ASSERT_EQUAL_UINT8(0x01, mock_ap_mac.mac[5]);

    // Sub Case 5: Parsing "OK" response line
    memset(data_line_buf, 0, sizeof(data_line_buf));
    buf_idx = 0;
    isEndOfATresp = 0;
    const char *ok_line = "OK" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, ok_line);
    buf_idx = strlen((char *)data_line_buf);

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);
    TEST_ASSERT_EQUAL_UINT8(1, isEndOfATresp);
    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);
}

TEST(ATcmdRespParser, CIPSEND) {
    espMsg_t test_msg = {
        .cmd = ESP_CMD_TCPIP_CIPSEND,
        .res = espINPROG // Initial state, expecting SEND OK/FAIL
    };
    espGlbl_t gbl = {.msg = &test_msg};

    uint8_t  data_line_buf[128] = {0}, isEndOfATresp = 0;
    uint16_t buf_idx = 0;

    // Sub Case 1: Successful send - "SEND OK"
    const char *send_ok_line = "SEND OK" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, send_ok_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0;

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espOK, gbl.msg->res);    // Should be espOK
    TEST_ASSERT_EQUAL_UINT8(1, isEndOfATresp); // Should be end of response

    // Sub Case 2: Failed send - "SEND FAIL"
    // Reset state for the next test
    memset(data_line_buf, 0, sizeof(data_line_buf));
    buf_idx = 0;
    isEndOfATresp = 0;
    gbl.msg->res = espINPROG; // Reset result

    const char *send_fail_line = "SEND FAIL" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, send_fail_line);
    buf_idx = strlen((char *)data_line_buf);

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    TEST_ASSERT_EQUAL(espERR, gbl.msg->res);   // Should be espERR
    TEST_ASSERT_EQUAL_UINT8(1, isEndOfATresp); // Should be end of response
}

TEST(ATcmdRespParser, GenericErrorResponse) {
    // This test verifies that a generic "ERROR" response correctly sets the message result to
    // espERR and marks the end of the AT response.
    espMsg_t test_msg = {
        .cmd = ESP_CMD_GMR, // Use an arbitrary command type
        .res = espOK        // Initial state, simulating an ongoing command
    };
    espGlbl_t gbl = {.msg = &test_msg};

    uint8_t  data_line_buf[128] = {0};
    uint16_t buf_idx = 0;
    uint8_t  isEndOfATresp = 0;

    // Simulate receiving an "ERROR" line
    const char *error_line = "ERROR" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, error_line);
    buf_idx = strlen((char *)data_line_buf);
    isEndOfATresp = 0; // Ensure it's reset

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    // Assert that the message result is espERR
    TEST_ASSERT_EQUAL(espERR, gbl.msg->res);
    // Assert that isEndOfATresp is set to 1
    TEST_ASSERT_EQUAL_UINT8(1, isEndOfATresp);

    // Test with a different initial error state (e.g., espBUSY) to ensure it's not overwritten
    // by a generic "ERROR" if a more specific error was already set.
    memset(data_line_buf, 0, sizeof(data_line_buf));
    buf_idx = 0;
    isEndOfATresp = 0;
    gbl.msg->res = espBUSY; // Simulate a specific error already set

    strcpy((char *)data_line_buf, error_line);
    buf_idx = strlen((char *)data_line_buf);

    vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

    // The result should remain espBUSY, as it's a more specific error than generic espERR
    TEST_ASSERT_EQUAL(espBUSY, gbl.msg->res);
    TEST_ASSERT_EQUAL_UINT8(1, isEndOfATresp);
}

TEST(ATcmdRespParser, UnknownCommandType) {
    typedef struct {
        const char *line;
        uint8_t     expected_isEndOfATresp;
        espRes_t    expected_res;
        const char *description;
    } UnknownCmdTest_t;

    const UnknownCmdTest_t tests[] = {
        {.line = "OK" ESP_CHR_CR ESP_CHR_LF,
         .expected_isEndOfATresp = 1,
         .expected_res = espSKIP,
         .description = "OK response for unknown command"},
        {.line = "ERROR" ESP_CHR_CR ESP_CHR_LF,
         .expected_isEndOfATresp = 1,
         .expected_res = espSKIP,
         .description = "ERROR response for unknown command"},
        {.line = "+UNKNOWN_CMD_DATA:1,2,3" ESP_CHR_CR ESP_CHR_LF,
         .expected_isEndOfATresp = 1,
         .expected_res = espSKIP,
         .description = "Unknown data line for unknown command"}
    };

    uint8_t  data_line_buf[128] = {0};
    uint16_t buf_idx = 0;
    uint8_t  isEndOfATresp = 0;

    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
        // Initialize espMsg_t and espGlbl_t for each iteration
        espMsg_t test_msg = {
            .cmd = (espCmd_t)999, // An arbitrary value not corresponding to a known command
            .res = espOK          // Initial state
        };
        espGlbl_t gbl = {.msg = &test_msg};

        // Reset buffer and flags for each test case
        memset(data_line_buf, 0, sizeof(data_line_buf));
        isEndOfATresp = 0;

        // Copy the current test line
        strcpy((char *)data_line_buf, tests[i].line);
        buf_idx = strlen((char *)data_line_buf);

        // Call the function under test
        vESPparseRecvATrespLine(&gbl, data_line_buf, buf_idx, &isEndOfATresp);

        // Assertions
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(
            tests[i].expected_isEndOfATresp, isEndOfATresp, tests[i].description
        );
        TEST_ASSERT_EQUAL_MESSAGE(tests[i].expected_res, gbl.msg->res, tests[i].description);
    }
}

TEST(ATnetConnStatusParser, ConnEstablished) {
    espMsg_t  test_msg = {.cmd = ESP_CMD_TCPIP_CIPSTART, .res = espOK};
    espGlbl_t gbl = {.msg = &test_msg, .dev = {0}};

    uint8_t  data_line_buf[128] = {0};
    espRes_t parse_res;

    // Sub Case 1: Parsing "3,CONNECT" for connection ID 3
    const char *connect_line_3 = "3,CONNECT" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, connect_line_3);
    parse_res = eESPparseNetConnStatus(&gbl, data_line_buf);

    TEST_ASSERT_EQUAL(espOK, parse_res);
    TEST_ASSERT_EQUAL(ESP_CONN_ESTABLISHED, gbl.dev.conns[3].status.flg.active);
    TEST_ASSERT_EQUAL(ESP_CONN_TYPE_UNKNOWN, gbl.dev.conns[3].type);
    TEST_ASSERT_EQUAL_UINT16(0, gbl.dev.conns[3].remote_port);
    TEST_ASSERT_EQUAL_UINT16(0, gbl.dev.conns[3].local_port);
    TEST_ASSERT_EQUAL_UINT16(1, gbl.dev.conns[3].status.flg.client);
    // Only connection 3 should be active in the bitmask
    TEST_ASSERT_EQUAL_UINT8((1 << 3), gbl.dev.active_conns);

    // Sub Case 2: Parsing "0,CONNECT" for connection ID 0
    // Reset buffer and flags for the next test
    gbl.msg = NULL;
    memset(data_line_buf, 0, sizeof(data_line_buf));
    const char *connect_line_0 = "0,CONNECT" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, connect_line_0);
    parse_res = eESPparseNetConnStatus(&gbl, data_line_buf);

    TEST_ASSERT_EQUAL(espOK, parse_res);
    TEST_ASSERT_EQUAL(ESP_CONN_ESTABLISHED, gbl.dev.conns[0].status.flg.active);
    TEST_ASSERT_EQUAL(ESP_CONN_TYPE_UNKNOWN, gbl.dev.conns[0].type);
    TEST_ASSERT_EQUAL_UINT16(0, gbl.dev.conns[0].remote_port);
    TEST_ASSERT_EQUAL_UINT16(0, gbl.dev.conns[0].local_port);
    TEST_ASSERT_EQUAL_UINT16(0, gbl.dev.conns[0].status.flg.client);
    // Connections 0 and 3 should now be active
    TEST_ASSERT_EQUAL_UINT8((1 << 3) | (1 << 0), gbl.dev.active_conns);
}

TEST(ATnetConnStatusParser, ConnEstablishedExtMsg) {
    espMsg_t  test_msg = {.cmd = ESP_CMD_IDLE, .res = espOK};
    espGlbl_t gbl = {.msg = &test_msg, .dev = {0}};

    uint8_t  data_line_buf[128] = {0};
    espRes_t parse_res;

    // Sub Case 1: Successful TCP client connection (link_id 0)
    // +LINK_CONN:0,0,"TCP",0,"192.168.1.100",80,1024
    // establish_fail=0, link_id=0, type="TCP", conn4server=0 (client), remote_ip, remote_port,
    // local_port
    const char *conn_ext_line_0 = "+LINK_CONN:0,0,\"TCP\",0,\"192.168.1.100\""
                                  ",80,1024" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, conn_ext_line_0);
    parse_res = eESPparseNetConnStatus(&gbl, data_line_buf);

    TEST_ASSERT_EQUAL(espOK, parse_res);
    TEST_ASSERT_EQUAL(ESP_CONN_ESTABLISHED, gbl.dev.conns[0].status.flg.active);
    TEST_ASSERT_EQUAL(ESP_CONN_TYPE_TCP, gbl.dev.conns[0].type);
    TEST_ASSERT_EQUAL_UINT8(192, gbl.dev.conns[0].remote_ip.ip[0]);
    TEST_ASSERT_EQUAL_UINT8(168, gbl.dev.conns[0].remote_ip.ip[1]);
    TEST_ASSERT_EQUAL_UINT8(1, gbl.dev.conns[0].remote_ip.ip[2]);
    TEST_ASSERT_EQUAL_UINT8(100, gbl.dev.conns[0].remote_ip.ip[3]);
    TEST_ASSERT_EQUAL_UINT16(80, gbl.dev.conns[0].remote_port);
    TEST_ASSERT_EQUAL_UINT16(1024, gbl.dev.conns[0].local_port);
    // conn4server=0 means client
    TEST_ASSERT_EQUAL_UINT8(1, gbl.dev.conns[0].status.flg.client);
    TEST_ASSERT_EQUAL_UINT8((1 << 0), gbl.dev.active_conns);

    // Sub Case 2: Successful UDP server connection (link_id 1)
    // +LINK_CONN:0,1,"UDP",1,"10.59.0.5",1234,5678
    // establish_fail=0, link_id=1, type="UDP", conn4server=1 (server), remote_ip, remote_port,
    // local_port
    memset(data_line_buf, 0, sizeof(data_line_buf)); // Clear buffer for next test
    const char *conn_ext_line_1 = "+LINK_CONN:0,1,\"UDP\",1,\"10.59.0.5\""
                                  ",1234,5678" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, conn_ext_line_1);
    parse_res = eESPparseNetConnStatus(&gbl, data_line_buf);

    TEST_ASSERT_EQUAL(espOK, parse_res);
    TEST_ASSERT_EQUAL(ESP_CONN_ESTABLISHED, gbl.dev.conns[1].status.flg.active);
    TEST_ASSERT_EQUAL(ESP_CONN_TYPE_UDP, gbl.dev.conns[1].type);
    TEST_ASSERT_EQUAL_UINT8(10, gbl.dev.conns[1].remote_ip.ip[0]);
    TEST_ASSERT_EQUAL_UINT8(59, gbl.dev.conns[1].remote_ip.ip[1]);
    TEST_ASSERT_EQUAL_UINT8(0, gbl.dev.conns[1].remote_ip.ip[2]);
    TEST_ASSERT_EQUAL_UINT8(5, gbl.dev.conns[1].remote_ip.ip[3]);
    TEST_ASSERT_EQUAL_UINT16(1234, gbl.dev.conns[1].remote_port);
    TEST_ASSERT_EQUAL_UINT16(5678, gbl.dev.conns[1].local_port);
    // conn4server=1 means server
    TEST_ASSERT_EQUAL_UINT8(0, gbl.dev.conns[1].status.flg.client);
    TEST_ASSERT_EQUAL_UINT8((1 << 0) | (1 << 1), gbl.dev.active_conns);

    // Sub Case 3: Connection establishment failure (link_id 2)
    // +LINK_CONN:1,2,"TCP",0,"0.0.0.0",0,0
    // establish_fail=1, link_id=2, type="TCP", conn4server=0 (client), remote_ip, remote_port,
    // local_port
    memset(data_line_buf, 0, sizeof(data_line_buf));
    const char *conn_ext_fail_line = "+LINK_CONN:1,2,\"TCP\",0,\"0.0.0.0\""
                                     ",0,0" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, conn_ext_fail_line);
    parse_res = eESPparseNetConnStatus(&gbl, data_line_buf);

    TEST_ASSERT_EQUAL(espERRCONNFAIL, parse_res);
    TEST_ASSERT_EQUAL(ESP_CONN_CLOSED, gbl.dev.conns[2].status.flg.active);
    // Active connections should remain unchanged
    TEST_ASSERT_EQUAL_UINT8((1 << 0) | (1 << 1), gbl.dev.active_conns);
}

TEST(ATnetConnStatusParser, ConnClosed) {
    espGlbl_t  gbl = {0};
    uint8_t    data_line_buf[128] = {0}, link_id_to_close = 4;
    espRes_t   parse_res;
    espConn_t *mock_conn = &gbl.dev.conns[link_id_to_close]; // Refactored line

    // Pre-condition: Set up a connection as active and with some dummy data
    // This simulates an existing connection that is about to be closed.
    mock_conn->status.flg.active = ESP_CONN_ESTABLISHED;
    mock_conn->status.flg.client = 1;
    mock_conn->type = ESP_CONN_TYPE_TCP;
    mock_conn->remote_port = 1234;
    mock_conn->local_port = 5678;
    mock_conn->remote_ip.ip[0] = 192;
    mock_conn->remote_ip.ip[1] = 168;
    mock_conn->remote_ip.ip[2] = 1;
    mock_conn->remote_ip.ip[3] = 10;
    gbl.dev.active_conns |= (1 << link_id_to_close); // Mark as active in bitmask

    // Verify pre-conditions
    TEST_ASSERT_EQUAL(ESP_CONN_ESTABLISHED, mock_conn->status.flg.active);
    TEST_ASSERT_EQUAL_UINT8((1 << link_id_to_close), gbl.dev.active_conns);
    TEST_ASSERT_EQUAL(ESP_CONN_TYPE_TCP, mock_conn->type);
    TEST_ASSERT_EQUAL_UINT16(1234, mock_conn->remote_port);

    // Test Case: Parsing "4,CLOSED" for connection ID 5
    const char *closed_line = "4,CLOSED" ESP_CHR_CR ESP_CHR_LF;
    strcpy((char *)data_line_buf, closed_line);
    parse_res = eESPparseNetConnStatus(&gbl, data_line_buf);

    TEST_ASSERT_EQUAL(espOK, parse_res); // Function should return espOK on successful parsing
    TEST_ASSERT_EQUAL(ESP_CONN_CLOSED, mock_conn->status.flg.active);
    TEST_ASSERT_EQUAL_UINT8(0, mock_conn->status.flg.client);
    // Connection 5 should be removed from active bitmask
    TEST_ASSERT_EQUAL_UINT8(0, gbl.dev.active_conns);
    // Verify that the connection structure has been zeroed out
    // (or at least key fields are reset to their default/initial state)
    TEST_ASSERT_EQUAL(ESP_CONN_TYPE_UNKNOWN, mock_conn->type);
    TEST_ASSERT_EQUAL_UINT16(0, mock_conn->remote_port);
    TEST_ASSERT_EQUAL_UINT16(0, mock_conn->local_port);
    TEST_ASSERT_EQUAL_UINT8(0, mock_conn->remote_ip.ip[0]);
    TEST_ASSERT_EQUAL_UINT8(0, mock_conn->remote_ip.ip[1]);
    TEST_ASSERT_EQUAL_UINT8(0, mock_conn->remote_ip.ip[2]);
    TEST_ASSERT_EQUAL_UINT8(0, mock_conn->remote_ip.ip[3]);
}

TEST(ATnetDataParser, IPDsetupOk) {
    espGlbl_t  gbl = {.dev = {0}};
    espIPD_t  *ipd_chosen = NULL;
    espRes_t   parse_res;
    uint8_t    link_id = 1;
    espConn_t *chosen_conn = &gbl.dev.conns[link_id];

    gbl.status.flg.mux_conn = ESP_TCP_MULTIPLE_CONNECTION;

#define TOTAL_PAYLOAD_LEN 300
#define SEGMENT_LEN       (TOTAL_PAYLOAD_LEN / 3)
    uint8_t full_payload[TOTAL_PAYLOAD_LEN];
    for (size_t i = 0; i < TOTAL_PAYLOAD_LEN; ++i) {
        full_payload[i] = (uint8_t)('A' + (i % 26));
    }

    char ipd_metadata_line[128];
    sprintf(ipd_metadata_line, "+IPD,%u,%u,192.168.1.100,8080:", link_id, TOTAL_PAYLOAD_LEN);

    uint8_t *metadata_ptr = (uint8_t *)ipd_metadata_line;
    parse_res = eESPparseIPDsetup(&gbl, metadata_ptr, &ipd_chosen);

    TEST_ASSERT_EQUAL(espOK, parse_res);
    TEST_ASSERT_NOT_NULL(ipd_chosen);
    TEST_ASSERT_EQUAL_UINT32(TOTAL_PAYLOAD_LEN, ipd_chosen->tot_len);
    TEST_ASSERT_EQUAL_UINT32(TOTAL_PAYLOAD_LEN, ipd_chosen->rem_len);
    TEST_ASSERT_EQUAL_UINT8(1, ipd_chosen->read);
    TEST_ASSERT_EQUAL_PTR(&chosen_conn->ipd, ipd_chosen);
    TEST_ASSERT_EQUAL_PTR(chosen_conn, ipd_chosen->conn);
    TEST_ASSERT_EQUAL_UINT8(1, chosen_conn->status.flg.data_received);

    TEST_ASSERT_EQUAL_UINT8(192, chosen_conn->remote_ip.ip[0]);
    TEST_ASSERT_EQUAL_UINT8(168, chosen_conn->remote_ip.ip[1]);
    TEST_ASSERT_EQUAL_UINT8(1, chosen_conn->remote_ip.ip[2]);
    TEST_ASSERT_EQUAL_UINT8(100, chosen_conn->remote_ip.ip[3]);
    TEST_ASSERT_EQUAL_UINT16(8080, chosen_conn->remote_port);

    uint8_t *segment1_data = full_payload;
    uint32_t segment1_len_arg = SEGMENT_LEN;
    parse_res = eESPparseIPDcopyData(ipd_chosen, segment1_data, &segment1_len_arg);

    TEST_ASSERT_EQUAL(espINPROG, parse_res);
    TEST_ASSERT_EQUAL_UINT32(TOTAL_PAYLOAD_LEN - SEGMENT_LEN, ipd_chosen->rem_len);
    TEST_ASSERT_NOT_NULL(ipd_chosen->pbuf_head);
    TEST_ASSERT_EQUAL_UINT8(1, ipd_chosen->pbuf_head->chain_len);
    TEST_ASSERT_EQUAL_UINT32(SEGMENT_LEN, ipd_chosen->pbuf_head->payload_len);
    TEST_ASSERT_EQUAL_UINT32(SEGMENT_LEN, segment1_len_arg);

    uint8_t *segment2_data = full_payload + SEGMENT_LEN;
    uint32_t segment2_len_arg = SEGMENT_LEN;
    parse_res = eESPparseIPDcopyData(ipd_chosen, segment2_data, &segment2_len_arg);

    TEST_ASSERT_EQUAL(espINPROG, parse_res);
    TEST_ASSERT_EQUAL_UINT32(TOTAL_PAYLOAD_LEN - (2 * SEGMENT_LEN), ipd_chosen->rem_len);
    TEST_ASSERT_NOT_NULL(ipd_chosen->pbuf_head->next);
    TEST_ASSERT_EQUAL_UINT8(2, ipd_chosen->pbuf_head->chain_len);
    TEST_ASSERT_EQUAL_UINT32(SEGMENT_LEN, ipd_chosen->pbuf_head->next->payload_len);
    TEST_ASSERT_EQUAL_UINT32(SEGMENT_LEN, segment2_len_arg);

    uint8_t *segment3_data = full_payload + (2 * SEGMENT_LEN);
    uint32_t segment3_len_arg = SEGMENT_LEN;
    parse_res = eESPparseIPDcopyData(ipd_chosen, segment3_data, &segment3_len_arg);

    TEST_ASSERT_EQUAL(espOK, parse_res);
    TEST_ASSERT_EQUAL_UINT32(0, ipd_chosen->rem_len);
    TEST_ASSERT_NOT_NULL(ipd_chosen->pbuf_head->next->next);
    TEST_ASSERT_EQUAL_UINT8(3, ipd_chosen->pbuf_head->chain_len);
    TEST_ASSERT_EQUAL_UINT32(SEGMENT_LEN, ipd_chosen->pbuf_head->next->next->payload_len);
    TEST_ASSERT_EQUAL_UINT32(SEGMENT_LEN, segment3_len_arg);
    TEST_ASSERT_EQUAL_PTR(ipd_chosen->pbuf_head, ipd_chosen->conn->pbuf);

    uint8_t received_payload[TOTAL_PAYLOAD_LEN];
    memset(received_payload, 0, TOTAL_PAYLOAD_LEN);
    espPbuf_t *current_pbuf = ipd_chosen->pbuf_head;
    size_t     current_offset = 0;

    while (current_pbuf != NULL) {
        TEST_ASSERT_LESS_OR_EQUAL(TOTAL_PAYLOAD_LEN - current_offset, current_pbuf->payload_len);
        memcpy(received_payload + current_offset, current_pbuf->payload, current_pbuf->payload_len);
        current_offset += current_pbuf->payload_len;
        current_pbuf = current_pbuf->next;
    }
    TEST_ASSERT_EQUAL_UINT32(TOTAL_PAYLOAD_LEN, current_offset);

    TEST_ASSERT_EQUAL_HEX8_ARRAY(full_payload, received_payload, TOTAL_PAYLOAD_LEN);

    vESPpktBufChainDelete(ipd_chosen->conn->pbuf);
    ipd_chosen->conn->pbuf = NULL;
    TEST_ASSERT_NULL(chosen_conn->pbuf);

    parse_res = eESPparseIPDreset(ipd_chosen);
    TEST_ASSERT_EQUAL(espOK, parse_res);
    TEST_ASSERT_EQUAL_UINT8(0, ipd_chosen->read);
    TEST_ASSERT_EQUAL_UINT8(0, chosen_conn->status.flg.data_received);
    TEST_ASSERT_EQUAL_UINT32(0, ipd_chosen->tot_len);
    TEST_ASSERT_EQUAL_UINT32(0, ipd_chosen->rem_len);
    TEST_ASSERT_NULL(ipd_chosen->conn);
    TEST_ASSERT_NULL(ipd_chosen->pbuf_head);
#undef TOTAL_PAYLOAD_LEN
#undef SEGMENT_LEN
}

TEST_GROUP_RUNNER(EspRespParser) {
    RUN_TEST_CASE(ATcmdRespParser, GMRversion);
    RUN_TEST_CASE(ATcmdRespParser, CWLAP);
    RUN_TEST_CASE(ATcmdRespParser, CWJAP_GET);
    RUN_TEST_CASE(ATcmdRespParser, CWJAP_JOIN);
    RUN_TEST_CASE(ATcmdRespParser, CIPSTATUS);
    RUN_TEST_CASE(ATcmdRespParser, PING);
    RUN_TEST_CASE(ATcmdRespParser, CIFSR);
    RUN_TEST_CASE(ATcmdRespParser, CIPSEND);
    RUN_TEST_CASE(ATcmdRespParser, GenericErrorResponse);
    RUN_TEST_CASE(ATcmdRespParser, UnknownCommandType);
    RUN_TEST_CASE(ATnetConnStatusParser, ConnEstablished);
    RUN_TEST_CASE(ATnetConnStatusParser, ConnEstablishedExtMsg);
    RUN_TEST_CASE(ATnetConnStatusParser, ConnClosed);
    RUN_TEST_CASE(ATnetDataParser, IPDsetupOk);
}
