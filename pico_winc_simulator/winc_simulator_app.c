#include <stdio.h>
#include <string.h>
#include <stdarg.h> // Required for va_list
#include "pico/stdlib.h"
#include "hardware/clocks.h" // Added for set_sys_clock_khz
#include "winc1500_registers.h"
#include "../config/conf_simulator.h"
#include "winc_simulator_app.h"
#include "pio_spi.h"



// Global WINC memory simulation
uint8_t winc_memory[WINC_MEM_SIZE];

// Global log buffer instance
sim_log_ring_buffer_t g_sim_log_buffer = {0};

// Function to enqueue a log message
void sim_log_enqueue(sim_log_type_t type, const char *message_str, uint32_t value1, uint32_t value2) {
    if (g_sim_log_buffer.count < SIM_LOG_BUFFER_SIZE) {
        sim_log_message_t *msg = &g_sim_log_buffer.buffer[g_sim_log_buffer.head];
        msg->type = type;
        strncpy(msg->message_str, message_str, SIM_LOG_MESSAGE_STRING_LEN - 1);
        msg->message_str[SIM_LOG_MESSAGE_STRING_LEN - 1] = '\0'; // Ensure null termination
        msg->value1 = value1;
        msg->value2 = value2;

        g_sim_log_buffer.head = (g_sim_log_buffer.head + 1) % SIM_LOG_BUFFER_SIZE;
        g_sim_log_buffer.count++;
    }
}

// Function to process (print) one log message from the queue
bool sim_log_process_one_message(void) {
    if (g_sim_log_buffer.count > 0) {
        sim_log_message_t *msg = &g_sim_log_buffer.buffer[g_sim_log_buffer.tail];

        switch (msg->type) {
            case SIM_LOG_TYPE_COMMAND:
                printf("[SIMULATOR] %s: 0x%X -> 0x%X\n", msg->message_str, msg->value1, msg->value2);
                break;
            case SIM_LOG_TYPE_ADDRESS:
                printf("[SIMULATOR] %s: 0x%06x\n", msg->message_str, msg->value1);
                break;
            case SIM_LOG_TYPE_DATA:
                printf("[SIMULATOR] %s: %02x %02x %02x %02x\n", msg->message_str,
                       (uint8_t)(msg->value1 >> 24), (uint8_t)(msg->value1 >> 16),
                       (uint8_t)(msg->value1 >> 8), (uint8_t)msg->value1);
                break;
            case SIM_LOG_TYPE_ADDRESS_DATA:
                printf("[SIMULATOR] %s Address: 0x%06x Data: %02x %02x %02x %02x\n", msg->message_str, msg->value1,
                       (uint8_t)(msg->value2 >> 24), (uint8_t)(msg->value2 >> 16),
                       (uint8_t)(msg->value2 >> 8), (uint8_t)msg->value2);
                break;
            case SIM_LOG_TYPE_UNKNOWN_COMMAND:
                printf("[SIMULATOR] %s: 0x%02x\n", msg->message_str, (uint8_t)msg->value1);
                break;
            case SIM_LOG_TYPE_NONE:
            default:
                printf("[SIMULATOR] %s\n", msg->message_str);
                break;
        }

        g_sim_log_buffer.tail = (g_sim_log_buffer.tail + 1) % SIM_LOG_BUFFER_SIZE;
        g_sim_log_buffer.count--;
        return true;
    }
    return false;
}



