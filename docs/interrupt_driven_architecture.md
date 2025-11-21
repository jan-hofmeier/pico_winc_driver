# Interrupt-Driven SPI Architecture for WINC1500 Simulator

This document outlines a new architecture for the WINC1500 simulator's SPI handling, transitioning from a blocking, CPU-bound model to an interrupt-driven model using the RP2040's DMA controller.

## 1. Core Concepts

The primary goal is to free up CPU Core 1, which is currently dedicated to polling for and processing SPI commands. The new design will be asynchronous, using interrupts to handle events and DMA to transfer data, allowing the CPU to remain in a low-power wait state for most of the time.

The external behavior of the simulator on the SPI bus must remain identical to the current implementation to ensure the WINC driver operates correctly.

## 2. Architectural Components

### 2.1. PIO State Machines

The existing PIO program (`spi_slave.pio`) for the SPI slave is robust and will be reused without modification. Its DREQ signals (`pio_sm_is_rx_fifo_empty` and `pio_sm_is_tx_fifo_full`) are the key hooks for integrating with the DMA controller.

### 2.2. Interrupts

Two main sources of interrupts will be used:

1.  **PIO Interrupt:** A PIO IRQ will be configured to fire when the PIO RX FIFO has data (i.e., a new command is arriving from the host). This will be the entry point for all SPI transactions.
2.  **DMA Interrupt:** A DMA IRQ will be configured to fire upon the completion of a DMA transfer (e.g., command payload received, response sent). This will drive the state machine forward.

### 2.3. DMA Controller

The DMA controller is central to this design. It will be used for:

-   **Command Reception:** Reading command arguments from the PIO RX FIFO into a buffer.
-   **Data Transfer:** Handling large `CMD_DMA_WRITE` and `CMD_DMA_READ` data blocks, moving data between the PIO FIFOs and the main `winc_memory` buffer.
-   **Response Transmission (Optional):** While DMA can be used to send responses, it is not necessary for small, fixed-size packets (e.g., status responses). These can be written directly to the PIO TX FIFO from an ISR, simplifying the logic.
-   **DMA Chaining:** DMA channels will be chained together to create sequences of operations without CPU intervention. For example, receiving a DMA_WRITE header, then receiving the data payload, and finally triggering a response.

## 3. State Machine and Workflow

The simulator will operate as a state machine, driven by interrupts. The main `while(true)` loop in `winc_simulator_app.c` will be changed from actively calling `handle_spi_transaction()` to a simple `__wfi()` (wait for interrupt) instruction.

A simplified state transition diagram is as follows:

`[IDLE]` -> PIO IRQ (command byte arrives) -> `[RECEIVING_COMMAND]`
`[RECEIVING_COMMAND]` -> DMA IRQ (command received) -> `[EXECUTING_COMMAND & SENDING_RESPONSE]`
`[EXECUTING_COMMAND & SENDING_RESPONSE]` -> (execution complete) -> `[IDLE]`

### Detailed Workflow Example: `CMD_SINGLE_WRITE`

1.  **IDLE State:** The CPU is sleeping.
2.  **Command Arrives:** The host sends the first byte of the `CMD_SINGLE_WRITE` command (`0xC9`). The PIO RX FIFO becomes non-empty, triggering a **PIO IRQ**.
3.  **PIO ISR (`pio_irq_handler`):**
    -   Reads the single command byte from the PIO RX FIFO using `pio_spi_read_blocking`.
    -   Disables the PIO IRQ to prevent re-entry.
    -   Determines the command is `CMD_SINGLE_WRITE` and that it has a 7-byte payload (3-byte address + 4-byte data).
    -   Configures a DMA channel to read 7 bytes from the PIO RX FIFO into a command buffer. The DMA transfer is triggered by the PIO RX DREQ.
    -   Enables a DMA completion IRQ (`dma_irq_handler`).
    -   Acknowledges and clears the PIO IRQ.
4.  **Payload Transfer:** The DMA controller reads the 7 bytes from the host without any CPU involvement.
5.  **DMA ISR (`dma_irq_handler`):**
    -   The DMA transfer completes, triggering the **DMA IRQ**.
    -   The ISR now has the full command and payload in the command buffer.
    -   It parses the address and data, and performs the `memcpy` into `winc_memory`.
    -   It then prepares the 2-byte response (command echo + status).
    -   It writes the 2-byte response directly to the PIO TX FIFO using `pio_spi_write_blocking()`. Since the TX FIFO has space, this will not block for any significant time.
    -   **Drain RX FIFO:** The ISR drains any remaining dummy bytes from the PIO RX FIFO to prevent spurious PIO IRQs.
    -   The transaction is now finished. The ISR re-enables the initial PIO IRQ to arm the system for the next command.
    -   The system returns to the `IDLE` state.

## 4. Implementation Changes

-   **`pico_winc_simulator/winc_simulator_app.c`:**
    -   The `handle_spi_transaction` function will be removed.
    -   The main loop will become `while(true) { __wfi(); }`.
    -   New ISR functions (`pio_irq_handler`, `dma_irq_handler`) will be added to contain the state machine logic.
    -   A global state variable will be needed to track the current state of the transaction.
-   **`pico_winc_simulator/pio_spi.c`:**
    -   The `pio_spi_read_blocking` function will be used for reading the initial command byte from the PIO RX FIFO. For command payloads, it will be replaced by a non-blocking, DMA-based approach.
    -   The `pio_spi_write_blocking` function will be retained for sending small, fixed-size responses from within an ISR.
    -   The `pio_spi_slave_init` function will be updated to configure and enable the PIO and DMA interrupts.

## 5. Potential Issues

-   **IRQ Safety:** Logging functions (`SIM_LOG`) must be made IRQ-safe, for example by using a lockless ring buffer that the main loop can poll and print from when the CPU is not waiting for an interrupt (though for this project, the main loop will be entirely idle).
-   **Complexity:** The state management and DMA channel configuration will be significantly more complex than the current blocking code. Careful design and testing are required.
-   **DMA Write/Read with Prefixes:** `CMD_DMA_WRITE` and `CMD_DMA_READ` involve multiple packets with prefix bytes. DMA chaining will be essential to handle these sequences efficiently without CPU intervention. For example, one DMA channel can receive the data chunk, and upon completion, trigger a second DMA channel to send the next prefix byte.
