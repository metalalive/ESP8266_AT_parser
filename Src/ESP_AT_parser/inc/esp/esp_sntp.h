#ifndef __ESP_SNTP_H
#define __ESP_SNTP_H

#ifdef __cplusplus
extern "C" {
#endif


// Simple network time protocol supported by AT commands

espRes_t      eESPsntpCfg( uint8_t en, int8_t tz, const char* h1, const char* h2, const char* h3,
                           const espApiCmdCbFn cb, void* const evt_arg, const uint32_t blocking);

espRes_t      eESPsntpGetTime( espDatetime_t* dt, const espApiCmdCbFn cb, 
                               void* const evt_arg, const uint32_t blocking );


#ifdef __cplusplus
}
#endif
#endif // __ESP_SNTP_H
