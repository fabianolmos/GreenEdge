cmake_minimum_required(VERSION 3.20.0)
set(BOARD esp32c6_devkitc)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(GreenEdge)

target_sources(app PRIVATE src/main.c)
