#include "pico_winc_simulator/winc_hif.h"
#include "pico_winc_simulator/winc_callback_manager.h"
#include "pico_winc_simulator/winc_simulator_app.h"
#include "pico_winc_simulator/sim_log.h"
#include "pico_winc_simulator/pio_spi.h"
#include "common/include/nm_common.h" // For NM_BSP_B_L_16
#include "driver/include/m2m_types.h"
#include "wifi_credentials.h"
#include <string.h>

// The HIF header structure, copied from m2m_hif.h
typedef struct
{
    uint8_t   u8Gid;
    uint8_t   u8Opcode;
    uint16_t  u16Length;
} tstrHifHdr;

// Response buffer location in WINC memory (Safe area)
#define HIF_RESPONSE_ADDR 0x10000

// Handles the IRQ acknowledge from the host
static void hif_irq_ack_callback(uint32_t address, uint32_t value) {
    // Value has format: (Size << 2) | (1 << 0) if New Interrupt
    // or just clearing bit 0.
    // The driver reads RCV_CTRL_0 to check interrupt bit (bit 0).
    // Then it writes back to clear it.

    // We only care if the host is writing to clear the interrupt.
    // However, the driver writes `reg &= ~(1<<0)` so bit 0 will be 0.

    // Actually, winc_creg_process_requests passes the *current value* of the register
    // BEFORE the write if we look at `winc_creg_process_requests` implementation in `winc_callback_manager.c`.
    // Wait, the `winc_creg_process_requests` reads the memory *after* pulling from queue.
    // But `winc_creg_handle_write` is called *after* `memcpy` in `winc_process_command`.
    // So `value` passed to callback is the NEW value written by the host.

    if ((value & 1) == 0) {
        SIM_LOG(SIM_LOG_TYPE_COMMAND, "[HIF] Host cleared IRQ.", 0, 0);
        pio_spi_set_irq_pin(false);
    }
}

static void winc_hif_send_response(uint8_t gid, uint8_t opcode, uint8_t *payload, uint16_t payload_len) {
    tstrHifHdr header;
    header.u8Gid = gid;
    header.u8Opcode = opcode;
    header.u16Length = sizeof(tstrHifHdr) + payload_len; // Little Endian

    // 1. Write Header
    uint8_t *mem_ptr = get_memory_ptr(HIF_RESPONSE_ADDR, sizeof(tstrHifHdr));
    if (mem_ptr) {
        memcpy(mem_ptr, &header, sizeof(tstrHifHdr));
    }

    // 2. Write Payload
    if (payload && payload_len > 0) {
        mem_ptr = get_memory_ptr(HIF_RESPONSE_ADDR + sizeof(tstrHifHdr), payload_len);
        if (mem_ptr) {
            memcpy(mem_ptr, payload, payload_len);
        }
    }

    // 3. Update Registers
    // WIFI_HOST_RCV_CTRL_1 = Address of the packet
    uint32_t pkt_addr = HIF_RESPONSE_ADDR;
    mem_ptr = get_memory_ptr(WIFI_HOST_RCV_CTRL_1, 4);
    if (mem_ptr) memcpy(mem_ptr, &pkt_addr, 4);

    // WIFI_HOST_RCV_CTRL_0 = (Size << 2) | 1 (IRQ Pending)
    uint32_t size_reg = (header.u16Length << 2) | 1;
    mem_ptr = get_memory_ptr(WIFI_HOST_RCV_CTRL_0, 4);
    if (mem_ptr) memcpy(mem_ptr, &size_reg, 4);

    // 4. Trigger Interrupt
    SIM_LOG(SIM_LOG_TYPE_COMMAND, "[HIF] Sending Response GID:%d Op:%02X Len:%d", gid, opcode, header.u16Length);
    pio_spi_set_irq_pin(true);
}

