cmake_minimum_required(VERSION 3.13)

# initialize pico_sdk from an external folder
include(pico_sdk_import.cmake)

project(pico_winc_projects C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(WINC_DRIVER_VERSION "19_3_0" CACHE STRING "WINC driver version to use: 19_7_7 or 19_3_0")
set_property(CACHE WINC_DRIVER_VERSION PROPERTY STRINGS "19_7_7" "19_3_0")

set(BUILD_MODE "COMBINED" CACHE STRING "Build mode: DRIVER, SIMULATOR, or COMBINED")
set_property(CACHE BUILD_MODE PROPERTY STRINGS "DRIVER" "SIMULATOR" "COMBINED")

# initialize the raspberry pi pico sdk
pico_sdk_init()

# --- Common sources and includes for the driver ---
set(WINC_DRIVER_SOURCES)
set(WINC_DRIVER_INCLUDES)

if(WINC_DRIVER_VERSION STREQUAL "19_7_7")
    list(APPEND WINC_DRIVER_SOURCES
        host_drv_19_7_7/common/source/nm_common.c
        host_drv_19_7_7/driver/source/m2m_hif.c
        host_drv_19_7_7/driver/source/m2m_ota.c
        host_drv_19_7_7/driver/source/m2m_periph.c
        host_drv_19_7_7/driver/source/m2m_ssl.c
        host_drv_19_7_7/driver/source/m2m_wifi.c
        host_drv_19_7_7/driver/source/nmasic.c
        host_drv_19_7_7/driver/source/nmdrv.c
        host_drv_19_7_7/driver/source/nmspi.c
        host_drv_19_7_7/driver/source/nmbus.c
        host_drv_19_7_7/socket/source/socket.c
        host_drv_19_7_7/spi_flash/source/spi_flash.c
        host_drv_19_7_7/bsp/source/nm_bsp_pico.c
        host_drv_19_7_7/bus_wrapper/source/nm_bus_wrapper_pico.c
        iot/http/http_client.c
    )
    list(APPEND WINC_DRIVER_INCLUDES
        .
        host_drv_19_7_7
        host_drv_19_7_7/common/include
        host_drv_19_7_7/driver/include
        host_drv_19_7_7/bsp/include
        host_drv_19_7_7/bus_wrapper/include
        host_drv_19_7_7/socket/include
        host_drv_19_7_7/spi_flash/include
        iot/http
    )
elseif(WINC_DRIVER_VERSION STREQUAL "19_3_0")
    list(APPEND WINC_DRIVER_SOURCES
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
        iot/http/http_client.c
    )
    list(APPEND WINC_DRIVER_INCLUDES
        .
        host_drv_19_3_0
        host_drv_19_3_0/common/include
        host_drv_19_3_0/driver/include
        host_drv_19_3_0/bsp/include
        host_drv_19_3_0/bus_wrapper/include
        host_drv_19_3_0/socket/include
        host_drv_19_3_0/spi_flash/include
        iot/http
    )
else()
    message(FATAL_ERROR "Invalid WINC_DRIVER_VERSION selected: ${WINC_DRIVER_VERSION}")
endif()

# --- Build configuration logic ---
if(BUILD_MODE STREQUAL "DRIVER")
    add_executable(pico_winc_driver
        src/main_driver_only.c
        driver/winc_driver_app.c
        ${WINC_DRIVER_SOURCES}
    )
    target_include_directories(pico_winc_driver PUBLIC
        driver
        config
        ${WINC_DRIVER_INCLUDES}
    )
    target_link_libraries(pico_winc_driver pico_stdlib hardware_spi hardware_gpio)
    pico_enable_stdio_usb(pico_winc_driver 1)
    pico_enable_stdio_uart(pico_winc_driver 1)
    target_compile_definitions(pico_winc_driver PUBLIC PICO_WINC)
    pico_add_extra_outputs(pico_winc_driver)

elseif(BUILD_MODE STREQUAL "SIMULATOR")
    add_executable(pico_winc_simulator
        src/main_simulator_only.c
        pico_winc_simulator/winc_simulator_app.c
        pico_winc_simulator/pio_spi.c
        pico_winc_simulator/sim_log.c
    )
    pico_generate_pio_header(pico_winc_simulator ${CMAKE_CURRENT_SOURCE_DIR}/pico_winc_simulator/spi_slave.pio)
    target_include_directories(pico_winc_simulator PUBLIC
        src
        pico_winc_simulator
        config
        ${PICO_SDK_PATH}/src/rp2_common/hardware_spi/include
    )
    target_link_libraries(pico_winc_simulator pico_stdlib hardware_spi hardware_gpio hardware_pio)
    pico_enable_stdio_usb(pico_winc_simulator 1)
    pico_enable_stdio_uart(pico_winc_simulator 0)
    pico_add_extra_outputs(pico_winc_simulator)

elseif(BUILD_MODE STREQUAL "COMBINED")
    add_executable(pico_winc_combined
        src/main_combined_app.c
        driver/winc_driver_app.c
        pico_winc_simulator/winc_simulator_app.c
        pico_winc_simulator/pio_spi.c
        pico_winc_simulator/sim_log.c
        ${WINC_DRIVER_SOURCES}
    )
    pico_generate_pio_header(pico_winc_combined ${CMAKE_CURRENT_SOURCE_DIR}/pico_winc_simulator/spi_slave.pio)
    target_include_directories(pico_winc_combined PUBLIC
        driver
        pico_winc_simulator
        config
        ${WINC_DRIVER_INCLUDES}
        ${PICO_SDK_PATH}/src/rp2_common/hardware_spi/include
    )
    target_link_libraries(pico_winc_combined pico_stdlib hardware_spi hardware_gpio pico_multicore hardware_pio)
    pico_enable_stdio_usb(pico_winc_combined 1)
    pico_enable_stdio_uart(pico_winc_combined 1)
    target_compile_definitions(pico_winc_combined PUBLIC PICO_WINC COMBINED_BUILD)
    pico_add_extra_outputs(pico_winc_combined)

else()
    message(FATAL_ERROR "Invalid BUILD_MODE selected: ${BUILD_MODE}")
endif()