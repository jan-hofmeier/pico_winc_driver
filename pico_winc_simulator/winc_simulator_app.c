#include <stdio.h>
#include <string.h>
#include <stdarg.h> // Required for va_list
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/clocks.h" // Added for set_sys_clock_khz
#include "winc1500_registers.h"
#include "../config/conf_simulator.h"
#include "winc_simulator_app.h"

// Global WINC memory simulation
uint8_t winc_memory[WINC_MEM_SIZE];

// CRC7 implementation for SPI commands
static bool g_use_crc = true;

static const uint8_t crc7_syndrome_table[256] = {
	0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f,
	0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
	0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26,
	0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
	0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d,
	0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
	0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14,
	0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
	0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b,
	0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
	0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42,
	0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
	0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69,
	0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
	0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70,
	0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
	0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e,
	0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
	0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67,
	0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
	0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c,
	0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
	0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55,
	0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
	0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a,
	0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
	0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03,
	0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
	0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28,
	0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
	0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31,
	0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
};

static inline uint8_t crc7_byte(uint8_t crc, uint8_t data) {
	return crc7_syndrome_table[(crc << 1) ^ data];
}

static inline uint8_t crc7(uint8_t crc, const uint8_t *buffer, uint32_t len) {
	while (len--)
		crc = crc7_byte(crc, *buffer++);
	return crc;
}

#include "pico/multicore.h"

// Global log buffer instance
static sim_log_ring_buffer_t g_sim_log_buffer = {0};

// Function to enqueue a log message
void sim_log_enqueue(sim_log_type_t type, const char *message_str, ...) {
    if (g_sim_log_buffer.count < SIM_LOG_BUFFER_SIZE) {
        sim_log_message_t *msg = &g_sim_log_buffer.buffer[g_sim_log_buffer.head];
        msg->type = type;

        va_list args;
        va_start(args, message_str);

        strncpy(msg->message_str, message_str, SIM_LOG_MESSAGE_STRING_LEN - 1);
        msg->message_str[SIM_LOG_MESSAGE_STRING_LEN - 1] = '\0'; // Ensure null termination

        if (type != SIM_LOG_TYPE_STRING)
        {
            msg->value1 = va_arg(args, uint32_t);
            if (type == SIM_LOG_TYPE_CMD_ADDR_DATA) {
                msg->value2 = va_arg(args, uint32_t);
            }
        }

        va_end(args);

        g_sim_log_buffer.head = (g_sim_log_buffer.head + 1) % SIM_LOG_BUFFER_SIZE;
        g_sim_log_buffer.count++;
    }
}

static bool sim_log_process_one_message(void) {
    if (g_sim_log_buffer.count > 0) {
        sim_log_message_t *msg = &g_sim_log_buffer.buffer[g_sim_log_buffer.tail];

        switch (msg->type) {
            case SIM_LOG_TYPE_CMD:
                 printf("[SIM] %s: 0x%02x\n", msg->message_str, (uint8_t)msg->value1);
                break;
            case SIM_LOG_TYPE_CMD_ADDR:
                printf("[SIM] %s: (0x%06x)\n", msg->message_str, msg->value1);
                break;
            case SIM_LOG_TYPE_CMD_DATA:
                printf("[SIM] %s: (0x%02x 0x%02x 0x%02x 0x%02x)\n", msg->message_str,
                       (uint8_t)(msg->value1 >> 24), (uint8_t)(msg->value1 >> 16),
                       (uint8_t)(msg->value1 >> 8), (uint8_t)msg->value1);
                break;
            case SIM_LOG_TYPE_CMD_ADDR_DATA:
                printf("[SIM] %s: (0x%06x) -> (0x%02x 0x%02x 0x%02x 0x%02x)\n", msg->message_str, msg->value1,
                       (uint8_t)(msg->value2 >> 24), (uint8_t)(msg->value2 >> 16),
                       (uint8_t)(msg->value2 >> 8), (uint8_t)msg->value2);
                break;
            case SIM_LOG_TYPE_UNKNOWN_CMD:
                printf("[SIM] %s: 0x%02x\n", msg->message_str, (uint8_t)msg->value1);
                break;
            case SIM_LOG_TYPE_STRING:
            default:
                printf("[SIM] %s\n", msg->message_str);
                break;
        }

        g_sim_log_buffer.tail = (g_sim_log_buffer.tail + 1) % SIM_LOG_BUFFER_SIZE;
        g_sim_log_buffer.count--;
        return true;
    }
    return false;
}

#if (SIMULATOR_LOG_CORE_ENABLE == 1)
void log_core_entry() {
    while (1) {
        if (multicore_fifo_rvalid())
        {
            multicore_fifo_pop_blocking();
            while(sim_log_process_one_message());
        }
    }
}
#endif

