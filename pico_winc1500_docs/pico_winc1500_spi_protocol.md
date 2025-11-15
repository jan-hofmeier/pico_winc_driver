# WINC1500 SPI Protocol

This document describes the SPI protocol used to communicate with the WINC1500 Wi-Fi module.

## SPI Configuration

*   **Mode:** SPI Mode 0 (CPOL=0, CPHA=0)
*   **Data Order:** MSB first
*   **Clock Speed:** Up to 48 MHz

## SPI Transaction Format

All SPI transactions consist of a command phase and a data phase. The host initiates a transaction by asserting the CS line, sending a command, and then sending or receiving data.

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

### `CMD_SINGLE_READ`

This command is used to read a single word from a register.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_SINGLE_READ`  |
| 1     | Address[23:16]     |
| 2     | Address[15:8]      |
| 3     | Address[7:0]       |

**Response:**

| Byte  | Description        |
|-------|--------------------|
| 0     | Data[7:0]          |
| 1     | Data[15:8]         |
| 2     | Data[23:16]        |
| 3     | Data[31:24]        |

### `CMD_INTERNAL_WRITE`

This command is used to write to an internal register.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_INTERNAL_WRITE`|
| 1     | Address[15:8]      |
| 2     | Address[7:0]       |
| 3     | Data[7:0]          |
| 4     | Data[15:8]         |
| 5     | Data[23:16]        |
| 6     | Data[31:24]        |

**Response:**

None.

### `CMD_RESET`

This command is used to reset the WINC1500.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_RESET`        |

**Response:**

None.

### `CMD_SINGLE_WRITE`

This command is used to write a single word to a register.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_SINGLE_WRITE` |
| 1     | Address[23:16]     |
| 2     | Address[15:8]      |
| 3     | Address[7:0]       |
| 4     | Data[7:0]          |
| 5     | Data[15:8]         |
| 6     | Data[23:16]        |
| 7     | Data[31:24]        |

**Response:**

None.

### `CMD_INTERNAL_READ`

This command is used to read from an internal register.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_INTERNAL_READ`|
| 1     | Address[15:8]      |
| 2     | Address[7:0]       |

**Response:**

| Byte  | Description        |
|-------|--------------------|
| 0     | Data[7:0]          |
| 1     | Data[15:8]         |
| 2     | Data[23:16]        |
| 3     | Data[31:24]        |

### `CMD_DMA_EXT_READ`

This command is used to read a block of data from memory using extended DMA.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_DMA_EXT_READ` |
| 1     | Address[23:16]     |
| 2     | Address[15:8]      |
| 3     | Address[7:0]       |
| 4     | Size[23:16]        |
| 5     | Size[15:8]         |
| 6     | Size[7:0]          |

**Response:**

A block of data of the specified size.

### `CMD_DMA_EXT_WRITE`

This command is used to write a block of data to memory using extended DMA.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_DMA_EXT_WRITE`|
| 1     | Address[23:16]     |
| 2     | Address[15:8]      |
| 3     | Address[7:0]       |
| 4     | Size[23:16]        |
| 5     | Size[15:8]         |
| 6     | Size[7:0]          |

**Data:**

A block of data of the specified size.

**Response:**

None.

### `CMD_DMA_READ`

This command is used to read a block of data from memory using DMA.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_DMA_READ`     |
| 1     | Address[23:16]     |
| 2     | Address[15:8]      |
| 3     | Address[7:0]       |
| 4     | Size[15:8]         |
| 5     | Size[7:0]          |

**Response:**

A block of data of the specified size.

### `CMD_DMA_WRITE`

This command is used to write a block of data to memory using DMA.

**Command:**

| Byte  | Description        |
|-------|--------------------|
| 0     | `CMD_DMA_WRITE`    |
| 1     | Address[23:16]     |
| 2     | Address[15:8]      |
| 3     | Address[7:0]       |
| 4     | Size[15:8]         |
| 5     | Size[7:0]          |

**Data:**

A block of data of the specified size.

**Response:**

None.

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

## Chip Initialization Sequence

The following sequence must be followed to initialize the WINC1500:

1.  **Reset the chip:** Send the `CMD_RESET` command to reset the chip.
2.  **Read the chip ID:** Read the `CHIPID` register to verify that the chip is present and responding. The expected value is `0x1002a0`.
3.  **Configure the SPI protocol:** Write `0x00000054` to the `NMI_SPI_PROTOCOL_CONFIG` register to configure the SPI protocol with CRC enabled and a packet size of 8192 bytes. `NMI_SPI_PROTOCOL_CONFIG` is at address `0xE824`.
5.  **Enable interrupts:** Write `0x00000001` to the `NMI_INTR_ENABLE` register to enable interrupts.
6.  **Configure clocks:** Write `0x00000002` to the `WAKE_CLK_REG` register and `0x00000004` to the `CLOCKS_EN_REG` register to configure the clocks.
7.  **Load the firmware:** Write the firmware to the WINC1500's memory at address `0x0000` using the `CMD_DMA_EXT_WRITE` command.
8.  **Verify the firmware:** Read the firmware from the WINC1500's memory at address `0x0000` using the `CMD_DMA_EXT_READ` command and verify it against the original firmware.
9.  **Start the firmware:** Write `0x00000001` to the `BOOTROM_REG` register to start the firmware.
10. **Wait for the firmware to start:** Poll the `NMI_STATE_REG` register until it is equal to `0x00000001`.

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
