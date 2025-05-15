#ifndef __ESP_DNS_H
#define __ESP_DNS_H

#ifdef __cplusplus
extern "C" {
#endif

// Domain name server

espRes_t eESPdnsGetHostByName(
    const char *host, espIp_t *const ip, const espApiCmdCbFn cb, void *const evt_arg,
    const uint32_t blocking
);

espRes_t eESPdnsSetCfg(
    uint8_t en, const char *s1, const char *s2, uint8_t def, const espApiCmdCbFn cb,
    void *const evt_arg, const uint32_t blocking
);

#ifdef __cplusplus
}
#endif
#endif /* __ESP_DNS_H */
