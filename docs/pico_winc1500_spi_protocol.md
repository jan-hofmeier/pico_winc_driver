# WINC1500 SPI Protocol

This document describes the SPI protocol used to communicate with the WINC1500 Wi-Fi module.

## SPI Configuration

*   **Mode:** SPI Mode 0 (CPOL=0, CPHA=0)
*   **Data Order:** MSB first
*   **Clock Speed:** Up to 48 MHz

## SPI Transaction Format

All SPI transactions consist of a command phase and a data phase. The host initiates a transaction by asserting the CS line, sending a command, and then sending or receiving data.

**Packet Order Prefix (0xFx):** For multi-packet transfers, a 0xFx prefix is used to indicate the packet order:
*   `0xF1`: First packet in a multi-packet transfer.
*   `0xF2`: Continued packet in a multi-packet transfer.
*   `0xF3`: Last packet in a multi-packet transfer, or a single-packet transfer.

### Command Phase

The command phase is a variable-length sequence of bytes that specifies the operation to be performed. The first byte is always the command byte, which has the following format:

| Bit(s) | Description        |
| ------ | ------------------ |
| 7:0    | Command            |

The command byte is followed by a variable number of address and data bytes, depending on the command.

### Data Phase

The data phase follows the command phase and can be either a read or a write operation. The length of the data phase is determined by the command.

## SPI Commands

The following table lists the available SPI commands:

| Command              | Value | Description                                       |
|----------------------|-------|---------------------------------------------------|
| `CMD_DMA_WRITE`      | 0xc1  | Write a block of data to memory using DMA.        |
| `CMD_DMA_READ`       | 0xc2  | Read a block of data from memory using DMA.       |
| `CMD_INTERNAL_WRITE` | 0xc3  | Write to an internal register.                    |
| `CMD_INTERNAL_READ`  | 0xc4  | Read from an internal register.                   |
| `CMD_TERMINATE`      | 0xc5  | Terminate the current operation.                  |
| `CMD_REPEAT`         | 0xc6  | Repeat the previous command.                      |
| `CMD_DMA_EXT_WRITE`  | 0xc7  | Write a block of data to memory using extended DMA.|
| `CMD_DMA_EXT_READ`   | 0xc8  | Read a block of data from memory using extended DMA.|
| `CMD_SINGLE_WRITE`   | 0xc9  | Write a single word to a register.                |
| `CMD_SINGLE_READ`    | 0xca  | Read a single word from a register.               |
| `CMD_RESET`          | 0xcf  | Reset the WINC1500.                               |

## Command Formats

### `CMD_SINGLE_READ (0xca)`

This command is used to read a single word from a register.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_SINGLE_READ (0xca)`  |
| 1     | Address[23:16]     |
| 2     | Address[15:8]      |
| 3     | Address[7:0]       |

**Response:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_SINGLE_READ`  |
| 1     | `0xF3` (Prefix)    |
| 2     | Data[7:0]          |
| 3     | Data[15:8]         |
| 4     | Data[23:16]        |
| 5     | Data[31:24]        |

### `CMD_INTERNAL_WRITE (0xc3)`

This command is used to write to an internal register.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_INTERNAL_WRITE (0xc3)`|
| 1     | Address[15:8]      |
| 2     | Address[7:0]       |
| 3     | Data[7:0]          |
| 4     | Data[15:8]         |
| 5     | Data[23:16]        |
| 6     | Data[31:24]        |

**Response:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_INTERNAL_WRITE`|
| 1     | Status             |

### `CMD_RESET (0xcf)`

This command is used to reset the WINC1500.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_RESET (0xcf)`        |

**Response:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_RESET`        |
| 1     | Status             |

### `CMD_SINGLE_WRITE (0xc9)`

This command is used to write a single word to a register.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_SINGLE_WRITE (0xc9)` |
| 1     | Address[23:16]     |
| 2     | Address[15:8]      |
| 3     | Address[7:0]       |
| 4     | Data[7:0]          |
| 5     | Data[15:8]         |
| 6     | Data[23:16]        |
| 7     | Data[31:24]        |

**Response:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_SINGLE_WRITE` |
| 1     | Status             |

### `CMD_INTERNAL_READ (0xc4)`

