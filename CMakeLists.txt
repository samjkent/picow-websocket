cmake_minimum_required(VERSION 3.13)

set(NAME pico)
set(PICO_BOARD pico_w)

# initialize pico-sdk from GIT
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(${NAME} C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add your source files
add_executable(${NAME}
    main.cpp
    ws.cpp
)

target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR} )

target_link_libraries(${NAME}
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
)

pico_enable_stdio_usb(${NAME} 1)

pico_add_extra_outputs(${NAME})

# rest of your project
