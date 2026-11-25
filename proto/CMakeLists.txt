include(FetchContent)
FetchContent_Declare(
    nanopb
    GIT_REPOSITORY https://github.com/nanopb/nanopb.git
    GIT_TAG        nanopb-0.4.9.3
    SOURCE_SUBDIR  none # only the sources are needed, not nanopb's own CMake project
)
FetchContent_MakeAvailable(nanopb)

find_program(PROTOC protoc)
if(NOT PROTOC)
    message(FATAL_ERROR "protoc not found")
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(NANOPB_PYTHON_DEPS "${CMAKE_CURRENT_BINARY_DIR}/nanopb-deps")
if(NOT EXISTS "${NANOPB_PYTHON_DEPS}/google/protobuf")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m pip install --quiet --disable-pip-version-check
                --target "${NANOPB_PYTHON_DEPS}" "protobuf==7.36.2"
        COMMAND_ERROR_IS_FATAL ANY)
endif()

set(PROTO_OUT "${CMAKE_CURRENT_BINARY_DIR}/generated/proto")
file(MAKE_DIRECTORY "${PROTO_OUT}")
add_custom_command(
    OUTPUT "${PROTO_OUT}/triangle.pb.c" "${PROTO_OUT}/triangle.pb.h"
    COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=${NANOPB_PYTHON_DEPS}"
            "${Python3_EXECUTABLE}" "${nanopb_SOURCE_DIR}/generator/nanopb_generator.py"
            -I "${CMAKE_CURRENT_SOURCE_DIR}" -D "${PROTO_OUT}" "${CMAKE_CURRENT_SOURCE_DIR}/triangle.proto"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/triangle.proto" "${CMAKE_CURRENT_SOURCE_DIR}/triangle.options"
    COMMENT "Generating nanopb code from triangle.proto"
    VERBATIM
)

add_library(protocol STATIC
    "${PROTO_OUT}/triangle.pb.c"
    "${nanopb_SOURCE_DIR}/pb_common.c"
    "${nanopb_SOURCE_DIR}/pb_encode.c"
    "${nanopb_SOURCE_DIR}/pb_decode.c"
)
target_include_directories(protocol PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/generated" "${nanopb_SOURCE_DIR}")
