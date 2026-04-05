/**
 * @file shell_cmds.c
 * @brief Shell command implementations
 */

#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include "sensors.h"
#include "sensor_thread.h"

static int cmd_data(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    struct sensor_data data;

    sensors_get(&data);

    shell_print(sh, "=== Sensor Data ===");

    if (!data.valid) {
        shell_warn(sh, "Datos aún no disponibles (esperando primera lectura)");
        return 0;
    }

    shell_print(sh, "Temperatura: %d.%02d °C",
                data.temperature / 100,
                abs(data.temperature % 100));
    shell_print(sh, "Humedad    : %d.%02d %%",
                data.humidity / 100,
                abs(data.humidity % 100));

    return 0;
}

static int cmd_read(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    sensor_thread_trigger();
    shell_print(sh, "Lectura disparada...");

    /* Dar tiempo al thread para completar la lectura */
    k_sleep(K_MSEC(200));

    return cmd_data(sh, 0, NULL);
}

static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    shell_print(sh, "=== System Status ===");
    shell_print(sh, "Uptime: %lld ms", k_uptime_get());
    shell_print(sh, "Cycles: %llu",    k_cycle_get_64());

    return 0;
}

static int cmd_start_periodic(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t period_ms = 5000; /* Default: 5 segundos (razonable para un invernadero) */

    if (argc > 1) {
        period_ms = (uint32_t)atoi(argv[1]);
        if (period_ms < 1000) {
            shell_error(sh, "Mínimo 1000 ms (limitación del SHT40 en modo alta repetibilidad)");
            return -EINVAL;
        }
    }

    sensor_thread_start_periodic(period_ms);
    shell_print(sh, "Lecturas periódicas iniciadas: %u ms", period_ms);

    return 0;
}

static int cmd_stop_periodic(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    sensor_thread_stop_periodic();
    shell_print(sh, "Lecturas periódicas detenidas");

    return 0;
}

/* Registro de comandos */
SHELL_CMD_REGISTER(data,  NULL, "Mostrar datos del sensor",              cmd_data);
SHELL_CMD_REGISTER(read,  NULL, "Forzar lectura del sensor",             cmd_read);
SHELL_CMD_REGISTER(status,NULL, "Estado del sistema",                    cmd_status);
SHELL_CMD_REGISTER(start, NULL, "Iniciar lecturas periódicas [ms]",      cmd_start_periodic);
SHELL_CMD_REGISTER(stop,  NULL, "Detener lecturas periódicas",           cmd_stop_periodic);
