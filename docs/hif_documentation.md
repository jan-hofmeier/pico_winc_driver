# WINC1500 HIF Protocol

This document describes the Host Interface (HIF) protocol used by the WINC1500 driver.

## Overview

The HIF protocol is used for communication between the host processor and the WINC1500 firmware. It is a command/response protocol that uses a set of shared registers and memory regions.

## HIF Registers

The following registers are used by the HIF protocol:

| Address | Name                   | Description                                                                                                                                                                                            |
|---------|------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 0x108c  | `NMI_STATE_REG`        | Used by the host to send commands to the firmware. The host writes a 32-bit value with the format `(length << 16) | (opcode << 8) | group_id`.                                                            |
| 0x1070  | `WIFI_HOST_RCV_CTRL_0` | Used by the firmware to signal a received packet. Bit 0 is `RX_DONE`, and bits 2-13 contain the packet size. The host writes to this register to acknowledge packet reception.                               |
| 0x1084  | `WIFI_HOST_RCV_CTRL_1` | Used by the firmware to provide the address of the received packet in the WINC1500's memory.                                                                                                            |
| 0x1078  | `WIFI_HOST_RCV_CTRL_2` | Used by the host to request a DMA transfer from the WINC1500. The host writes `1` to bit 1 to request a transfer.                                                                                          |
| 0x106c  | `WIFI_HOST_RCV_CTRL_3` | Used by the host to provide the DMA address in the host's memory where the received data should be stored. After writing the address, the host sets bit 1 to start the DMA transfer.                        |
| 0x1074  | `WAKE_REG`             | Used to wake the chip from sleep mode. Writing `0x5678` wakes the chip, and writing `0x4321` puts it to sleep.                                                                                             |

## HIF Communication Flow

### Host to WINC1500 (hif_send)

1.  The host wakes up the chip by writing to `WAKE_REG`.
2.  The host writes the command to `NMI_STATE_REG`. The command consists of a group ID, an opcode, and the length of the packet.
3.  The host requests a DMA transfer by writing to `WIFI_HOST_RCV_CTRL_2`.
4.  The host polls `WIFI_HOST_RCV_CTRL_2` until the firmware is ready.
5.  The host gets the DMA address from a firmware-provided address (the exact register is not clear from the source code, it seems to be hardcoded to `0x150400` which is suspicious).
6.  The host writes the HIF header and data to the DMA address.
7.  The host starts the DMA transfer by writing to `WIFI_HOST_RCV_CTRL_3`.
8.  The host puts the chip back to sleep by writing to `WAKE_REG`.

### WINC1500 to Host (hif_isr)

1.  The WINC1500 firmware generates an interrupt to the host.
2.  The host's interrupt service routine (`hif_isr`) wakes up the chip.
3.  The host reads `WIFI_HOST_RCV_CTRL_0` to check for a new packet and get its size.
4.  The host reads the packet's address from `WIFI_HOST_RCV_CTRL_1`.
5.  The host reads the HIF header and data from the packet's address.
6.  The host processes the packet based on the group ID and opcode in the HIF header.
7.  The host acknowledges the packet reception by writing to `WIFI_HOST_RCV_CTRL_0`.
8.  The host puts the chip back to sleep.
