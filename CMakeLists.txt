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

    host_drv/common/source/nm_common.c
    host_drv/driver/source/m2m_hif.c
    host_drv/driver/source/m2m_ota.c
    host_drv/driver/source/m2m_periph.c
    host_drv/driver/source/m2m_ssl.c
    host_drv/driver/source/m2m_wifi.c
    host_drv/driver/source/nmasic.c
    host_drv/driver/source/nmbus.c
    host_drv/driver/source/nmdrv.c
    host_drv/driver/source/nmspi.c
    host_drv/bsp/source/nm_bsp_pico.c
    host_drv/bus_wrapper/source/nm_bus_wrapper_pico.c
    host_drv/spi_flash/source/spi_flash.c
)

target_include_directories(pico_winc_driver PUBLIC
    "."
    "host_drv"
    "host_drv/driver"
    "host_drv/driver/include"
    "host_drv/driver/source"
    "host_drv/common/include"
    "host_drv/bus_wrapper/include"
    "host_drv/bsp/include"
    "host_drv/pico_hal"
    "host_drv/spi_flash/include"
    "pico-sdk/include"
    "pico-sdk/common/include"
    "pico-sdk/rp2040/include"
)

target_compile_definitions(pico_winc_driver PUBLIC
    "CONF_WINC_USE_SPI"
)


target_link_libraries(pico_winc_driver pico_stdlib hardware_spi)

# enable usb output
pico_enable_stdio_usb(pico_winc_driver 1)
pico_enable_stdio_uart(pico_winc_driver 1)

# allow resetting into bootloader mode by connecting at 1200 baud
target_compile_definitions(pico_winc_driver PUBLIC
    PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=1
)

# create map file
pico_add_extra_outputs(pico_winc_driver)
