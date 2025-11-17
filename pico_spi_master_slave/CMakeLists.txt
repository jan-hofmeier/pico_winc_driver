cmake_minimum_required(VERSION 3.13)

# Initialize the SDK
include(../pico_sdk_import.cmake)

project(pico_spi_master_slave_project C CXX ASM)

pico_sdk_init()

# Add the subdirectory with the source code
add_subdirectory(src)
