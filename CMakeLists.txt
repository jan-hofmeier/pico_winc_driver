cmake_minimum_required(VERSION 3.13)

# initialize pico_sdk from an external folder
include(pico_sdk_import.cmake)

project(pico_winc_driver C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(WINC_DRIVER_VERSION "CURRENT" CACHE STRING "WINC driver version to use: CURRENT or 19_3_0")
set_property(CACHE WINC_DRIVER_VERSION PROPERTY STRINGS CURRENT 19_3_0)

# initialize the raspberry pi pico sdk
pico_sdk_init()

# add executable and link libraries
add_executable(pico_winc_driver
    main.c
)

if(WINC_DRIVER_VERSION STREQUAL "CURRENT")
    target_sources(pico_winc_driver PRIVATE
        host_drv/common/source/nm_common.c
        host_drv/driver/source/m2m_hif.c
        host_drv/driver/source/m2m_ota.c
        host_drv/driver/source/m2m_periph.c
        host_drv/driver/source/m2m_ssl.c
        host_drv/driver/source/m2m_wifi.c
        host_drv/driver/source/nmasic.c
        host_drv/driver/source/nmdrv.c
        host_drv/driver/source/nmspi.c
        host_drv/driver/source/nmbus.c
        host_drv/socket/source/socket.c
        host_drv/spi_flash/source/spi_flash.c
        host_drv/bsp/source/nm_bsp_pico.c
        host_drv/bus_wrapper/source/nm_bus_wrapper_pico.c
    )
    target_include_directories(pico_winc_driver PUBLIC
        .
        host_drv
        host_drv/common/include
        host_drv/driver/include
        host_drv/bsp/include
        host_drv/bus_wrapper/include
        host_drv/socket/include
        host_drv/spi_flash/include
    )
elseif(WINC_DRIVER_VERSION STREQUAL "19_3_0")
    target_sources(pico_winc_driver PRIVATE
        host_drv_19_3_0/common/source/nm_common.c
        host_drv_19_3_0/driver/source/m2m_hif.c
        host_drv_19_3_0/driver/source/m2m_ota.c
        host_drv_19_3_0/driver/source/m2m_periph.c
        host_drv_19_3_0/driver/source/m2m_wifi.c
        host_drv_19_3_0/driver/source/nmasic.c
        host_drv_19_3_0/driver/source/nmdrv.c
        host_drv_19_3_0/driver/source/nmspi.c
        host_drv_19_3_0/driver/source/nmbus.c
        host_drv_19_3_0/socket/source/socket.c
        host_drv_19_3_0/socket/source/socket_buffer.c
        host_drv_19_3_0/spi_flash/source/spi_flash.c
        host_drv_19_3_0/bsp/source/nm_bsp_pico.c
        host_drv_19_3_0/bus_wrapper/source/nm_bus_wrapper_pico.c
    )
    target_include_directories(pico_winc_driver PUBLIC
        .
        host_drv_19_3_0
        host_drv_19_3_0/common/include
        host_drv_19_3_0/driver/include
        host_drv_19_3_0/bsp/include
        host_drv_19_3_0/bus_wrapper/include
        host_drv_19_3_0/socket/include
        host_drv_19_3_0/spi_flash/include
    )
else()
    message(FATAL_ERROR "Invalid WINC_DRIVER_VERSION selected: ${WINC_DRIVER_VERSION}")
endif()

target_link_libraries(pico_winc_driver pico_stdlib hardware_spi hardware_gpio)

# enable usb output
pico_enable_stdio_usb(pico_winc_driver 1)
pico_enable_stdio_uart(pico_winc_driver 1)

# allow resetting into bootloader mode by connecting at 1200 baud
target_compile_definitions(pico_winc_driver PUBLIC
    PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=1
    PICO_WINC
)

# create map file
pico_add_extra_outputs(pico_winc_driver)
