cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(pico_serprog)

pico_sdk_init()

add_executable(pico_serprog)
target_sources(pico_serprog PRIVATE main.c)
target_link_libraries(pico_serprog PRIVATE pico_stdlib hardware_spi)
pico_add_extra_outputs(pico_serprog)

# enable usb output, disable uart output
pico_enable_stdio_usb(pico_serprog 1)
pico_enable_stdio_uart(pico_serprog 0)
