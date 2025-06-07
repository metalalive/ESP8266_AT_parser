#ifndef __ESP_TYPEDEF_H
#define __ESP_TYPEDEF_H
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

// early declaration , for all functions that need the type
// symbol without knowing the detail
struct espConn;
struct espPbuf;
struct espGlbl_s;

// ------------------------ structure, data type declaration
// ------------------------------ result / status transferred among core
// functions of this library
typedef enum {
    espOK = 0,              // Function succeeded
    espOKIGNOREMORE,        // Function succedded, but ignore sending more data. This
                            // result is possible on connection data receive callback
    espOKNOCMDREQ,          // few API functions can get their job done wihtout
                            // generating AT command request. such the API calls can
                            // return this state.
    espSKIP,                // skip and return from functions, without completing it
    espERR,                 // other unknown errors
    espBUSY,                // busy signal from underlying hardware platform
    espERRARGS,             // Wrong arguments on function call
    espERRMEM,              // Memory error occurred
    espTIMEOUT,             // Timeout occurred on command
    espINPROG,              // Operation is in progress, used in both AT-command mode and IPD
                            // receiving mode.
    espERRNOIP,             // Station does not have IP address
    espERRNOAVAILCONN,      // There is no free connection available to start */
    espERRCONNTIMEOUT,      // Timeout received on connection to access point */
    espERRPASS,             // Invalid password for access point */
    espERRNOAP,             // No access point found with specific SSID and MAC address */
    espERRCONNFAIL,         // Connection failed to access point */
    espERRWIFINOTCONNECTED, // Wifi not connected to access point */
    espERRNODEVICE,         // Device is not present */
    espERRBLOCKING,         // Blocking mode command is not allowed */
} espRes_t;

// List of supported ESP devices name by firmware
typedef enum {
    ESP_DEVICE_ESP8266, /*!< Device is ESP8266 */
    ESP_DEVICE_UNKNOWN, /*!< Unknown device */
} espDevName_t;

typedef enum {
    ESP_DIGIT_BASE_DECIMAL,
    ESP_DIGIT_BASE_HEX,
} espDigitBase_t;

typedef enum {
    ESP_DISABLE = 0,
    ESP_ENABLE = 1,
} espFnEn_t;

typedef enum {
    ESP_STATION_DISCONNECTED = 0,
    ESP_STATION_CONNECTED,
} espStaConnStatus_t;

typedef enum {
    ESP_CONN_CLOSED = 0,
    ESP_CONN_ESTABLISHED,
} espConnStatus_t;

// List of supported encryption on ESP device
typedef enum {
    ESP_ECN_OPEN = 0x00,         // No encryption on access point
    ESP_ECN_WEP = 0x01,          // WEP (Wired Equivalent Privacy) encryption
    ESP_ECN_WPA_PSK = 0x02,      // WPA (Wifi Protected Access) encryption
    ESP_ECN_WPA2_PSK = 0x03,     // WPA2 (Wifi Protected Access 2) encryption
    ESP_ECN_WPA_WPA2_PSK = 0x04, // WPA/2 (Wifi Protected Access 1/2) encryption
} espEncrypt_t;

// IPv4 address structure
typedef struct {
    uint8_t ip[4];
} espIp_t;

// port number, for server mode
typedef uint16_t espPort_t;

// MAC address structure
typedef struct {
    uint8_t mac[6];
} espMac_t;

// firmware version of the ESP device
typedef struct {
    uint8_t major; // Major version
    uint8_t minor; // Minor version
    uint8_t patch; // Patch version
} espFwVer_t;

// Access point data structure
typedef struct {
    espEncrypt_t ecn;                        // Encryption mode
    char         ssid[ESP_CFG_MAX_SSID_LEN]; // Access point name
    int16_t      rssi;                       // Received signal strength indicator
    espMac_t     mac;                        // MAC physical address
    uint8_t      ch;                         // WiFi channel used on access point
    int8_t       offset;                     // Access point offset
    uint8_t      cal;                        // Calibration value
    uint8_t      bgn;                        // Information about 802.11[b|g|n] support
    uint8_t      wps;                        // Status if WPS function is supported
} espAP_t;

// Access point information on which station is connected to
typedef struct {
    char     ssid[ESP_CFG_MAX_SSID_LEN]; // Access point name
    int16_t  rssi;                       // RSSI
    espMac_t mac;                        // MAC address
    uint8_t  ch;                         // Channel information
} espStaInfoAP_t;

