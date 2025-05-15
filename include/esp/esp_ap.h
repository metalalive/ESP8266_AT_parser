#ifndef __ESP_AP_H
#define __ESP_AP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp/esp.h"

// Functions to manage access point (AP) on ESP device.
// In order to be able to use AP feature, ESP_CFG_MODE_ACCESS_POINT must be
// enabled.

espRes_t eESPapGetIP(
    espIp_t *ip, espIp_t *gw, espIp_t *nm, const espApiCmdCbFn cb, void *const cb_arg,
    const uint32_t blocking
);
espRes_t eESPapSetIP(
    const espIp_t *ip, const espIp_t *gw, const espIp_t *nm, uint8_t saveDef,
    const espApiCmdCbFn cb, void *const cb_arg, const uint32_t blocking
);
espRes_t
eESPapGetMAC(espMac_t *mac, const espApiCmdCbFn cb, void *const cb_arg, const uint32_t blocking);
espRes_t eESPapSetMAC(
    const espMac_t *mac, uint8_t saveDef, const espApiCmdCbFn cb, void *const cb_arg,
    const uint32_t blocking
);

espRes_t eESPapConfigure(
    const char *ssid, const char *pwd, uint8_t ch, espEncrypt_t ecn, uint8_t max_sta, uint8_t hid,
    uint8_t def, const espApiCmdCbFn cb, void *const cb_arg, const uint32_t blocking
);

espRes_t eESPapListSta(
    espSta_t *sta, size_t stal, size_t *staf, const espApiCmdCbFn cb, void *const cb_arg,
    const uint32_t blocking
);

#ifdef __cplusplus
}
#endif
#endif /* __ESP_AP_H */
