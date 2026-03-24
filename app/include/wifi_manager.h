/**
 * @file wifi_manager.h
 * @brief WiFi connection manager with persistent credentials
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

#define WIFI_SSID_MAX_LEN   32
#define WIFI_PSK_MAX_LEN    64

/**
 * @brief WiFi manager connection states
 * Prefixed with MGR_ to avoid collision with Zephyr's wifi_iface_state enum
 */
enum wifi_manager_state {
    MGR_WIFI_DISCONNECTED,
    MGR_WIFI_CONNECTING,
    MGR_WIFI_CONNECTED,
    MGR_WIFI_ERROR,
};

int  wifi_manager_init(void);
int  wifi_manager_connect(void);
void wifi_manager_disconnect(void);
int  wifi_manager_set_credentials(const char *ssid, const char *psk);
enum wifi_manager_state wifi_manager_get_state(void);
bool wifi_manager_is_connected(void);

#endif /* WIFI_MANAGER_H */
