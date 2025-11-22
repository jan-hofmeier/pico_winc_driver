#include "winc_callback_manager.h"
#include "winc_simulator_app.h" // For get_memory_ptr
#include "sim_log.h"
#include <string.h>

#define MAX_CALLBACKS 16
#define MAX_QUEUE_SIZE 32

typedef struct {
    uint32_t address;
    winc_reg_callback_t callback;
} callback_registration_t;

typedef struct {
    uint32_t address;
} write_request_t;

static callback_registration_t callback_table[MAX_CALLBACKS];
static uint32_t callback_count = 0;

static write_request_t write_queue[MAX_QUEUE_SIZE];
static uint32_t queue_head = 0;
static uint32_t queue_tail = 0;
static uint32_t queue_count = 0;

void winc_creg_init(void) {
    memset(callback_table, 0, sizeof(callback_table));
    callback_count = 0;
    queue_head = 0;
    queue_tail = 0;
    queue_count = 0;
}

void winc_creg_register_callback(uint32_t address, winc_reg_callback_t callback) {
    if (callback_count < MAX_CALLBACKS) {
        callback_table[callback_count].address = address;
        callback_table[callback_count].callback = callback;
        callback_count++;
    }
}

void winc_creg_handle_write(uint32_t address) {
    // Enqueue the request for the main loop if a callback might exist
    if (queue_count < MAX_QUEUE_SIZE) {
        write_queue[queue_tail].address = address;
        queue_tail = (queue_tail + 1) % MAX_QUEUE_SIZE;
        queue_count++;
    } else {
        SIM_LOG(SIM_LOG_TYPE_ERROR, "WINC CREG queue full, dropping request", address, queue_count);
    }
}

void winc_creg_process_requests(void) {
    while (queue_count > 0) {
        // Dequeue the request
        write_request_t request = write_queue[queue_head];
        queue_head = (queue_head + 1) % MAX_QUEUE_SIZE;
        queue_count--;

        // Read the value from memory just before executing the callback
        uint32_t current_value = 0;
        uint8_t* mem_ptr = get_memory_ptr(request.address, 4);
        if (mem_ptr) {
            memcpy(&current_value, mem_ptr, 4);
        }

        // Find and execute the callback
        for (uint32_t i = 0; i < callback_count; i++) {
            if (callback_table[i].address == request.address) {
                callback_table[i].callback(request.address, current_value);
                break; // Assuming only one callback per address
            }
        }
    }
}
