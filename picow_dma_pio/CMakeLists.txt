cmake_minimum_required(VERSION 3.13)

# set project name
set(PROJECT picow_dma_pio)
# set pico board
set(PICO_BOARD pico_w)

# initialize the SDK based on PICO_SDK_PATH
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# set the project name
project(${PROJECT} C CXX ASM)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# add the executable
add_executable(
    ${PROJECT} 
    src/main.c
)

# compile the program.pio file
pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/src/dma_pio.pio)

# add target link libraries
target_link_libraries(
    ${PROJECT}
    pico_stdlib
    pico_cyw43_arch_none
    hardware_dma
    hardware_irq
    hardware_pio
)

# add compile options
target_compile_options(${PROJECT} PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT})
# enable USB output
pico_enable_stdio_usb(${PROJECT} 1)
# enable UART output
pico_enable_stdio_uart(${PROJECT} 1)