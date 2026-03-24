/**
 * @file wifi_shell_cmds.c
 * @brief WiFi shell commands
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/wifi_mgmt.h>
#include "wifi_manager.h"

/* ------------------------------------------------------------------ */
/* Scan result handler                                                  */
/* ------------------------------------------------------------------ */

static struct net_mgmt_event_callback scan_cb;
static bool scan_in_progress = false;
static const struct shell *scan_shell_ref = NULL;

static void handle_scan_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_scan_result *entry =
        (const struct wifi_scan_result *)cb->info;

    if (!scan_shell_ref) {
        return;
    }

    if (entry->ssid_length == 0) {
        /* Empty SSID = scan complete sentinel in some drivers */
        return;
    }

    /* Security type label */
    const char *sec;
    switch (entry->security) {
    case WIFI_SECURITY_TYPE_NONE:    sec = "OPEN";     break;
    case WIFI_SECURITY_TYPE_PSK:     sec = "WPA2-PSK"; break;
    case WIFI_SECURITY_TYPE_PSK_SHA256: sec = "WPA2-PSK-SHA256"; break;
    case WIFI_SECURITY_TYPE_SAE:     sec = "WPA3-SAE"; break;
    default:                         sec = "UNKNOWN";  break;
    }

    /* Band label */
    const char *band;
    switch (entry->band) {
    case WIFI_FREQ_BAND_2_4_GHZ: band = "2.4G"; break;
    case WIFI_FREQ_BAND_5_GHZ:   band = "5G";   break;
    case WIFI_FREQ_BAND_6_GHZ:   band = "6G";   break;
    default:                     band = "?";    break;
    }

    shell_print(scan_shell_ref,
                "  %-32.*s  RSSI: %4d dBm  CH: %3d  %-4s  %s",
                entry->ssid_length, entry->ssid,
                entry->rssi,
                entry->channel,
                band,
                sec);
}

static void handle_scan_done(struct net_mgmt_event_callback *cb)
{
    ARG_UNUSED(cb);

    if (scan_shell_ref) {
        shell_print(scan_shell_ref, "Scan complete.");
    }

    scan_in_progress = false;
    scan_shell_ref   = NULL;
}

static void scan_event_handler(struct net_mgmt_event_callback *cb,
                                uint64_t mgmt_event,
                                struct net_if *iface)
{
    ARG_UNUSED(iface);

    switch (mgmt_event) {
    case NET_EVENT_WIFI_SCAN_RESULT:
        handle_scan_result(cb);
        break;
    case NET_EVENT_WIFI_SCAN_DONE:
        handle_scan_done(cb);
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Shell commands                                                       */
/* ------------------------------------------------------------------ */

/**
 * Usage: wifi scan
 */
static int cmd_wifi_scan(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    if (scan_in_progress) {
        shell_warn(sh, "Scan already in progress, please wait...");
        return -EBUSY;
    }

    struct net_if *iface = net_if_get_default();
    if (!iface) {
        shell_error(sh, "No network interface found");
        return -ENODEV;
    }

    /* Register scan callbacks (lazy init — only once) */
    static bool scan_cb_registered = false;
    if (!scan_cb_registered) {
        net_mgmt_init_event_callback(&scan_cb, scan_event_handler,
                                      NET_EVENT_WIFI_SCAN_RESULT |
                                      NET_EVENT_WIFI_SCAN_DONE);
        net_mgmt_add_event_callback(&scan_cb);
        scan_cb_registered = true;
    }

    scan_in_progress = true;
    scan_shell_ref   = sh;

    shell_print(sh, "Scanning... (results appear below)");
    shell_print(sh, "  %-32s  %-14s  %-6s  %-4s  %s",
                "SSID", "RSSI", "CH", "BAND", "SECURITY");
    shell_print(sh, "  %s", "----------------------------------------------------------------");

    int rc = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);
    if (rc) {
        shell_error(sh, "Scan request failed: %d", rc);
        scan_in_progress = false;
        scan_shell_ref   = NULL;
        return rc;
    }

    return 0;
}

/**
 * Usage: wifi set <ssid> [password]
 */
static int cmd_wifi_set(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_error(sh, "Usage: wifi set <ssid> [password]");
        return -EINVAL;
    }

    int rc = wifi_manager_set_credentials(argv[1], (argc > 2) ? argv[2] : NULL);
    if (rc) {
        shell_error(sh, "Failed to save credentials: %d", rc);
        return rc;
    }

    shell_print(sh, "Credentials saved. Run 'wifi connect' to connect.");
    return 0;
}

/**
 * Usage: wifi connect
 */
static int cmd_wifi_connect(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int rc = wifi_manager_connect();
    if (rc == -ENOENT) {
        shell_error(sh, "No credentials stored. Use: wifi set <ssid> [password]");
        return rc;
    }
    if (rc) {
        shell_error(sh, "Connection failed: %d", rc);
        return rc;
    }

    shell_print(sh, "Connecting... (check logs for IP address)");
    return 0;
}

/**
 * Usage: wifi disconnect
 */
static int cmd_wifi_disconnect(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    wifi_manager_disconnect();
    shell_print(sh, "Disconnected");
    return 0;
}

/**
 * Usage: wifi status
 */
static int cmd_wifi_status(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    static const char * const state_str[] = {
        "DISCONNECTED",
        "CONNECTING",
        "CONNECTED",
        "ERROR",
    };

    shell_print(sh, "=== WiFi Status ===");
    shell_print(sh, "State: %s", state_str[wifi_manager_get_state()]);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Command registration                                                 */
/* ------------------------------------------------------------------ */

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_cmds,
    SHELL_CMD(scan,       NULL, "Scan for nearby networks",
              cmd_wifi_scan),
    SHELL_CMD_ARG(set,    NULL, "Save credentials: wifi set <ssid> [pass]",
                  cmd_wifi_set, 2, 1),
    SHELL_CMD(connect,    NULL, "Connect using stored credentials",
              cmd_wifi_connect),
    SHELL_CMD(disconnect, NULL, "Disconnect from WiFi",
              cmd_wifi_disconnect),
    SHELL_CMD(status,     NULL, "Show connection status",
              cmd_wifi_status),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(wifi, &wifi_cmds, "WiFi management", NULL);
