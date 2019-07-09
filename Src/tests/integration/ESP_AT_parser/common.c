#include "esp/esp.h"

#define  MAX_NUM_AP_FOUND    15



typedef struct {
   const char* ssid ;
   const char* passwd ; 
   uint8_t     ssid_len ;
   uint8_t     passwd_len ; 
} apEntry_t;


// TODO : figure out why we connot create static array of struct in C
static espAP_t    foundAPs[ MAX_NUM_AP_FOUND ];
static PRIVILEGED_DATA apEntry_t  preferredAP = { "FamilyOps", "F0963369701", 9, 11 };
 



espRes_t  eESPtestConnAP( uint8_t waitUntilConnected )
{
    espRes_t  response = espOK;
    uint16_t  num_ap_found = 0;
    // do we cache this connection to internal flash memory of ESP device (if any preferred AP is found)
    uint8_t   tried_conn = 0;
    uint16_t  idx = 0;
    do {
        if((response = eESPstaHasIP()) == espOK) break; 
        // clear the APs the ESP device found previously
        num_ap_found = 0;
        ESP_MEMSET( &foundAPs, 0x00, sizeof(espAP_t) * MAX_NUM_AP_FOUND );
        response = eESPstaListAP( NULL, 0, &foundAPs, ESP_ARRAYSIZE(foundAPs), &num_ap_found, 
                                  NULL, NULL, ESP_AT_CMD_BLOCKING );
        if((response == espOK) || (response == espOKIGNOREMORE)) {
            // Print all access points found by ESP 
            //// for (idx = 0; idx < num_ap_found; idx++) {
            ////     printf("[INFO] AP found: %s, CH: %d, RSSI: %d\r\n", foundAPs[idx].ssid, foundAPs[idx].ch, foundAPs[idx].rssi );
            //// }
            // seek for & connect to preferred AP
            tried_conn = 0;
            for (idx = 0; idx < num_ap_found; idx++) {
                // start connecting if preferred AP is found in this scan
                if(!strcmp(preferredAP.ssid , foundAPs[idx].ssid)) 
                {
                    tried_conn = 1;
                    response =  eESPstaJoin( preferredAP.ssid,   preferredAP.ssid_len   ,
                                             preferredAP.passwd, preferredAP.passwd_len , 
                                             NULL,  ESP_SETVALUE_NOT_SAVE , NULL, NULL, ESP_AT_CMD_BLOCKING);
                    if (response == espOK) {
                            espIp_t   sta_ip,  ap_ip;
                            espMac_t  sta_mac, ap_mac;
                            // TODO: figure out why my ESP-01s never responds when we ask for IP address
                            eESPgetLocalIPmac( &sta_ip, &sta_mac, &ap_ip, &ap_mac,  NULL, NULL, ESP_AT_CMD_BLOCKING );
                            waitUntilConnected = 0x0;
                            //// printf("[INFO] Connected to %s network!\r\n", foundAPs[idx].ssid);
                            //// printf("[INFO] Station IP address: %d.%d.%d.%d\r\n",
                            ////    (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3]);
                            break;
                    }
                    else {  // Connection error 
                         vESPsysDelay(5000);
                    }
                }
            } // end of for-loop
            if (tried_conn == 0) { // preferred AP not found 
                vESPsysDelay(5000);
            }
        }
        else if (response == espERRNODEVICE) {
            // Device is not present!
            break;
        }
        else {
            // Other Errors on WIFI scan procedure
            vESPsysDelay(5000);
        }
    }
    while (waitUntilConnected != 0x0);
    return  response;
} // end of eESPtestConnAP


