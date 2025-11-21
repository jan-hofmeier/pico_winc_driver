# WINC1500 Register Map

This document describes the WINC1500's register map, with corrections and additions based on analysis of the driver source code.

## Register Summary

| Address | Name                      | Description                                                                                             |
|---------|---------------------------|---------------------------------------------------------------------------------------------------------|
| 0x1000  | `CHIPID`                  | Chip ID                                                                                                 |
| 0x13f4  | `RF_REV`                  | RF Revision ID                                                                                          |
| 0x108c  | `NMI_STATE_REG`           | Command/State register. Used by the host to send commands to the firmware.                              |
| 0x1408  | `NMI_PIN_MUX_0`           | Pin multiplexer configuration.                                                                          |
| 0x1a00  | `NMI_INTR_ENABLE`         | Interrupt enable register.                                                                              |
| 0xc000c | `BOOTROM_REG`             | Boot ROM control register.                                                                              |
| 0x1048  | `NMI_REV_REG_ATE`         | ATE Firmware Revision register.                                                                         |
| 0x149c  | `rNMI_GP_REG_0`           | General purpose register 0.                                                                             |
| 0x14a0  | `rNMI_GP_REG_1`           | General purpose register 1.                                                                             |
| 0xc0008 | `rNMI_GP_REG_2`           | General purpose register 2.                                                                             |
| 0x1118  | `rNMI_BOOT_RESET_MUX`     | Boot reset mux control.                                                                                 |
| 0x207bc | `M2M_WAIT_FOR_HOST_REG`   | Register used by firmware to signal that it is waiting for the host.                                    |
| 0x1074  | `WAKE_REG`                | Wakeup register. Used to wake the chip from sleep mode.                                                 |
| 0x1070  | `WIFI_HOST_RCV_CTRL_0`    | HIF receive control register 0. Used by firmware to signal a received packet and its size.                |
| 0x1084  | `WIFI_HOST_RCV_CTRL_1`    | HIF receive control register 1. Contains the address of the received packet.                            |
| 0x1078  | `WIFI_HOST_RCV_CTRL_2`    | HIF receive control register 2. Used by the host to request a DMA transfer.                               |
| 0x106c  | `WIFI_HOST_RCV_CTRL_3`    | HIF receive control register 3. Used by the host to provide the DMA address and start the transfer.       |
| 0xe824  | `NMI_SPI_PROTOCOL_CONFIG` | SPI protocol configuration.                                                                             |
| 0x207ac | `NMI_REV_REG`             | Firmware Revision register.                                                                             |
| 0xe800  | `NMI_SPI_CTL`             | SPI control register.                                                                                   |
| 0xe804  | `NMI_SPI_MASTER_DMA_ADDR` | SPI master DMA address register.                                                                        |
| 0xe808  | `NMI_SPI_MASTER_DMA_COUNT`| SPI master DMA count register.                                                                          |
| 0xe80c  | `NMI_SPI_SLAVE_DMA_ADDR`  | SPI slave DMA address register.                                                                         |
| 0xe810  | `NMI_SPI_SLAVE_DMA_COUNT` | SPI slave DMA count register.                                                                           |
| 0xe820  | `NMI_SPI_TX_MODE`         | SPI TX mode register.                                                                                   |
| 0xe82c  | `NMI_SPI_INTR_CTL`        | SPI interrupt control register.                                                                         |

## Register Details

### `CHIPID`

*   **Address:** 0x1000
*   **Access:** Read-only
*   **Description:** This register contains the chip ID.

### `NMI_STATE_REG`

*   **Address:** 0x108c
*   **Access:** Read/Write
*   **Description:** This register is used by the host to send commands to the firmware. The host writes a 32-bit value with the format `(length << 16) | (opcode << 8) | group_id`. The firmware also uses this register to indicate its state to the host.

### `BOOTROM_REG`

*   **Address:** 0xc000c
*   **Access:** Read/Write
*   **Description:** This register is used to control the boot ROM.

### `WIFI_HOST_RCV_CTRL_0`

*   **Address:** 0x1070
*   **Access:** Read/Write
*   **Description:** This register is used by the firmware to signal that a packet has been received.
| Bit(s) | Name          | Description                                     |
|--------|---------------|-------------------------------------------------|
| 0      | `RX_DONE`     | When set to 1, indicates that a packet has been received. |
| 13:2   | `RX_SIZE`     | The size of the received packet.                |

### `WIFI_HOST_RCV_CTRL_1`

*   **Address:** 0x1084
*   **Access:** Read-only
*   **Description:** This register contains the address of the received packet in the WINC1500's memory.

### `WIFI_HOST_RCV_CTRL_2`

*   **Address:** 0x1078
*   **Access:** Read/Write
*   **Description:** This register is used by the host to request a DMA transfer from the WINC1500.
| Bit | Name     | Description                            |
|-----|----------|----------------------------------------|
| 1   | `RX_REQ` | When set to 1, requests a DMA transfer. |

### `WIFI_HOST_RCV_CTRL_3`

*   **Address:** 0x106c
*   **Access:** Write-only
*   **Description:** This register is used by the host to provide the DMA address and start the transfer.
| Bit(s) | Name        | Description                             |
|--------|-------------|-----------------------------------------|
| 1      | `DMA_START` | When set to 1, starts the DMA transfer.   |
| 31:2   | `DMA_ADDR`  | The address of the DMA buffer.          |