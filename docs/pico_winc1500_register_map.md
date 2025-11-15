# WINC1500 Register Map

This document describes the WINC1500's register map.

## Register Summary

| Address | Name                      | Description              |
| ------- | ------------------------- | ------------------------ |
| 0x0000  | `CHIPID`                  | Chip ID                  |
| 0x0004  | `VERSION`                 | Firmware version         |
| 0x0008  | `STATUS`                  | Device status            |
| 0x000C  | `CONTROL`                 | Device control           |
| 0x0010  | `INT_STATUS`              | Interrupt status         |
| 0x0014  | `INT_ENABLE`              | Interrupt enable         |
| 0x1000  | `WIFI_STATUS`             | Wi-Fi status             |
| 0x1004  | `WIFI_CONTROL`            | Wi-Fi control            |
| 0x1008  | `WIFI_TX_DATA`            | Wi-Fi transmit data      |
| 0x100C  | `WIFI_RX_DATA`            | Wi-Fi receive data       |
| 0x2000  | `SOCKET_STATUS`           | Socket status            |
| 0x2004  | `SOCKET_CONTROL`          | Socket control           |
| 0x2008  | `SOCKET_TX_DATA`          | Socket transmit data     |
| 0x200C  | `SOCKET_RX_DATA`          | Socket receive data      |
| 0x1400  | `NMI_GLB_RESET_0`         | Global reset             |
| 0x1408  | `NMI_PIN_MUX_0`           | Pin multiplexer          |
| 0x1A00  | `NMI_INTR_ENABLE`         | Interrupt enable         |
| 0x108C  | `NMI_STATE_REG`           | State                    |
| 0x3000  | `BOOTROM_REG`             | Boot ROM                 |
| 0x1504  | `WIFI_HOST_RCV_CTRL_4`    | Wi-Fi host receive control 4 |
| 0x1084  | `WIFI_HOST_RCV_CTRL_1`    | Wi-Fi host receive control 1 |
| 0x1078  | `WIFI_HOST_RCV_CTRL_2`    | Wi-Fi host receive control 2 |
| 0x106C  | `WIFI_HOST_RCV_CTRL_3`    | Wi-Fi host receive control 3 |
| 0x1088  | `WIFI_HOST_RCV_CTRL_5`    | Wi-Fi host receive control 5 |
| 0xE800  | `NMI_SPI_REG_BASE`        | SPI register base          |
| 0xE824  | `NMI_SPI_PROTOCOL_CONFIG` | SPI protocol configuration |
| 0x1     | `WAKE_CLK_REG`            | Wake clock               |
| 0xF     | `CLOCKS_EN_REG`           | Clocks enable            |
| 0x1070  | `WIFI_HOST_RCV_CTRL_0`    | Wi-Fi host receive control 0 |
| 0x207AC | `NMI_REV_REG`             | Revision                 |

## Register Details

### `CHIPID`

*   **Address:** 0x0000
*   **Access:** Read-only
*   **Description:** This register contains the chip ID.

### `VERSION`

*   **Address:** 0x0004
*   **Access:** Read-only
*   **Description:** This register contains the firmware version.

### `STATUS`

*   **Address:** 0x0008
*   **Access:** Read-only
*   **Description:** This register contains the device status.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 0   | `READY`      | When set to 1, the device is ready.       |
| 1   | `ERROR`      | When set to 1, an error has occurred.     |

### `CONTROL`

*   **Address:** 0x000C
*   **Access:** Read/write
*   **Description:** This register is used to control the device.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 0   | `ENABLE`     | When set to 1, enables the device.        |
| 1   | `RESET`      | When set to 1, resets the device.         |

### `INT_STATUS`

*   **Address:** 0x0010
*   **Access:** Read-only
*   **Description:** This register contains the interrupt status.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 0   | `RX_DONE`    | When set to 1, a packet has been received.|
| 1   | `TX_DONE`    | When set to 1, a packet has been sent.    |

### `INT_ENABLE`

