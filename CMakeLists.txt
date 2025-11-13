cmake_minimum_required(VERSION 3.13)

# initialize pico_sdk from an external folder
include(pico_sdk_import.cmake)

project(pico_winc_driver C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize the raspberry pi pico sdk
pico_sdk_init()

# add executable and link libraries
add_executable(pico_winc_driver
    main.c
)

target_link_libraries(pico_winc_driver pico_stdlib)

# enable usb output
pico_enable_stdio_usb(pico_winc_driver 1)
pico_enable_stdio_uart(pico_winc_driver 1)

# allow resetting into bootloader mode by connecting at 1200 baud
target_compile_definitions(pico_winc_driver PUBLIC
    PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=1
)

# create map file
pico_add_extra_outputs(pico_winc_driver)
