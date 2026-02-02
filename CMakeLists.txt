cmake_minimum_required(VERSION 3.20.0)
set(BOARD esp32c6_devkitc)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(GreenEdge)

target_include_directories(app PRIVATE include)

target_sources(app PRIVATE 
	src/main.c
	src/sensor.c
	src/sensor_thread.c
	src/shell_cmds.c
)
