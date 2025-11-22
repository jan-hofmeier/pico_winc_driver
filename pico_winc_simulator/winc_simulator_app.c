#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h" // Added for set_sys_clock_khz
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "winc1500_registers.h"
#include "winc_simulator_app.h"
#include "pio_spi.h"

#define MAX_SPI_PACKET_SIZE 8192


// Global WINC memory simulation
uint8_t winc_memory[WINC_MEM_SIZE];
uint8_t clockless_regs[256];
uint8_t periph_regs[4096];
uint8_t spi_regs[256];
uint8_t bootrom_regs[256];

// CRC state
bool crc_off = false;
bool reset_triggered = false;

// Function to get a pointer to the correct memory location, with OOB checks
uint8_t* get_memory_ptr(uint32_t addr, uint32_t size) {
    if ((addr + size) <= 0x100) { // clockless_regs
        return &clockless_regs[addr];
    } else if (addr >= 0x1000 && (addr + size) <= 0x2000) { // periph_regs
        return &periph_regs[addr - 0x1000];
    } else if (addr >= 0xe800 && (addr + size) <= 0xe900) { // spi_regs
        return &spi_regs[addr - 0xe800];
    } else if (addr >= 0xc0000 && (addr + size) <= 0xc0100) { // bootrom_regs
        return &bootrom_regs[addr - 0xc0000];
    } else if ((addr + size) <= WINC_MEM_SIZE) { // winc_memory
        return &winc_memory[addr];
    }
    SIM_LOG(SIM_LOG_TYPE_COMMAND, "OOB unknown region", addr, size);
    return NULL;
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

void winc_spi_interrupt_handler(void) {
    uint8_t command = 0;
    uint8_t cmd_buf[12]; // Increased buffer size for safety
    uint8_t response_buf[5]; // Buffer for command echo + 4 bytes data/status
    
    // Wait for the command byte
    do {
        pio_spi_read_blocking(&command, 1); // Read into 'command'
    } while(!command); // ignore zero bytes. 

    response_buf[0] = command; // Prepend command to response buffer

    switch(command) {
        case CMD_SINGLE_READ: {
            // Read 3-byte address and 1 byte crc
            pio_spi_read_blocking(cmd_buf + 1, crc_off ? 3 : 4);
            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];
            uint8_t *mem_ptr = get_memory_ptr(addr, 4);

            if (mem_ptr == NULL) {
                response_buf[1] = 0xFF; // Respond with status byte (error)
                pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
                break;
            }

            if (addr == NMI_STATE_REG && reset_triggered) {
                uint32_t state_reg = 0x02532636;
                // Send command echo, status byte, and 0xF3 prefix
                uint8_t single_read_prefix[3] = {command, 0x00, 0xF3};
                pio_spi_write_blocking(single_read_prefix, 3);
                // Send data
                spi_send_data_with_crc((uint8_t*)&state_reg, 4);
                SIM_LOG(SIM_LOG_TYPE_COMMAND, "SINGLE_READ (reset)", addr, state_reg);
                reset_triggered = false; // Clear the flag
                break;
            }

            // Send command echo, status byte, and 0xF3 prefix
            uint8_t single_read_prefix[3] = {command, 0x00, 0xF3};
            pio_spi_write_blocking(single_read_prefix, 3);

            // Send data
            spi_send_data_with_crc(mem_ptr, 4);

            SIM_LOG(SIM_LOG_TYPE_COMMAND, "SINGLE_READ", addr, *(uint32_t*)(mem_ptr));
            break;
        }
        case CMD_SINGLE_WRITE: {
            // Read 3-byte address and 4-byte data and 1 byte crc
            pio_spi_read_blocking(cmd_buf + 1, crc_off ? 7 : 8);
            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];
            uint32_t data_val = (cmd_buf[4] << 24) | (cmd_buf[5] << 16) | (cmd_buf[6] << 8) | cmd_buf[7];

            if (addr == CHIPID) {
                SIM_LOG(SIM_LOG_TYPE_COMMAND, "Software Reset", 0, 0);
                reset_triggered = true;
                // Don't actually write to CHIPID
                response_buf[1] = 0x00; // Respond with status byte (0x00 for success)
                pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
                break;
            }
            
            uint8_t *mem_ptr = get_memory_ptr(addr, 4);
            if(mem_ptr == NULL) {
                response_buf[1] = 0xFF; // Respond with status byte (error)
                pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
                break;
            }
            
            memcpy(mem_ptr, &data_val, 4);

            response_buf[1] = 0x00; // Respond with status byte (0x00 for success)
            pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
            SIM_LOG(SIM_LOG_TYPE_COMMAND, "SINGLE_WRITE", addr, data_val);
            break;
        }
        case CMD_INTERNAL_READ: {
            // Read 2-byte address and 1 byte dummy and 1 byte crc
            pio_spi_read_blocking(cmd_buf + 1, crc_off ? 3 : 4);
            uint32_t addr = (cmd_buf[1] << 8) | cmd_buf[2];
            if(cmd_buf[1] & 0x80) addr &= ~0x8000;

            uint8_t *mem_ptr = get_memory_ptr(addr, 4);
            if(mem_ptr == NULL) {
                response_buf[1] = 0xFF; // Respond with status byte (error)
                pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
                break;
            }

            // Send command echo, status byte, and 0xF3 prefix
            uint8_t internal_read_prefix[3] = {command, 0x00, 0xF3};
            pio_spi_write_blocking(internal_read_prefix, 3);

            // Send data
            spi_send_data_with_crc(mem_ptr, 4);
            SIM_LOG(SIM_LOG_TYPE_COMMAND, "INTERNAL_READ", addr, *(uint32_t*)(mem_ptr));
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
            
            uint8_t *mem_ptr = get_memory_ptr(addr, 4);
            if(mem_ptr == NULL) {
                response_buf[1] = 0xFF; // Respond with status byte (error)
                pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
                break;
            }
            
            memcpy(mem_ptr, &data_val, 4);
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
            uint32_t total_size;
            if (command == CMD_DMA_READ) {
                total_size = (cmd_buf[4] << 8) | cmd_buf[5];
            } else { // CMD_DMA_EXT_READ
                total_size = (cmd_buf[4] << 16) | (cmd_buf[5] << 8) | cmd_buf[6];
            }

            uint8_t *mem_ptr = get_memory_ptr(addr, total_size);
            if(mem_ptr == NULL) {
                response_buf[1] = 0xFF; // Respond with status byte (error)
                pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
                SIM_LOG(SIM_LOG_TYPE_COMMAND, "OOB DMA read", addr, total_size);
                break;
            }

            // Send command echo and status byte
            response_buf[1] = 0x00;
            pio_spi_write_blocking(response_buf, 2);

            uint32_t remaining_size = total_size;
            uint32_t bytes_read = 0;

            while (remaining_size > 0) {
                uint32_t chunk_size = (remaining_size > MAX_SPI_PACKET_SIZE) ? MAX_SPI_PACKET_SIZE : remaining_size;
                
                uint8_t prefix_byte = remaining_size <= MAX_SPI_PACKET_SIZE ? 0xF3 : bytes_read ? 0xF2 : 0xF1;
                pio_spi_write_blocking(&prefix_byte, 1);
                
                // Send data from memory
                spi_send_data_with_crc(mem_ptr + bytes_read, chunk_size);
                
                remaining_size -= chunk_size;
                bytes_read += chunk_size;
            }

            SIM_LOG(SIM_LOG_TYPE_COMMAND, (command == CMD_DMA_READ) ? "DMA_READ" : "DMA_EXT_READ", addr, total_size);
            break;
        }
        case CMD_DMA_WRITE:
        case CMD_DMA_EXT_WRITE: {
            // Read 3-byte address and 2-byte size (for CMD_DMA_WRITE) or 3-byte size (for CMD_DMA_EXT_WRITE)
            uint8_t addr_size_bytes = (command == CMD_DMA_WRITE) ? (crc_off ? 5 : 6) : (crc_off ? 6 : 7);
            pio_spi_read_blocking(cmd_buf + 1, addr_size_bytes);

            uint32_t addr = (cmd_buf[1] << 16) | (cmd_buf[2] << 8) | cmd_buf[3];
            uint32_t total_size;
            if (command == CMD_DMA_WRITE) {
                total_size = (cmd_buf[4] << 8) | cmd_buf[5];
            } else { // CMD_DMA_EXT_WRITE
                total_size = (cmd_buf[4] << 16) | (cmd_buf[5] << 8) | cmd_buf[6];
            }

            uint32_t remaining_size = total_size;
            uint32_t bytes_written = 0;
            bool oob_error = false;

            while (remaining_size > 0) {
                uint8_t prefix_byte;
                do {
                    pio_spi_read_blocking(&prefix_byte, 1);
                } while (prefix_byte == 0);

                if ((prefix_byte & 0xF0) != 0xF0) {
                    SIM_LOG(SIM_LOG_TYPE_COMMAND, "Unexpected DMA prefix", prefix_byte, 0);
                    // The host driver seems to continue even if the prefix is wrong.
                    // We will do the same and just log it.
                }

                uint32_t chunk_size = (remaining_size > MAX_SPI_PACKET_SIZE) ? MAX_SPI_PACKET_SIZE : remaining_size;

                uint8_t* mem_ptr = get_memory_ptr(addr + bytes_written, chunk_size);
                if (mem_ptr == NULL || oob_error) {
                    if (!oob_error) { // Log only on the first OOB occurrence
                        SIM_LOG(SIM_LOG_TYPE_COMMAND, "OOB DMA write", addr + bytes_written, chunk_size);
                        oob_error = true;
                    }
                    // Discard the chunk
                    uint8_t dummy_buf[256];
                    uint32_t discard_remaining = chunk_size;
                    while(discard_remaining > 0) {
                        uint32_t read_size = (discard_remaining > sizeof(dummy_buf)) ? sizeof(dummy_buf) : discard_remaining;
                        pio_spi_read_blocking(dummy_buf, read_size);
                        discard_remaining -= read_size;
                    }
                } else {
                    // Read data into memory
                    pio_spi_read_blocking(mem_ptr, chunk_size);
                }

                // After every chunk, there is a CRC
                spi_read_dummy_crc_if_needed_on_write();
                
                remaining_size -= chunk_size;
                bytes_written += chunk_size;
            }

            // After the loop, send the response
            if (oob_error) {
                response_buf[1] = 0xFF; // Error
            } else {
                response_buf[1] = 0x00; // Success
            }
            pio_spi_write_blocking(response_buf, 2);

            SIM_LOG(SIM_LOG_TYPE_COMMAND, (command == CMD_DMA_WRITE) ? "DMA_WRITE" : "DMA_EXT_WRITE", addr, total_size);
            break;
        }
        case CMD_RESET: {
            SIM_LOG(SIM_LOG_TYPE_COMMAND, "CMD_RESET", 0, 0);
            response_buf[1] = 0x00; // Respond with status byte (0x00 for success)
            pio_spi_write_blocking(response_buf, 2); // Write command + 1 byte status
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
    memset(bootrom_regs, 0, sizeof(bootrom_regs));

    // Pre-populate some read-only registers with default values
    uint32_t chip_id = 0x1002b0;
    memcpy(get_memory_ptr(CHIPID, 4), &chip_id, sizeof(chip_id));
    uint32_t rev_id = 0x4;
    memcpy(get_memory_ptr(0x13f4, 4), &rev_id, sizeof(rev_id));
    uint32_t proto_conf = 0x2E;
    memcpy(get_memory_ptr(NMI_SPI_PROTOCOL_CONFIG, 4), &proto_conf, sizeof(proto_conf));
    uint32_t state_reg = 0x02532636;
    memcpy(get_memory_ptr(NMI_STATE_REG, 4), &state_reg, sizeof(state_reg));
    uint32_t wait_for_host = 0x3f00;
    memcpy(get_memory_ptr(M2M_WAIT_FOR_HOST_REG, 4), &wait_for_host, sizeof(wait_for_host));
    uint32_t reg_1014 = 0x807c082d;
    memcpy(get_memory_ptr(0x1014, 4), &reg_1014, sizeof(reg_1014));
    uint32_t bootrom_reg = 0x10add09e;
    memcpy(get_memory_ptr(BOOTROM_REG, 4), &bootrom_reg, sizeof(bootrom_reg));
    uint32_t pin_mux_0 = 0x31111044;
    memcpy(get_memory_ptr(NMI_PIN_MUX_0, 4), &pin_mux_0, sizeof(pin_mux_0));
    uint32_t rev_reg = 0x1330134a;
    memcpy(get_memory_ptr(NMI_REV_REG, 4), &rev_reg, sizeof(rev_reg));

    pio_spi_slave_init(winc_spi_interrupt_handler);

    printf("Pico WINC1500 Simulator Initialized. Waiting for SPI commands.\n");

    // We will now be interrupt driven. The main loop can just sleep.
    while (true) {
        __wfi(); // Wait for interrupt
        printf("Woke up from interrupt\n");
    }

    return 0;
}
