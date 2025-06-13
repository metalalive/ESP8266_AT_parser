#include "esp/esp.h"
#include "esp/esp_private.h"

static espRes_t eESPparseVersion(uint8_t **curr_chr_pp, espFwVer_t *ver) {
    ver->major = iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    ver->minor = iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    ver->patch = iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    // TODO: minimum version check
    return espOK;
}

static void vESPparseIPfromStr(uint8_t **curr_chr_pp, espIp_t *ip) {
    ip->ip[0] = iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    ip->ip[1] = iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    ip->ip[2] = iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    ip->ip[3] = iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
}

static void vESPparseMACfromStr(uint8_t **curr_chr_pp, espMac_t *mac) {
    mac->mac[0] = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_HEX);
    mac->mac[1] = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_HEX);
    mac->mac[2] = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_HEX);
    mac->mac[3] = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_HEX);
    mac->mac[4] = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_HEX);
    mac->mac[5] = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_HEX);
}

// TODO: verify this function after ESP device connected to AP & get IP address
static espRes_t eESPparseCIPstatus(uint8_t **curr_chr_pp, espDev_t *dev) {
    espRes_t response = espOK;
    uint8_t  conn_id;
    if (strncmp((const char *)*curr_chr_pp, "+CIPSTATUS", 10) == 0) {
        *curr_chr_pp += 10;
        conn_id = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
        dev->active_conns |= (1 << conn_id);
        *curr_chr_pp += 3; // skip the type ",TCP" or ".UDP"
        vESPparseIPfromStr(curr_chr_pp, &(dev->conns[conn_id].remote_ip));
        dev->conns[conn_id].remote_port =
            (espPort_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
        dev->conns[conn_id].local_port =
            (espPort_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
        dev->conns[conn_id].status.flg.client =
            (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    } else if (strncmp((const char *)*curr_chr_pp, "STATUS:", 7) == 0) {
        // backup previous active connections
        dev->active_conns_last = dev->active_conns;
        dev->active_conns = 0;
    }
    return response;
} // end of eESPparseCIPstatus

static void vESPparseAPstaIP(uint8_t **curr_chr_pp, espMsg_t *msg) {
    if (strncmp((const char *)*curr_chr_pp, "ip:", 3) == 0) {
        *curr_chr_pp += 3;
        vESPparseIPfromStr(curr_chr_pp, msg->body.sta_ap_getip.ip);
    } else if (strncmp((const char *)*curr_chr_pp, "gateway:", 8) == 0) {
        *curr_chr_pp += 8;
        vESPparseIPfromStr(curr_chr_pp, msg->body.sta_ap_getip.gw);
    } else if (strncmp((const char *)*curr_chr_pp, "netmask:", 8) == 0) {
        *curr_chr_pp += 8;
        vESPparseIPfromStr(curr_chr_pp, msg->body.sta_ap_getip.nm);
    }
} // end of vESPparseAPstaIP

static void vESPparseCIFSR(uint8_t **curr_chr_pp, espMsg_t *msg) {
    if (strncmp((const char *)*curr_chr_pp, "STAIP", 5) == 0) {
        *curr_chr_pp += 5;
        vESPparseIPfromStr(curr_chr_pp, msg->body.local_ip_mac.sta_ip);
    } else if (strncmp((const char *)*curr_chr_pp, "STAMAC", 6) == 0) {
        *curr_chr_pp += 6;
        vESPparseMACfromStr(curr_chr_pp, msg->body.local_ip_mac.sta_mac);
    } else if (strncmp((const char *)*curr_chr_pp, "APIP", 4) == 0) {
        *curr_chr_pp += 4;
        vESPparseIPfromStr(curr_chr_pp, msg->body.local_ip_mac.ap_ip);
    } else if (strncmp((const char *)*curr_chr_pp, "APMAC", 5) == 0) {
        *curr_chr_pp += 5;
        vESPparseMACfromStr(curr_chr_pp, msg->body.local_ip_mac.ap_mac);
    }
} // end of vESPparseCIFSR

static espRes_t eESPparseFoundAP(uint8_t **curr_chr_pp, espMsg_t *msg) {
    espRes_t response = espOK;
    uint16_t idx, max_num;
    short    num_chrs_copied;
    espAP_t *aps = NULL;

    idx = *(msg->body.ap_list.num_ap_found);
    max_num = msg->body.ap_list.apslen;
    aps = &(msg->body.ap_list.aps[idx]);
    // get rid of useless characters at the beginning
    *curr_chr_pp = (uint8_t *)strstr((const char *)*curr_chr_pp, "+CWLAP:");
    *curr_chr_pp += 7; // skip the beginning part +CWLAP:xxx...
    // get encryption method
    aps->ecn = (espEncrypt_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    *curr_chr_pp += 1; // skip dot char,
    if (**curr_chr_pp == ESP_ASCII_DOUBLE_QUOTE) {
        *curr_chr_pp += 1; // skip double quote
        num_chrs_copied = uESPparseStrUntilToken(
            &aps->ssid[0], (const char *)*curr_chr_pp, ESP_CFG_MAX_SSID_LEN, ESP_ASCII_DOUBLE_QUOTE
        );
        ESP_ASSERT(num_chrs_copied >= 0);
        *curr_chr_pp += 1; // skip double quote
    } else {
        num_chrs_copied = uESPparseStrUntilToken(
            &aps->ssid[0], (const char *)*curr_chr_pp, ESP_CFG_MAX_SSID_LEN, ESP_ASCII_DOT
        );
        if (num_chrs_copied < 0) {
            return espERRMEM;
        } // TODO, consider error variant specific for data corruption
    }
    *curr_chr_pp += num_chrs_copied;
    // check whether the currently found AP is what we are looking for ?
    if (msg->body.ap_list.ssid != NULL) {
        if (strncmp(
                (const char *)&msg->body.ap_list.ssid[0], (const char *)&aps->ssid[0],
                msg->body.ap_list.ssid_len
            ) != 0) {
            return response;
        } // skip received bytes of this line since application caller already
          // specified ssid & this is NOT the AP application wants to connect
    }
    // parse RSSI
    aps->rssi = (int16_t)iESPparseFirstNumFromStr(
        curr_chr_pp,
        ESP_DIGIT_BASE_DECIMAL
    ); // Received signal strength indicator

    if (**curr_chr_pp == ESP_ASCII_DOUBLE_QUOTE) {
        *curr_chr_pp += 1; // skip double quote
        vESPparseMACfromStr(curr_chr_pp, &(aps->mac));
        *curr_chr_pp += 1; // skip double quote
    } else {
        vESPparseMACfromStr(curr_chr_pp, &(aps->mac));
    }
    // WiFi channel used on access point
    aps->ch = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    aps->offset = (int8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    // Calibration value
    aps->cal = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    // skip comma chars following the fields `calibration`, `pairwise-cipher` (pwc),
    // and `group-cipher` (gc)
    for (uint8_t jdx = 0; jdx < 3; jdx++) {
        char *pat = strstr((const char *)*curr_chr_pp, ESP_CHR_COMMA);
        *curr_chr_pp = (uint8_t *)pat + 1;
    }
    // Information about 802.11[b|g|n] support
    aps->bgn = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    // Status if WPS function is supported
    aps->wps = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);

    *msg->body.ap_list.num_ap_found = ++idx;
    // if the given array msg->body.ap_list.aps[] is full, then we skip the AP
    // we find subsequently.
    if (idx == max_num) {
        response = espOKIGNOREMORE;
    }
    return response;
} // end of eESPparseFoundAP

static void vESPparseJoinedAP(uint8_t **curr_chr_pp, espMsg_t *msg) {
    short           num_chrs_copied = 0;
    espStaInfoAP_t *info = NULL;

    info = msg->body.sta_info_ap.info;
    *curr_chr_pp = (uint8_t *)strstr((const char *)*curr_chr_pp, "+CWJAP_CUR:") + 11;
    if (**curr_chr_pp == ESP_ASCII_DOUBLE_QUOTE) {
        *curr_chr_pp += 1; // skip double quote
        num_chrs_copied = uESPparseStrUntilToken(
            &info->ssid[0], (const char *)*curr_chr_pp, ESP_CFG_MAX_SSID_LEN, ESP_ASCII_DOUBLE_QUOTE
        );
        ESP_ASSERT(num_chrs_copied >= 0);
        *curr_chr_pp += 1; // skip double quote
    } else {
        num_chrs_copied = uESPparseStrUntilToken(
            &info->ssid[0], (const char *)*curr_chr_pp, ESP_CFG_MAX_SSID_LEN, ESP_ASCII_DOT
        );
        ESP_ASSERT(num_chrs_copied >= 0);
    }
    *curr_chr_pp += num_chrs_copied;
    *curr_chr_pp += 1; // skip dot char,
    if (**curr_chr_pp == ESP_ASCII_DOUBLE_QUOTE) {
        *curr_chr_pp += 1; // skip double quote
        vESPparseMACfromStr(curr_chr_pp, &(info->mac));
        *curr_chr_pp += 1; // skip double quote
    } else {
        vESPparseMACfromStr(curr_chr_pp, &(info->mac));
    }
    info->ch = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
    info->rssi = (int16_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
} // end of vESPparseJoinedAP

static void vESPparseJoinStatus(espNetAttr_t *d, espStaConnStatus_t status) {
    d->is_connected = status;
    // ip adress must be update for each time ESP device connected to some AP
    d->has_ip = 0;
}

static espRes_t
eESPparseStaJoinResult(espNetAttr_t *netattr, uint8_t **curr_chr_pp, espMsg_t *msg) {
    espRes_t response = espERR;
    uint8_t  error_num = 0;
    if (strncmp((const char *)*curr_chr_pp, "+CWJAP_", 7) == 0) {
        // when CPU gets herem that means something wrong happened when connecting the AP
        *curr_chr_pp += 11; // skip first few words +CWJAP_CUR:
        error_num = (uint8_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
        msg->body.sta_join.error_num = error_num;
        switch (error_num) {
        case 1:
            response = espERRCONNTIMEOUT;
            break;
        case 2:
            response = espERRPASS;
            break;
        case 3:
            response = espERRNOAP;
            break;
        case 4:
            response = espERRCONNFAIL;
            break;
        default:
            break;
        }
        return response;
    }
    if (strncmp((const char *)*curr_chr_pp, "WIFI CONNECTED", 14) == 0) {
        response = espOK;
        vESPparseJoinStatus(netattr, ESP_STATION_CONNECTED);
    }
    return response;
} // end of  eESPparseStaJoinResult

static espRes_t eESPparseStaQuitResult(espNetAttr_t *netattr, uint8_t **curr_chr_pp) {
    // TODO: figure out why ESP quits from AP without response.
    espRes_t response = espERR;
    if (strncmp((const char *)*curr_chr_pp, "WIFI DISCONNECT", 15) == 0) {
        response = espOK;
        vESPparseJoinStatus(netattr, ESP_STATION_DISCONNECTED);
    }
    return response;
}

static void vESPparsePing(uint8_t **curr_chr_pp, espMsg_t *msg) {
    uint32_t resptime = 0;
    if (**curr_chr_pp == '+') {
        resptime = (uint32_t)iESPparseFirstNumFromStr(curr_chr_pp, ESP_DIGIT_BASE_DECIMAL);
        *(msg->body.tcpip_ping.resptime) = resptime;
    }
}

static void vESPparseTCPmuxConn(espGlbl_t *gbl, espMsg_t *msg) {
    gbl->status.flg.mux_conn = msg->body.tcpip_attri.mux;
}

static void vESPparseTCPserverEn(espGlbl_t *gbl, espMsg_t *msg) {
    gbl->evt_server = msg->body.tcpip_server.cb;
}

static espRes_t eESPparseConnSend(const uint8_t *curr_chr_p, uint8_t *isEndOfATresp) {
    espRes_t response = espINPROG;
    if (strncmp((const char *)curr_chr_p, "SEND OK" ESP_CHR_CR ESP_CHR_LF, 9) == 0) {
        response = espOK;
        *isEndOfATresp = 1;
    } else if (strncmp((const char *)curr_chr_p, "SEND FAIL" ESP_CHR_CR ESP_CHR_LF, 11) == 0) {
        response = espERR;
        *isEndOfATresp = 1;
    }
    return response;
}

void vESPparseRecvATrespLine(
    espGlbl_t *gbl, uint8_t *data_line_buf, uint16_t buf_idx, uint8_t *isEndOfATresp
) {
    int       result = 0;
    uint8_t   is_ok = 0, is_err = 0, is_rdy = 0;
    uint8_t  *curr_chr_p = data_line_buf;
    espMsg_t *msg = gbl->msg;
    if (msg == NULL) {
        return;
    }

    espCmd_t curr_cmd = GET_CURR_CMD(msg);

    // return immediately if there are only CR & LF characters in this line
    // buffer.
    if (strncmp((const char *)data_line_buf, ESP_CHR_CR ESP_CHR_LF, buf_idx) == 0) {
        return;
    }
    // check end of response line we've received for the AT command.
    result = strncmp((const char *)data_line_buf, "OK" ESP_CHR_CR ESP_CHR_LF, buf_idx);
    is_ok = (result == 0 ? 1 : 0);
    if (is_ok == 0) {
        result = strncmp((const char *)data_line_buf, "ERROR" ESP_CHR_CR ESP_CHR_LF, buf_idx);
        is_err = (result == 0 ? 1 : 0);
        if (is_err == 0) {
            result = strncmp((const char *)data_line_buf, "ready" ESP_CHR_CR ESP_CHR_LF, buf_idx);
            is_rdy = (result == 0 ? 1 : 0);
        } else {
            // if is_err is NOT equal to zero, few status below can cover espERR, they
            // mean error reported by API function with extra information.
            switch (msg->res) {
            case espBUSY:
            case espERRMEM:
            case espERRNOIP:
            case espERRNOAVAILCONN:
            case espERRCONNTIMEOUT:
            case espERRPASS:
            case espERRNOAP:
            case espERRCONNFAIL:
            case espERRWIFINOTCONNECTED:
            case espERRNODEVICE:
                break;
            default:
                msg->res = espERR;
                break;
            } // end of switch-case statement
        }
    } else { // if is_ok is NOT equal to zero
        // check whether response of the message should be modified.
        // few status below can cover espOK, they mean API function works OK
        // with extra information.
        switch (msg->res) {
        case espOKIGNOREMORE:
            break;
        default:
            msg->res = espOK;
            break;
        }
    } // end of if-statement (check ok, error, fail characters)

    if ((is_ok | is_err | is_rdy) != 0) {
        *isEndOfATresp = 1;
    }

    // for some AT commands we can safely ignore the subsequent received string
    // from ESP device then skip this parsing function. TODO: find better
    // position to place following line of code
    if (msg->res == espOKIGNOREMORE) {
        return;
    }

    // check if ESP device is receiving IPD data or response string of AT
    // commands.
    switch (curr_cmd) {
    case ESP_CMD_GMR:
        // there might be 2 types of response : starts with 'AT version' ,
        // or starts with 'SDK version'
        if (strncmp((const char *)curr_chr_p, "AT version", 10) == 0) {
            curr_chr_p += 10;
            eESPparseVersion(&curr_chr_p, &gbl->dev.version_at);
            // we can confirm that the ESP device is present & ready ONLY when
            // we get correct response of AT+GMR command.
            gbl->status.flg.dev_present = 1;
        } else if (strncmp((const char *)curr_chr_p, "SDK version", 11) == 0) {
            curr_chr_p += 11;
            eESPparseVersion(&curr_chr_p, &gbl->dev.version_sdk);
        }
        break;
#if (ESP_CFG_MODE_STATION != 0)
    case ESP_CMD_WIFI_CWLAP:
        if (*isEndOfATresp == 0) {
            msg->res = eESPparseFoundAP(&curr_chr_p, msg);
        }
        break;
    case ESP_CMD_WIFI_CWJAP_GET:
        if (is_ok == 0) {
            vESPparseJoinedAP(&curr_chr_p, msg);
        }
        break;
    case ESP_CMD_WIFI_CWJAP:
        msg->res = eESPparseStaJoinResult(&gbl->dev.sta, &curr_chr_p, msg);
        *isEndOfATresp = 1;
        break;
    case ESP_CMD_WIFI_CWQAP:
        msg->res = eESPparseStaQuitResult(&gbl->dev.sta, &curr_chr_p);
        if (msg->res == espOK) {
            *isEndOfATresp = 1;
        }
        break;
    case ESP_CMD_WIFI_CIPSTA_GET:
        curr_chr_p += 12; // skip first few characters +CIPSTA_xxx:
        vESPparseAPstaIP(&curr_chr_p, msg);
        break;
    case ESP_CMD_TCPIP_CIPMUX:
        // copy the setting to mux_conn flag in ESP global structure
        vESPparseTCPmuxConn(gbl, msg);
        break;
    case ESP_CMD_TCPIP_CIPSTATUS:
        eESPparseCIPstatus(&curr_chr_p, &gbl->dev);
        break;
#endif // ESP_CFG_MODE_STATION
    case ESP_CMD_TCPIP_CIPSERVER:
        if (is_ok == 1) {
            vESPparseTCPserverEn(gbl, msg);
        }
        break;
    case ESP_CMD_TCPIP_CIPSTO:
    case ESP_CMD_TCPIP_CIPSTART:
    case ESP_CMD_TCPIP_CIPCLOSE:
    case ESP_CMD_TCPIP_CIPDINFO:
    case ESP_CMD_SYSMSG:
        break;
    case ESP_CMD_WIFI_CWLIF:
        break;
#if (ESP_CFG_PING != 0)
    case ESP_CMD_TCPIP_PING:
        vESPparsePing(&curr_chr_p, msg);
        break;
#endif // ESP_CFG_PING
    case ESP_CMD_TCPIP_CIFSR:
        curr_chr_p += 7; // skip the first 7 characters +CIFSR:
        vESPparseCIFSR(&curr_chr_p, msg);
        break;
    case ESP_CMD_TCPIP_CIPSEND:
        if (*isEndOfATresp == 0) {
            msg->res = eESPparseConnSend(curr_chr_p, isEndOfATresp);
        }
        break;
#if (ESP_CFG_MODE_ACCESS_POINT != 0)
    case ESP_CMD_WIFI_CIPAP_GET:
        curr_chr_p += 11; // skip first few characters +CIPAP_xxx:
        vESPparseAPstaIP(&curr_chr_p, msg);
        break;
#endif       // ESP_CFG_MODE_ACCESS_POINT
    default: // unknown commands
        msg->res = espSKIP;
        *isEndOfATresp = 1;
        break;
    } // end of switch-case-statement
} // end of vESPparseRecvATrespLine

static espRes_t eESPparseConnExtension(espConn_t *c, uint8_t *curr_chr_p) {
    char  proto_label_raw[4] = {0};
    char *quote_begin = strchr((const char *)curr_chr_p, ESP_ASCII_DOUBLE_QUOTE);
    if (!quote_begin) {
        return espERRMEM;
    } // TODO, consider specific error for data corruption
    curr_chr_p = (uint8_t *)quote_begin + 1;
    short tok_result = uESPparseStrUntilToken(
        proto_label_raw, (const char *)curr_chr_p, 4, ESP_ASCII_DOUBLE_QUOTE
    );
    if (tok_result < 0) {
        return espERRMEM;
    }
    if (!strncmp(proto_label_raw, "TCP", 3)) {
        c->type = ESP_CONN_TYPE_TCP;
    } else if (!strncmp(proto_label_raw, "UDP", 3)) {
        c->type = ESP_CONN_TYPE_UDP;
    } else if (!strncmp(proto_label_raw, "SSL", 3)) {
        c->type = ESP_CONN_TYPE_SSL;
    } else {
        return espERRMEM;
    }
    uint8_t conn4server = (uint8_t)iESPparseFirstNumFromStr(&curr_chr_p, ESP_DIGIT_BASE_DECIMAL);
    c->status.flg.client = (conn4server ? 0 : 1);
    vESPparseIPfromStr(&curr_chr_p, &c->remote_ip);
    c->remote_port = (espPort_t)iESPparseFirstNumFromStr(&curr_chr_p, ESP_DIGIT_BASE_DECIMAL);
    c->local_port = (espPort_t)iESPparseFirstNumFromStr(&curr_chr_p, ESP_DIGIT_BASE_DECIMAL);
    return espOK;
} // end of eESPparseConnExtension

// current ESP8266 device supports up to 5 active connections at the same time,
// also in this ESP AT software, multiple connection mode is always enabled,
// so we can simply assume that ESP device only uses recv_data_line_buf[0] to
// represent the link ID used by ESP devive.
espRes_t eESPparseNetConnStatus(espGlbl_t *glb, uint8_t *data_line_buf) {
#define CONN_EXT_MSG_PREFIX    "+LINK_CONN:"
#define CONN_EXT_MSG_PREFIX_SZ sizeof(CONN_EXT_MSG_PREFIX) - 1
    uint8_t *curr_chr_p = data_line_buf, link_id = 0;
    uint8_t  is_connect = (strncmp((const char *)&curr_chr_p[1], ",CONNECT", 8) == 0) ? 1 : 0;
    uint8_t  is_close = (strncmp((const char *)&curr_chr_p[1], ",CLOSED", 7) == 0) ? 1 : 0;
    uint8_t  is_connect_ext =
        (strncmp((const char *)&curr_chr_p[0], CONN_EXT_MSG_PREFIX, CONN_EXT_MSG_PREFIX_SZ) == 0)
             ? 1
             : 0;

    if (is_connect == 0 && is_close == 0 && is_connect_ext == 0) {
        return espSKIP;
    }
    if (is_connect_ext) {
        curr_chr_p += CONN_EXT_MSG_PREFIX_SZ;
        uint8_t establish_fail =
            (uint8_t)iESPparseFirstNumFromStr(&curr_chr_p, ESP_DIGIT_BASE_DECIMAL);
        if (establish_fail) {
            return espERRCONNFAIL;
        } // "+LINK_CONN:0,0,\"TCP\",1,\"192.168.2.145\",58072,80\r\n23,0,4,4,7,1)\r\n"
        link_id = (uint8_t)iESPparseFirstNumFromStr(&curr_chr_p, ESP_DIGIT_BASE_DECIMAL);
    } else {
        link_id = ESP_CHARTONUM(curr_chr_p[0]);
    }
    if (link_id >= ESP_CFG_MAX_CONNS) {
        return espERR;
    }
    espConn_t *c = &glb->dev.conns[link_id];
    if (is_connect || is_connect_ext) {
        c->status.flg.active = ESP_CONN_ESTABLISHED;
        // check whether the new active connection acts as client (e.g.
        // AT+CIPSTART, AT+CIPSEND) or a server (e.g. AT+CIPSERVER) on ESP
        // device's side, then parse callback function
        espMsg_t *msg = glb->msg;
        if (is_connect_ext) {
            espRes_t result = eESPparseConnExtension(c, curr_chr_p);
            if (result != espOK) {
                return result;
            }
        } else if (is_connect) {
            if (msg != NULL && GET_CURR_CMD(msg) == ESP_CMD_TCPIP_CIPSTART) {
                c->status.flg.client = 1;
            }
        }
        glb->dev.active_conns |= (1 << link_id);
        if (c->status.flg.client) { // the new connection acts as client
            if (msg) {
                c->cb = msg->body.conn_start.cb;
            }
        } else { // the new connection acts as server
            c->cb = glb->evt_server;
        }
    } else if (is_close != 0) {
        // reset values of previous network connection (also clear active flag)
        ESP_MEMSET(c, 0x00, sizeof(espConn_t));
        glb->dev.active_conns &= ~(1 << link_id);
    }
    return espOK;
#undef CONN_EXT_MSG_PREFIX
#undef CONN_EXT_MSG_PREFIX_SZ
} // end of  eESPparseNetConnStatus

espRes_t eESPparseIPDsetup(espGlbl_t *glb, uint8_t *metadata, espIPD_t **ipd_chosen) {
    uint8_t link_id = 0;
    if (glb->status.flg.mux_conn == ESP_TCP_MULTIPLE_CONNECTION) {
        link_id = (uint8_t)iESPparseFirstNumFromStr(&metadata, ESP_DIGIT_BASE_DECIMAL);
        if (link_id >= ESP_CFG_MAX_CONNS) {
            return espERR;
        }
    }
    espConn_t *c = &glb->dev.conns[link_id];
    espIPD_t  *ipdp = &c->ipd;
    if (ipdp->read == 1) {
        return espBUSY;
    }
    c->status.flg.data_received = 1;
    uint32_t len = (uint32_t)iESPparseFirstNumFromStr(&metadata, ESP_DIGIT_BASE_DECIMAL);
    // retrieve and save IP address / port number of remote sender
    // , this works if CIPDINFO is set to 1
    uint8_t ip_zeros[4] = {0};
    if (strncmp((const char *)ip_zeros, (const char *)c->remote_ip.ip, 4) == 0) {
        vESPparseIPfromStr(&metadata, &c->remote_ip);
    }
    if (c->remote_port == 0) {
        c->remote_port = (espPort_t)iESPparseFirstNumFromStr(&metadata, ESP_DIGIT_BASE_DECIMAL);
    }
    ipdp->tot_len = len;
    ipdp->rem_len = len;
    ipdp->conn = c;
    ipdp->pbuf_head = NULL;
    ipdp->read = 1;
    *ipd_chosen = ipdp;
    return espOK;
} // end of eESPparseIPDsetup

espRes_t eESPparseIPDcopyData(espIPD_t *ipdp, const uint8_t *data, uint32_t *data_len) {
    espRes_t   response = espINPROG;
    uint32_t   remain_len = ipdp->rem_len, copy_len = 0;
    uint8_t    chain_cnt = 0;
    espPbuf_t *prev_p = NULL, *curr_p = NULL;
    // loop through all packet buffer & find out the last one
    curr_p = ipdp->pbuf_head;
    while (curr_p != NULL) {
        prev_p = curr_p;
        curr_p = curr_p->next;
        chain_cnt += 1;
    }
    // create new packet buffer to hold IPD
    copy_len = ESP_MIN(*data_len, remain_len);
    curr_p = (espPbuf_t *)pxESPpktBufCreate(copy_len);
    // copy associated connection object to this packet buffer item
    curr_p->conn = ipdp->conn;
    // append the new packet buffer to the last of the chain
    if (prev_p != NULL) {
        prev_p->next = curr_p;
    } else {
        ipdp->pbuf_head = curr_p;
    }
    ipdp->pbuf_head->chain_len = chain_cnt + 1;
    // copy IPD data to packet buffer
    eESPpktBufCopy(curr_p, (void *)data, copy_len);
    // re-calculate rest of data that hasn't been received
    if (*data_len >= remain_len) {
        *data_len = remain_len;
        remain_len = 0;
        ipdp->conn->pbuf = ipdp->pbuf_head;
        response = espOK;
    } else {
        remain_len -= *data_len; // in this case, *data_len == copy_len
    }
    ipdp->rem_len = remain_len;
    return response;
} // end of eESPparseIPDcopyData

espRes_t eESPparseIPDreset(espIPD_t *ipdp) {
    ipdp->conn->status.flg.data_received = 0;
    ESP_MEMSET(ipdp, 0x00, sizeof(espIPD_t));
    ////ipdp->read  = 0;
    return espOK;
}