void handle_spi_transaction() {
    uint8_t command;
    uint8_t cmd_buf[8]; // Buffer to hold command and address commanded by the host
    uint8_t data_buf[4]; // Buffer for read/write data in the host command
    uint8_t response_buf[5]; // Buffer for command echo + 4 bytes data/status

    // uint8_t buff[16];
    // int read = spi_read_blocking(0, buff, sizeof(buff)); // Read into 'command'
    // printf("SIM SPI read %d bytes:");
    // for(int i=0; i<read; i++)  {
    //     printf(" %02X", buff[i]);
    // }
    // printf("\n");

    // // Wait for the command byte
    do {
        pio_spi_read_blocking(&command, 1); // Read into 'command'
    } while(!command); // ignore zero bytes.

    // spi_read_blocking(0, cmd_buf, 4);
    // printf("SIM SPI READ 0x%X: %08X\n", command, *(uint32_t*)cmd_buf);

    response_buf[0] = command; // Prepend command to response buffer

    switch(command) {
        case CMD_SINGLE_READ: {
            // Read 3-byte address
            pio_spi_read_blocking(cmd_buf + 1, 3);
            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];

            // Send command echo, status byte, and 0xF3 prefix
            uint8_t single_read_prefix[3] = {command, 0x00, 0xF3};
            pio_spi_write_blocking(single_read_prefix, 3);

            // Send data
            if (addr < WINC_MEM_SIZE - 4) {
                pio_spi_write_blocking(&winc_memory[addr], 4);
            } else {
                // Address out of bounds, return zeros
                uint8_t zero_buf[4] = {0};
                pio_spi_write_blocking(zero_buf, 4);
            }
            SIM_LOG(SIM_LOG_TYPE_COMMAND, "SINGLE_READ", addr, *(uint32_t*)(winc_memory+addr));
            break;
        }
        case CMD_SINGLE_WRITE: {
            // Read 3-byte address and 4-byte data
            pio_spi_read_blocking(cmd_buf + 1, 7);
            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];
            uint32_t data_val = (cmd_buf[4] << 24) | (cmd_buf[5] << 16) | (cmd_buf[6] << 8) | cmd_buf[7];

            if (addr < WINC_MEM_SIZE - 4) {
                memcpy(&winc_memory[addr], cmd_buf + 4, 4);
            }
            response_buf[1] = 0x00; // Respond with status byte (0x00 for success)
            pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
            SIM_LOG(SIM_LOG_TYPE_COMMAND, "SINGLE_WRITE", addr, *(uint32_t*)(cmd_buf + 4));
            break;
        }
        case CMD_INTERNAL_READ: {
            // Read 2-byte address
            pio_spi_read_blocking(cmd_buf + 1, 2);
            uint32_t addr = (cmd_buf[1] << 8) | (cmd_buf[2]);

            // Send command echo, status byte, and 0xF3 prefix
            uint8_t internal_read_prefix[3] = {command, 0x00, 0xF3};
            pio_spi_write_blocking(internal_read_prefix, 3);

            // Send data
            if (addr < WINC_MEM_SIZE - 4) {
                pio_spi_write_blocking(&winc_memory[addr], 4);
            }
            else {
                uint8_t zero_buf[4] = {0};
                pio_spi_write_blocking(zero_buf, 4);
            }
            SIM_LOG(SIM_LOG_TYPE_COMMAND, "INTERNAL_READ", addr, *(uint32_t*)(winc_memory+addr));
            break;
        }
        case CMD_INTERNAL_WRITE: {
            // Read 2-byte address and 4-byte data
            pio_spi_read_blocking(cmd_buf + 1, 6);
            uint32_t addr = (cmd_buf[1] << 8) | (cmd_buf[2]);
            uint32_t data_val = (cmd_buf[3] << 24) | (cmd_buf[4] << 16) | (cmd_buf[5] << 8) | cmd_buf[6];

            if (addr < WINC_MEM_SIZE - 4) {
                memcpy(&winc_memory[addr], cmd_buf + 3, 4);
            }
            response_buf[1] = 0x00; // Respond with status byte (0x00 for success)
            pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
            SIM_LOG(SIM_LOG_TYPE_COMMAND, "INTERNAL_WRITE", addr, *(uint32_t*)(cmd_buf + 3));
            break;
        }
        case CMD_DMA_READ:
        case CMD_DMA_EXT_READ: {
            // Read 3-byte address and 2-byte size (for CMD_DMA_READ) or 3-byte size (for CMD_DMA_EXT_READ)
            uint8_t addr_size_bytes = (command == CMD_DMA_READ) ? 5 : 6;
            pio_spi_read_blocking(cmd_buf + 1, addr_size_bytes);

            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];
            uint16_t size;
            if (command == CMD_DMA_READ) {
                size = (cmd_buf[4] << 8) | cmd_buf[5];
            } else { // CMD_DMA_EXT_READ
                size = (cmd_buf[4] << 16) | (cmd_buf[5] << 8) | cmd_buf[6];
            }

            // Send command echo, status byte, and 0xF3 prefix
            uint8_t dma_read_prefix[3] = {command, 0x00, 0xF3};
            pio_spi_write_blocking(dma_read_prefix, 3);

            // Send data
            if (addr < WINC_MEM_SIZE && (addr + size) <= WINC_MEM_SIZE) {
                pio_spi_write_blocking(&winc_memory[addr], size);
            } else {
                // Address out of bounds, send zeros
                uint8_t zero_buf[256]; // Use a smaller buffer for sending zeros
                memset(zero_buf, 0, sizeof(zero_buf));
                for (int i = 0; i < size; i += sizeof(zero_buf)) {
                    pio_spi_write_blocking(zero_buf, (size - i > sizeof(zero_buf)) ? sizeof(zero_buf) : (size - i));
                }
            }
            SIM_LOG(SIM_LOG_TYPE_COMMAND, (command == CMD_DMA_READ) ? "DMA_READ" : "DMA_EXT_READ", addr, size);
            break;
        }
        case CMD_DMA_WRITE:
        case CMD_DMA_EXT_WRITE: {
            // Read 3-byte address and 2-byte size (for CMD_DMA_WRITE) or 3-byte size (for CMD_DMA_EXT_WRITE)
            uint8_t addr_size_bytes = (command == CMD_DMA_WRITE) ? 5 : 6;
            pio_spi_read_blocking(cmd_buf + 1, addr_size_bytes);

            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];
            uint16_t size;
            if (command == CMD_DMA_WRITE) {
                size = (cmd_buf[4] << 8) | cmd_buf[5];
            } else { // CMD_DMA_EXT_WRITE
                size = (cmd_buf[4] << 16) | (cmd_buf[5] << 8) | cmd_buf[6];
            }

            // Read and discard the 0xF3 prefix from the host
            uint8_t prefix_byte;
            pio_spi_read_blocking(&prefix_byte, 1);
            // Optionally, log if prefix_byte is not 0xF3 for debugging

            // Read data and write to memory
            if (addr < WINC_MEM_SIZE && (addr + size) <= WINC_MEM_SIZE) {
                pio_spi_read_blocking(&winc_memory[addr], size);
            } else {
                // Address out of bounds, just consume the data
                uint8_t dummy_buf[256];
                for (int i = 0; i < size; i += sizeof(dummy_buf)) {
                    pio_spi_read_blocking(dummy_buf, (size - i > sizeof(dummy_buf)) ? sizeof(dummy_buf) : (size - i));
                }
            }

            response_buf[1] = 0x00; // Respond with status byte (0x00 for success)
            pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
            SIM_LOG(SIM_LOG_TYPE_COMMAND, (command == CMD_DMA_WRITE) ? "DMA_WRITE" : "DMA_EXT_WRITE", addr, size);
            break;
        }
        // TODO: Implement other commands (DMA, RESET, etc.)
        default: {
            // For unknown commands, just consume a few bytes to prevent bus errors
            // and respond with a dummy status.
            uint32_t dummy;
            pio_spi_read_blocking((uint8_t*)&dummy, 8);
            response_buf[1] = 0xFF; // Error status
            pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
            SIM_LOG(SIM_LOG_TYPE_COMMAND, "Unknown Command", command, dummy);
            break;
        }
    }
}



#define LOG_PROCESS_INTERVAL 100 // Process log queue every 100 transactions

int winc_simulator_app_main() {
    set_sys_clock_khz(125000, true); // Set system clock to 125 MHz
    stdio_init_all();

    // Initialize the memory space
    memset(winc_memory, 0, WINC_MEM_SIZE);

    // Pre-populate some read-only registers with default values
    uint32_t chip_id = 0x1002a0;
    memcpy(&winc_memory[CHIPID], &chip_id, sizeof(chip_id));

    pio_spi_slave_init();

    printf("Pico WINC1500 Simulator Initialized. Waiting for SPI commands.\n");

    uint32_t transaction_count = 0;
    while (true) {
        // The core logic is handled in the handler function which blocks
        // until a transaction is complete.
        handle_spi_transaction();

        // The log processing is now handled cooperatively within the SPI bus wrapper.
        // No need for periodic processing here anymore.
    }

    return 0;
}
