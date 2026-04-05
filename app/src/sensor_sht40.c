/**
 * @file sensor_sht40.c
 * @brief Backend real para el sensor Sensirion SHT40 via I2C
 *
 * Implementa la interfaz de sensors.h usando el driver nativo
 * de Zephyr para la familia SHT4x (SHT40/SHT41/SHT45).
 *
 * Requiere en el devicetree:
 *   - &i2c0 habilitado con GPIO6=SDA, GPIO7=SCL
 *   - Nodo sht4x@44 con compatible = "sensirion,sht4x"
 *
 * Activado cuando CONFIG_GREENEDGE_SENSOR_REAL=y.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "sensors.h"

LOG_MODULE_REGISTER(sensor_sht40, LOG_LEVEL_DBG);

/* Estado compartido */
static struct sensor_data g_sensors;
static struct k_mutex     g_sensors_mutex;

/* Referencia al dispositivo SHT40 */
static const struct device *sht40_dev;

int sensors_init(void)
{
    k_mutex_init(&g_sensors_mutex);

    g_sensors.temperature = 0;
    g_sensors.humidity    = 0;
    g_sensors.valid       = false;

    sht40_dev = DEVICE_DT_GET_ANY(sensirion_sht4x);

    if (sht40_dev == NULL) {
        LOG_ERR("SHT40: no se encontró el dispositivo en el devicetree");
        return -ENODEV;
    }

    if (!device_is_ready(sht40_dev)) {
        LOG_ERR("SHT40: dispositivo '%s' no está listo", sht40_dev->name);
        return -ENODEV;
    }

    LOG_INF("SHT40 inicializado: %s", sht40_dev->name);
    return 0;
}

int sensors_read(void)
{
    struct sensor_value temp, hum;
    struct sensor_data local;
    int ret;

    ret = sensor_sample_fetch(sht40_dev);
    if (ret != 0) {
        LOG_ERR("SHT40: sensor_sample_fetch() falló: %d", ret);
        goto fail;
    }

    ret = sensor_channel_get(sht40_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    if (ret != 0) {
        LOG_ERR("SHT40: channel_get(TEMP) falló: %d", ret);
        goto fail;
    }

    ret = sensor_channel_get(sht40_dev, SENSOR_CHAN_HUMIDITY, &hum);
    if (ret != 0) {
        LOG_ERR("SHT40: channel_get(HUM) falló: %d", ret);
        goto fail;
    }

    /*
     * sensor_value: val1 = parte entera, val2 = parte fraccionaria en millonésimas
     * Convertimos a centidegrees: val1*100 + val2/10000
     * Ejemplo: {23, 500000} -> 2350 (23.50°C)
     */
    local.temperature = (int32_t)(temp.val1 * 100 + temp.val2 / 10000);
    local.humidity    = (int32_t)(hum.val1  * 100 + hum.val2  / 10000);
    local.valid       = true;

    k_mutex_lock(&g_sensors_mutex, K_FOREVER);
    g_sensors = local;
    k_mutex_unlock(&g_sensors_mutex);

    LOG_DBG("SHT40: T=%d.%02d°C  H=%d.%02d%%",
            local.temperature / 100, abs(local.temperature % 100),
            local.humidity    / 100, abs(local.humidity    % 100));

    return 0;

fail:
    k_mutex_lock(&g_sensors_mutex, K_FOREVER);
    g_sensors.valid = false;
    k_mutex_unlock(&g_sensors_mutex);

    return ret;
}

void sensors_get(struct sensor_data *data)
{
    if (data == NULL) {
        LOG_ERR("NULL pointer en sensors_get");
        return;
    }

    k_mutex_lock(&g_sensors_mutex, K_FOREVER);
    *data = g_sensors;
    k_mutex_unlock(&g_sensors_mutex);
}
