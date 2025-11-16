#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "winc1500_registers.h"
#include "../config/conf_simulator.h"
#include "winc_simulator_app.h"

// WINC1500 Memory Simulation (256KB)
#define WINC_MEM_SIZE (64 * 1024)
uint8_t winc_memory[WINC_MEM_SIZE];

// SPI Command Codes from the documentation
#define CMD_DMA_WRITE      0xc1
#define CMD_DMA_READ       0xc2
#define CMD_INTERNAL_WRITE 0xc3
#define CMD_INTERNAL_READ  0xc4
#define CMD_TERMINATE      0xc5
#define CMD_REPEAT         0xc6
#define CMD_DMA_EXT_WRITE  0xc7
#define CMD_DMA_EXT_READ   0xc8
#define CMD_SINGLE_WRITE   0xc9
#define CMD_SINGLE_READ    0xca
#define CMD_RESET          0xcf

void handle_spi_transaction() {
    uint8_t cmd_buf[8]; // Buffer to hold command and address
    uint8_t data_buf[4]; // Buffer for read/write data

    // Wait for the command byte
    while (!spi_is_readable(SPI_PORT));
    spi_read_blocking(SPI_PORT, 0, cmd_buf, 1);
    uint8_t command = cmd_buf[0];

#if SIMULATOR_SPI_LOG_ENABLE
    printf("[SIMULATOR] Received Command: 0x%02x\n", command);
#endif

    // Echo the command byte
    spi_write_blocking(SPI_PORT, &command, 1);

    switch (command) {
        case CMD_SINGLE_READ: {
            // Read 3-byte address
            spi_read_blocking(SPI_PORT, 0, cmd_buf + 1, 3);
            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];

#if SIMULATOR_SPI_LOG_ENABLE
            printf("[SIMULATOR] SINGLE_READ Address: 0x%06x\n", addr);
#endif

            if (addr < WINC_MEM_SIZE - 4) {
                memcpy(data_buf, &winc_memory[addr], 4);
            } else {
                // Address out of bounds, return zeros
                memset(data_buf, 0, 4);
            }
            spi_write_blocking(SPI_PORT, data_buf, 4);

#if SIMULATOR_SPI_LOG_ENABLE
            printf("[SIMULATOR] SINGLE_READ Data: %02x %02x %02x %02x\n", data_buf[0], data_buf[1], data_buf[2], data_buf[3]);
#endif
            break;
        }
        case CMD_SINGLE_WRITE: {
            // Read 3-byte address and 4-byte data
            spi_read_blocking(SPI_PORT, 0, cmd_buf + 1, 7);
            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];

#if SIMULATOR_SPI_LOG_ENABLE
            printf("[SIMULATOR] SINGLE_WRITE Address: 0x%06x, Data: %02x %02x %02x %02x\n", addr, cmd_buf[4], cmd_buf[5], cmd_buf[6], cmd_buf[7]);
#endif

            if (addr < WINC_MEM_SIZE - 4) {
                memcpy(&winc_memory[addr], cmd_buf + 4, 4);
            }
            // Respond with status byte (0x00 for success)
            uint8_t status = 0x00;
            spi_write_blocking(SPI_PORT, &status, 1);
            break;
        }
        case CMD_INTERNAL_READ: {
            // Read 2-byte address
            spi_read_blocking(SPI_PORT, 0, cmd_buf + 1, 2);
            uint32_t addr = (cmd_buf[1] << 8) | cmd_buf[2];

#if SIMULATOR_SPI_LOG_ENABLE
            printf("[SIMULATOR] INTERNAL_READ Address: 0x%04x\n", addr);
#endif

            if (addr < WINC_MEM_SIZE - 4) {
                memcpy(data_buf, &winc_memory[addr], 4);
            } else {
                memset(data_buf, 0, 4);
            }
            spi_write_blocking(SPI_PORT, data_buf, 4);

#if SIMULATOR_SPI_LOG_ENABLE
            printf("[SIMULATOR] INTERNAL_READ Data: %02x %02x %02x %02x\n", data_buf[0], data_buf[1], data_buf[2], data_buf[3]);
#endif
            break;
        }
        case CMD_INTERNAL_WRITE: {
            // Read 2-byte address and 4-byte data
            spi_read_blocking(SPI_PORT, 0, cmd_buf + 1, 6);
            uint32_t addr = (cmd_buf[1] << 8) | (cmd_buf[2]);

#if SIMULATOR_SPI_LOG_ENABLE
            printf("[SIMULATOR] INTERNAL_WRITE Address: 0x%04x, Data: %02x %02x %02x %02x\n", addr, cmd_buf[3], cmd_buf[4], cmd_buf[5], cmd_buf[6]);
#endif

            if (addr < WINC_MEM_SIZE - 4) {
                memcpy(&winc_memory[addr], cmd_buf + 3, 4);
            }
            uint8_t status = 0x00;
            spi_write_blocking(SPI_PORT, &status, 1);
            break;
        }
        // TODO: Implement other commands (DMA, RESET, etc.)
        default: {
#if SIMULATOR_SPI_LOG_ENABLE
            printf("[SIMULATOR] Unknown Command: 0x%02x\n", command);
#endif
            // For unknown commands, just consume a few bytes to prevent bus errors
            // and respond with a dummy status.
            uint8_t dummy[8];
            spi_read_blocking(SPI_PORT, 0, dummy, 8);
            uint8_t status = 0xFF; // Error status
            spi_write_blocking(SPI_PORT, &status, 1);
            break;
        }
    }
}


int winc_simulator_app_main() {
    stdio_init_all();

    // Initialize the memory space
    memset(winc_memory, 0, WINC_MEM_SIZE);

    // Pre-populate some read-only registers with default values
    uint32_t chip_id = 0x1002a0;
    memcpy(&winc_memory[CHIPID], &chip_id, sizeof(chip_id));

    // SPI initialization
    spi_init(SPI_PORT, 1000 * 1000); // 1 MHz clock
    spi_set_slave(SPI_PORT, true);

    // GPIO pin setup
    gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SCK_PIN,  GPIO_FUNC_SPI);
    gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(CS_PIN,   GPIO_FUNC_SPI);

    printf("Pico WINC1500 Simulator Initialized. Waiting for SPI commands.\n");

    while (true) {
        // The core logic is handled in the handler function which blocks
        // until a transaction is complete.
        handle_spi_transaction();
    }

    return 0;
}