This command is used to read from an internal register.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_INTERNAL_READ (0xc4)`|
| 1     | Address[15:8]      |
| 2     | Address[7:0]       |

**Response:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_INTERNAL_READ`|
| 1     | `0xF3` (Prefix)    |
| 2     | Data[7:0]          |
| 3     | Data[15:8]         |
| 4     | Data[23:16]        |
| 5     | Data[31:24]        |

### `CMD_DMA_EXT_READ (0xc8)`

This command is used to read a block of data from memory using extended DMA.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_DMA_EXT_READ (0xc8)` |
| 1     | Address[23:16]     |
| 2     | Address[15:8]      |
| 3     | Address[7:0]       |
| 4     | Size[23:16]        |
| 5     | Size[15:8]         |
| 6     | Size[7:0]          |

**Response:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_DMA_EXT_READ` |
| 1     | `0xFx` (Prefix)    |
| 2..n  | Data               |

### `CMD_DMA_EXT_WRITE (0xc7)`

This command is used to write a block of data to memory using extended DMA.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_DMA_EXT_WRITE (0xc7)`|
| 1     | Address[23:16]     |
| 2     | Address[15:8]      |
| 3     | Address[7:0]       |
| 4     | Size[23:16]        |
| 5     | Size[15:8]         |
| 6     | Size[7:0]          |

**Data:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `0xFx` (Prefix)    |
| 1..n  | A block of data of the specified size. |

**Response:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_DMA_EXT_WRITE`|
| 1     | Status             |

### `CMD_DMA_READ (0xc2)`

This command is used to read a block of data from memory using DMA.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_DMA_READ (0xc2)`     |
| 1     | Address[23:16]     |
| 2     | Address[15:8]      |
| 3     | Address[7:0]       |
| 4     | Size[15:8]         |
| 5     | Size[7:0]          |

**Response:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_DMA_READ`     |
| 1     | `0xF3` (Prefix)    |
| 2..n  | Data               |

### `CMD_DMA_WRITE (0xc1)`

This command is used to write a block of data to memory using DMA.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_DMA_WRITE (0xc1)`    |
| 1     | Address[23:16]     |
| 2     | Address[15:8]      |
| 3     | Address[7:0]       |
| 4     | Size[15:8]         |
| 5     | Size[7:0]          |

**Data:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `0xF3` (Prefix)    |
| 1..n  | A block of data of the specified size. |

**Response:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_DMA_WRITE`    |
| 1     | Status             |

## Command Response

After sending a command, the host must read the SPI bus to receive a response from the WINC1500. The first byte of the response is always an echo of the command that was sent.

For DMA read commands (`CMD_DMA_READ`, `CMD_DMA_EXT_READ`), the second byte of the response is a `0xFx` prefix, followed by the requested data.

For write commands (`CMD_SINGLE_WRITE`, `CMD_INTERNAL_WRITE`, `CMD_DMA_WRITE`, `CMD_DMA_EXT_WRITE`), the second byte of the response is a status byte that indicates whether the command was successful. A value of `0x00` indicates success, while any other value indicates an error. Note that for DMA write commands, the host is expected to send a `0xFx` prefix before the actual data.

For other read commands (`CMD_SINGLE_READ`, `CMD_INTERNAL_READ`), the second byte of the response is a `0xF3` prefix, followed by the first byte of the requested data.

## Interrupt Handling

The WINC1500 uses a single interrupt line to notify the host of various events, such as the completion of a command or the receipt of a data packet. The host can enable or disable interrupts by writing to the `NMI_INTR_ENABLE` register.

When an interrupt occurs, the host must read the `INT_STATUS` register to determine the cause of the interrupt. The following table lists the possible interrupt sources:

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 0   | `RX_DONE`    | A packet has been received.               |
| 1   | `TX_DONE`    | A packet has been sent.                   |

If the `RX_DONE` bit is set, the host can read the `WIFI_HOST_RCV_CTRL_1` register to get the address of the received data packet. After processing the interrupt, the host must clear the interrupt by writing to the `INT_STATUS` register.

## Host Interface Format (HIF)

All data transfers between the host and the WINC1500 are done using the Host Interface Format (HIF). A HIF packet consists of a 4-byte header followed by a variable-length payload.

### HIF Header

The HIF header has the following format:

| Bit(s)  | Description        |
|---------|--------------------|
| 15:0    | Length             |
| 23:16   | Group ID           |
| 31:24   | Opcode             |

The `Length` field specifies the total length of the HIF packet, including the header. The `Group ID` field identifies the functional group to which the packet belongs (e.g., Wi-Fi, IP, OTA). The `Opcode` field specifies the operation to be performed.