// Helper function to read the rest of a command and verify CRC
static bool read_command_and_verify_crc(uint8_t* cmd_buf, uint8_t params_len) {
    uint16_t read_len = params_len;
    if (g_use_crc) {
        read_len++; // Read one more byte for CRC
    }

    // Read the rest of the command (params + optional CRC)
    spi_read_blocking(SPI_PORT, 0, cmd_buf + 1, read_len);

    if (g_use_crc) {
        uint8_t received_crc = cmd_buf[params_len + 1];
        uint8_t calculated_crc = crc7(0x7f, cmd_buf, params_len + 1);
        calculated_crc = (calculated_crc << 1) | 1;

        if (calculated_crc != received_crc) {
            SIM_LOG(SIM_LOG_TYPE_STRING, "CRC mismatch! received: 0x%02x, calculated: 0x%02x", received_crc, calculated_crc);
            return false; // Indicate failure
        }
    }
    return true;
}

void handle_spi_transaction() {
    uint8_t command;
    uint8_t cmd_buf[10]; // Buffer to hold command and parameters
    uint8_t response_buf[10]; // Buffer for command echo + 4 bytes data/status
    
    // Wait for the command byte
    do {
        spi_read_blocking(SPI_PORT, 0, &command, 1); // Read into 'command'
    } while(!command); // ignore zero bytes.

    cmd_buf[0] = command;
    response_buf[0] = command; // Prepend command to response buffer

    switch(command) {
        case CMD_SINGLE_READ: {
            if (!read_command_and_verify_crc(cmd_buf, 3)) {
                // On CRC error, send zeros
                memset(response_buf + 1, 0, 4);
                spi_write_blocking(SPI_PORT, response_buf, 5);
                break;
            }
            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];

            // Send data
            response_buf[1] = 0x00; // Status OK
            response_buf[2] = 0xF3; // Prefix
            if (addr < WINC_MEM_SIZE - 4) {
                memcpy(response_buf + 3, &winc_memory[addr], 4);
            } else {
                // Address out of bounds, return zeros
                memset(response_buf + 3, 0, 4);
            }
            spi_write_blocking(SPI_PORT, response_buf, 7);
            SIM_LOG(SIM_LOG_TYPE_CMD_ADDR, "SINGLE_READ", addr);
            break;
        }
        case CMD_SINGLE_WRITE: {
            if (!read_command_and_verify_crc(cmd_buf, 7)) {
                break; // CRC error
            }
            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];
            uint32_t data_val = (cmd_buf[4] << 24) | (cmd_buf[5] << 16) | (cmd_buf[6] << 8) | cmd_buf[7];

            if (addr < WINC_MEM_SIZE - 4) {
                memcpy(&winc_memory[addr], cmd_buf + 4, 4);
            }
            SIM_LOG(SIM_LOG_TYPE_CMD_ADDR_DATA, "SINGLE_WRITE", addr, data_val);
            response_buf[1] = 0x00; // Respond with status byte (0x00 for success)
            spi_write_blocking(SPI_PORT, response_buf, 2); // Write command + 1 byte status
            break;
        }
        case CMD_INTERNAL_READ: {
             if (!read_command_and_verify_crc(cmd_buf, 3)) { // 2 addr + 1 dummy
                // On CRC error, send zeros
                memset(response_buf + 1, 0, 4);
                spi_write_blocking(SPI_PORT, response_buf, 5);
                break;
            }
            uint32_t addr = (cmd_buf[1] << 8) | (cmd_buf[2]);

            // Send data
            response_buf[1] = 0x00; // Status OK
            response_buf[2] = 0xF3; // Prefix
            if (addr < WINC_MEM_SIZE - 4) {
                memcpy(response_buf + 3, &winc_memory[addr], 4);
            }
            else {
                memset(response_buf + 3, 0, 4);
            }
            spi_write_blocking(SPI_PORT, response_buf, 7);
            SIM_LOG(SIM_LOG_TYPE_CMD_ADDR, "INTERNAL_READ", addr);
            break;
        }
        case CMD_INTERNAL_WRITE: {
            if (!read_command_and_verify_crc(cmd_buf, 6)) {
                break; // CRC error
            }
            uint32_t addr = (cmd_buf[1] << 8) | (cmd_buf[2]);
            uint32_t data_val = (cmd_buf[3] << 24) | (cmd_buf[4] << 16) | (cmd_buf[5] << 8) | cmd_buf[6];

            if (addr == NMI_SPI_PROTOCOL_CONFIG) {
                if ((data_val & 0xc) == 0) {
                    g_use_crc = false;
                    SIM_LOG(SIM_LOG_TYPE_STRING, "CRC disabled");
                } else {
                    g_use_crc = true;
                    SIM_LOG(SIM_LOG_TYPE_STRING, "CRC enabled");
                }
            }

            if (addr < WINC_MEM_SIZE - 4) {
                memcpy(&winc_memory[addr], cmd_buf + 3, 4);
            }
            SIM_LOG(SIM_LOG_TYPE_CMD_ADDR_DATA, "INTERNAL_WRITE", addr, data_val);
            response_buf[1] = 0x00; // Respond with status byte (0x00 for success)
            spi_write_blocking(SPI_PORT, response_buf, 2); // Write command + 1 byte status
            break;
        }
        case CMD_DMA_READ:
        case CMD_DMA_EXT_READ: {
            uint8_t params_len = (command == CMD_DMA_READ) ? 5 : 6;
            if (!read_command_and_verify_crc(cmd_buf, params_len)) {
                break; // CRC error
            }

            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];
            uint32_t size;
            if (command == CMD_DMA_READ) {
                size = (cmd_buf[4] << 8) | cmd_buf[5];
            } else { // CMD_DMA_EXT_READ
                size = (cmd_buf[4] << 16) | (cmd_buf[5] << 8) | cmd_buf[6];
            }
            // Send data
            response_buf[1] = 0x00; // Status OK
            response_buf[2] = 0xF3; // Prefix
            spi_write_blocking(SPI_PORT, response_buf, 3);
            if (addr < WINC_MEM_SIZE && (addr + size) <= WINC_MEM_SIZE) {
                spi_write_blocking(SPI_PORT, &winc_memory[addr], size);
            } else {
                // Address out of bounds, send zeros
                uint8_t zero_buf[256]; // Use a smaller buffer for sending zeros
                memset(zero_buf, 0, sizeof(zero_buf));
                for (int i = 0; i < size; i += sizeof(zero_buf)) {
                    spi_write_blocking(SPI_PORT, zero_buf, (size - i > sizeof(zero_buf)) ? sizeof(zero_buf) : (size - i));
                }
            }
            SIM_LOG(SIM_LOG_TYPE_CMD_ADDR, (command == CMD_DMA_READ) ? "DMA_READ" : "DMA_EXT_READ", addr);
            break;
        }
        case CMD_DMA_WRITE:
        case CMD_DMA_EXT_WRITE: {
            uint8_t params_len = (command == CMD_DMA_WRITE) ? 5 : 6;
            if (!read_command_and_verify_crc(cmd_buf, params_len)) {
                break; // CRC error
            }

            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];
            uint32_t size;
            if (command == CMD_DMA_WRITE) {
                size = (cmd_buf[4] << 8) | cmd_buf[5];
            } else { // CMD_DMA_EXT_WRITE
                size = (cmd_buf[4] << 16) | (cmd_buf[5] << 8) | cmd_buf[6];
            }

            // Read and discard the 0xF3 prefix from the host
            uint8_t prefix_byte;
            spi_read_blocking(SPI_PORT, 0, &prefix_byte, 1);
            // Optionally, log if prefix_byte is not 0xF3 for debugging

            // Read data and write to memory
            if (addr < WINC_MEM_SIZE && (addr + size) <= WINC_MEM_SIZE) {
                spi_read_blocking(SPI_PORT, 0, &winc_memory[addr], size);
            } else {
                // Address out of bounds, just consume the data
                uint8_t dummy_buf[256];
                for (int i = 0; i < size; i += sizeof(dummy_buf)) {
                    spi_read_blocking(SPI_PORT, 0, dummy_buf, (size - i > sizeof(dummy_buf)) ? sizeof(dummy_buf) : (size - i));
                }
            }

            response_buf[1] = 0x00; // Respond with status byte (0x00 for success)
            spi_write_blocking(SPI_PORT, response_buf, 2); // Write command + 1 byte status
            SIM_LOG(SIM_LOG_TYPE_CMD_ADDR, (command == CMD_DMA_WRITE) ? "DMA_WRITE" : "DMA_EXT_WRITE", addr);
            break;
        }
        case CMD_RESET: {
            if (!read_command_and_verify_crc(cmd_buf, 4)) {
                break; // CRC error
            }
            break;
        }
        // TODO: Implement other commands (TERMINATE, REPEAT)
        default: {
            // For unknown commands, just consume a few bytes to prevent bus errors
            // and respond with a dummy status.
            uint8_t dummy[8];
            spi_read_blocking(SPI_PORT, 0, dummy, 8);
            response_buf[1] = 0xFF; // Error status
            spi_write_blocking(SPI_PORT, response_buf, 2); // Write command + 1 byte status
            SIM_LOG(SIM_LOG_TYPE_UNKNOWN_CMD, "Unknown Command", command);
            break;
        }
    }
    winc_simulator_app_log();
}

void winc_simulator_app_log(void)
{
#if (SIMULATOR_LOG_CORE_ENABLE == 1)
    if (multicore_fifo_wready())
    {
        multicore_fifo_push_blocking(0);
    }
#else
    while(sim_log_process_one_message());
#endif
}

int winc_simulator_app_main() {
    set_sys_clock_khz(125000, true); // Set system clock to 125 MHz
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

#if (SIMULATOR_LOG_CORE_ENABLE == 1)
    SIM_LOG(SIM_LOG_TYPE_STRING, "Launching log core");
    winc_simulator_app_log();
    multicore_launch_core1(log_core_entry);
#endif

    while (true) {
        // The core logic is handled in the handler function which blocks
        // until a transaction is complete.
        handle_spi_transaction();
    }

    return 0;
}
