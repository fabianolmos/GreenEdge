/**
 * @file wifi_manager.c
 * @brief WiFi manager implementation
 *
 * Credentials stored under settings subtree "wifi":
 *   wifi/ssid  -> SSID string
 *   wifi/psk   -> Password string
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/settings/settings.h>
#include <string.h>
#include <errno.h>

#include "wifi_manager.h"

LOG_MODULE_REGISTER(wifi_manager, LOG_LEVEL_INF);

/* ------------------------------------------------------------------ */
/* Private state                                                        */
/* ------------------------------------------------------------------ */

static char stored_ssid[WIFI_SSID_MAX_LEN + 1];
static char stored_psk[WIFI_PSK_MAX_LEN + 1];

static enum wifi_manager_state state = MGR_WIFI_DISCONNECTED;
static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

static struct k_work_delayable reconnect_work;

/* ------------------------------------------------------------------ */
/* Settings backend                                                     */
/* ------------------------------------------------------------------ */

static int wifi_settings_set(const char *name, size_t len,
                              settings_read_cb read_cb, void *cb_arg)
{
    if (strcmp(name, "ssid") == 0) {
        if (len > WIFI_SSID_MAX_LEN) {
            LOG_WRN("Stored SSID too long, ignoring");
            return -EINVAL;
        }
        ssize_t rc = read_cb(cb_arg, stored_ssid, len);
        if (rc >= 0) {
            stored_ssid[rc] = '\0';
            LOG_INF("Loaded SSID: %s", stored_ssid);
        }
        return (rc >= 0) ? 0 : (int)rc;
    }

    if (strcmp(name, "psk") == 0) {
        if (len > WIFI_PSK_MAX_LEN) {
            LOG_WRN("Stored PSK too long, ignoring");
            return -EINVAL;
        }
        ssize_t rc = read_cb(cb_arg, stored_psk, len);
        if (rc >= 0) {
            stored_psk[rc] = '\0';
            LOG_DBG("Loaded PSK (len=%d)", (int)rc);
        }
        return (rc >= 0) ? 0 : (int)rc;
    }

    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(wifi, "wifi", NULL, wifi_settings_set, NULL, NULL);

/* ------------------------------------------------------------------ */
/* Network event handlers                                               */
/* ------------------------------------------------------------------ */

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (status->conn_status == WIFI_STATUS_CONN_SUCCESS) {
        LOG_INF("WiFi connected to: %s", stored_ssid);
        state = MGR_WIFI_CONNECTED;
    } else {
        LOG_WRN("WiFi connection failed (status=%d), retrying in 5s...",
                status->conn_status);
        state = MGR_WIFI_ERROR;
        k_work_schedule(&reconnect_work, K_SECONDS(5));
    }
}

static void handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (state == MGR_WIFI_CONNECTED) {
        /* disconn_reason is the correct field name in Zephyr v4.x */
        LOG_WRN("WiFi disconnected (reason=%d), reconnecting in 5s...",
                status->disconn_reason);
        state = MGR_WIFI_DISCONNECTED;
        k_work_schedule(&reconnect_work, K_SECONDS(5));
    } else {
        LOG_INF("WiFi disconnected");
        state = MGR_WIFI_DISCONNECTED;
    }
}

static void handle_ipv4_addr(struct net_mgmt_event_callback *cb)
{
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        return;
    }

    struct net_if_ipv4 *ipv4 = iface->config.ip.ipv4;
    if (!ipv4) {
        return;
    }

    for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
        if (ipv4->unicast[i].ipv4.is_used) {
            char buf[NET_IPV4_ADDR_LEN];
            LOG_INF("IP address: %s",
                    net_addr_ntop(AF_INET,
                                  &ipv4->unicast[i].ipv4.address.in_addr,
                                  buf, sizeof(buf)));
            break;
        }
    }
}

/*
 * mgmt_event is uint64_t in Zephyr v4.x (net_mgmt_event_handler_t signature).
 * Using uint32_t caused incompatible-pointer-types warnings/errors.
 */
static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                                uint64_t mgmt_event,
                                struct net_if *iface)
{
    ARG_UNUSED(iface);

    switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT:
        handle_wifi_connect_result(cb);
        break;
    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        handle_wifi_disconnect_result(cb);
        break;
    default:
        break;
    }
}

