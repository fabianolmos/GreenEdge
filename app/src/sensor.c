/**
 * @file sensor.c
 * @brief Sensor data management implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "sensors.h"

LOG_MODULE_REGISTER(sensors, LOG_LEVEL_DBG);

/* Private data - only accessed through public API */
static struct sensor_data sensors;
static struct k_mutex sensors_mutex;

void sensors_init(void)
{
    k_mutex_init(&sensors_mutex);

    /* Initialize with default values */
    k_mutex_lock(&sensors_mutex, K_FOREVER);
    sensors.temperature = 20;
    sensors.humidity = 50;
    k_mutex_unlock(&sensors_mutex);

    LOG_INF("Sensors initialized (T=%d, H=%d)", 
            sensors.temperature, sensors.humidity);
}

void sensors_read(void)
{
    /* Simulate sensor reading with incremental values */
    sensors.temperature += 1;
    sensors.humidity += 2;

    /* Wrap around to simulate realistic sensor behavior */
    if (sensors.temperature > 30) {
        sensors.temperature = 20;
    }

    if (sensors.humidity > 80) {
        sensors.humidity = 50;
    }

    LOG_DBG("Sensor read: T=%d, H=%d", sensors.temperature, sensors.humidity);
}

void sensors_get(struct sensor_data *data)
{
    if (data == NULL) {
        LOG_ERR("NULL pointer passed to sensors_get");
        return;
    }

    k_mutex_lock(&sensors_mutex, K_FOREVER);
    data->temperature = sensors.temperature;
    data->humidity = sensors.humidity;
    k_mutex_unlock(&sensors_mutex);
}
