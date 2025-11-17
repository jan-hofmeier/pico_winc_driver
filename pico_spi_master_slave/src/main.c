#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

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

void slave_core() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    spi_init(SPI_SLAVE_INSTANCE, 1000 * 1000); // 1 MHz
    spi_set_slave(SPI_SLAVE_INSTANCE, true);
    gpio_set_function(SPI_SLAVE_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SLAVE_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SLAVE_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SLAVE_CS_PIN, GPIO_FUNC_SPI);

    uint8_t rx_data;
    while (true) {
        spi_read_blocking(SPI_SLAVE_INSTANCE, 0, &rx_data, 1);
        printf("Slave received: %d\n", rx_data);
        gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
    }
}

int main() {
    stdio_init_all();

    spi_init(SPI_MASTER_INSTANCE, 1000 * 1000); // 1 MHz
    gpio_set_function(SPI_MASTER_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MASTER_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MASTER_TX_PIN, GPIO_FUNC_SPI);

    gpio_init(SPI_MASTER_CS_PIN);
    gpio_set_dir(SPI_MASTER_CS_PIN, GPIO_OUT);
    gpio_put(SPI_MASTER_CS_PIN, 1);

    multicore_launch_core1(slave_core);

    uint8_t tx_data = 0;
    while (true) {
        gpio_put(SPI_MASTER_CS_PIN, 0);
        spi_write_blocking(SPI_MASTER_INSTANCE, &tx_data, 1);
        gpio_put(SPI_MASTER_CS_PIN, 1);
        printf("Master sent: %d\n", tx_data);
        tx_data++;
        sleep_ms(1000);
    }

    return 0;
}
