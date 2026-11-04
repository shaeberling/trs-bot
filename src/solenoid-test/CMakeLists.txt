
cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD pico2_w)            # must come before the SDK import
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(solenoid C CXX ASM)
pico_sdk_init()

add_executable(solenoid main.c)
target_link_libraries(solenoid pico_stdlib pico_cyw43_arch_none)
pico_enable_stdio_usb(solenoid 1)  # printf over USB serial
pico_enable_stdio_uart(solenoid 0)
pico_add_extra_outputs(solenoid)   # makes the .uf2
