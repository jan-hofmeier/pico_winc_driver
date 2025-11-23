#include "pico_winc_simulator/winc_hif.h"
#include "pico_winc_simulator/winc_callback_manager.h"
#include "pico_winc_simulator/winc_simulator_app.h"
#include "pico_winc_simulator/sim_log.h"
#include "host_drv_19_3_0/driver/source/m2m_hif.h" // For tstrHifHdr and M2M_HIF_HDR_OFFSET
#include "host_drv_19_3_0/driver/include/m2m_wifi.h" // For scan result structures
#include "host_drv_19_3_0/socket/include/m2m_socket_host_if.h" // For tstrDnsReply
#include "wifi_credentials.h"
#include <string.h> // For memcpy
#include "hardware/gpio.h"

#define M2M_HIF_HDR_OFFSET (sizeof(tstrHifHdr) + 4)
#define WORD_ALIGN(val) (((val) & 0x03) ? ((val) + 4 - ((val) & 0x03)) : (val))
#define SWAP_16(x) ((((x) & 0x00FF) << 8) | (((x) & 0xFF00) >> 8))
#define SWAP_32(x) ((((x) & 0x000000FF) << 24) | (((x) & 0x0000FF00) << 8) | (((x) & 0x00FF0000) >> 8) | (((x) & 0xFF000000) >> 24))

typedef enum {
    HIF_STATE_IDLE,
    HIF_STATE_PENDING_DHCP
} hif_state_t;

static hif_state_t g_hif_state = HIF_STATE_IDLE;
static uint32_t winc_hif_response_address = SHARED_MEM_BASE;

#define WIFI_HOST_RCV_CTRL_0    (0x1070)
#define WIFI_HOST_RCV_CTRL_1    (0x1084)

// Macro to byte-swap a 16-bit value
#define SWAP_16(x) ((((x) & 0x00FF) << 8) | (((x) & 0xFF00) >> 8))

// Function to send a HIF response back to the host
static void winc_hif_send_response(uint8_t u8Gid, uint8_t u8Opcode, uint16_t u16Length, const uint8_t* payload_ptr, uint16_t payload_size) {
    tstrHifHdr header;
    header.u8Gid = u8Gid;
    header.u8Opcode = u8Opcode;
    header.u16Length = u16Length; // Total word-aligned length

    // Get a pointer to the shared memory where the response will be placed
    uint8_t* response_buffer_ptr = get_memory_ptr(winc_hif_response_address, u16Length);
    if (!response_buffer_ptr) {
        SIM_LOG(SIM_LOG_TYPE_ERROR, "[HIF] Failed to get memory for response.", winc_hif_response_address, u16Length);
        return;
    }

    // The length field in the header must be big-endian.
    header.u16Length = SWAP_16(header.u16Length);
    
    // Copy header
    memcpy(response_buffer_ptr, &header, sizeof(tstrHifHdr));

    // Copy payload if exists, at the correct offset
    if (payload_ptr && payload_size > 0) {
        memcpy(response_buffer_ptr + M2M_HIF_HDR_OFFSET, payload_ptr, payload_size);
    }
    
    // Simulate writing to HIF registers to signal response
    // 1. Write the DMA address of the response to WIFI_HOST_RCV_CTRL_1
    winc_creg_write(WIFI_HOST_RCV_CTRL_1, winc_hif_response_address);

    // 2. Write the size and set the interrupt bit in WIFI_HOST_RCV_CTRL_0
    // The size in this register is the word-aligned bus transfer size.
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
    // Check if the RX Done bit (bit 1) is being set
    if (value & (1 << 1)) {
        // The host writes to this register to signal it has processed the response.
        // We can now de-assert the interrupt line.
        gpio_put(22, 1);
        SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Host signaled RX done. Interrupt de-asserted.", address, value);

        // Check state machine for pending actions
        if (g_hif_state == HIF_STATE_PENDING_DHCP) {
            // Now that the host has processed the connect response, send the DHCP response.
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Sending dummy DHCP response.", 0, 0);
            tstrM2MIPConfig ip_config;
            memset(&ip_config, 0, sizeof(tstrM2MIPConfig));
            
            // IP addresses are in network byte order (big endian)
            ip_config.u32StaticIP = SWAP_32(0xC0A80164); // 192.168.1.100
            ip_config.u32Gateway = SWAP_32(0xC0A80101);   // 192.168.1.1
            ip_config.u32SubnetMask = SWAP_32(0xFFFFFF00); // 255.255.255.0
            ip_config.u32DNS = SWAP_32(0xC0A80101);       // 192.168.1.1
            ip_config.u32DhcpLeaseTime = 86400; // 1 day

            uint16_t payload_size = sizeof(tstrM2MIPConfig);
            uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
            uint16_t aligned_length = WORD_ALIGN(logical_length);

            winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_REQ_DHCP_CONF, aligned_length, (uint8_t*)&ip_config, payload_size);
            
            g_hif_state = HIF_STATE_IDLE;
        }
    }
}

