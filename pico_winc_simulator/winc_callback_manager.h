#ifndef WINC_CALLBACK_MANAGER_H
#define WINC_CALLBACK_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

// Define the callback function pointer type
typedef void (*winc_reg_callback_t)(uint32_t address, uint32_t value);

/**
 * @brief Initializes the callback manager.
 */
void winc_creg_init(void);

/**
 * @brief Registers a callback function for a specific register address.
 * 
 * @param address The register address to monitor.
 * @param callback The function to call when the register is written to.
 */
void winc_creg_register_callback(uint32_t address, winc_reg_callback_t callback);

/**
 * @brief Handles a register write. This function should be called when a register write is detected.
 * It will perform the memory write and then queue the request for the main loop to process.
 * 
 * @param address The address of the register being written.
 * @param value The value being written to the register.
 * @return uint8_t* A pointer to the memory location where the value was written, or NULL on failure.
 */
void winc_creg_handle_write(uint32_t address);

/**
 * @brief Processes the queue of register write requests. 
 * This should be called from the main application loop.
 */
void winc_creg_process_requests(void);
void winc_creg_write(uint32_t addr, uint32_t value);

#endif // WINC_CALLBACK_MANAGER_H
