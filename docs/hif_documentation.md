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

## HIF Structures

### tstrHifHdr

This is the main header structure for all HIF messages.

| Member      | Type      | Description                                     |
|-------------|-----------|-------------------------------------------------|
| `u8Gid`     | `uint8_t` | The Group ID of the message (e.g., WiFi, IP, OTA). |
| `u8Opcode`  | `uint8_t` | The Operation ID for the specific command.        |
| `u16Length` | `uint16_t`| The total length of the HIF packet, including the header. |

## HIF Communication Flow

### Host to WINC1500 (hif_send)

This flow describes how the host driver sends a command packet to the WINC1500 firmware.

1.  **Wake Chip:** The host ensures the WINC1500 is awake by calling `hif_chip_wake()`.
2.  **Write Command State:** The host packs the Group ID, Opcode, and total packet length into a single 32-bit value and writes it to `NMI_STATE_REG` (0x108c). This signals the start of a new HIF command.
3.  **Request DMA Buffer:** The host writes `0x02` to `WIFI_HOST_RCV_CTRL_2` (0x1078). This tells the firmware that the host needs a memory buffer to write the HIF packet into.
4.  **Poll for 'Ready' and Get DMA Address:** The host polls `WIFI_HOST_RCV_CTRL_2`, waiting for the firmware to clear the bit that was just set. Once cleared, the host knows the firmware is ready and has provided a DMA address. The host reads this DMA address from a hardcoded memory location (`0x150400`).
5.  **Write HIF Packet via DMA:** Using the acquired DMA address, the host performs block writes to transfer the HIF packet, which consists of:
    *   The `tstrHifHdr` (HIF Header).
    *   A control buffer (if any).
    *   A data buffer (if any).
6.  **Signal Transfer Complete:** The host writes the DMA address (shifted left by 2) plus a 'transfer complete' flag into `WIFI_HOST_RCV_CTRL_3` (0x106c). **This is the final step and the key trigger.** It tells the firmware, "The complete HIF packet is now available for you to process at this DMA address."
7.  **Sleep Chip:** The host may put the chip back to sleep by calling `hif_chip_sleep()`.

### WINC1500 to Host (hif_isr)

1.  The WINC1500 firmware generates an interrupt to the host.
2.  The host's interrupt service routine (`hif_isr`) wakes up the chip.
3.  The host reads `WIFI_HOST_RCV_CTRL_0` to check for a new packet and get its size.
4.  The host reads the packet's address from `WIFI_HOST_RCV_CTRL_1`.
5.  The host reads the HIF header and data from the packet's address.
6.  The host processes the packet based on the group ID and opcode in the HIF header.
7.  The host acknowledges the packet reception by writing to `WIFI_HOST_RCV_CTRL_0`.
8.  The host puts the chip back to sleep.