### Group IDs

| Group ID            | Value | Description                                       |
|---------------------|-------|---------------------------------------------------|
| `M2M_REQ_GROUP_IP`  | 0x01  | IP stack                                          |
| `M2M_REQ_GROUP_WIFI`| 0x02  | Wi-Fi                                             |
| `M2M_REQ_GROUP_OTA` | 0x03  | Over-the-air firmware update                      |
| `M2M_REQ_GROUP_HIF` | 0x04  | Host Interface                                    |
| `M2M_REQ_GROUP_CRYPTO`| 0x05  | Crypto                                            |
| `M2M_REQ_GROUP_SIGMA`| 0x06  | Sigma                                             |
| `M2M_REQ_GROUP_SSL` | 0x07  | SSL                                               |

### Wi-Fi Opcodes

| Opcode                      | Value | Description                                       |
|-----------------------------|-------|---------------------------------------------------|
| `M2M_WIFI_REQ_CONNECT`      | 0x01  | Connect to a Wi-Fi network.                       |
| `M2M_WIFI_REQ_DISCONNECT`   | 0x02  | Disconnect from a Wi-Fi network.                  |
| `M2M_WIFI_REQ_GET_CONN_INFO`| 0x03  | Get information about the current connection.     |
| `M2M_WIFI_REQ_SCAN`         | 0x04  | Scan for available Wi-Fi networks.                |
| `M2M_WIFI_REQ_SET_MAC_ADDRESS`| 0x05  | Set the MAC address.                              |
| `M2M_WIFI_REQ_ENABLE_MONITORING`| 0x06  | Enable Wi-Fi monitoring mode.                     |
| `M2M_WIFI_REQ_DISABLE_MONITORING`| 0x07  | Disable Wi-Fi monitoring mode.                    |
| `M2M_WIFI_REQ_SEND_WIFI_PACKET`| 0x08  | Send a Wi-Fi packet.                              |

### IP Opcodes

| Opcode                      | Value | Description                                       |
|-----------------------------|-------|---------------------------------------------------|
| `SOCKET_CMD_BIND`           | 0x01  | Bind a socket to a local address.                 |
| `SOCKET_CMD_LISTEN`         | 0x02  | Listen for connections on a socket.               |
| `SOCKET_CMD_ACCEPT`         | 0x03  | Accept a connection on a socket.                  |
| `SOCKET_CMD_CONNECT`        | 0x04  | Initiate a connection on a socket.                |
| `SOCKET_CMD_SEND`           | 0x05  | Send data on a socket.                            |
| `SOCKET_CMD_RECV`           | 0x06  | Receive data on a socket.                         |
| `SOCKET_CMD_CLOSE`          | 0x07  | Close a socket.                                   |
| `SOCKET_CMD_DNS_RESOLVE`    | 0x08  | Resolve a domain name.                            |

### HIF Command Payloads

#### `M2M_WIFI_REQ_CONNECT`

This command is used to connect to a Wi-Fi network.

**Payload:**

| Field              | Type      | Description                                       |
|--------------------|-----------|---------------------------------------------------|
| `SSID`             | `char[]`  | The SSID of the network to connect to.            |
| `SSID_Length`      | `uint8`   | The length of the SSID.                           |
| `Security_Type`    | `uint8`   | The security type of the network.                 |
| `Auth_Info`        | `void*`   | A pointer to a structure containing authentication information. |
| `Channel`          | `uint16`  | The channel to connect to.                        |

#### `M2M_WIFI_REQ_DISCONNECT`

This command is used to disconnect from a Wi-Fi network.

**Payload:**

None.

#### `M2M_WIFI_REQ_SCAN`

This command is used to scan for available Wi-Fi networks.

**Payload:**

| Field      | Type    | Description                                       |
|------------|---------|---------------------------------------------------|
| `Channel`  | `uint8` | The channel to scan. Use `M2M_WIFI_CH_ALL` to scan all channels. |

#### `SOCKET_CMD_BIND`

This command is used to bind a socket to a local address.

**Payload:**

| Field      | Type    | Description                                       |
|------------|---------|---------------------------------------------------|
| `Socket`   | `uint8` | The socket to bind.                               |
| `Address`  | `struct sockaddr_in` | The local address to bind to.                    |

#### `SOCKET_CMD_CONNECT`

This command is used to initiate a connection on a socket.

**Payload:**