static void winc_hif_send_scan_done(void) {
    tstrM2mScanDone scan_done;
    memset(&scan_done, 0, sizeof(scan_done));
    scan_done.u8NumofCh = 1; // 1 AP found
    scan_done.s8ScanState = 0; // Success

    winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_RESP_SCAN_DONE, (uint8_t*)&scan_done, sizeof(scan_done));
}

static void winc_hif_send_scan_result(void) {
    tstrM2mWifiscanResult scan_result;
    memset(&scan_result, 0, sizeof(scan_result));

    scan_result.u8index = 0;
    scan_result.s8rssi = -50;
    scan_result.u8AuthType = M2M_WIFI_SEC_WPA_PSK;
    scan_result.u8ch = 6;

    // Dummy BSSID: 00:11:22:33:44:55
    scan_result.au8BSSID[0] = 0x00;
    scan_result.au8BSSID[1] = 0x11;
    scan_result.au8BSSID[2] = 0x22;
    scan_result.au8BSSID[3] = 0x33;
    scan_result.au8BSSID[4] = 0x44;
    scan_result.au8BSSID[5] = 0x55;

    strncpy((char*)scan_result.au8SSID, WIFI_SSID, M2M_MAX_SSID_LEN - 1);

    winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_RESP_SCAN_RESULT, (uint8_t*)&scan_result, sizeof(scan_result));
}

static void hif_request_callback(uint32_t address, uint32_t value) {
    // The value passed is the DMA address with a flag.
    // The driver code shows: reg = dma_addr << 2; reg |= (1 << 1);
    // So, we need to shift right by 2 to get the actual DMA address.
    uint32_t dma_addr = value >> 2;

    SIM_LOG(SIM_LOG_TYPE_COMMAND, "[HIF] Received request. DMA Addr: 0x%08X", dma_addr, 0);

    // Get a pointer to the start of the HIF message in simulated memory
    // The size of the header is what we need to read first.
    uint8_t* hif_packet_ptr = get_memory_ptr(dma_addr, sizeof(tstrHifHdr));

    if (hif_packet_ptr) {
        // Cast the memory pointer to the HIF header structure
        tstrHifHdr* header = (tstrHifHdr*)hif_packet_ptr;

        // The u16Length is in Little Endian format as sent by the host driver.
        // The RP2040 is a Little Endian architecture, so no byte swapping is needed to read u16Length.
        uint16_t length = header->u16Length;

        // Format the log message into a buffer
        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "[HIF] GID: %u, Opcode: 0x%02X, Length: %u", header->u8Gid, header->u8Opcode, length);

        // Log the pre-formatted message. Pass dma_addr as the value.
        SIM_LOG(SIM_LOG_TYPE_COMMAND, log_msg, dma_addr, 0);

        // Handle Request
        if (header->u8Gid == M2M_REQ_GROUP_WIFI) {
            if (header->u8Opcode == M2M_WIFI_REQ_SCAN) {
                SIM_LOG(SIM_LOG_TYPE_COMMAND, "[HIF] Processing SCAN Request", 0, 0);
                winc_hif_send_scan_done();
            } else if (header->u8Opcode == M2M_WIFI_REQ_SCAN_RESULT) {
                SIM_LOG(SIM_LOG_TYPE_COMMAND, "[HIF] Processing SCAN RESULT Request", 0, 0);
                winc_hif_send_scan_result();
            }
        }

    } else {
        SIM_LOG(SIM_LOG_TYPE_ERROR, "[HIF] Invalid DMA address received", dma_addr, 0);
    }
}

void winc_hif_init(void) {
    // Register the callback for writes to WIFI_HOST_RCV_CTRL_3 (0x106c) - HIF Request
    winc_creg_register_callback(0x106c, hif_request_callback);

    // Register callback for WIFI_HOST_RCV_CTRL_0 (0x1070) - Host IRQ Ack
    winc_creg_register_callback(0x1070, hif_irq_ack_callback);

    SIM_LOG(SIM_LOG_TYPE_NONE, "[HIF] Initialized and callback registered for 0x106c/0x1070.", 0, 0);
}
