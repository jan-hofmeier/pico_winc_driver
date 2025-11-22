#ifndef WINC_DMA_H
#define WINC_DMA_H

#include <stdint.h>
#include <stddef.h>

// Callback type for when DMA transfer is complete
typedef void (*winc_dma_complete_cb_t)(void);

/**
 * @brief Initialize the DMA for WINC simulator
 *
 * @param callback Function to call when DMA transfer completes
 */
void winc_dma_init(winc_dma_complete_cb_t callback);

/**
 * @brief Start a DMA read transfer from the SPI RX FIFO
 *
 * @param buffer Destination buffer
 * @param length Number of bytes to read
 */
void winc_dma_read(uint8_t *buffer, size_t length);

#endif // WINC_DMA_H
