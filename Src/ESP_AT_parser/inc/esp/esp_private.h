#ifndef __ESP_PRIVATE_H
#define __ESP_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "esp/esp_typedef.h"
#include "system/esp_sys.h"


// the data structure / macros that are NOT used globally.

#define CHK_CURR_CMD( _msg, cmdtype )        ( _msg != NULL && (_msg)->cmd == (cmdtype))

#define GET_CURR_CMD( _msg )                 ((espCmd_t)(((_msg != NULL) ? (_msg)->cmd: ESP_CMD_IDLE)))


// lists available commands implemented in this ESP AT library.
typedef enum {
    ESP_CMD_IDLE = 0,                           /*!< IDLE mode */

    /* Basic AT commands */
    ESP_CMD_RESET,                              /*!< Reset device */
    ESP_CMD_ATE0,                               /*!< Disable ECHO mode on AT commands */
    ESP_CMD_ATE1,                               /*!< Enable ECHO mode on AT commands */
    ESP_CMD_GMR,                                /*!< Get AT commands version */
    ESP_CMD_GSLP,                               /*!< Set ESP to sleep mode */
    ESP_CMD_RESTORE,                            /*!< Restore ESP internal settings to default values */
    ESP_CMD_UART,
    ESP_CMD_WAKEUPGPIO,
    ESP_CMD_RFPOWER,
    ESP_CMD_SYSRAM,
    ESP_CMD_SYSADC,
    ESP_CMD_SYSIOSETCFG,
    ESP_CMD_SYSIOGETCFG,
    ESP_CMD_SYSGPIODIR,
    ESP_CMD_SYSGPIOWRITE,
    ESP_CMD_SYSGPIOREAD,
    ESP_CMD_SYSMSG,
    ESP_CMD_WIFI_CWLAPOPT,                      /*!< Configure what is visible on CWLAP response */

    /* WiFi based commands */
    ESP_CMD_WIFI_CWMODE,                        /*!< Set/Get wifi mode */
#if (ESP_CFG_MODE_STATION  != 0)
    ESP_CMD_WIFI_CWJAP,                         /*!< Connect to access point */
    ESP_CMD_WIFI_CWJAP_GET,                     /*!< Info of the connected access point */
    ESP_CMD_WIFI_CWQAP,                         /*!< Disconnect from access point */
    ESP_CMD_WIFI_CWLAP,                         /*!< List available access points */
    ESP_CMD_WIFI_CIPSTAMAC_GET,                 /*!< Get MAC address of ESP station */
    ESP_CMD_WIFI_CIPSTAMAC_SET,                 /*!< Set MAC address of ESP station */
    ESP_CMD_WIFI_CIPSTA_GET,                    /*!< Get IP address of ESP station */
    ESP_CMD_WIFI_CIPSTA_SET,                    /*!< Set IP address of ESP station */
    ESP_CMD_WIFI_CWAUTOCONN,                    /*!< Configure auto connection to access point */
#endif /* ESP_CFG_MODE_STATION */
#if (ESP_CFG_MODE_ACCESS_POINT  != 0)
    ESP_CMD_WIFI_CWSAP_GET,                     /*!< Get software access point configuration */
    ESP_CMD_WIFI_CWSAP_SET,                     /*!< Set software access point configuration */
    ESP_CMD_WIFI_CIPAPMAC_GET,                  /*!< Get MAC address of ESP access point */
    ESP_CMD_WIFI_CIPAPMAC_SET,                  /*!< Set MAC address of ESP access point */
    ESP_CMD_WIFI_CIPAP_GET,                     /*!< Get IP address of ESP access point */
    ESP_CMD_WIFI_CIPAP_SET,                     /*!< Set IP address of ESP access point */
    ESP_CMD_WIFI_CWLIF,                         /*!< Get connected stations on access point */
#endif /* ESP_CFG_MODE_STATION */
#if (ESP_CFG_WPS  != 0)
    ESP_CMD_WIFI_WPS,                           /*!< Set WPS option */
#endif /* ESP_CFG_WPS  */
#if (ESP_CFG_MDNS  != 0)
    ESP_CMD_WIFI_MDNS,                          /*!< Configure MDNS function */
#endif /* ESP_CFG_MDNS  */
#if (ESP_CFG_HOSTNAME  != 0)
    ESP_CMD_WIFI_CWHOSTNAME_SET,                /*!< Set device hostname */
    ESP_CMD_WIFI_CWHOSTNAME_GET,                /*!< Get device hostname */
#endif /* ESP_CFG_HOSTNAME  */

    /* TCP/IP related commands */
#if (ESP_CFG_DNS  != 0)
    ESP_CMD_TCPIP_CIPDOMAIN,                    /*!< Get IP address from domain name = DNS function */
    ESP_CMD_TCPIP_CIPDNS_SET,                   /*!< Configure user specific DNS servers */
    ESP_CMD_TCPIP_CIPDNS_GET,                   /*!< Get DNS configuration */
#endif /* ESP_CFG_DNS  */
    ESP_CMD_TCPIP_CIPSTATUS,                    /*!< Get status of connections */
    ESP_CMD_TCPIP_CIPSTART,                     /*!< Start client connection */
    ESP_CMD_TCPIP_CIPSEND,                      /*!< Send network data */
    ESP_CMD_TCPIP_CIPCLOSE,                     /*!< Close active connection */
    ESP_CMD_TCPIP_CIFSR,                        /*!< Get local IP */
    ESP_CMD_TCPIP_CIPMUX,                       /*!< Set single or multiple connections */
    ESP_CMD_TCPIP_CIPSERVER,                    /*!< Enables/Disables server mode */
    ESP_CMD_TCPIP_CIPMODE,                      /*!< Transmission mode, either transparent or normal one */
    ESP_CMD_TCPIP_CIPSTO,                       /*!< Sets connection timeout */
#if (ESP_CFG_CONN_MANUAL_TCP_RECV  != 0)
    ESP_CMD_TCPIP_CIPRECVMODE,                  /*!< Sets mode for TCP data receive (manual or automatic) */
    ESP_CMD_TCPIP_CIPRECVDATA,                  /*!< Manually reads TCP data from device */
#endif // end of ESP_CFG_CONN_MANUAL_TCP_RECV 
#if (ESP_CFG_PING  != 0)
    ESP_CMD_TCPIP_PING,                         /*!< Ping domain */
#endif /* ESP_CFG_PING  */
    ESP_CMD_TCPIP_CIUPDATE,                     /*!< Perform self-update */
#if (ESP_CFG_SNTP  != 0)
    ESP_CMD_TCPIP_CIPSNTPCFG,                   /*!< Configure SNTP servers */
    ESP_CMD_TCPIP_CIPSNTPTIME,                  /*!< Get current time using SNTP */
#endif /* ESP_SNT  */
    ESP_CMD_TCPIP_CIPDINFO,                     /*!< Configure what data are received on +IPD statement */
} espCmd_t;



