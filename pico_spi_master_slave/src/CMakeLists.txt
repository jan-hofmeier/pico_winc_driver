add_executable(pico_spi_master_slave
    main.c
)

target_link_libraries(pico_spi_master_slave pico_stdlib hardware_spi hardware_gpio pico_multicore)

# enable usb output, disable uart output
pico_enable_stdio_usb(pico_spi_master_slave 1)
pico_enable_stdio_uart(pico_spi_master_slave 0)

pico_add_extra_outputs(pico_spi_master_slave)
