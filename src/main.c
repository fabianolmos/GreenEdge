#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/* =========================
 * Fake sensor (por ahora)
 * ========================= */
static int read_sensor(void)
{
	/* Simulación */
	return 42;
}

/* =========================
 * Shell commands
 * ========================= */
static int cmd_data(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int value = read_sensor();
	shell_print(sh, "Sensor value: %d", value);

	return 0;
}

static int cmd_status(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Uptime: %lld ms", k_uptime_get());

	return 0;
}

/* Register commands */
SHELL_CMD_REGISTER(data,   NULL, "Read sensor once", cmd_data);
SHELL_CMD_REGISTER(status, NULL, "Show system status", cmd_status);

int main(void)
{
	LOG_INF("GreenEdge basic shell started");
	return 0;
}

