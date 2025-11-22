#ifndef PIO_SPI_H
#define PIO_SPI_H

#include "pico/stdlib.h"
#include "hardware/irq.h"

void pio_spi_slave_init(irq_handler_t handler);
size_t pio_spi_write_blocking(uint8_t* buffer, size_t len);
size_t pio_spi_read_blocking(uint8_t* buffer, size_t len);

// New functions for DMA support
uint pio_spi_get_rx_dreq(void);
volatile void* pio_spi_get_rx_fifo_address(void);
void pio_spi_set_rx_irq_enabled(bool enabled);

#endif // PIO_SPI_H