| Field      | Type    | Description                                       |
|------------|---------|---------------------------------------------------|
| `Socket`   | `uint8` | The socket to connect.                            |
| `Address`  | `struct sockaddr_in` | The remote address to connect to.                 |

#### `SOCKET_CMD_SEND`

This command is used to send data on a socket.

**Payload:**

| Field      | Type    | Description                                       |
|------------|---------|---------------------------------------------------|
| `Socket`   | `uint8` | The socket to send data on.                       |
| `Data`     | `char[]`| The data to send.                                 |
| `Length`   | `uint16`| The length of the data to send.                   |

### HIF Responses

#### `M2M_WIFI_RESP_CON_STATE_CHANGED`

This response is sent when the Wi-Fi connection state changes.

**Payload:**

| Field      | Type    | Description                                       |
|------------|---------|---------------------------------------------------|
| `State`    | `uint8` | The new connection state. See `tenuM2mConnState`. |
| `Error`    | `uint8` | An error code if the connection failed.           |

#### `M2M_WIFI_RESP_SCAN_RESULT`

This response is sent for each AP found during a scan.

**Payload:**

| Field      | Type    | Description                                       |
|------------|---------|---------------------------------------------------|
| `Index`    | `uint8` | The index of the AP in the scan results.          |
| `RSSI`     | `sint8` | The signal strength of the AP.                    |
| `Auth_Type`| `uint8` | The authentication type of the AP.                |
| `Channel`  | `uint8` | The channel of the AP.                            |
| `BSSID`    | `uint8[6]`| The MAC address of the AP.                        |
| `SSID`     | `char[]`| The SSID of the AP.                               |

#### `SOCKET_MSG_BIND`

This response is sent in response to a `SOCKET_CMD_BIND` command.

**Payload:**

| Field      | Type    | Description                                       |
|------------|---------|---------------------------------------------------|
| `Status`   | `sint8` | The status of the bind operation.                 |

#### `SOCKET_MSG_CONNECT`

This response is sent in response to a `SOCKET_CMD_CONNECT` command.

**Payload:**

| Field      | Type    | Description                                       |
|------------|---------|---------------------------------------------------|
| `Socket`   | `uint8` | The socket that was connected.                    |
| `Error`    | `sint8` | An error code if the connection failed.           |

#### `SOCKET_MSG_RECV` / `SOCKET_MSG_RECVFROM`

This response is sent when data is received on a socket.

**Payload:**

| Field              | Type      | Description                                       |
|--------------------|-----------|---------------------------------------------------|
| `Buffer`           | `char[]`  | The received data.                                |
| `Buffer_Size`      | `sint16`  | The size of the received data.                    |
| `Remaining_Size`   | `uint16`  | The amount of data remaining in the socket buffer.|
| `Remote_Address`   | `struct sockaddr_in` | The address of the sender.                      |


## HIF Transaction Flow

This section describes the sequence of operations required to send a HIF command to the WINC1500 and to process a response received from it.

### Sending a HIF Command

Sending a HIF command involves requesting a buffer from the WINC1500, writing the command and its payload to that buffer, and then notifying the firmware that the command is ready for processing.

1.  **Wake the Chip:** Before any transaction, ensure the chip is awake by writing `0x1` to the `WAKE_CLK_REG` (address `0x1`) and polling the `CLOCKS_EN_REG` (address `0xF`) until bit 2 (`CLK_EN`) is set.

2.  **Request a DMA Buffer:**
    a. Prepare the request details. This involves combining the `Group ID`, `Opcode`, and total packet `Length` (HIF Header + Payload) into a 32-bit value and writing it to the `NMI_STATE_REG` (address `0x108C`). The format is `(Length << 16) | (Opcode << 8) | Group ID`.
    b. Signal the request to the firmware by setting bit 1 (`RX_REQ`) of the `WIFI_HOST_RCV_CTRL_2` register (address `0x1078`).
    c. Poll bit 1 of `WIFI_HOST_RCV_CTRL_2` until the firmware clears it. This indicates that a buffer has been allocated.
    d. Read the 32-bit DMA address of the allocated buffer from the `WIFI_HOST_RCV_CTRL_4` register (address `0x1504`).

3.  **Write the HIF Packet:**
    a. Create the 4-byte `HIF Header` in host memory.
    b. Use the `CMD_DMA_EXT_WRITE` SPI command to transfer the `HIF Header` and the command `Payload` to the DMA address obtained in the previous step. The payload (if any) should be placed immediately after the header.

