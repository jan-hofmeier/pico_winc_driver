#include "pico_winc_simulator/winc_hif.h"
#include "pico_winc_simulator/winc_callback_manager.h"
#include "pico_winc_simulator/winc_simulator_app.h"
#include "pico_winc_simulator/sim_log.h"
#include "common/include/nm_common.h" // For NM_BSP_B_L_16
#include "host_drv_19_3_0/driver/include/m2m_types.h" // For HIF group and opcode definitions
#include <string.h> // For memcpy
#include "hardware/gpio.h"

#define NMI_AHB_SHARE_MEM_BASE 0xd0000


// A global buffer to hold the HIF response, placed in the simulated shared memory.
// Max packet size from m2m_hif.h is 1600 - 4 = 1596 bytes.
// For now, let's make it a reasonable size for a small response like a scan done message.
// The simulator's get_memory_ptr does not allocate, so we need to ensure this is in the simulator's
// memory space. For now, this will be handled by the simulator's global memory.
static uint8_t winc_hif_response_buffer[256]; 
static uint32_t winc_hif_response_address = NMI_AHB_SHARE_MEM_BASE;

// The HIF header structure, copied from m2m_hif.h
typedef struct
{
    uint8_t   u8Gid;
    uint8_t   u8Opcode;
    uint16_t  u16Length;
} tstrHifHdr;

#define WIFI_HOST_RCV_CTRL_0    (0x1070)
#define WIFI_HOST_RCV_CTRL_1    (0x1084)

// Function to send a HIF response back to the host
static void winc_hif_send_response(uint8_t u8Gid, uint8_t u8Opcode, uint16_t u16Length, const uint8_t* payload_ptr) {
    tstrHifHdr header;
    header.u8Gid = u8Gid;
    header.u8Opcode = u8Opcode;
    header.u16Length = u16Length; // Total length including HIF header

    // Ensure response buffer is large enough
    if (u16Length > sizeof(winc_hif_response_buffer)) {
        SIM_LOG(SIM_LOG_TYPE_ERROR, "[HIF] Response too large for buffer.", u16Length, sizeof(winc_hif_response_buffer));
        return;
    }

    // Copy header
    memcpy(winc_hif_response_buffer, &header, sizeof(tstrHifHdr));

    // Copy payload if exists
    if (payload_ptr && u16Length > sizeof(tstrHifHdr)) {
        memcpy(winc_hif_response_buffer + sizeof(tstrHifHdr), payload_ptr, u16Length - sizeof(tstrHifHdr));
    }

    // Set the response in simulated memory (this is a conceptual step for a simulator)
    // In a real simulator, this would involve placing the data in a memory region accessible by the host.
    // For this simple sim, we assume the winc_hif_response_buffer is directly accessible.
    
    // Simulate writing to HIF registers to signal response
    // 1. Write the DMA address of the response to WIFI_HOST_RCV_CTRL_1
    winc_creg_write(WIFI_HOST_RCV_CTRL_1, winc_hif_response_address);

    // 2. Write the size and set the interrupt bit in WIFI_HOST_RCV_CTRL_0
    // (size << 2) | (1 << 0)
    uint32_t reg_val = (u16Length << 2) | (1 << 0);
    winc_creg_write(WIFI_HOST_RCV_CTRL_0, reg_val);

    // Assert the interrupt to the host
    gpio_put(22, 0);

    char log_msg_response[SIM_LOG_MESSAGE_STRING_LEN];
    snprintf(log_msg_response, sizeof(log_msg_response), "[HIF] Sent response GID: %u, Opcode: 0x%02X, Length: %u", u8Gid, u8Opcode, u16Length);
    SIM_LOG(SIM_LOG_TYPE_COMMAND, log_msg_response, 0, 0);
}

// Callback to de-assert interrupt when host signals RX is done
static void hif_rx_done_callback(uint32_t address, uint32_t value) {
    // The host writes to this register to signal it has processed the response.
    // We can now de-assert the interrupt line.
    gpio_put(22, 1);
    SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Host signaled RX done. Interrupt de-asserted.", address, value);
}

static void hif_request_callback(uint32_t address, uint32_t value) {
    uint32_t dma_addr = value >> 2;

    SIM_LOG(SIM_LOG_TYPE_COMMAND, "[HIF] Received request. DMA Addr: 0x%08X", dma_addr, 0);

    uint8_t* hif_packet_ptr = get_memory_ptr(dma_addr, sizeof(tstrHifHdr));

    if (hif_packet_ptr) {
        tstrHifHdr* header = (tstrHifHdr*)hif_packet_ptr;
        uint16_t length = header->u16Length;

        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "[HIF] GID: %u, Opcode: 0x%02X, Length: %u", header->u8Gid, header->u8Opcode, length);
        SIM_LOG(SIM_LOG_TYPE_COMMAND, log_msg, dma_addr, 0);

        if (header->u8Gid == M2M_REQ_GROUP_WIFI && header->u8Opcode == M2M_WIFI_REQ_SCAN) {
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Responding to M2M_WIFI_REQ_SCAN with M2M_WIFI_RESP_SCAN_DONE (dummy).", 0, 0);
            
            // Prepare dummy M2M_WIFI_RESP_SCAN_DONE
            tstrM2mScanDone scan_done_resp;
            scan_done_resp.u8NumofCh = 0;   // No APs found
            scan_done_resp.s8ScanState = M2M_SUCCESS; // Success
            // Padding will be handled by memcpy

            // Total length includes HIF header + payload
            uint16_t total_length = sizeof(tstrHifHdr) + sizeof(tstrM2mScanDone);
            
            winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_RESP_SCAN_DONE, total_length, (uint8_t*)&scan_done_resp);
        }

    } else {
        SIM_LOG(SIM_LOG_TYPE_ERROR, "[HIF] Invalid DMA address received", dma_addr, 0);
    }
}

void winc_hif_init(void) {
    // Register the callback for writes to the HIF request trigger register
    winc_creg_register_callback(0x106c, hif_request_callback);
    // Register the callback for writes to the RX done register
    winc_creg_register_callback(WIFI_HOST_RCV_CTRL_0, hif_rx_done_callback);
    SIM_LOG(SIM_LOG_TYPE_NONE, "[HIF] Initialized and callbacks registered.", 0, 0);
}

