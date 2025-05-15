#ifndef __ESP_WPS_H
#define __ESP_WPS_H

#ifdef __cplusplus
extern "C" {
#endif

// Wireless Protected Setup function on ESP device

espRes_t
eESPwpsCfg(uint8_t en, const espApiCmdCbFn cb, void *const evt_arg, const uint32_t blocking);

#ifdef __cplusplus
}
#endif
#endif /* __ESP_WPS_H */
