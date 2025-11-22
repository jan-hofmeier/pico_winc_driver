# WINC1500 Driver Architecture

This document provides a high-level overview of the WINC1500 driver architecture.

## Layers

The driver is divided into the following layers:

*   **Bus Wrapper:** This layer provides a hardware-independent interface to the underlying SPI bus.
*   **Host Driver:** This layer implements the core driver logic, including device initialization, interrupt handling, and data transfer.
*   **Socket:** This layer provides a standard socket-based API for network communication.

## Interaction

The following diagram illustrates the interaction between the different layers:

The `Socket` layer sits at the top of the stack and provides a standard socket-based API for network communication. It is responsible for creating and managing sockets, as well as sending and receiving data. The `Socket` layer calls into the `Host Driver` layer to perform these operations.

The `Host Driver` layer is the core of the driver. It is responsible for initializing the WINC1500, handling interrupts, and managing the flow of data between the host and the device. The `Host Driver` layer calls into the `Bus Wrapper` layer to communicate with the WINC1500 over the SPI bus.

The `Bus Wrapper` layer provides a hardware-independent interface to the underlying SPI bus. It is responsible for reading and writing data to the WINC1500's registers and memory. This layer is platform-specific and must be implemented for each new platform.

## Simulator Architecture

When running in `COMBINED` mode, a WINC1500 simulator runs on the second core of the RP2040. It is designed to mimic the behavior of a real WINC1500 device, allowing the host driver to run on the first core as if it were communicating with actual hardware.

The simulator's primary responsibilities are:
-   Simulating the WINC1500's memory and register space.
-   Responding to SPI commands from the host driver.
-   Simulating firmware behavior, such as handling HIF requests and generating responses.

### Register Write Callbacks

A key feature of the simulator is its event-driven design, which is centered around a callback system for register writes. The host driver communicates with the WINC1500 by writing to specific memory-mapped registers. The simulator uses a callback manager to "listen" for writes to these registers and trigger corresponding actions.

This decouples the driver's actions from the simulator's response, preventing the driver from being stalled. For example:

1.  The simulator registers a callback for writes to address **`0x106c`** (`WIFI_HOST_RCV_CTRL_3`).
2.  The host driver, when sending a HIF request, completes the process by writing the final DMA address to this register.
3.  The simulator's callback manager detects this write and enqueues a notification.
4.  The main simulator loop processes the queue and invokes the registered callback.
5.  The callback function then begins processing the HIF request that the driver just sent, reading the HIF packet from the simulated DMA memory location.

This mechanism allows the simulator to react to driver commands asynchronously, just as the real firmware would.
