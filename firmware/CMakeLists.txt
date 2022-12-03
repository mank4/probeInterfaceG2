cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(probeInterfaceG2 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(probeInterfaceG2)

pico_generate_pio_header(probeInterfaceG2 ${CMAKE_CURRENT_LIST_DIR}/pio_spi.pio)

target_sources(probeInterfaceG2 PRIVATE
        main.cpp
        pio_spi.cpp
        pio_spi.hpp
        )

pico_enable_stdio_usb(probeInterfaceG2 1)
pico_enable_stdio_uart(probeInterfaceG2 0)
        
pico_add_extra_outputs(probeInterfaceG2)
        
target_link_libraries(probeInterfaceG2 PRIVATE pico_stdlib hardware_pio)

