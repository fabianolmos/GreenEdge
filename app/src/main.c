#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "sensors.h"
#include "sensor_thread.h"
#include "wifi_manager.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("GreenEdge starting");

    sensor_thread_start();

        /* Initialize WiFi manager (loads credentials from flash) */
    int rc = wifi_manager_init();
    if (rc) {
        LOG_WRN("WiFi manager init failed: %d", rc);
    } else {
        /* Auto-connect if credentials exist */
        wifi_manager_connect();
    }

    return 0;
}

