/**
 * @file shell_cmds.c
 * @brief Shell command implementations
 */
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include "sensors.h"
#include "sensor_thread.h"

/**
 * @brief Read and display current sensor data
 */
static int cmd_data(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    struct sensor_data data;

    sensors_get(&data);

    shell_print(sh, "=== Sensor Data ===");
    shell_print(sh, "Temperature: %d °C", data.temperature);
    shell_print(sh, "Humidity   : %d %%", data.humidity);

    return 0;
}

/**
 * @brief Trigger a single sensor reading
 */
static int cmd_read(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    sensor_thread_trigger();
    shell_print(sh, "Sensor reading triggered");

    /* Wait a bit for the reading to complete */
    k_sleep(K_MSEC(100));

    /* Show the updated data */
    return cmd_data(sh, 0, NULL);
}

/**
 * @brief Show system status
 */
static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "=== System Status ===");
    shell_print(sh, "Uptime: %lld ms", k_uptime_get());
    shell_print(sh, "Cycles: %llu", k_cycle_get_64());

    return 0;
}

/**
 * @brief Start periodic sensor readings
 */
static int cmd_start_periodic(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t period_ms = 1000; /* Default: 1 second */

    if (argc > 1) {
        period_ms = atoi(argv[1]);
        if (period_ms < 100) {
            shell_error(sh, "Period must be >= 100 ms");
            return -EINVAL;
        }
    }

    sensor_thread_start_periodic(period_ms);
    shell_print(sh, "Periodic readings started: %u ms", period_ms);

    return 0;
}

/**
 * @brief Stop periodic sensor readings
 */
static int cmd_stop_periodic(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    sensor_thread_stop_periodic();
    shell_print(sh, "Periodic readings stopped");

    return 0;
}

/* Register commands */
SHELL_CMD_REGISTER(data, NULL, "Read current sensor data", cmd_data);
SHELL_CMD_REGISTER(read, NULL, "Trigger sensor reading", cmd_read);
SHELL_CMD_REGISTER(status, NULL, "Show system status", cmd_status);
SHELL_CMD_REGISTER(start, NULL, "Start periodic readings [period_ms]", cmd_start_periodic);
SHELL_CMD_REGISTER(stop, NULL, "Stop periodic readings", cmd_stop_periodic);