// station data structure
typedef struct {
    espIp_t  ip;  /*!< IP address of connected station */
    espMac_t mac; /*!< MAC address of connected station */
} espSta_t;

// date and time structure
typedef struct {
    uint8_t  date;    /* Day in a month, from 1 to up to 31 */
    uint8_t  month;   /* Month in a year, from 1 to 12 */
    uint16_t year;    /* Year */
    uint8_t  day;     /* Day in a week, from 1 to 7 */
    uint8_t  hours;   /* Hours in a day, from 0 to 23 */
    uint8_t  minutes; /* Minutes in a hour, from 0 to 59 */
    uint8_t  seconds; /* Seconds in a minute, from 0 to 59 */
} espDatetime_t;

typedef enum {
#if (ESP_CFG_MODE_STATION != 0)
    ESP_MODE_STA = 1, // Set WiFi mode to station only */
#endif                /* ESP_CFG_MODE_STATION */
#if (ESP_CFG_MODE_ACCESS_POINT != 0)
    ESP_MODE_AP = 2, // Set WiFi mode to access point only */
#endif               /* ESP_CFG_MODE_ACCESS_POINT */
#if (ESP_CFG_MODE_STATION_ACCESS_POINT != 0)
    ESP_MODE_STA_AP = 3, // Set WiFi mode to both of station and access point */
#endif                   /* (ESP_CFG_MODE_STATION_ACCESS_POINT) */
} espMode_t;

typedef enum {
    ESP_TRNASMIT_NORMAL_MODE = 0,
    ESP_TRNASMIT_PASSTHROUGH_MODE = 1,
} espTransMode_t;

typedef enum {
    ESP_TCP_SINGLE_CONNECTION = 0,
    ESP_TCP_MULTIPLE_CONNECTION = 1,
} espMultiConn_t;

// List of possible connection types
typedef enum {
    ESP_CONN_TYPE_TCP, // Connection type is TCP
    ESP_CONN_TYPE_UDP, // Connection type is UDP
    ESP_CONN_TYPE_SSL, // connection type is SSL, should be deprecated since it
                       // was proven insecure.
} espConnType_t;

// forward declaration
struct espMsg;

// pointer to espMsg_t structure
typedef struct espMsg *espMsgPtr;

// List all possible event types sent to user in callback function
typedef enum {
    ESP_EVT_NO_EVENT,
    ESP_EVT_INIT_FINISH,              /* Initialization has been finished at this point */
    ESP_EVT_RESET_DETECTED,           /* Device reset detected */
    ESP_EVT_RESET,                    /* Device reset operation finished */
    ESP_EVT_RESTORE,                  /* Device restore operation finished */
    ESP_EVT_CMD_TIMEOUT,              /* Timeout on command.
                                          When application receives this event,
                                          it may reset system as there was (maybe) a problem
                                         in device */
    ESP_EVT_DEVICE_PRESENT,           /* Notification when device present status changes
                                       */
    ESP_EVT_AT_VERSION_NOT_SUPPORTED, /* Library does not support firmware
                                         version on ESP device. */
    ESP_EVT_CONN_RECV,                /* Connection data received */
    ESP_EVT_CONN_SEND,                /* Connection data send */
    ESP_EVT_CONN_ACTIVE,              /* Connection just became active */
    ESP_EVT_CONN_ERROR,               /* Client connection start was not successful */
    ESP_EVT_CONN_CLOSED,              /* Connection was just closed */
    ESP_EVT_CONN_POLL,                /* Poll for connection if there are any changes */
    ESP_EVT_SERVER,                   /* Server status changed */
#if ESP_CFG_MODE_STATION || __DOXYGEN__
    ESP_EVT_WIFI_CONNECTED,    /* Station just connected to AP */
    ESP_EVT_WIFI_GOT_IP,       /* Station has valid IP */
    ESP_EVT_WIFI_DISCONNECTED, /* Station just disconnected from AP */
    ESP_EVT_WIFI_IP_ACQUIRED,  /* Station IP address acquired */
    ESP_EVT_STA_LIST_AP,       /* Station listed APs event */
    ESP_EVT_STA_JOIN_AP,       /* Join to access point */
    ESP_EVT_STA_INFO_AP,       /* Station AP info (name, mac, channel, rssi) */
#endif                         /* ESP_CFG_MODE_STATION || __DOXYGEN__ */
#if ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__
    ESP_EVT_AP_CONNECTED_STA,    /* New station just connected to ESP's access
                                    point */
    ESP_EVT_AP_DISCONNECTED_STA, /* New station just disconnected from ESP's
                                    access point */
    ESP_EVT_AP_IP_STA,           /* New station just received IP from ESP's access point
                                  */
#endif                           /* ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__ */
#if ESP_CFG_DNS || __DOXYGEN__
    ESP_EVT_DNS_HOSTBYNAME, /* DNS domain service finished */
#endif                      /* ESP_CFG_DNS || __DOXYGEN__ */
#if ESP_CFG_PING || __DOXYGEN__
    ESP_EVT_PING, /* PING service finished */
#endif            /* ESP_CFG_PING || __DOXYGEN__ */
} espEvtType_t;

