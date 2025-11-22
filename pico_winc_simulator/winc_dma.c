#include "winc_dma.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pio_spi.h"

static int dma_channel = -1;
static winc_dma_complete_cb_t dma_callback = NULL;

static void dma_handler() {
    if (dma_channel >= 0 && dma_hw->ints0 & (1u << dma_channel)) {
        dma_hw->ints0 = 1u << dma_channel; // Clear interrupt
        if (dma_callback) {
            dma_callback();
        }
    }
}

void winc_dma_init(winc_dma_complete_cb_t callback) {
    dma_callback = callback;
    dma_channel = dma_claim_unused_channel(true);

    // Setup interrupt
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void winc_dma_read(uint8_t *buffer, size_t length) {
    if (dma_channel < 0) return;

    dma_channel_config c = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);

    uint dreq = pio_spi_get_rx_dreq();
    channel_config_set_dreq(&c, dreq);

    volatile void *read_addr = pio_spi_get_rx_fifo_address();

    dma_channel_configure(
        dma_channel,
        &c,
        buffer,
        read_addr,
        length,
        true // start immediately
    );
}
