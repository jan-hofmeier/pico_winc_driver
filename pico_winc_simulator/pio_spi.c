#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "spi_slave.pio.h"
#include "pio_spi.h"

// Define GPIO connections for the SPI slave
#define MOSI_PIN 16
#define CS_PIN   17
#define SCK_PIN  18
#define MISO_PIN 19

static PIO pio = pio0;
static uint sm_rx;
static uint sm_tx;
static uint sm_oe;
static irq_handler_t app_irq_handler = NULL;

static void pio_spi_irq();

// --- WRITE (TX) ---
// Send a byte to the Master.
// If the FIFO is full, this hangs until there is space.
size_t pio_spi_write_blocking(uint8_t* buffer, size_t len) {
    for(size_t i=0; i<len; i++){
        pio_sm_put_blocking(pio0, sm_tx, (uint32_t)buffer[i] <<24 | 1);
        // prevent rx fifo from filling up with dummy data
        if(!pio_sm_is_rx_fifo_empty(pio, sm_rx)) {
            pio_sm_get(pio, sm_rx);
        }
    }
    return len;
}

// --- READ (RX) ---
// Receive a byte from the Master.
// If the FIFO is empty, this hangs until a byte arrives.
size_t pio_spi_read_blocking(uint8_t* buffer, size_t len) {
    for(size_t i=0; i<len; i++){
        buffer[i] = (uint8_t) pio_sm_get_blocking(pio, sm_rx);
    }
    return len;
}

void pio_spi_slave_init(irq_handler_t handler) {
    app_irq_handler = handler;
    sm_rx = pio_claim_unused_sm(pio, true);
    sm_tx = pio_claim_unused_sm(pio, true);
    sm_oe = pio_claim_unused_sm(pio, true);

    // 1. Load the PIO programs
    uint offset_rx = pio_add_program(pio, &spi_rx_program);
    uint offset_tx = pio_add_program(pio, &spi_tx_program);
    uint offset_oe = pio_add_program(pio, &spi_tristate_program);

    // ============================================================
    // RX STATE MACHINE CONFIGURATION
    // ============================================================
    pio_sm_config c_rx = spi_rx_program_get_default_config(offset_rx);

    // Pin Configuration:
    // Set IN Base to MOSI_PIN (16). 
    // This allows the assembly to see:
    //   - 'pin 0' = MOSI (16)
    //   - 'pin 1' = CS   (17)
    //   - 'pin 2' = SCK  (18)
    sm_config_set_in_pins(&c_rx, MOSI_PIN);
    
    // Set JMP Pin to CS_PIN for the 'jmp pin' instruction
    sm_config_set_jmp_pin(&c_rx, CS_PIN);

    // Shift Configuration:
    // Shift Left (false), Auto-Push (true), Threshold 8 bits
    sm_config_set_in_shift(&c_rx, false, false, 8);
    sm_config_set_fifo_join(&c_rx, PIO_FIFO_JOIN_RX); // Double RX FIFO depth

    // GPIO Initialization:
    pio_gpio_init(pio, MOSI_PIN);
    pio_gpio_init(pio, CS_PIN);
    pio_gpio_init(pio, SCK_PIN);
    
    // Set MOSI, CS, and SCK as Inputs.
    // Since they are sequential (16,17,18), we can set 3 pins starting at MOSI_PIN.
    pio_sm_set_consecutive_pindirs(pio, sm_rx, MOSI_PIN, 3, false);

    // Enable RX SM
    pio_sm_init(pio, sm_rx, offset_rx, &c_rx);
    pio_sm_set_enabled(pio, sm_rx, true);


    // ============================================================
    // OE SETUP (Tristate Control)
    // ============================================================
    pio_sm_config c_oe = spi_tristate_program_get_default_config(offset_oe);

    // IN Base = CS (17) -> 'wait' monitors CS
    sm_config_set_in_pins(&c_oe, CS_PIN);
    
    // SIDESET Base = MISO (19) -> controls Direction
    sm_config_set_sideset_pins(&c_oe, MISO_PIN);
    
    // GPIO Init for MISO
    pio_gpio_init(pio, MISO_PIN);
    // Start as Input (Hi-Z)
    pio_sm_set_consecutive_pindirs(pio, sm_oe, MISO_PIN, 1, false);

    pio_sm_init(pio, sm_oe, offset_oe, &c_oe);
    pio_sm_set_enabled(pio, sm_oe, true);

    // ============================================================
    // TX STATE MACHINE CONFIGURATION
    // ============================================================
    pio_sm_config c_tx = spi_tx_program_get_default_config(offset_tx);

    // Pin Configuration:
    // OUT Base: MISO_PIN (19)
    sm_config_set_out_pins(&c_tx, MISO_PIN, 1);
    
    // IN Base: MOSI_PIN (16) - CRITICAL!
    // Even though TX doesn't read MOSI, we set the IN base to 16 
    // so that the 'wait pin 1' (CS) and 'wait pin 2' (SCK) instructions 
    // point to the correct GPIOs relative to this base.
    sm_config_set_in_pins(&c_tx, MOSI_PIN);
    
    // JMP Pin: CS_PIN (17)
    sm_config_set_jmp_pin(&c_tx, CS_PIN);

    // Shift Configuration:
    // Shift Left (false), Auto-Pull (true), Threshold 8 bits
    sm_config_set_out_shift(&c_tx, false, false, 8);
    sm_config_set_fifo_join(&c_tx, PIO_FIFO_JOIN_TX); // Double TX FIFO depth

    // GPIO Initialization:
    pio_gpio_init(pio, MISO_PIN);
    // Set MISO as Output
    pio_sm_set_consecutive_pindirs(pio, sm_tx, MISO_PIN, 1, false);

    // ------------------------------------------------------------
    // TX Requirement 1: Zero Padding
    // ------------------------------------------------------------
    // Initialize the X register to 0. If the FIFO is empty, 
    // 'pull noblock' uses X, ensuring we send 0x00 padding.
    pio_sm_exec(pio, sm_tx, pio_encode_mov(pio_x, pio_null));

    // ------------------------------------------------------------
    // TX Requirement 2: Start at 'first_pull'
    // ------------------------------------------------------------
    // We start execution at the 'first_pull' label (bottom of assembly).
    // This triggers an immediate pull to preload the OSR before the 
    // first transaction begins.
    uint start_offset = offset_tx + spi_tx_offset_first_pull;
    
    pio_sm_init(pio, sm_tx, start_offset, &c_tx);
    pio_sm_set_enabled(pio, sm_tx, true);

    // --- Interrupt Setup ---
    irq_set_exclusive_handler(PIO0_IRQ_0, app_irq_handler);
    irq_set_enabled(PIO0_IRQ_0, true);
    pio_set_irq0_source_enabled(pio, pio_get_rx_fifo_not_empty_interrupt_source(sm_rx), true);
}

uint pio_spi_get_rx_dreq(void) {
    return pio_get_dreq(pio, sm_rx, false);
}

volatile void* pio_spi_get_rx_fifo_address(void) {
    return &pio->rxf[sm_rx];
}

void pio_spi_set_rx_irq_enabled(bool enabled) {
    pio_set_irq0_source_enabled(pio, pio_get_rx_fifo_not_empty_interrupt_source(sm_rx), enabled);
}
