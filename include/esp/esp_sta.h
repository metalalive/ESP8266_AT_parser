#ifndef __ESP_STA_H
#define __ESP_STA_H

#ifdef __cplusplus
extern "C" {
#endif

// ESP AT library, station API

espRes_t eESPstaListAP(
    const char *ssid, uint16_t ssid_len, espAP_t *aps, uint16_t apslen, uint16_t *num_ap_found,
    const espApiCmdCbFn cb, void *const cb_arg, const uint32_t blocking
);
espRes_t eESPstaJoin(
    const char *ssid, uint16_t ssid_len, const char *pass, uint16_t pass_len, const espMac_t *mac,
    uint8_t saveDef, const espApiCmdCbFn cb, void *const cb_arg, const uint32_t blocking
);
espRes_t eESPstaQuit(const espApiCmdCbFn cb, void *const cb_arg, const uint32_t blocking);

espRes_t
eESPstaAutojoin(uint8_t en, const espApiCmdCbFn cb, void *const cb_arg, const uint32_t blocking);
espRes_t eESPstaGetIP(
    espIp_t *ip, espIp_t *gw, espIp_t *nm, uint8_t saveDef, const espApiCmdCbFn cb,
    void *const cb_arg, const uint32_t blocking
);
espRes_t eESPstaSetIP(
    const espIp_t *ip, const espIp_t *gw, const espIp_t *nm, uint8_t saveDef,
    const espApiCmdCbFn cb, void *const cb_arg, const uint32_t blocking
);
espRes_t eESPstaGetMAC(
    espMac_t *mac, uint8_t def, const espApiCmdCbFn cb, void *const cb_arg, const uint32_t blocking
);
espRes_t eESPstaSetMAC(
    const espMac_t *mac, uint8_t def, const espApiCmdCbFn cb, void *const cb_arg,
    const uint32_t blocking
);

espRes_t eESPstaCopyIP(espIp_t *ip, espIp_t *gw, espIp_t *nm);
espRes_t eESPstaGetAPinfo(
    espStaInfoAP_t *info, const espApiCmdCbFn cb, void *const cb_arg, const uint32_t blocking
);

espRes_t eESPstaHasIP(void);
espRes_t eESPstaIsJoined(void);

// uint8_t    ucESPstaIsAP_802_11b(espAP_t* ap);
// uint8_t    ucESPstaIsAP_802_11g(espAP_t* ap);
// uint8_t    ucESPstaIsAP_802_11n(espAP_t* ap);

#ifdef __cplusplus
}
#endif

#endif /* __ESP_STA_H */
