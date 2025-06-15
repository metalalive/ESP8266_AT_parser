#include "unity.h"
#include "unity_fixture.h"

#include "esp/esp.h"
#include "esp/esp_private.h"
#include "system/esp_system_mock.h"
#include <string.h>

extern ut_sys_mock_returns_t ut_sys_mock_returns;

static espRes_t mock_msg_fn_impl_ret;
static uint8_t  mock_msg_fn_num_calls = 0;

static espRes_t mock_msg_fn_impl(espMsg_t *msg, espGlbl_t *gbl) {
    (void)msg;
    (void)gbl;
    mock_msg_fn_num_calls += 1;
    return mock_msg_fn_impl_ret;
}

static uint8_t mock_ll_recv_resp_proc_ret;
static uint8_t mock_ll_recv_resp_proc_num_calls = 0;

static uint8_t mock_ll_recv_resp_proc_impl(espGlbl_t *gbl, espBuf_t *buff) {
    (void)gbl;
    (void)buff;
    mock_ll_recv_resp_proc_num_calls += 1;
    return mock_ll_recv_resp_proc_ret;
}

TEST_GROUP(ATcmdReqThread);
TEST_GROUP(ATcmdRespThread);

TEST_SETUP(ATcmdReqThread) {}
TEST_SETUP(ATcmdRespThread) {}

TEST_TEAR_DOWN(ATcmdReqThread) { mock_msg_fn_num_calls = 0; }
TEST_TEAR_DOWN(ATcmdRespThread) { mock_ll_recv_resp_proc_num_calls = 0; }

TEST(ATcmdReqThread, blocking_success) {
    espGlbl_t mock_gbl = {
        .mbox_cmd_req = (espSysMbox_t)1,
        .sem_th_sync = (espSysSem_t)2,
    };
    espMsg_t mock_msg = {
        .block_time = 100,
        .is_blocking = ESP_AT_CMD_BLOCKING,
        .fn = mock_msg_fn_impl,
        .sem = (espSysSem_t)1,
    };
    ut_sys_mock_returns.eESPsysMboxGet_msg = (void *)&mock_msg;
    ut_sys_mock_returns.eESPsysMboxGet_ret = espOK;
    ut_sys_mock_returns.eESPsysSemWait_ret = espOK;
    ut_sys_mock_returns.eESPsysSemRelease_ret = espOK;
    mock_msg_fn_impl_ret = espOK;

    espRes_t result = eESPthreadATreqIteration(&mock_gbl);

    TEST_ASSERT_EQUAL(espOK, result);
    TEST_ASSERT_EQUAL(1, mock_msg_fn_num_calls);
    TEST_ASSERT_NULL(mock_gbl.msg);
}

TEST(ATcmdReqThread, recv_msg_error) {
    espGlbl_t mock_gbl = {
        .mbox_cmd_req = (espSysMbox_t)1,
        .sem_th_sync = (espSysSem_t)2,
    };
    ut_sys_mock_returns.eESPsysMboxGet_msg = NULL;
    ut_sys_mock_returns.eESPsysMboxGet_ret = espBUSY;

    espRes_t result = eESPthreadATreqIteration(&mock_gbl);

    TEST_ASSERT_EQUAL(espBUSY, result);
    TEST_ASSERT_EQUAL(0, mock_msg_fn_num_calls);
    TEST_ASSERT_NULL(mock_gbl.msg);
}

TEST(ATcmdReqThread, resp_hdlr_sync_timeout) {
    espGlbl_t mock_gbl = {
        .mbox_cmd_req = (espSysMbox_t)1,
        .sem_th_sync = (espSysSem_t)2,
    };
    espMsg_t mock_msg = {
        .block_time = 100,
        .is_blocking = ESP_AT_CMD_BLOCKING,
        .fn = mock_msg_fn_impl,
        .sem = (espSysSem_t)1,
    };
    ut_sys_mock_returns.eESPsysMboxGet_msg = (void *)&mock_msg;
    ut_sys_mock_returns.eESPsysMboxGet_ret = espOK;
    ut_sys_mock_returns.eESPsysSemWait_ret = espTIMEOUT;
    ut_sys_mock_returns.eESPsysSemRelease_ret = espOK;
    mock_msg_fn_impl_ret = espOK;

    espRes_t result = eESPthreadATreqIteration(&mock_gbl);

    TEST_ASSERT_EQUAL(espOK, result);
    TEST_ASSERT_EQUAL(1, mock_msg_fn_num_calls);
    TEST_ASSERT_NULL(mock_gbl.msg);
    TEST_ASSERT_EQUAL(espTIMEOUT, mock_msg.res);
}

TEST(ATcmdRespThread, success) {
    espGlbl_t mock_gbl = {
        .mbox_cmd_resp = (espSysMbox_t)3,
        .sem_th_sync = (espSysSem_t)2,
        .ops =
            {
                .recv_resp_proc = mock_ll_recv_resp_proc_impl,
            },
    };
    espBuf_t *mock_recv_buf_ptr = (espBuf_t *)ESP_MALLOC(sizeof(espBuf_t) + 10);
    TEST_ASSERT_NOT_NULL(mock_recv_buf_ptr);
    mock_recv_buf_ptr->buff = (uint8_t *)(mock_recv_buf_ptr + 1);
    mock_recv_buf_ptr->size = 10;
    memcpy(mock_recv_buf_ptr->buff, "DUMMY_DATA", 10);

    ut_sys_mock_returns.eESPsysMboxGet_msg = (void *)mock_recv_buf_ptr;
    ut_sys_mock_returns.eESPsysMboxGet_ret = espOK;
    mock_ll_recv_resp_proc_ret = 1;
    ut_sys_mock_returns.eESPsysSemRelease_ret = espOK;
    ut_sys_mock_returns.eESPsysThreadYield_ret = espOK;

    espRes_t result = eESPthreadATrespIteration(&mock_gbl);

    TEST_ASSERT_EQUAL(espOK, result);
    TEST_ASSERT_EQUAL(1, mock_ll_recv_resp_proc_num_calls);
}

TEST(ATcmdRespThread, recv_msg_error) {
    espGlbl_t mock_gbl = {
        .mbox_cmd_resp = (espSysMbox_t)3,
        .sem_th_sync = (espSysSem_t)2,
        .ops =
            {
                .recv_resp_proc = mock_ll_recv_resp_proc_impl,
            },
    };

    ut_sys_mock_returns.eESPsysMboxGet_msg = NULL;
    ut_sys_mock_returns.eESPsysMboxGet_ret = espBUSY;

    espRes_t result = eESPthreadATrespIteration(&mock_gbl);

    TEST_ASSERT_EQUAL(espBUSY, result);
    TEST_ASSERT_EQUAL(0, mock_ll_recv_resp_proc_num_calls);
}

TEST_GROUP_RUNNER(EspThread) {
    RUN_TEST_CASE(ATcmdReqThread, blocking_success);
    RUN_TEST_CASE(ATcmdReqThread, recv_msg_error);
    RUN_TEST_CASE(ATcmdReqThread, resp_hdlr_sync_timeout);
    RUN_TEST_CASE(ATcmdRespThread, success);
    RUN_TEST_CASE(ATcmdRespThread, recv_msg_error);
}
