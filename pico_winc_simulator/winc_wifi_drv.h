#ifndef WINC_WIFI_DRV_H
#define WINC_WIFI_DRV_H

#include <stdint.h>
#include <stdbool.h>

// Scan result structure (simplified)
typedef struct {
    char ssid[33];
    uint8_t bssid[6];
    int16_t rssi;
    uint8_t channel;
    uint8_t auth_type;
    bool valid;
} winc_drv_scan_result_t;

// Driver API
void winc_drv_init(void);
void winc_drv_poll(void);

// Wi-Fi Control
int winc_drv_scan_start(void);
bool winc_drv_scan_active(void);
int winc_drv_scan_get_results(int index, winc_drv_scan_result_t *result);
int winc_drv_connect(const char *ssid, const char *password, uint8_t auth_type);
void winc_drv_get_ip_config(uint32_t *ip, uint32_t *gw, uint32_t *mask);

// Socket API
#define WINC_SOCK_TCP 1
#define WINC_SOCK_UDP 2

int winc_drv_sock_create(int type);
int winc_drv_sock_connect(int sock_id, uint32_t ip, uint16_t port);
int winc_drv_sock_send(int sock_id, const uint8_t *data, uint16_t len);
int winc_drv_sock_recv(int sock_id, uint8_t *buf, uint16_t max_len);
void winc_drv_sock_close(int sock_id);
int winc_drv_resolve_dns(const char *hostname, uint32_t *ip);

#endif // WINC_WIFI_DRV_H
