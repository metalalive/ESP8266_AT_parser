#ifndef __ESP_HOSTNAME_H
#define __ESP_HOSTNAME_H

#ifdef __cplusplus
extern "C" {
#endif

espRes_t eESPhostnameSet(
    const char *hostname, const espApiCmdCbFn cb, void *const evt_arg, const uint32_t blocking
);

espRes_t eESPhostnameGet(
    char *hostname, size_t length, const espApiCmdCbFn cb, void *const evt_arg,
    const uint32_t blocking
);

#ifdef __cplusplus
}
#endif

#endif // end of  __ESP_HOSTNAME_H
