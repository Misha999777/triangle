cmake_minimum_required(VERSION 3.19)

set(PICO_BOARD pico2_w)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(triangle C CXX ASM)
pico_sdk_init()

add_subdirectory(proto)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(triangle ${SOURCES})

target_link_libraries(triangle
    protocol
    pico_cyw43_arch_none
    pico_stdlib
    pico_multicore
    pico_btstack_cyw43
    pico_btstack_ble
    hardware_spi
    hardware_i2c
    hardware_pwm
    hardware_adc
)

target_include_directories(triangle PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bluetooth/config")
pico_btstack_make_gatt_header(triangle PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bluetooth/config/ble_config.gatt")

pico_enable_stdio_usb(triangle 1)
pico_add_extra_outputs(triangle)
