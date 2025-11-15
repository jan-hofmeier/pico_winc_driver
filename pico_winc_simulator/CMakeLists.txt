cmake_minimum_required(VERSION 3.13)

# Include the Pico SDK from the location specified by PICO_SDK_PATH
# You can override this on the command line, e.g. -DPICO_SDK_PATH=/path/to/pico-sdk
include(pico_sdk_import.cmake)

project(pico_winc_simulator C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initializes the Pico SDK
pico_sdk_init()

# Add the executable target
add_executable(pico_winc_simulator
    main.c
)

# Link to the Pico SDK libraries
target_link_libraries(pico_winc_simulator pico_stdlib)

# Enable usb output, disable uart output
pico_enable_stdio_usb(pico_winc_simulator 1)
pico_enable_stdio_uart(pico_winc_simulator 0)
