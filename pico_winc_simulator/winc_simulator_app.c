#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h" // Added for set_sys_clock_khz
#include "winc1500_registers.h"
#include "winc_simulator_app.h"
#include "pio_spi.h"



// Global WINC memory simulation
uint8_t winc_memory[WINC_MEM_SIZE];
uint8_t clockless_regs[256];
uint8_t periph_regs[4096];
uint8_t spi_regs[256];

// CRC state
bool crc_off = false;

// Function to get a pointer to the correct memory location
uint8_t* get_memory_ptr(uint32_t addr) {
    if (addr <= 0xff) {
        return &clockless_regs[addr];
    } else if (addr >= 0x1000 && addr < 0x2000) {
        return &periph_regs[addr - 0x1000];
    } else if (addr >= 0xe800 && addr < 0xe900) {
        return &spi_regs[addr - 0xe800];
    }
    return &winc_memory[addr];
}

// Function to write 2 dummy CRC bytes if CRC is not off (used for read responses)
void spi_write_dummy_crc_if_needed_on_read() {
    if (!crc_off) {
        uint8_t dummy_crc[2] = {0};
        pio_spi_write_blocking(dummy_crc, 2);
    }
}

// Function to read and discard 2 dummy CRC bytes if CRC is not off (used for write commands)
void spi_read_dummy_crc_if_needed_on_write() {
    if (!crc_off) {
        uint8_t dummy_crc[2] = {0};
        pio_spi_read_blocking(dummy_crc, 2);
    }
}

// Function to send data with CRC (if CRC is not off)
void spi_send_data_with_crc(uint8_t *data, uint32_t size) {
    pio_spi_write_blocking(data, size);
    spi_write_dummy_crc_if_needed_on_read();
}

// Function to receive data with CRC (if CRC is not off)
void spi_receive_data_and_crc(uint8_t *data, uint32_t size) {
    pio_spi_read_blocking(data, size);
    spi_read_dummy_crc_if_needed_on_write();
}

