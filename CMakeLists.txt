cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "Can't find pico-sdk, path should be specified using PICO_SDK_PATH environment variable.")
endif()

set(PICO_BOARD pico2_w)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(triangle C CXX ASM)
pico_sdk_init()

file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "src/*.hpp"
)
add_executable(triangle ${SOURCES})

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

target_compile_definitions(triangle PRIVATE
        HAVE_EMBEDDED_TIME_MS
        HAVE_MALLOC
        HAVE_ASSERT
        ENABLE_LE_PERIPHERAL
        ENABLE_PRINTF_HEXDUMP
        MAX_NR_LE_DEVICE_DB_ENTRIES=16
        NVM_NUM_DEVICE_DB_ENTRIES=16
        HCI_ACL_PAYLOAD_SIZE=259
        HCI_OUTGOING_PRE_BUFFER_SIZE=4
        HCI_ACL_CHUNK_SIZE_ALIGNMENT=4
)
target_include_directories(triangle PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bluetooth/config")
pico_btstack_make_gatt_header(triangle PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bluetooth/config/ble_config.gatt")

pico_add_extra_outputs(triangle)
