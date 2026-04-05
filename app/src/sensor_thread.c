/**
 * @file sensor_thread.c
 * @brief Sensor thread implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include "sensors.h"
#include "sensor_thread.h"

LOG_MODULE_REGISTER(sensor_thread, LOG_LEVEL_INF);

#define SENSOR_STACK_SIZE 2048
#define SENSOR_PRIORITY   5

/* Thread control */
K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
static struct k_thread sensor_thread_data;
static struct k_sem    sensor_sem;

/* Periodic reading control */
static struct k_timer sensor_timer;
static bool           periodic_enabled;

/* Forward declarations */
static void sensor_thread_entry(void *a, void *b, void *c);
static void sensor_timer_handler(struct k_timer *timer);

static void sensor_thread_entry(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    struct sensor_data local;
    int ret;

    LOG_INF("Sensor thread iniciado");

    while (1) {
        k_sem_take(&sensor_sem, K_FOREVER);

        ret = sensors_read();

        if (ret == 0) {
            sensors_get(&local);
            LOG_INF("T=%d.%02d°C  H=%d.%02d%%",
                    local.temperature / 100, abs(local.temperature % 100),
                    local.humidity    / 100, abs(local.humidity    % 100));
        } else {
            LOG_WRN("Error en lectura de sensores: %d", ret);
        }
    }
}

static void sensor_timer_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);

    if (periodic_enabled) {
        k_sem_give(&sensor_sem);
    }
}

void sensor_thread_start(void)
{
    LOG_INF("Inicializando sensor thread");

    int ret = sensors_init();
    if (ret != 0) {
        LOG_ERR("sensors_init() falló: %d - continuando en modo degradado", ret);
    }

    k_sem_init(&sensor_sem, 0, 1);
    k_timer_init(&sensor_timer, sensor_timer_handler, NULL);

    k_thread_create(
        &sensor_thread_data,
        sensor_stack,
        K_THREAD_STACK_SIZEOF(sensor_stack),
        sensor_thread_entry,
        NULL, NULL, NULL,
        SENSOR_PRIORITY,
        0,
        K_NO_WAIT
    );

    k_thread_name_set(&sensor_thread_data, "sensor");

    LOG_INF("Sensor thread listo");
}

void sensor_thread_trigger(void)
{
    k_sem_give(&sensor_sem);
}

void sensor_thread_start_periodic(uint32_t period_ms)
{
    if (period_ms < 1000) {
        LOG_WRN("El SHT40 necesita mínimo ~1000ms entre lecturas de alta repetibilidad");
        period_ms = 1000;
    }

    periodic_enabled = true;
    k_timer_start(&sensor_timer, K_MSEC(period_ms), K_MSEC(period_ms));
    LOG_INF("Lecturas periódicas: %u ms", period_ms);
}

void sensor_thread_stop_periodic(void)
{
    periodic_enabled = false;
    k_timer_stop(&sensor_timer);
    LOG_INF("Lecturas periódicas detenidas");
}
