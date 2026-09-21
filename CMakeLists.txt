cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "Can't find pico-sdk, path should be specified using PICO_SDK_PATH environment variable.")
endif()

set(PICO_BOARD pico2_w)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(triangle C CXX ASM)
pico_sdk_init()

add_executable(triangle
        src/hardware/Driver.cpp
        src/hardware/MotorSensor.cpp
        src/hardware/Motor.cpp
        src/hardware/AngleSensor.cpp
        src/controller/Storage.cpp
        src/controller/Controller.cpp
        src/bluetooth/Communication.cpp
        src/main.cpp
)

pico_enable_stdio_uart(triangle 0)
pico_enable_stdio_usb(triangle 1)

target_link_libraries(triangle
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

pico_add_extra_outputs(triangle)
