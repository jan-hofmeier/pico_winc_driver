#ifndef SIM_LOG_H
#define SIM_LOG_H

#include <stdio.h> // Required for printf
#include "config/conf_simulator.h" // Include for SIMULATOR_SPI_LOG_ENABLE

// Asynchronous logging buffer
#define SIM_LOG_BUFFER_SIZE 16
#define SIM_LOG_MESSAGE_STRING_LEN 100 // Max length for the log string part

// Enum to define the type of log message for formatting
typedef enum {
    SIM_LOG_TYPE_NONE,
    SIM_LOG_TYPE_COMMAND,
    SIM_LOG_TYPE_ADDRESS,
    SIM_LOG_TYPE_DATA,
    SIM_LOG_TYPE_UNKNOWN_COMMAND,
    SIM_LOG_TYPE_ADDRESS_DATA, // New type for logging address and data
    SIM_LOG_TYPE_INFO, // Added for informational messages
    SIM_LOG_TYPE_ERROR
} sim_log_type_t;

typedef struct {
    sim_log_type_t type;
    char message_str[SIM_LOG_MESSAGE_STRING_LEN];
    uint32_t value1; // Renamed from 'value'
    uint32_t value2; // New field for second value
} sim_log_message_t;

#if (SIMULATOR_SPI_LOG_ENABLE == 1)
#define SIM_LOG(type, str, val1, ...) sim_log_enqueue(type, str, val1, ##__VA_ARGS__)
#else
#define SIM_LOG(type, str, val1, ...) do {} while(0)
#endif

void sim_log_init(void);
void sim_log_enqueue(sim_log_type_t type, const char *message_str, uint32_t value1, uint32_t value2);
void sim_log_process_all_messages(void);

#endif // SIM_LOG_H
