#ifndef __ESP_CONFIG_DEFAULT_H
#define __ESP_CONFIG_DEFAULT_H

#ifndef ESP_CFG_DEV_ESP01
#define ESP_CFG_DEV_ESP01 0
#endif // end of ESP_CFG_DEV_ESP01

#ifndef ESP_CFG_DEV_ESP12
#define ESP_CFG_DEV_ESP12 0
#endif // end of ESP_CFG_DEV_ESP12

// address alignment for dynamic memory allocation
// Some CPUs can work faster with the aligned address, e.g. 4-bytes, 8-bytes
// aligned address. The following value must be power of 2.
#ifndef ESP_CFG_MEM_ALIGNMENT
#define ESP_CFG_MEM_ALIGNMENT 4
#endif // end of ESP_CFG_MEM_ALIGNMENT

// maximum number of connections this AT-command library can support on ESP
// device, for many of ESP8266 devices (e.g. ESP-01) , the maximum number of
// supported TCP connections is 5, so we set the default value to following
// macro.
#ifndef ESP_CFG_MAX_CONNS
#define ESP_CFG_MAX_CONNS 5
#endif // end of ESP_CFG_MAX_CONNS

// maximum number of bytes we can send at a single AT commnad, due to limitation
// of ESP8266 device, the size cannot exceed 64 bytes.
#ifndef ESP_CFG_MAX_AT_CMD_SIZE
#define ESP_CFG_MAX_AT_CMD_SIZE 0x40
#endif // end of ESP_CFG_MAX_AT_CMD_SIZE

// default baudrate for the ESP device
#ifndef ESP_CFG_BAUDRATE
#define ESP_CFG_BAUDRATE 115200
#endif // end of ESP_CFG_BAUDRATE

// enable / disable station mode in the AT library. For any ESP device in
// station mode, it can connect to other access point(s) found.
#ifndef ESP_CFG_MODE_STATION
#define ESP_CFG_MODE_STATION 1
#endif // end of ESP_CFG_MODE_STATION

// enable / disable access-point mode in the AT library. For any ESP device
// acting as access point, it can accept connections from  other station(s).
#ifndef ESP_CFG_MODE_ACCESS_POINT
#define ESP_CFG_MODE_ACCESS_POINT 1
#endif // end of ESP_CFG_MODE_ACCESS_POINT

// enable / disable reset command sequence after esp_init() call.
// When this function is disabled, user must manually call esp_reset() to send
// reset command sequence to ESP device.
#ifndef ESP_CFG_RST_ON_INIT
#define ESP_CFG_RST_ON_INIT 1
#endif

// enable / disable restoring factory settings after esp_init() call.
// When this function is enabled, it will automatically clear the current
// setting stored on the ESP device, and load factory settings to the device.
// [Note]
// Take great care while using this function in applications.
#ifndef ESP_CFG_RESTORE_ON_INIT
#define ESP_CFG_RESTORE_ON_INIT 0
#endif

// enable / disable reset sequence device after esp_device_set_present() call
#ifndef ESP_CFG_RST_ON_DEV_PRESENT
#define ESP_CFG_RST_ON_DEV_PRESENT 1
#endif

// Default delay time (in milliseconds) before sending first AT command on reset
// sequence
#ifndef ESP_CFG_RST_DLY_DEFAULT
#define ESP_CFG_RST_DLY_DEFAULT 500
#endif

// maximum number of characters for SSID
#ifndef ESP_CFG_MAX_SSID_LEN
#define ESP_CFG_MAX_SSID_LEN 21
#endif // end of ESP_CFG_MAX_SSID_LEN

// turn on/off AT echo mode, the mode is useful when debugging ESP communication
#ifndef ESP_CFG_AT_ECHO
#define ESP_CFG_AT_ECHO 0
#endif

// Maximum number of items stored in a message box for
// AT-command-request-handling thread, [Note] Message queue is used for storing
// address to AT-command data.
#ifndef ESP_CFG_AT_CMD_REQ_MBOX_SIZE
#define ESP_CFG_AT_CMD_REQ_MBOX_SIZE 16
#endif

// Maximum number of items stored in a message queue for
// AT-command-response-handling thread,
#ifndef ESP_CFG_AT_CMD_RESP_MBOX_SIZE
#define ESP_CFG_AT_CMD_RESP_MBOX_SIZE 16
#endif

// Enables or disables receive timeout feature
// When this option is enabled, user will get an option to set timeout value for
// receive data on netconn, before function returns timeout error.
//
// [NOTE]  Even if this option is enabled, user must still manually set timeout,
//         by default time will be set to 0 which means no timeout.
#ifndef ESP_CFG_NETCONN_RECV_TIMEOUT
#define ESP_CFG_NETCONN_RECV_TIMEOUT 1
#endif

//  queue length for apccepting number of new client connections used in netconn
//  server , it represents number of maximal clients waiting in accept queue of
//  server connection
#ifndef ESP_CFG_NETCONN_ACCEPT_Q_LEN
#define ESP_CFG_NETCONN_ACCEPT_Q_LEN ESP_CFG_MAX_CONNS
#endif

//  queue length for incoming data (received on Rx of ESP device), packed within
//  pbuf structure. Defines maximal number of pbuf data packet references for
//  receive
#ifndef ESP_CFG_NETCONN_RECV_Q_LEN
#define ESP_CFG_NETCONN_RECV_Q_LEN 8
#endif

// Enables / disables support for DNS functions
#ifndef ESP_CFG_DNS
#define ESP_CFG_DNS 0
#endif

