#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

#define SPI_MASTER_INSTANCE spi0
#define SPI_SLAVE_INSTANCE spi1

#define SPI_MASTER_TX_PIN 3
#define SPI_MASTER_RX_PIN 4
#define SPI_MASTER_SCK_PIN 2
#define SPI_MASTER_CS_PIN 5

#define SPI_SLAVE_TX_PIN 11
#define SPI_SLAVE_RX_PIN 12
#define SPI_SLAVE_SCK_PIN 10
#define SPI_SLAVE_CS_PIN 13

// Interrupt handler for SPI slave
void spi_slave_irq_handler() {
    // Check for a receive FIFO overflow interrupt
    if (spi_get_hw(SPI_SLAVE_INSTANCE)->mis & SPI_SSPMIS_RORMIS_BITS) {
        printf("SPI Slave FIFO Overflow!\n");
        // Clear the interrupt by writing to the Clear Register
        spi_get_hw(SPI_SLAVE_INSTANCE)->icr = SPI_SSPICR_RORIC_BITS;
    }
}

void slave_core() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    spi_init(SPI_SLAVE_INSTANCE, 1000 * 1000); // 1 MHz
    spi_set_slave(SPI_SLAVE_INSTANCE, true);
    gpio_set_function(SPI_SLAVE_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SLAVE_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SLAVE_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SLAVE_CS_PIN, GPIO_FUNC_SPI);

    // Set up the interrupt
    irq_set_exclusive_handler(SPI1_IRQ, spi_slave_irq_handler);
    // Enable the interrupt for Receive FIFO Overrun at the peripheral level
    hw_set_bits(&spi_get_hw(SPI_SLAVE_INSTANCE)->imsc, SPI_SSPIMSC_RORIM_BITS);
    // Enable the SPI1 IRQ
    irq_set_enabled(SPI1_IRQ, true);

    uint8_t rx_data;
    while (true) {
        // The slave reads slowly, which will cause the master to overflow the FIFO
        spi_read_blocking(SPI_SLAVE_INSTANCE, 0, &rx_data, 1);
        printf("Slave received: %d\n", rx_data);
        gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
        sleep_ms(10);
    }
}

int main() {
    stdio_init_all();
    // Add a delay to give time to connect the serial console
    sleep_ms(2000);

    spi_init(SPI_MASTER_INSTANCE, 1000 * 1000); // 1 MHz
    gpio_set_function(SPI_MASTER_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MASTER_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MASTER_TX_PIN, GPIO_FUNC_SPI);

    gpio_init(SPI_MASTER_CS_PIN);
    gpio_set_dir(SPI_MASTER_CS_PIN, GPIO_OUT);
    gpio_put(SPI_MASTER_CS_PIN, 1);

    multicore_launch_core1(slave_core);

    // The master will send a burst of data to cause a FIFO overflow on the slave
    uint8_t tx_data[10];
    for (int i = 0; i < 10; i++) {
        tx_data[i] = i;
    }

    while (true) {
        gpio_put(SPI_MASTER_CS_PIN, 0);
        spi_write_blocking(SPI_MASTER_INSTANCE, tx_data, sizeof(tx_data));
        gpio_put(SPI_MASTER_CS_PIN, 1);
        printf("Master sent burst of %zu bytes\n", sizeof(tx_data));
        sleep_ms(1000);
    }

    return 0;
}
