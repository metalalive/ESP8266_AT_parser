#ifndef __ESP_PING_H
#define __ESP_PING_H

#ifdef __cplusplus
extern "C" {
#endif

// Ping server and get response time
espRes_t eESPping(
    const char *host, uint16_t host_len, uint32_t *resptime, const espApiCmdCbFn cb,
    void *const cb_arg, const uint32_t blocking
);

#ifdef __cplusplus
}
#endif
#endif /* __ESP_PING_H */
