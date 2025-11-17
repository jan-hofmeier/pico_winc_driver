#ifndef WINC_SIMULATOR_APP_H
#define WINC_SIMULATOR_APP_H

#include <stdio.h> // Required for printf
#include "hardware/pio.h"
#include "config/conf_simulator.h" // Include for SIMULATOR_SPI_LOG_ENABLE

// Asynchronous logging buffer
#define SIM_LOG_BUFFER_SIZE 16
#define SIM_LOG_MESSAGE_STRING_LEN 28 // Max length for the log string part

// Enum to define the type of log message for formatting
typedef enum {
    SIM_LOG_TYPE_NONE,
    SIM_LOG_TYPE_COMMAND,
    SIM_LOG_TYPE_ADDRESS,
    SIM_LOG_TYPE_DATA,
    SIM_LOG_TYPE_UNKNOWN_COMMAND,
    SIM_LOG_TYPE_ADDRESS_DATA // New type for logging address and data
} sim_log_type_t;

typedef struct {
    sim_log_type_t type;
    char message_str[SIM_LOG_MESSAGE_STRING_LEN];
    uint32_t value1; // Renamed from 'value'
    uint32_t value2; // New field for second value
} sim_log_message_t;

typedef struct {
    sim_log_message_t buffer[SIM_LOG_BUFFER_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} sim_log_ring_buffer_t;

extern sim_log_ring_buffer_t g_sim_log_buffer;

#if (SIMULATOR_SPI_LOG_ENABLE == 1)
#define SIM_LOG(type, str, val1, ...) sim_log_enqueue(type, str, val1, ##__VA_ARGS__)
#else
#define SIM_LOG(type, str, val1, ...) do {} while(0)
#endif

void sim_log_enqueue(sim_log_type_t type, const char *message_str, uint32_t value1, uint32_t value2);
bool sim_log_process_one_message(void);

// Define WINC memory size
#define WINC_MEM_SIZE (1024 * 16) // 16KB for simulation

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

void pio_spi_slave_init(PIO pio, uint sm, uint offset, uint miso_pin, uint mosi_pin, uint sck_pin, uint cs_pin);
int winc_simulator_app_main(void);

#endif // WINC_SIMULATOR_APP_H