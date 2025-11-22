#include "pico_winc_simulator/winc_hif.h"
#include "pico_winc_simulator/winc_callback_manager.h"
#include "pico_winc_simulator/winc_simulator_app.h"
#include "pico_winc_simulator/sim_log.h"
#include "common/include/nm_common.h" // For NM_BSP_B_L_16

// The HIF header structure, copied from m2m_hif.h
typedef struct
{
    uint8_t   u8Gid;
    uint8_t   u8Opcode;
    uint16_t  u16Length;
} tstrHifHdr;

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

    } else {
        SIM_LOG(SIM_LOG_TYPE_ERROR, "[HIF] Invalid DMA address received", dma_addr, 0);
    }
}

void winc_hif_init(void) {
    // Register the callback for writes to WIFI_HOST_RCV_CTRL_3 (0x106c)
    winc_creg_register_callback(0x106c, hif_request_callback);
    SIM_LOG(SIM_LOG_TYPE_NONE, "[HIF] Initialized and callback registered for 0x106c.", 0, 0);
}
