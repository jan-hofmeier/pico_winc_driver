#include "winc_callback_manager.h"
#include "winc_simulator_app.h" // For get_memory_ptr
#include "sim_log.h"
#include <string.h>
#include "pico/util/queue.h"

#define MAX_CALLBACKS 16
#define MAX_QUEUE_SIZE 32

typedef struct {
    uint32_t address;
    winc_reg_callback_t callback;
} callback_registration_t;

static callback_registration_t callback_table[MAX_CALLBACKS];
static uint32_t callback_count = 0;

static queue_t creg_queue;

void winc_creg_init(void) {
    memset(callback_table, 0, sizeof(callback_table));
    callback_count = 0;
    queue_init(&creg_queue, sizeof(uint32_t), MAX_QUEUE_SIZE);
}

void winc_creg_register_callback(uint32_t address, winc_reg_callback_t callback) {
    if (callback_count < MAX_CALLBACKS) {
        callback_table[callback_count].address = address;
        callback_table[callback_count].callback = callback;
        callback_count++;
    }
}

void winc_creg_handle_write(uint32_t address) {
    if (!queue_try_add(&creg_queue, &address)) {
        SIM_LOG(SIM_LOG_TYPE_ERROR, "WINC CREG queue full, dropping request", address, queue_get_level(&creg_queue));
    }
}

void winc_creg_process_requests(void) {
    uint32_t request_address;
    while (queue_try_remove(&creg_queue, &request_address)) {
        // Read the value from memory just before executing the callback
        uint32_t current_value = 0;
        uint8_t* mem_ptr = get_memory_ptr(request_address, 4);
        if (mem_ptr) {
            memcpy(&current_value, mem_ptr, 4);
        }

        // Find and execute the callback
        for (uint32_t i = 0; i < callback_count; i++) {
            if (callback_table[i].address == request_address) {
                callback_table[i].callback(request_address, current_value);
                break; // Assuming only one callback per address
            }
        }
    }
}
