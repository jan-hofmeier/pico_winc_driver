#ifndef WINC_SIMULATOR_APP_H
#define WINC_SIMULATOR_APP_H

#include "pico/stdlib.h"
#include "sim_log.h"

// Define WINC memory size
// Reduced to fit in RAM with Wi-Fi stack
#define WINC_MEM_SIZE (1024 * 64)

// Shared memory region for HIF communication
#define SHARED_MEM_BASE 0x03658c

// WINC1500 SPI commands (simplified for simulator)
#define CMD_SINGLE_READ         0xca
#define CMD_SINGLE_WRITE        0xc9
#define CMD_INTERNAL_READ       0xc4
#define CMD_INTERNAL_WRITE      0xc3
#define CMD_DMA_WRITE           0xc1
#define CMD_DMA_READ            0xc2
#define CMD_DMA_EXT_WRITE       0xc7
#define CMD_DMA_EXT_READ        0xc8
#define CMD_RESET               0xcf

// Simulator states
typedef enum {
    SIM_STATE_IDLE,
    SIM_STATE_SENDING_DATA_RSP, // New state for sending data response header
    SIM_STATE_SENDING_DATA
} simulator_state_t;

extern uint8_t winc_memory[WINC_MEM_SIZE];
extern simulator_state_t simulator_current_state;
extern uint8_t simulator_response_buffer[4];
extern uint8_t simulator_response_index;

int winc_simulator_app_main(void);

void winc_spi_interrupt_handler(void);

uint8_t* get_memory_ptr(uint32_t addr, uint32_t size);
void winc_creg_write(uint32_t addr, uint32_t value);



#endif // WINC_SIMULATOR_APP_H