4.  **Notify Firmware of Completion:**
    a. Prepare a 32-bit notification value by taking the DMA address, shifting it left by 2 bits, and setting bit 1. For example: `(dma_address << 2) | 0x2`.
    b. Write this notification value to the `WIFI_HOST_RCV_CTRL_3` register (address `0x106C`). This signals to the firmware that the data has been written and is ready for processing.

### Receiving a HIF Response (Interrupt-Driven)

Receiving a response is typically handled in an interrupt service routine (ISR) that is triggered by the WINC1500.

1.  **Interrupt Trigger:** The WINC1500 asserts the host's interrupt pin.

2.  **Acknowledge and Read Status:** In the ISR, the host must first read the `WIFI_HOST_RCV_CTRL_0` register (address `0x1070`).
    a. Check if bit 0 (`RX_DONE`) is set. If it is, a new packet is available.
    b. The size of the available packet (in bytes) is contained in bits 11:2 of this same register value.

3.  **Get Packet Address:** Read the 32-bit DMA address where the received packet is stored from the `WIFI_HOST_RCV_CTRL_1` register (address `0x1084`).

4.  **Read the HIF Packet:**
    a. Use the `CMD_DMA_EXT_READ` SPI command to read the entire packet (or just the 4-byte header first) from the DMA address obtained in the previous step.
    b. Parse the `HIF Header` to determine the `Group ID` and `Opcode` of the response.
    c. Based on the Group ID and Opcode, the host application can now read and process the payload of the packet.

5.  **Signal Completion to Firmware:** After the host has finished processing the packet and reading all necessary data from the DMA buffer, it **must** notify the firmware that the buffer is now free. This is done by setting bit 1 (`RX_DONE`) of the `WIFI_HOST_RCV_CTRL_0` register (address `0x1070`).

## Chip Initialization Sequence

The following sequence must be followed to initialize the WINC1500:

1.  **Reset the chip:** Send the `CMD_RESET` command to reset the chip.
2.  **Read the chip ID:** Read the `CHIPID` register to verify that the chip is present and responding. The expected value is `0x1002a0`.
3.  **Configure the SPI protocol:** Write `0x00000054` to the `NMI_SPI_PROTOCOL_CONFIG` register to configure the SPI protocol with CRC enabled and a packet size of 8192 bytes. `NMI_SPI_PROTOCOL_CONFIG` is at address `0xE824`.
4.  **Enable interrupts:** Write `0x00000001` to the `NMI_INTR_ENABLE` register to enable interrupts.
6.  **Load the firmware:** Write the firmware to the WINC1500's memory at address `0x0000` using the `CMD_DMA_EXT_WRITE` command.
7.  **Verify the firmware:** Read the firmware from the WINC1500's memory at address `0x0000` using the `CMD_DMA_EXT_READ` command and verify it against the original firmware.
8.  **Start the firmware:** Write `0x00000001` to the `BOOTROM_REG` register to start the firmware.
9.  **Wait for the firmware to start:** Poll the `NMI_STATE_REG` register until it is equal to `0x00000001`.

## State Machine

The WINC1500's SPI interface is controlled by a state machine that ensures proper synchronization between the host and the module. The state machine has the following states:

*   **Reset:** The initial state of the device.
*   **Wait for Host:** The device is waiting for the host to send a command.
*   **Command:** The device is processing a command from the host.
*   **Data:** The device is transferring data to or from the host.
*   **Interrupt:** The device has generated an interrupt and is waiting for the host to acknowledge it.

The host can transition the state machine from one state to another by sending the appropriate command. The following is a textual description of the state machine's operation:

*   **Reset:** The initial state of the device. The host can transition the state machine to the **Wait for Host** state by sending the `CMD_RESET` command.
*   **Wait for Host:** The device is waiting for the host to send a command. When a command is received, the state machine transitions to the **Command** state.
*   **Command:** The device is processing a command from the host. When the command is complete, the state machine transitions to the **Data** state if there is data to be transferred, or back to the **Wait for Host** state if there is no data to be transferred.
*   **Data:** The device is transferring data to or from the host. When the data transfer is complete, the state machine transitions back to the **Wait for Host** state.
*   **Interrupt:** The device has generated an interrupt and is waiting for the host to acknowledge it. When the host acknowledges the interrupt, the state machine transitions back to the **Wait for Host** state.