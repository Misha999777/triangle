cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "Can't find pico-sdk, path should be specified using PICO_SDK_PATH environment variable.")
endif()

set(PICO_BOARD pico)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(triangle C CXX ASM)
pico_sdk_init()

add_executable(triangle
        src/hardware/Driver.cpp
        src/hardware/MotorSensor.cpp
        src/hardware/Motor.cpp
        src/hardware/AngleSensor.cpp
        src/hardware/Battery.cpp
        src/controller/Storage.cpp
        src/controller/Controller.cpp
        src/main.cpp
)

pico_enable_stdio_uart(triangle 0)
pico_enable_stdio_usb(triangle 1)

target_link_libraries(triangle
        pico_stdlib
        pico_multicore
        hardware_spi
        hardware_pwm
        hardware_i2c
        hardware_adc
)

pico_add_extra_outputs(triangle)