static void hif_request_callback(uint32_t address, uint32_t value) {
    uint32_t dma_addr = value >> 2;

    SIM_LOG(SIM_LOG_TYPE_COMMAND, "[HIF] Received request. DMA Addr: 0x%08X", dma_addr, 0);

    uint8_t* hif_packet_ptr = get_memory_ptr(dma_addr, sizeof(tstrHifHdr));

    if (hif_packet_ptr) {
        tstrHifHdr* header = (tstrHifHdr*)hif_packet_ptr;
        uint16_t length = header->u16Length; // Length in request header is little-endian

        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "[HIF] GID: %u, Opcode: 0x%02X, Length: %u", header->u8Gid, header->u8Opcode, length);
        SIM_LOG(SIM_LOG_TYPE_COMMAND, log_msg, dma_addr, 0);

        if (header->u8Gid == M2M_REQ_GROUP_WIFI && header->u8Opcode == M2M_WIFI_REQ_SCAN) {
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Responding to M2M_WIFI_REQ_SCAN with SCAN_DONE.", 0, 0);
            
            // Just send the SCAN_DONE message. The host will request results individually.
            tstrM2mScanDone scan_done_resp;
            scan_done_resp.u8NumofCh = 1; // One AP found
            scan_done_resp.s8ScanState = M2M_SUCCESS;

            uint16_t payload_size = sizeof(tstrM2mScanDone);
            uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
            uint16_t aligned_length = WORD_ALIGN(logical_length);
            
            winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_RESP_SCAN_DONE, aligned_length, (uint8_t*)&scan_done_resp, payload_size);
        
        } else if (header->u8Gid == M2M_REQ_GROUP_WIFI && header->u8Opcode == M2M_WIFI_REQ_SCAN_RESULT) {
            
            tstrM2mReqScanResult* req_scan_result = (tstrM2mReqScanResult*)(hif_packet_ptr + M2M_HIF_HDR_OFFSET);
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Received M2M_WIFI_REQ_SCAN_RESULT for index %u", req_scan_result->u8Index, 0);

            if (req_scan_result->u8Index == 0) {
                SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Sending dummy scan result for index 0.", 0, 0);
                
                tstrM2mWifiscanResult scan_result;
                memset(&scan_result, 0, sizeof(tstrM2mWifiscanResult));

                scan_result.u8index = 0;
                scan_result.s8rssi = -50;
                scan_result.u8AuthType = M2M_WIFI_SEC_WPA_PSK;
                scan_result.u8ch = 1;
                uint8_t dummy_bssid[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
                memcpy(scan_result.au8BSSID, dummy_bssid, sizeof(dummy_bssid));
                strncpy((char*)scan_result.au8SSID, WIFI_SSID, M2M_MAX_SSID_LEN);

                uint16_t payload_size = sizeof(tstrM2mWifiscanResult);
                uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
                uint16_t aligned_length = WORD_ALIGN(logical_length);
                
                winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_RESP_SCAN_RESULT, aligned_length, (uint8_t*)&scan_result, payload_size);
            }
        
        } else if (header->u8Gid == M2M_REQ_GROUP_WIFI && header->u8Opcode == M2M_WIFI_REQ_CONNECT) {
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Responding to M2M_WIFI_REQ_CONNECT with dummy success.", 0, 0);

            // 1. Send connection state changed response
            tstrM2mWifiStateChanged con_state;
            con_state.u8CurrState = M2M_WIFI_CONNECTED;
            con_state.u8ErrCode = M2M_SUCCESS;

            uint16_t payload_size = sizeof(tstrM2mWifiStateChanged);
            uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
            uint16_t aligned_length = WORD_ALIGN(logical_length);

            winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_RESP_CON_STATE_CHANGED, aligned_length, (uint8_t*)&con_state, payload_size);
            
            // 2. Set state to send DHCP response after acknowledgement
            g_hif_state = HIF_STATE_PENDING_DHCP;
        
        } else if (header->u8Gid == M2M_REQ_GROUP_IP && header->u8Opcode == 0x12) { // Undocumented DNS request?
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Responding to undocumented DNS request (Opcode 0x12).", 0, 0);
    
            // The payload of the request is the hostname string.
            char* hostname = (char*)(hif_packet_ptr + M2M_HIF_HDR_OFFSET);
            SIM_LOG(SIM_LOG_TYPE_INFO, hostname, 0, 0);
    
            tstrDnsReply dns_reply;
            memset(&dns_reply, 0, sizeof(tstrDnsReply));
            strncpy(dns_reply.acHostName, hostname, HOSTNAME_MAX_SIZE);
            dns_reply.u32HostIP = SWAP_32(0x01020304); // Dummy IP 1.2.3.4
    
            uint16_t payload_size = sizeof(tstrDnsReply);
            uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
            uint16_t aligned_length = WORD_ALIGN(logical_length);
    
            // The response opcode is the documented one.
            winc_hif_send_response(M2M_REQ_GROUP_IP, SOCKET_CMD_DNS_RESOLVE, aligned_length, (uint8_t*)&dns_reply, payload_size);
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