*   **Address:** 0x0014
*   **Access:** Read/write
*   **Description:** This register is used to enable or disable interrupts.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 0   | `RX_DONE_EN` | When set to 1, enables the RX_DONE interrupt. |
| 1   | `TX_DONE_EN` | When set to 1, enables the TX_DONE interrupt. |

### `WIFI_STATUS`

*   **Address:** 0x1000
*   **Access:** Read-only
*   **Description:** This register contains the Wi-Fi status.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 0   | `CONNECTED`  | When set to 1, the device is connected to a Wi-Fi network. |
| 1   | `SCANNING`   | When set to 1, the device is scanning for Wi-Fi networks. |

### `WIFI_CONTROL`

*   **Address:** 0x1004
*   **Access:** Read/write
*   **Description:** This register is used to control the Wi-Fi module.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 0   | `CONNECT`    | When set to 1, the device will connect to a Wi-Fi network. |
| 1   | `DISCONNECT` | When set to 1, the device will disconnect from a Wi-Fi network. |
| 2   | `SCAN`       | When set to 1, the device will scan for Wi-Fi networks. |

### `WIFI_TX_DATA`

*   **Address:** 0x1008
*   **Access:** Write-only
*   **Description:** This register is used to transmit Wi-Fi data.

### `WIFI_RX_DATA`

*   **Address:** 0x100C
*   **Access:** Read-only
*   **Description:** This register is used to receive Wi-Fi data.

### `SOCKET_STATUS`

*   **Address:** 0x2000
*   **Access:** Read-only
*   **Description:** This register contains the socket status.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 0   | `CONNECTED`  | When set to 1, the socket is connected.   |
| 1   | `LISTENING`  | When set to 1, the socket is listening for connections. |

### `SOCKET_CONTROL`

*   **Address:** 0x2004
*   **Access:** Read/write
*   **Description:** This register is used to control the sockets.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 0   | `BIND`       | When set to 1, the socket will be bound to a local address. |
| 1   | `LISTEN`     | When set to 1, the socket will listen for connections. |
| 2   | `ACCEPT`     | When set to 1, the socket will accept a connection. |
| 3   | `CONNECT`    | When set to 1, the socket will connect to a remote address. |
| 4   | `SEND`       | When set to 1, the socket will send data. |
| 5   | `RECV`       | When set to 1, the socket will receive data. |
| 6   | `CLOSE`      | When set to 1, the socket will be closed. |

### `SOCKET_TX_DATA`

*   **Address:** 0x2008
*   **Access:** Write-only
*   **Description:** This register is used to transmit socket data.

### `SOCKET_RX_DATA`

*   **Address:** 0x200C
*   **Access:** Read-only
*   **Description:** This register is used to receive socket data.

### `NMI_GLB_RESET_0`

*   **Address:** 0x1400
*   **Access:** Read/write
*   **Description:** This register is used to perform a global reset of the WINC1500.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 0   | `RESET_BIT`  | When set to 1, resets the entire chip.    |

### `NMI_PIN_MUX_0`

*   **Address:** 0x1408
*   **Access:** Read/write
*   **Description:** This register is used to configure the pin multiplexer.

| Bit(s) | Name        | Description                                       |
|--------|-------------|---------------------------------------------------|
| 7:0    | `MUX_SEL_0` | Selects the function for GPIO pin 0.              |
| 15:8   | `MUX_SEL_1` | Selects the function for GPIO pin 1.              |
| 23:16  | `MUX_SEL_2` | Selects the function for GPIO pin 2.              |
| 31:24  | `MUX_SEL_3` | Selects the function for GPIO pin 3.              |

### `NMI_INTR_ENABLE`

*   **Address:** 0x1A00
*   **Access:** Read/write
*   **Description:** This register is used to enable or disable interrupts.

| Bit | Name             | Description                               |
|-----|------------------|-------------------------------------------|
| 0   | `INT_ENABLE_BIT` | When set to 1, enables all interrupts.    |

### `NMI_STATE_REG`

*   **Address:** 0x108C
*   **Access:** Read/write
*   **Description:** This register is used to track the state of the WINC1500.

