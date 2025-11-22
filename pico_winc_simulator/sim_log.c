#include <stdio.h>
#include <string.h>
#include <stdarg.h> // Required for va_list
#include "pico/stdlib.h"
#include "sim_log.h"
#include "pico/util/queue.h"

// Global log buffer instance
static queue_t log_queue;

void sim_log_init(void) {
    queue_init(&log_queue, sizeof(sim_log_message_t), SIM_LOG_BUFFER_SIZE);
}

// Function to enqueue a log message
void sim_log_enqueue(sim_log_type_t type, const char *message_str, uint32_t value1, uint32_t value2) {
    sim_log_message_t msg;
    msg.type = type;
    strncpy(msg.message_str, message_str, SIM_LOG_MESSAGE_STRING_LEN - 1);
    msg.message_str[SIM_LOG_MESSAGE_STRING_LEN - 1] = '\0'; // Ensure null termination
    msg.value1 = value1;
    msg.value2 = value2;

    queue_try_add(&log_queue, &msg);
    // If the queue is full, the message is dropped.
}

// Function to process (print) all log messages from the queue
void sim_log_process_all_messages(void) {
    sim_log_message_t msg;
    while (queue_try_remove(&log_queue, &msg)) {
        switch (msg.type) {
            case SIM_LOG_TYPE_COMMAND:
                printf("[SIMULATOR] %s: 0x%X -> 0x%X\n", msg.message_str, msg.value1, msg.value2);
                break;
            case SIM_LOG_TYPE_ADDRESS:
                printf("[SIMULATOR] %s: 0x%06x\n", msg.message_str, msg.value1);
                break;
            case SIM_LOG_TYPE_DATA:
                printf("[SIMULATOR] %s: %02x %02x %02x %02x\n", msg.message_str,
                       (uint8_t)(msg.value1 >> 24), (uint8_t)(msg.value1 >> 16),
                       (uint8_t)(msg.value1 >> 8), (uint8_t)msg.value1);
                break;
            case SIM_LOG_TYPE_ADDRESS_DATA:
                printf("[SIMULATOR] %s Address: 0x%06x Data: %02x %02x %02x %02x\n", msg.message_str, msg.value1,
                       (uint8_t)(msg.value2 >> 24), (uint8_t)(msg.value2 >> 16),
                       (uint8_t)(msg.value2 >> 8), (uint8_t)msg.value2);
                break;
            case SIM_LOG_TYPE_UNKNOWN_COMMAND:
                printf("[SIMULATOR] %s: 0x%02x\n", msg.message_str, (uint8_t)msg.value1);
                break;
            case SIM_LOG_TYPE_ERROR:
                printf("[SIMULATOR ERROR] %s: 0x%X, 0x%X\n", msg.message_str, msg.value1, msg.value2);
                break;
            case SIM_LOG_TYPE_NONE:
            default:
                printf("[SIMULATOR] %s\n", msg.message_str);
                break;
        }
    }
}