static void ipv4_event_handler(struct net_mgmt_event_callback *cb,
                                uint64_t mgmt_event,
                                struct net_if *iface)
{
    ARG_UNUSED(iface);

    if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        handle_ipv4_addr(cb);
    }
}

/* ------------------------------------------------------------------ */
/* Reconnect work handler                                               */
/* ------------------------------------------------------------------ */

static void reconnect_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    LOG_INF("Attempting reconnect...");
    wifi_manager_connect();
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

int wifi_manager_init(void)
{
    int rc;

    k_work_init_delayable(&reconnect_work, reconnect_work_handler);

    rc = settings_subsys_init();
    if (rc) {
        LOG_ERR("settings_subsys_init failed: %d", rc);
        return rc;
    }

    rc = settings_load_subtree("wifi");
    if (rc) {
        LOG_WRN("No WiFi settings found (first boot?)");
    }

    net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler,
                                  NET_EVENT_WIFI_CONNECT_RESULT |
                                  NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);

    net_mgmt_init_event_callback(&ipv4_cb, ipv4_event_handler,
                                  NET_EVENT_IPV4_ADDR_ADD);
    net_mgmt_add_event_callback(&ipv4_cb);

    LOG_INF("WiFi manager initialized");
    return 0;
}

int wifi_manager_connect(void)
{
    if (strlen(stored_ssid) == 0) {
        LOG_ERR("No SSID configured. Use: wifi set <ssid> <password>");
        return -ENOENT;
    }

    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("No network interface found");
        return -ENODEV;
    }

    struct wifi_connect_req_params params = {
        .ssid        = (const uint8_t *)stored_ssid,
        .ssid_length = strlen(stored_ssid),
        .psk         = (const uint8_t *)stored_psk,
        .psk_length  = strlen(stored_psk),
        .channel     = WIFI_CHANNEL_ANY,
        .security    = (strlen(stored_psk) > 0)
                       ? WIFI_SECURITY_TYPE_PSK
                       : WIFI_SECURITY_TYPE_NONE,
        .band        = WIFI_FREQ_BAND_2_4_GHZ,
        .mfp         = WIFI_MFP_OPTIONAL,
    };

    state = MGR_WIFI_CONNECTING;
    LOG_INF("Connecting to: %s", stored_ssid);

    int rc = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
    if (rc) {
        LOG_ERR("Connection request failed: %d", rc);
        state = MGR_WIFI_ERROR;
        return rc;
    }

    return 0;
}

void wifi_manager_disconnect(void)
{
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        return;
    }

    k_work_cancel_delayable(&reconnect_work);
    state = MGR_WIFI_DISCONNECTED;
    net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
}

int wifi_manager_set_credentials(const char *ssid, const char *psk)
{
    if (!ssid || strlen(ssid) == 0) {
        return -EINVAL;
    }
    if (strlen(ssid) > WIFI_SSID_MAX_LEN) {
        LOG_ERR("SSID too long (max %d chars)", WIFI_SSID_MAX_LEN);
        return -EINVAL;
    }
    if (psk && strlen(psk) > WIFI_PSK_MAX_LEN) {
        LOG_ERR("PSK too long (max %d chars)", WIFI_PSK_MAX_LEN);
        return -EINVAL;
    }

    strncpy(stored_ssid, ssid, WIFI_SSID_MAX_LEN);
    stored_ssid[WIFI_SSID_MAX_LEN] = '\0';

    if (psk) {
        strncpy(stored_psk, psk, WIFI_PSK_MAX_LEN);
        stored_psk[WIFI_PSK_MAX_LEN] = '\0';
    } else {
        stored_psk[0] = '\0';
    }

    int rc = settings_save_one("wifi/ssid", stored_ssid, strlen(stored_ssid));
    if (rc) {
        LOG_ERR("Failed to save SSID: %d", rc);
        return rc;
    }

    rc = settings_save_one("wifi/psk", stored_psk, strlen(stored_psk));
    if (rc) {
        LOG_ERR("Failed to save PSK: %d", rc);
        return rc;
    }

    LOG_INF("Credentials saved for: %s", stored_ssid);
    return 0;
}

enum wifi_manager_state wifi_manager_get_state(void)
{
    return state;
}

bool wifi_manager_is_connected(void)
{
    return state == MGR_WIFI_CONNECTED;
}