// event structure that contains necessary information and passed to callback
// function
typedef struct {
    espEvtType_t type; /* Callback type */
    union {
        struct {
            uint8_t forced; /* Set to `1` if reset is forced by user */
        } reset_detected;   /* Reset occurred. Use with ESP_EVT_RESET_DETECTED
                               event */
        struct {
            espRes_t res; /* Reset operation result */
        } reset;          /* Reset sequence finish. Use with ESP_EVT_RESET event */
        struct {
            espRes_t res; /* Restore operation result */
        } restore;        /* Restore sequence finish. Use with ESP_EVT_RESTORE event */
        struct {
            struct espConn *conn; /* Connection where data were received */
        } connDataRecv;           /* Network data received. Use with ESP_EVT_CONN_RECV
                                     event */
        struct {
            struct espConn *conn; /* Connection where data were sent */
            size_t          sent; /* Number of bytes sent on connection */
            espRes_t        res;  /* Send data result */
        } connDataSend;           /* Data send. Use with ESP_EVT_CONN_SEND event */
        struct {
            const char   *host; /* Host to use for connection */
            espPort_t     port; /* Remote port used for connection */
            espConnType_t type; /* Connection type */
            void         *arg;  /* Connection user argument */
            espRes_t      err;  /* Error value */
        } connError;            /* Client connection start error. Use with
                                   ESP_EVT_CONN_ERROR event */
        struct {
            struct espConn *conn;   /* Pointer to connection */
            uint8_t         client; /* Set to 1 if connection is/was client mode */
            uint8_t         forced; /* Set to 1 if connection action was forced (when
                                       active: 1 = CLIENT, 0 = SERVER: when closed, 1 =
                                       CMD, 0 = REMOTE) */
        } connActiveClosed;         /* Process active and closed statuses at the same
                                       time. Use with ESP_EVT_CONN_ACTIVE or
                                       ESP_EVT_CONN_CLOSED events */
        struct {
            struct espConn *conn; /* Set connection pointer */
        } connPoll;               /* Polling active connection to check for timeouts. Use with
                                     \ref ESP_EVT_CONN_POLL event */
        struct {
            espRes_t  res;  /* Status of command */
            uint8_t   en;   /* Status to enable/disable server */
            espPort_t port; /* Server port number */
        } server;           /* Server change event. Use with ESP_EVT_SERVER event */
#if ESP_CFG_MODE_STATION || __DOXYGEN__
        struct {
            espRes_t res;          /* Result of command */
            espAP_t *aps;          /* Pointer to access points structure */
            uint16_t num_ap_found; /* Number of access points found */
        } staListAP;               /* Station list access points. Use with ESP_EVT_STA_LIST_AP
                                      event */
        struct {
            espRes_t res; /* Result of command */
        } staJoinAP;      /* Join to an access point. Use with ESP_EVT_STA_JOIN_AP
                             event */
        struct {
            espStaInfoAP_t *info; /* AP info on which current station is connected to */
            espRes_t        res;  /* Result of command */
        } staInfoAP;              /* Current AP informations (only for current connected AP
                                     ?). Use with ESP_EVT_STA_INFO_AP event */
#endif
#if ESP_CFG_MODE_ACCESS_POINT || __DOXYGEN__
        struct {
            espMac_t *mac;  /* Station MAC address */
        } apConnDisconnSta; /* A new station connected or disconnected to ESP's
                               access point. Use with ESP_EVT_AP_CONNECTED_STA
                               or \ref ESP_EVT_AP_DISCONNECTED_STA events */
        struct {
            espMac_t *mac; /* Station MAC address */
            espIp_t  *ip;  /* Station IP address */
        } staIPfromAP;     /* Station got IP address from ESP's access point. Use
                              with \ref ESP_EVT_AP_IP_STA event */
#endif
#if ESP_CFG_DNS || __DOXYGEN__
        struct {
            espRes_t    res;  /* Result of command */
            const char *host; /* Host name for DNS lookup */
            espIp_t    *ip;   /* Pointer to IP result */
        } dnsHostbyname;      /* DNS domain service finished. Use with \ref
                                 ESP_EVT_DNS_HOSTBYNAME event */
#endif
#if ESP_CFG_PING || __DOXYGEN__
        struct {
            espRes_t    res;      /* Result of command */
            const char *host;     /* Host name for ping */
            uint32_t   *resptime; /* Time required for ping. Valid only if
                                     operation succedded */
        } ping;                   /* Ping finished. Use with \ref ESP_EVT_PING event */
#endif
    } body; /* main body of event structure ( defined by union { ... }, the
               content depends on event type) */
} espEvt_t;

