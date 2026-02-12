/**
 * @file sensor_thread.c
 * @brief Sensor thread implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "sensors.h"
#include "sensor_thread.h"

LOG_MODULE_REGISTER(sensor_thread, LOG_LEVEL_INF);

#define SENSOR_STACK_SIZE 1024
#define SENSOR_PRIORITY 5

/* Thread control */
K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
static struct k_thread sensor_thread_data;
static struct k_sem sensor_sem;

/* Periodic reading control */
static struct k_timer sensor_timer;
static bool periodic_enabled = false;

/* Forward declarations */
static void sensor_thread_entry(void *a, void *b, void *c);
static void sensor_timer_handler(struct k_timer *timer);

/**
 * @brief Sensor thread main loop
 */
static void sensor_thread_entry(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    struct sensor_data local_data;

    LOG_INF("Sensor thread started");

    /* Get initial values */
    sensors_get(&local_data);

    while (1) {
        /* Wait for trigger signal */
        k_sem_take(&sensor_sem, K_FOREVER);

        /* Read new sensor values */
        sensors_read();

        /* Update shared data (thread-safe) */
        sensors_get(&local_data);  /* This will be updated by sensor.c internally */

        LOG_INF("Sensor reading: T=%d°C, H=%d%%",
                local_data.temperature,
                local_data.humidity);
    }
}

/**
 * @brief Timer handler for periodic readings
 */
static void sensor_timer_handler(struct k_timer *timer)
{
    ARG_UNUSED(timer);

    if (periodic_enabled) {
        k_sem_give(&sensor_sem);
    }
}

void sensor_thread_start(void)
{
    LOG_INF("Initializing sensor thread");

    /* Initialize sensors first */
    sensors_init();

    /* Initialize semaphore (starts at 0) */
    k_sem_init(&sensor_sem, 0, 1);

    /* Initialize timer for periodic readings */
    k_timer_init(&sensor_timer, sensor_timer_handler, NULL);

    /* Create and start thread */
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

    LOG_INF("Sensor thread initialized");
}

void sensor_thread_trigger(void)
{
    k_sem_give(&sensor_sem);
}

void sensor_thread_start_periodic(uint32_t period_ms)
{
    if (period_ms < 100) {
        LOG_WRN("Period too short, using 100ms minimum");
        period_ms = 100;
    }

    periodic_enabled = true;
    k_timer_start(&sensor_timer, K_MSEC(period_ms), K_MSEC(period_ms));

    LOG_INF("Periodic readings started: %u ms", period_ms);
}

void sensor_thread_stop_periodic(void)
{
    periodic_enabled = false;
    k_timer_stop(&sensor_timer);

    LOG_INF("Periodic readings stopped");
}
