cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(probeInterfaceG2 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(probeInterfaceG2)

#Load SCPI Device library: https://github.com/j123b567/scpi-parser
add_compile_definitions(HAVE_STDBOOL)
set(SCPI_SRC_PATH "../scpi-parser/libscpi/src")
add_library(scpi STATIC
        ${SCPI_SRC_PATH}/error.c
        ${SCPI_SRC_PATH}/fifo.c
        ${SCPI_SRC_PATH}/ieee488.c
        ${SCPI_SRC_PATH}/minimal.c
        ${SCPI_SRC_PATH}/parser.c
        ${SCPI_SRC_PATH}/units.c
        ${SCPI_SRC_PATH}/utils.c
        ${SCPI_SRC_PATH}/lexer.c
        ${SCPI_SRC_PATH}/expression.c
)
target_include_directories(scpi PUBLIC "scpi-parser/libscpi/inc")

#Load USB
set(USB_SRC_PATH "../usb")
add_library(usbinst STATIC
        ${USB_SRC_PATH}/usb_descriptors.c
        ${USB_SRC_PATH}/usbtmc_app.c
)
target_include_directories(usbinst PUBLIC "usb")
target_link_libraries(usbinst PUBLIC pico_stdlib pico_unique_id tinyusb_board tinyusb_device)

#Build main file
pico_generate_pio_header(probeInterfaceG2 ${CMAKE_CURRENT_LIST_DIR}/pio_spi.pio)

target_sources(probeInterfaceG2 PRIVATE
        main.cpp
        pio_spi.cpp
        pio_spi.hpp
        )

#pico_enable_stdio_usb(probeInterfaceG2 1)
#pico_enable_stdio_uart(probeInterfaceG2 0)
        
pico_add_extra_outputs(probeInterfaceG2)
        
target_link_libraries(probeInterfaceG2 PRIVATE pico_stdlib hardware_pio usbinst scpi)
