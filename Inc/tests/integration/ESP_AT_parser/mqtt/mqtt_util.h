#ifndef __INTEGRATION_ESP_AT_SW_MQTT_UTIL_H
#define __INTEGRATION_ESP_AT_SW_MQTT_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

// indicate number of bytes used in 4-bytes integer
#define MQTT_DSIZE_INT  4

// indicate number of bytes to represent number of characters in a UTF-8 string
#define MQTT_DSIZE_STR_LEN  2

// In MQTT specification, remaining length is stored as variable bytes
// in a MQTT packet, it can be at most 4 bytes.
#define  MQTT_PKT_MAX_BYTES_REMAIN_LEN   4

#define  MQTT_DEFAULT_KEEPALIVE_SEC      60 



// ------- topic naming rule --------
// use English letters / numbers for each level of topic string, forward slashes for levels separator.
// Do not start name with forward slash (/) or $ (reserved for broker)
// Example: "register/event/evt_id" */
/* The forward slash is used to define levels of topic matching */
#define MQTT_TOPIC_LEVEL_SEPERATOR   '/'

// available for Topic Filters on Subscribe only,  used to match on a single level */
// Example: "userid/home/+/cam/yesterday" 
#define MQTT_TOPIC_LEVEL_SINGLE      '+'

// used to match on a multiple levels
// Example: "userid/home/#" 
#define MQTT_TOPIC_LEVEL_MULTI       '#'




// Get/Set packet types : located in first byte of fixed header in bits 4-7 
#define MQTT_CTRL_PKT_TYPE_GET(x)  (((x) >> 4) & 0xF)
#define MQTT_CTRL_PKT_TYPE_SET(x)  (((x) & 0xF) << 4)
#define MQTT_CTRL_PKT_FLGS_GET(x)  ((x) & 0xF)
#define MQTT_CTRL_PKT_FLGS_SET(x)  (x)




/* GCC 7 has new switch() fall-through detection */
/* default to FALL_THROUGH stub */
#ifndef FALL_THROUGH
#define FALL_THROUGH

#if defined(__GNUC__)
    #if ((__GNUC__ > 7) || ((__GNUC__ == 7) && (__GNUC_MINOR__ >= 1)))
        #undef  FALL_THROUGH
        #define FALL_THROUGH __attribute__ ((fallthrough));
    #endif
#endif
#endif



#ifdef __cplusplus
}
#endif
#endif // end of  __INTEGRATION_ESP_AT_SW_MQTT_UTIL_H


