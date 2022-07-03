cmake_minimum_required(VERSION 3.13)

set(NAME pico)
set(PICO_BOARD pico_w)

# initialize pico-sdk from GIT
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_FETCH_FROM_GIT on)
set(PIMORONI_PICO_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pimoroni-pico)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pimoroni_pico_import.cmake)
include(pico_sdk_import.cmake)

project(${NAME} C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add your source files
add_executable(${NAME}
    main.cpp
)

include(drivers/st7789/st7789)
include(drivers/button/button)
include(drivers/rgbled/rgbled)
include(common/pimoroni_bus)
include(libraries/pico_display_2/pico_display_2)
include(libraries/pico_graphics/pico_graphics)
include(libraries/bitmap_fonts/bitmap_fonts)
include(libraries/hershey_fonts/hershey_fonts)

target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR} )

target_link_libraries(${NAME}
    pico_stdlib
    hardware_spi
    hardware_dma
    hardware_pio
    hardware_pwm
    st7789
    rgbled
    button
    pico_cyw43_arch_lwip_threadsafe_background
)

pico_enable_stdio_usb(${NAME} 1)

pico_add_extra_outputs(${NAME})

# rest of your project