// Incoming network data read structure
typedef struct {
    uint8_t             read;                   /*!< Set to non-zero when we recognize received string (from Rx of ESP device) as IPD data */
    uint32_t            tot_len;                /*!< Total length of packet */
    uint32_t            rem_len;                /*!< Remaining bytes to read in current +IPD statement */
    espConn_t          *conn;                   /*!< Pointer to connection, TODO: figure out its usage */
    espIp_t             ip;                     /*!< Remote IP address on from IPD data */
    espPort_t           port;                   /*!< Remote port on IPD data */
    espPbuf_t          *pbuf_head;              /*!< Pointer to buffer for collecting receiving data */
} espIPD_t;




// data structure used as item of message queue,  shared between threads processing AT command.
typedef struct espMsg  { 
    espCmd_t        cmd;                        /*!< Default message type received from queue */
                                                /*  Note: for any API function executing extended AT-command sequence
                                                    (more than one AT commands to execute), this ESP AT software will create
                                                    separated msg struture for each extended AT command requests, there are 
                                                    pros & cons about doing this :
                                                    pros: keep non-blocking feature, sequence of non-blocking AT-commands can
                                                          be safely executed, this also improves ease of code maintenance.
                                                    cons: one msg structure will NOT be shared among these extends AT commands,
                                                          this ESP AT software runs malloc() and free(), to create separate msg
                                                          structure for each AT command.
                                                 */
    espSysSem_t     sem;                        /*!< Semaphore used only for this message */
    uint8_t         is_blocking;                /*!< Status if command is blocking */
    uint32_t        block_time;                 /*!< Maximal blocking time in units of milliseconds. Use 0 to for non-blocking call */
    espRes_t        res;                        /*!< Result of message operation */
    espRes_t        (*fn)(struct espMsg *);     /*!< callback function to process packet, generate and send the AT command string */
    espApiCmdCbFn   api_cb ;                    /*!< Command callback API function */
    void*           api_cb_arg;                 /*!< Command callback API callback parameter */

    union {
        struct {
            uint32_t delay;                     /*!< Delay in units of milliseconds before executing first RESET command */
        } reset;                                /*!< Reset device */
        struct {
            uint32_t ms;
        } deepslp;
        struct {
            uint32_t baudrate;                  /*!< Baudrate for AT port */
        } uart;                                 /*!< UART configuration */
        struct {
            espFnEn_t ext_info_netconn;       // do we enable extra information to describe currently processing network connection ?
            espFnEn_t ext_info_ipd;           // do we enable extra information to describe currently processing IPD data ?
        } sysargs;
        struct {
            espMode_t mode;                     /*!< Mode of operation */
            uint8_t def;                        /*!< Value indicates to set mode as default or not */
        } wifi_mode;                            /*!< When message type \ref ESP_CMD_WIFI_CWMODE is used */
#if (ESP_CFG_MODE_STATION  != 0)
        struct {
            const char* name;                   /*!< AP name */
            const char* pass;                   /*!< AP password */
            uint16_t    name_len;
            uint16_t    pass_len;
            const espMac_t* mac;               /*!< Specific MAC address to use when connecting to AP */
            uint8_t def;                        /*!< Value indicates to connect as current only or as default */
            uint8_t error_num;                  /*!< Error number on connecting */
        } sta_join;                             /*!< Message for joining to access point */
        struct {
            uint8_t en;                         /*!< Status to enable/disable auto join feature */
        } sta_autojoin;                         /*!< Message for auto join procedure */
        struct {
            espStaInfoAP_t*  info;              /*!< Information structure */
        } sta_info_ap;                          /*!< Message for reading the AP information */
        struct {
            const char* ssid;                   /*!< Pointer to optional filter SSID name to search */
            uint16_t    ssid_len;               /*!< length of ssid string above */
            espAP_t* aps;                       /*!< Pointer to array to save access points */
            uint16_t  apslen;                   /*!< Length of input array of access points */
            uint16_t  apsi;                     /*!< Current access point array */
            uint16_t* num_ap_found;             /*!< Pointer to output variable holding number of access points found */
        } ap_list;                              /*!< List for access points */
#endif /* ESP_CFG_MODE_STATION  */
#if (ESP_CFG_MODE_ACCESS_POINT  != 0)
        struct {
            const char* ssid;                   /*!< Name of access point */
            const char* pwd;                    /*!< Password of access point */
            espEncrypt_t ecn;                   /*!< Ecryption used */
            uint8_t ch;                         /*!< RF Channel used */
            uint8_t max_sta;                    /*!< Max allowed connected stations */
            uint8_t hid;                        /*!< Configuration if network is hidden or visible */
            uint8_t def;                        /*!< Save as default configuration */
        } ap_conf;                              /*!< Parameters to configure access point */
        struct {
            espSta_t* stas;                    /*!< Pointer to array to save access points */
            size_t stal;                        /*!< Length of input array of access points */
            size_t stai;                        /*!< Current access point array */
            size_t* staf;                       /*!< Pointer to output variable holding number of access points found */
        } sta_list;                             /*!< List for stations */
#endif /* ESP_CFG_MODE_ACCESS_POINT  */
        struct {
            espIp_t*  ip;                       /*!< Pointer to IP variable */
            espIp_t*  gw;                       /*!< Pointer to gateway variable */
            espIp_t*  nm;                       /*!< Pointer to netmask variable */
            uint8_t   def;                      /*!< Value for receiving default or current settings */
        } sta_ap_getip;                         /*!< Message for reading station or access point IP */
        struct {
            espMac_t* mac;                      /*!< Pointer to MAC variable */
            uint8_t def;                        /*!< Value for receiving default or current settings */
        } sta_ap_getmac;                        /*!< Message for reading station or access point MAC address */
        struct {
            const espIp_t* ip;                 /*!< Pointer to IP variable */
            const espIp_t* gw;                 /*!< Pointer to gateway variable */
            const espIp_t* nm;                 /*!< Pointer to netmask variable */
            uint8_t def;                        /*!< Value for receiving default or current settings */
        } sta_ap_setip;                         /*!< Message for setting station or access point IP */
        struct {
            const espMac_t* mac;               /*!< Pointer to MAC variable */
            uint8_t def;                        /*!< Value for receiving default or current settings */
        } sta_ap_setmac;                        /*!< Message for setting station or access point MAC address */
        struct {
            const espIp_t*    sta_ip;
            const espMac_t*   sta_mac;
            const espIp_t*    ap_ip;
            const espMac_t*   ap_mac;
        } local_ip_mac;
        struct {
            const char* hostname_set;           /*!< Hostname set value */
            char*  hostname_get;                /*!< Hostname get value */
            size_t length;                      /*!< Length of buffer when reading hostname */
        } wifi_hostname;                        /*!< Set or get hostname structure */

        /* Connection based commands */
        struct {
            espConn_t**     conn;               /*!< Pointer to pointer to save connection used */
            const char*     host;               /*!< Host to use for connection */
            uint16_t        host_len ;
            espPort_t       port;               /*!< Remote port used for connection */
            espConnType_t   type;               /*!< Connection type */
            espEvtCbFn      cb;                 /*!< callback function for client connections */
            uint8_t         num;                /*!< Connection number used for start */
            uint8_t         success;            /*!< Status if connection AT+CIPSTART succedded */
        } conn_start;                           /*!< Structure for starting new connection */
        struct {
            espConn_t*  conn;                   /*!< Pointer to connection to close */
        } conn_close;                           /*!< Close connection */
        struct {
            espConn_t      *conn;               /*!< Pointer to connection to send data */
            const uint8_t  *data;               /*!< Data to send */
            uint16_t        d_size;             /*!< size of data (bytes) ready to transfer, shouldn't be greater than 2048 */
            const espIp_t  *remote_ip;          /*!< Remote IP address for UDP connection */
            espPort_t       remote_port;        /*!< Remote port address for UDP connection */
            uint8_t        val_id;              /*!< Connection current validation ID when command was sent to queue */
        } conn_send;                            /*!< Structure to send data on connection */

        struct {
            espConn_t*  conn;                   /*!< Connection handle */
            size_t      len;                     /*!< Number of bytes to read */
            espPbuf_t  *buff;                    /*!< Buffer handle */
        } ciprecvdata;                          /*!< Structure to manually read TCP data */

        /* TCP/IP based commands */
        struct {
            espTransMode_t  trans_mode;         // TCP transmission mode supported in ESP device
            espMultiConn_t  mux;                /*!< Mux status, either enabled or disabled */
        } tcpip_attri;                          /*!< attributes for setting up TCP connections */
        struct {
            espEvtCbFn  cb;                     // callback functions for server, especially useful when the server receives IPD data,
            uint8_t     en;                     /*!< Enable/Disable server status */
            espPort_t   port;                   /*!< Server port number */
            uint16_t    max_conn;               /*!< Maximal number of connections available for server */
            uint16_t    timeout;                /*!< Connection timeout */
        } tcpip_server;                         /*!< Server configuration */
        struct {
            size_t size;                        /*!< Size for SSL in uints of bytes */
        } tcpip_sslsize;                        /*!< TCP SSL size for SSL connections */
#if (ESP_CFG_DNS != 0)
        struct {
            const char* host;                   /*!< Hostname to resolve IP address for */
            espIp_t* ip;                       /*!< Pointer to IP address to save result */
        } dns_getbyhostname;                    /*!< DNS function */
        struct {
            uint8_t en;                         /*!< Enable/Disable status */
            const char* s1;                     /*!< DNS server 1 */
            const char* s2;                     /*!< DNS server 2 */
            uint8_t def;                        /*!< Default/current config */
        } dns_setconfig;                        /*!< Set DNS config */
#endif /* ESP_CFG_DNS */
#if (ESP_CFG_PING  != 0)
        struct {
            const char* host;                   /*!< Hostname to ping */
            uint16_t host_len;                  // string length of the host/IP
            uint32_t time;                      /*!< Time used for ping */
            uint32_t* resptime;                 /*!< Pointer to response time of the ping operation */
        } tcpip_ping;                           /*!< Pinging structure */
#endif /* ESP_CFG_PING  */
#if (ESP_CFG_SNTP  != 0)
        struct {
            uint8_t en;                         /*!< Status if SNTP is enabled or not */
            int8_t tz;                          /*!< Timezone setup */
            const char* h1;                     /*!< Optional server 1 */
            const char* h2;                     /*!< Optional server 2 */
            const char* h3;                     /*!< Optional server 3 */
        } tcpip_sntp_cfg;                       /*!< SNTP configuration */
        struct {
            espDatetime_t*  dt;                 /*!< Pointer to datetime structure */
        } tcpip_sntp_time;                      /*!< SNTP get time */
#endif /* ESP_CFG_SNTP  */
#if (ESP_CFG_WPS  != 0)
        struct {
            uint8_t en;                         /*!< Status if WPS is enabled or not */
        } wps_cfg;                              /*!< WPS configuration */
#endif /* ESP_CFG_WPS  */
    } body;                                      /*!< Group of different message contents */
} espMsg_t;