// Enables / disables support for ping functions
#ifndef ESP_CFG_PING
#define ESP_CFG_PING 0
#endif

// Enables or disables support for WPS functions
#ifndef ESP_CFG_WPS
#define ESP_CFG_WPS 0
#endif

// Enables or disables support for SNTP protocol with AT commands
#ifndef ESP_CFG_SNTP
#define ESP_CFG_SNTP 0
#endif

// Enables or disables support for mDNS
#ifndef ESP_CFG_MDNS
#define ESP_CFG_MDNS 0
#endif

// Enables or disables support for SNTP protocol with AT commands
#ifndef ESP_CFG_HOSTNAME
#define ESP_CFG_HOSTNAME 0
#endif

// interval time (in millisecond)  to call poll event on active connection
#ifndef ESP_CFG_CONN_POLL_INTERVAL
#define ESP_CFG_CONN_POLL_INTERVAL 500
#endif

// due to speed limits of ESP8266, users must set time interval (in
// milliseconds) between 2 packets, users can also adjust this parameter with
// respect to different ESP devices they use.
#ifndef ESP_CFG_SEND_PKT_INTERVAL_MS
#define ESP_CFG_SEND_PKT_INTERVAL_MS 2000
#endif

// maximum transfer size of each AT+CIPSEND is 2048 bytes in ESP8266 device, or
// it can be smaller for experiment
#ifndef ESP_CFG_MAX_BYTES_PER_CIPSEND
#define ESP_CFG_MAX_BYTES_PER_CIPSEND 512
#endif

#ifndef ESP_CFG_CMD_BLOCK_TIME_CWLAP
#define ESP_CFG_CMD_BLOCK_TIME_CWLAP 20000
#endif

#ifndef ESP_CFG_CMD_BLOCK_TIME_CWJAP
#define ESP_CFG_CMD_BLOCK_TIME_CWJAP 20000
#endif

#ifndef ESP_CFG_CMD_BLOCK_TIME_CWQAP
#define ESP_CFG_CMD_BLOCK_TIME_CWQAP 5000
#endif

#ifndef ESP_CFG_CMD_BLOCK_TIME_CIPSTART
#define ESP_CFG_CMD_BLOCK_TIME_CIPSTART 20000
#endif

#ifndef ESP_CFG_CMD_BLOCK_TIME_CIPCLOSE
#define ESP_CFG_CMD_BLOCK_TIME_CIPCLOSE 5000
#endif

#ifndef ESP_CFG_CMD_BLOCK_TIME_CIPSEND
#define ESP_CFG_CMD_BLOCK_TIME_CIPSEND 25000
#endif

//  [TODO] figure out behaviour of entire TCP transacion process
//
//  Normally ESP automatically sends received TCP data to host device
//  in async mode. When host device is slow or if there is memory constrain,
//  it may happen that processing cannot handle all received data.
//
//  When feature is enabled, ESP will notify host device about new data
//  available for read and then user may start read process
//
//  [note] This feature is only available for `TCP` connections.
#ifndef ESP_CFG_CONN_MANUAL_TCP_RECV
#define ESP_CFG_CONN_MANUAL_TCP_RECV 0
#endif

// Memory copy function declaration, users can specify other memory copy
// function in case they implement their own function for specific hardware
// architecture. (e.g. make use of hardware acceleration to speed up memory copy
// operations)
#ifndef ESP_MEMCPY
#define ESP_MEMCPY(dst, src, len) memcpy(dst, src, len)
#endif

// Memory set function declaration
#ifndef ESP_MEMSET
#define ESP_MEMSET(dst, b, len) memset(dst, b, len)
#endif

// Memory allocate function declaration
#ifndef ESP_MALLOC
#define ESP_MALLOC(sizebytes) malloc((size_t)(sizebytes))
#endif

#ifndef ESP_CALLOC
#define ESP_CALLOC calloc((nmemb), (size))
#endif

#ifndef ESP_REALLOC
#define ESP_REALLOC realloc((mem), (newsize))
#endif

#ifndef ESP_MEMFREE
#define ESP_MEMFREE(mptr) free((void *)(mptr))
#endif

#ifndef ESP_STRLEN
#define ESP_STRLEN(src) strlen(src)
#endif

#ifndef ESP_STRNCMP
#define ESP_STRNCMP(str1, str2, sz) strncmp((const char *)(str1), (const char *)(str2), (sz))
#endif

#define ESP_MIN_AT_VERSION_MAJOR_ESP8266 1
#define ESP_MIN_AT_VERSION_MINOR_ESP8266 6
#define ESP_MIN_AT_VERSION_PATCH_ESP8266 0

// define default mode of ESP device
#define ESP_CFG_MODE_STATION_ACCESS_POINT (ESP_CFG_MODE_STATION && ESP_CFG_MODE_ACCESS_POINT)

// At least one of them must be enabled
#if (!ESP_CFG_MODE_STATION && !ESP_CFG_MODE_ACCESS_POINT)
#error                                                                                             \
    "Invalid ESP configuration. ESP_CFG_MODE_STATION and ESP_CFG_MODE_STATION cannot be disabled at the same time!"
#endif

// WPS config
#if (ESP_CFG_WPS && !ESP_CFG_MODE_STATION)
#error "WPS function should be enabled in station mode"
#endif

// one of the ESP8266 device must be selected (defined) in user application
#if (!ESP_CFG_DEV_ESP01 && !ESP_CFG_DEV_ESP12)
#error                                                                                             \
    "Invalid ESP configuration. At least one of the ESP8266 device must be selected (defined) in user application "
#endif

#endif // end of __ESP_CONFIG_DEFAULT_H
