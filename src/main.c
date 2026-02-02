#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "sensors.h"
#include "sensor_thread.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("GreenEdge starting");

    sensor_thread_start();

    return 0;
}