// collect attribute of a network device (AP or station) like IP, MAC address, and gateway address.
typedef struct {
    espIp_t  ip;                                /*!< IP address */
    espIp_t  gw;                                /*!< Gateway address */
    espIp_t  nm;                                /*!< Netmask address */
    espMac_t mac;                               /*!< MAC address */
    uint8_t has_ip;                             /*!< Flag indicating ESP has IP */
    espStaConnStatus_t is_connected;            /*!< Flag indicating ESP is connected to wifi */
} espNetAttr_t;



// linked list item for event callback function
typedef struct espEvtCbFnLstItem {
    struct espEvtCbFnLstItem* next;             /*!< Next function in the list */
    espEvtCbFn cb;                              /*!< Function pointer itself */
} espEvtCbFnLstItem_t;




// ESP device status structure
typedef struct {
    espDevName_t        name;                   /*!< ESP device name */
    espFwVer_t          version_at;             /*!< Version of AT command software on ESP device */
    espFwVer_t          version_sdk;            /*!< Version of SDK used to build AT software */
    uint32_t            active_conns;           /*!< Bit field of currently active connections, 
                                                     [TODO]: for old version of ESP device e.g. ESP-01, 32-bit variable should be
                                                            enough to represent exiting TCP connections, in case user has more
                                                            than 32 connections, single variable is not enough */
    uint32_t            active_conns_last;      /*!< The same as previous but status before last check */
    espIPD_t            ipd;                    /*!< Connection incoming data structure */
    espConn_t           conns[ESP_CFG_MAX_CONNS];   /*!< Array of all connection structures */
#if (ESP_CFG_MODE_STATION  != 0)
    espNetAttr_t        sta;                    /*!< Station IP and MAC addressed */
#endif /* ESP_CFG_MODE_STATION  */
#if (ESP_CFG_MODE_ACCESS_POINT  != 0)
    espNetAttr_t        ap;                     /*!< Access point IP and MAC addressed */
#endif /* ESP_CFG_MODE_ACCESS_POINT  */
} espDev_t;