void handle_spi_transaction() {
    uint8_t command;
    uint8_t cmd_buf[8]; // Buffer to hold command and address commanded by the host
    uint8_t data_buf[4]; // Buffer for read/write data in the host command
    uint8_t response_buf[5]; // Buffer for command echo + 4 bytes data/status

    // // Wait for the command byte
    do {
        pio_spi_read_blocking(&command, 1); // Read into 'command'
    } while(!command); // ignore zero bytes.

    response_buf[0] = command; // Prepend command to response buffer

    switch(command) {
        case CMD_SINGLE_READ: {
            // Read 3-byte address and 1 byte crc
            pio_spi_read_blocking(cmd_buf + 1, crc_off ? 3 : 4);
            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];

            if (addr > WINC_MEM_SIZE - 4 && (addr < 0x1000 || addr >= 0x2000) && (addr < 0xe800 || addr >= 0xe900)) {
                SIM_LOG(SIM_LOG_TYPE_COMMAND, "SINGLE_READ OOB", addr, 0);
                response_buf[1] = 0xFF; // Respond with status byte (error)
                pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
                break;
            }

            // Send command echo, status byte, and 0xF3 prefix
            uint8_t single_read_prefix[3] = {command, 0x00, 0xF3};
            pio_spi_write_blocking(single_read_prefix, 3);

            // Send data
            spi_send_data_with_crc(get_memory_ptr(addr), 4);

            SIM_LOG(SIM_LOG_TYPE_COMMAND, "SINGLE_READ", addr, *(uint32_t*)(get_memory_ptr(addr)));
            break;
        }
        case CMD_SINGLE_WRITE: {
            // Read 3-byte address and 4-byte data and 1 byte crc
            pio_spi_read_blocking(cmd_buf + 1, crc_off ? 7 : 8);
            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];

            if (addr > WINC_MEM_SIZE - 4 && (addr < 0x1000 || addr >= 0x2000) && (addr < 0xe800 || addr >= 0xe900)) {
                SIM_LOG(SIM_LOG_TYPE_COMMAND, "SINGLE_WRITE OOB", addr, *(uint32_t*)(cmd_buf + 4));
                response_buf[1] = 0xFF; // Respond with status byte (error)
                pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
                break;
            }
            
            memcpy(get_memory_ptr(addr), cmd_buf + 4, 4);
            response_buf[1] = 0x00; // Respond with status byte (0x00 for success)
            pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
            SIM_LOG(SIM_LOG_TYPE_COMMAND, "SINGLE_WRITE", addr, *(uint32_t*)(cmd_buf + 4));
            break;
        }
        case CMD_INTERNAL_READ: {
            // Read 2-byte address and 1 byte dummy and 1 byte crc
            pio_spi_read_blocking(cmd_buf + 1, crc_off ? 3 : 4);
            uint32_t addr = (cmd_buf[1] << 8) | (cmd_buf[2]);
            if(cmd_buf[1] & 0x80) addr &= ~0x8000;


            if (addr > 0xff && (addr < 0x1000 || addr >= 0x2000) && (addr < 0xe800 || addr >= 0xe900)) {
                SIM_LOG(SIM_LOG_TYPE_COMMAND, "INTERNAL_READ OOB", addr, 0);
                response_buf[1] = 0xFF; // Respond with status byte (error)
                pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
                break;
            }

            // Send command echo, status byte, and 0xF3 prefix
            uint8_t internal_read_prefix[3] = {command, 0x00, 0xF3};
            pio_spi_write_blocking(internal_read_prefix, 3);

            // Send data
            spi_send_data_with_crc(get_memory_ptr(addr), 4);
            SIM_LOG(SIM_LOG_TYPE_COMMAND, "INTERNAL_READ", addr, *(uint32_t*)(get_memory_ptr(addr)));
            break;
        }
        case CMD_INTERNAL_WRITE: {
            // Read 2-byte address and 4-byte data and 1 byte crc
            pio_spi_read_blocking(cmd_buf + 1, crc_off ? 6 : 7);
            uint32_t addr = (cmd_buf[1] << 8) | (cmd_buf[2]);
            if(cmd_buf[1] & 0x80) addr &= ~0x8000;
            uint32_t data_val = (cmd_buf[3] << 24) | (cmd_buf[4] << 16) | (cmd_buf[5] << 8) | cmd_buf[6];

            if (addr == NMI_SPI_PROTOCOL_CONFIG) {
                if ((data_val & 0xc) == 0) {
                    crc_off = true;
                    SIM_LOG(SIM_LOG_TYPE_COMMAND, "CRC turned off", 0, 0);
                }
            }

            if (addr > 0xff && (addr < 0x1000 || addr >= 0x2000) && (addr < 0xe800 || addr >= 0xe900)) {
                SIM_LOG(SIM_LOG_TYPE_COMMAND, "INTERNAL_WRITE OOB", addr, data_val);
                response_buf[1] = 0xFF; // Respond with status byte (error)
                pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
                break;
            }

            memcpy(get_memory_ptr(addr), cmd_buf + 3, 4);
            response_buf[1] = 0x00; // Respond with status byte (0x00 for success)
            pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
            SIM_LOG(SIM_LOG_TYPE_COMMAND, "INTERNAL_WRITE", addr, data_val);
            break;
        }
        case CMD_DMA_READ:
        case CMD_DMA_EXT_READ: {
            // Read 3-byte address and 2-byte size (for CMD_DMA_READ) or 3-byte size (for CMD_DMA_EXT_READ)
            uint8_t addr_size_bytes = (command == CMD_DMA_READ) ? (crc_off ? 5 : 6) : (crc_off ? 6 : 7);
            pio_spi_read_blocking(cmd_buf + 1, addr_size_bytes);

            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];
            uint32_t size;
            if (command == CMD_DMA_READ) {
                size = (cmd_buf[4] << 8) | cmd_buf[5];
            } else { // CMD_DMA_EXT_READ
                size = (cmd_buf[4] << 16) | (cmd_buf[5] << 8) | cmd_buf[6];
            }

            if (addr > WINC_MEM_SIZE || (addr + size) > WINC_MEM_SIZE) {
                SIM_LOG(SIM_LOG_TYPE_COMMAND, (command == CMD_DMA_READ) ? "DMA_READ OOB" : "DMA_EXT_READ OOB", addr, size);
                response_buf[1] = 0xFF; // Respond with status byte (error)
                pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
                break;
            }

            // Send command echo, status byte, and 0xF3 prefix
            uint8_t dma_read_prefix[3] = {command, 0x00, 0xF3};
            pio_spi_write_blocking(dma_read_prefix, 3);

            // Send data
            spi_send_data_with_crc(get_memory_ptr(addr), size);

            SIM_LOG(SIM_LOG_TYPE_COMMAND, (command == CMD_DMA_READ) ? "DMA_READ" : "DMA_EXT_READ", addr, size);
            break;
        }
        case CMD_DMA_WRITE:
        case CMD_DMA_EXT_WRITE: {
            // Read 3-byte address and 2-byte size (for CMD_DMA_WRITE) or 3-byte size (for CMD_DMA_EXT_WRITE)
            uint8_t addr_size_bytes = (command == CMD_DMA_WRITE) ? (crc_off ? 5 : 6) : (crc_off ? 6 : 7);
            pio_spi_read_blocking(cmd_buf + 1, addr_size_bytes);

            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];
            uint32_t size;
            if (command == CMD_DMA_WRITE) {
                size = (cmd_buf[4] << 8) | cmd_buf[5];
            } else { // CMD_DMA_EXT_WRITE
                size = (cmd_buf[4] << 16) | (cmd_buf[5] << 8) | cmd_buf[6];
            }

            // Read and discard the 0xF3 prefix from the host
            uint8_t prefix_byte;
            pio_spi_read_blocking(&prefix_byte, 1);
            // Optionally, log if prefix_byte is not 0xF3 for debugging

            if (addr > WINC_MEM_SIZE || (addr + size) > WINC_MEM_SIZE) {
                SIM_LOG(SIM_LOG_TYPE_COMMAND, (command == CMD_DMA_WRITE) ? "DMA_WRITE OOB" : "DMA_EXT_WRITE OOB", addr, size);
                // Consume the data from the host
                uint8_t dummy_buf[256];
                for (int i = 0; i < size; i += sizeof(dummy_buf)) {
                    pio_spi_read_blocking(dummy_buf, (size - i > sizeof(dummy_buf)) ? sizeof(dummy_buf) : (size - i));
                }
                spi_read_dummy_crc_if_needed_on_write();
                response_buf[1] = 0xFF; // Respond with status byte (error)
                pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
                break;
            }

            // Read data and write to memory
            spi_receive_data_and_crc(get_memory_ptr(addr), size);
            
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
    memset(clockless_regs, 0, sizeof(clockless_regs));
    memset(periph_regs, 0, sizeof(periph_regs));
    memset(spi_regs, 0, sizeof(spi_regs));

    // Pre-populate some read-only registers with default values
    uint32_t chip_id = 0x1002a0;
    memcpy(get_memory_ptr(CHIPID), &chip_id, sizeof(chip_id));

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
