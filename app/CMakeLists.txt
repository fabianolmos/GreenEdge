#set(BOARD esp32c6_devkitc)

cmake_minimum_required(VERSION 3.20.0)

set(BOARD esp32c6_devkitc/esp32c6/hpcore)

set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/greenedge.overlay")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(GreenEdge)

target_include_directories(app PRIVATE include)

# Selección de backend según Kconfig:
#   CONFIG_GREENEDGE_SENSOR_REAL=y  -> sensor_sht40.c (hardware real)
#   CONFIG_GREENEDGE_SENSOR_REAL=n  -> sensor.c       (simulado)
if(CONFIG_GREENEDGE_SENSOR_REAL)
    target_sources(app PRIVATE src/sensor_sht40.c)
else()
    target_sources(app PRIVATE src/sensor.c)
endif()

target_sources(app PRIVATE
    src/main.c
    src/sensor_thread.c
    src/shell_cmds.c
    src/wifi_manager.c
    src/wifi_shell_cmds.c
)