// callback function prototype for event defined in espEvt_t structure
typedef espRes_t (*espEvtCbFn)(espEvt_t *evt);

// callback function after AT command is sent / before returning from the API
// function
typedef void (*espApiCmdCbFn)(espRes_t res, void *arg);

// callback function for memory deallocation to user-defined complex structure
typedef void (*espMemFreeStructCbFn)(void *p);

// function prototype which output string of AT commands
typedef espRes_t (*espLLvlSendFn)(void *data, size_t len, uint32_t timeout);

// function prototype which reset ESP device.
typedef espRes_t (*espLLvlRstFn)(uint8_t state);

//  Low level hardware-specific functions
typedef struct {
    espLLvlSendFn send_fn;  /*!< Callback function to transmit data */
    espLLvlRstFn  reset_fn; /*!< Reset callback function */
    struct {
        uint32_t baudrate; /*!< UART baudrate value */
        // [TODO] determine if we really need to keep the following handling
        // objects to send characters void      *handler;                     //
        // the pointer to the object that sends / receives (AT command) string
        // characters
        //                                         // to/from ESP hardware
        //                                         device, this is specified
        //                                         according to target hardware
        //                                         platform.
        // void      *dbg_printer;                 // the pointer to the object
        // that sends / receives characters for debugging purpose.
        //                                         // This is specified
        //                                         according to target hardware
        //                                         platform.
    } uart; /*!< UART communication parameters */
} espLLvl_t;

// Timeout callback function prototype
typedef void (*espTimeoutFn)(void *);

// thread function prototype, used in core function of this ESP AT library
typedef void (*espSysThreFunc)(void *params);

// Packet buffer structure
typedef struct espPbuf {
    struct espPbuf *next;        /*!< Next pbuf in chain list */
    size_t          payload_len; /*!< Length of payload */
    size_t          rd_ptr;
    espIp_t         ip;        /*!< Remote address for received IPD data */
    espPort_t       port;      /*!< Remote port for received IPD data */
    struct espConn *conn;      //   indicate the connection object associated with packet buffer
    uint8_t         chain_len; /*!< Total length of pbuf chain */
    uint8_t        *payload;   /*!< Pointer to payload memory */
} espPbuf_t;

// general buffer structure
typedef struct espBuf {
    uint8_t *buff;
    size_t   size;
} espBuf_t;

// connection structure
typedef struct espConn {
    uint16_t      num_recv_pkt; // number of received packets
    uint16_t      num_sent_pkt; // number of packets sent
    espConnType_t type;         // Connection type
    espIp_t       remote_ip;    // Remote IP address
    espPort_t     remote_port;  // Remote port number
    espPort_t     local_port;   // Local IP address
    espEvtCbFn    cb;           // Callback function for connection
    void         *arg;          // User custom argument
    uint8_t       val_id;       // Validation ID number. It is increased each time a new
                                // connection is established. It protects sending data to
                                // wrong connection in case we have data in send queue, and
                                // connection was closed and active again in between.
    espPbuf_t *pbuf;            // Linear buffer structure

    union {
        struct {
            espConnStatus_t active; // Status whether connection is active
            uint8_t         client; // Status whether connection is in client mode or
                                    // server mode
            uint8_t data_received;  // Status whether first data were received on
                                    // connection
        } flg;
    } status;
} espConn_t;

#ifdef __cplusplus
} // end of extern C statement
#endif
#endif // end of  __ESP_TYPEDEF_H