// data structure for collecting state which can be globally accessed in this ESP AT library.
typedef struct {
    espSysSem_t             sem_th_sync;        /*!< Synchronization semaphore between the command-handling threads */
    espSysMbox_t            mbox_cmd_req;       /*!< buffer AT-command requests sent from API function calls */
    espSysMbox_t            mbox_cmd_resp;      /*!< buffer AT-command responses received from ESP device */
    espSysThread_t          thread_cmd_req;     /*!< system thread that handles AT-command requests  */
    espSysThread_t          thread_cmd_resp;    /*!< system thread that handles AT-command responses */
    espLLvl_t               ll;                 /*!< Low level functions */
    espMsg_t*               msg;                /*!< Pointer to the message currently being executed */
    espEvt_t                evt;                /*!< Callback processing structure */
    espEvtCbFnLstItem_t*    evtCbLstHead;       /*!< starting point to the linked list of event callback function */
    espEvtCbFn              evt_server;         /*!< Default callback function for server connections */
    espDev_t                dev;                /*!< All device modules. */
    uint16_t                locked_cnt;         /*!< Count number of times (recursive) stack is currently locked */
    union {
        struct {
            espMultiConn_t   mux_conn;          // indicate whether current setting is multiple connection.
            uint8_t     initialized:1;          /*!< Flag indicating ESP library is initialized */
            uint8_t     dev_present:1;          /*!< Flag indicating if physical device is connected to host device */
        } flg;                                    /*!< Flags structure */
    } status;                                   /*!< Status structure */
} espGlbl_t;





// get ESP glabol data structure.
espGlbl_t*  pxESPgetGlobalData( void );

// create / delete message for new AT command request
espMsg_t* pxESPmsgCreate( espCmd_t cmd, espApiCmdCbFn  api_cb, void* cb_arg, const uint8_t blocking );

void  vESPmsgDelete(espMsg_t** msg);


// internal functions that handle message between APIs and low-level hardware operations
espRes_t    eESPsendReqToMbox (espMsg_t* msg, espRes_t (* initATcmdFn)(espMsg_t *) );

espRes_t    eESPrecvReqFromMbox( espMsg_t** msg, uint32_t max_block_time );



// the final step in the thread  vESPthreadATreqHandler() to generate send AT command string,
// then call low-level hardware-specific function to send the command string to ESP device
espRes_t    eESPinitATcmd( espMsg_t* msg );

// for few AT commands, host CPU must send extra data after sending out AT command to ESP device
// (e.g. AT+CIPSEND) and before getting final response, this functions will be used in such cases.
espRes_t    eESPcmdStartSendData( espMsg_t* msg );


// run the registered event callback function for some API functions
void        vESPapiRunEvtCallbacks( espMsg_t* msg );




#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif // end of  __ESP_PRIVATE_H