| Bit(s) | Name          | Description                                     |
|--------|---------------|-------------------------------------------------|
| 7:0    | `STATE`       | The current state of the WINC1500.              |
| 31:8   | `RESERVED`    | Reserved for future use.                        |

### `WAKE_CLK_REG`

*   **Address:** 0x1
*   **Access:** Read/write
*   **Description:** This register is used to control the wake clock.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 1   | `WAKE_CLK`   | When set to 1, wakes the chip.            |

### `CLOCKS_EN_REG`

*   **Address:** 0xF
*   **Access:** Read/write
*   **Description:** This register is used to enable or disable clocks.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 2   | `CLK_EN`     | When set to 1, enables the clocks.        |

### `WIFI_HOST_RCV_CTRL_0`

*   **Address:** 0x1070
*   **Access:** Read/write
*   **Description:** This register is used to control the Wi-Fi host receive.

| Bit(s) | Name          | Description                                     |
|--------|---------------|-------------------------------------------------|
| 0      | `RX_DONE`     | When set to 1, indicates that a packet has been received. |
| 11:2   | `RX_SIZE`     | The size of the received packet.                |

### `WIFI_HOST_RCV_CTRL_1`

*   **Address:** 0x1084
*   **Access:** Read-only
*   **Description:** This register contains the address of the received packet.

| Bit(s) | Name          | Description                                     |
|--------|---------------|-------------------------------------------------|
| 31:0   | `RX_ADDR`     | The address of the received packet.             |

### `WIFI_HOST_RCV_CTRL_2`

*   **Address:** 0x1078
*   **Access:** Read/write
*   **Description:** This register is used to control the Wi-Fi host receive.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 1   | `RX_REQ`     | When set to 1, requests to receive a packet. |

### `WIFI_HOST_RCV_CTRL_3`

*   **Address:** 0x106C
*   **Access:** Write-only
*   **Description:** This register is used to control the Wi-Fi host receive.

| Bit(s) | Name          | Description                                     |
|--------|---------------|-------------------------------------------------|
| 31:2   | `DMA_ADDR`    | The address of the DMA buffer.                  |
| 1      | `DMA_START`   | When set to 1, starts the DMA transfer.         |

### `WIFI_HOST_RCV_CTRL_4`

*   **Address:** 0x1504
*   **Access:** Read-only
*   **Description:** This register contains the DMA address.

| Bit(s) | Name          | Description                                     |
|--------|---------------|-------------------------------------------------|
| 31:0   | `DMA_ADDR`    | The address of the DMA buffer.                  |

### `WIFI_HOST_RCV_CTRL_5`

*   **Address:** 0x1088
*   **Access:** Read/write
*   **Description:** This register is used to control the Wi-Fi host receive.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 0   | `RX_MODE`    | When set to 1, enables DMA mode.          |

### `NMI_SPI_PROTOCOL_CONFIG`

*   **Address:** 0xE824
*   **Access:** Read/write
*   **Description:** This register is used to configure the SPI protocol.

| Bit(s) | Name          | Description                                     |
|--------|---------------|-------------------------------------------------|
| 2      | `CRC_EN`      | 0: Disable CRC, 1: Enable CRC.                  |
| 6:4    | `PACKET_SIZE` | The size of the SPI packets.                    |

### `BOOTROM_REG`

*   **Address:** 0x3000
*   **Access:** Read/write
*   **Description:** This register is used to control the boot ROM.

| Bit | Name         | Description                               |
|-----|--------------|-------------------------------------------|
| 0   | `BOOT_EN`    | When set to 1, enables the boot ROM.      |

### `NMI_REV_REG`

*   **Address:** 0x207AC
*   **Access:** Read-only
*   **Description:** This register contains the revision of the WINC1500.

| Bit(s) | Name          | Description                                     |
|--------|---------------|-------------------------------------------------|
| 7:0    | `REVISION`    | The revision of the WINC1500.                   |
| 31:8   | `RESERVED`    | Reserved for future use.                        |
