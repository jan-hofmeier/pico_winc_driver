#include "winc_hif.h"
#include "winc_callback_manager.h"
#include "winc_simulator_app.h"
#include "sim_log.h"
#include "driver/source/m2m_hif.h"
#include "m2m_wifi.h"
#include "m2m_socket_host_if.h"
#include "wifi_credentials.h"
#include <string.h>
#include "hardware/gpio.h"

#define WORD_ALIGN(val) (((val) & 0x03) ? ((val) + 4 - ((val) & 0x03)) : (val))
#define SWAP_16(x) ((((x) & 0x00FF) << 8) | (((x) & 0xFF00) >> 8))
#define SWAP_32(x) ((((x) & 0x000000FF) << 24) | (((x) & 0x0000FF00) << 8) | (((x) & 0x00FF0000) >> 8) | (((x) & 0xFF000000) >> 24))

typedef enum {
    HIF_STATE_IDLE,
    HIF_STATE_PENDING_DHCP,
    HIF_STATE_PENDING_HTTP_RESP
} hif_state_t;

static hif_state_t g_hif_state = HIF_STATE_IDLE;
static SOCKET g_http_socket = -1;
static uint16 g_http_session_id = 0;
static uint32_t winc_hif_response_address = SHARED_MEM_BASE;

#define WIFI_HOST_RCV_CTRL_0    (0x1070)
#define WIFI_HOST_RCV_CTRL_1    (0x1084)

static void winc_hif_send_response(uint8_t u8Gid, uint8_t u8Opcode, uint16_t u16Length, const uint8_t* payload_ptr, uint16_t payload_size) {
    tstrHifHdr header;
    header.u8Gid = u8Gid;
    header.u8Opcode = u8Opcode;
    header.u16Length = u16Length;

    uint8_t* response_buffer_ptr = get_memory_ptr(winc_hif_response_address, u16Length);
    if (!response_buffer_ptr) {
        SIM_LOG(SIM_LOG_TYPE_ERROR, "[HIF] Failed to get memory for response.", winc_hif_response_address, u16Length);
        return;
    }

    header.u16Length = SWAP_16(header.u16Length);
    
    memcpy(response_buffer_ptr, &header, sizeof(tstrHifHdr));

    if (payload_ptr && payload_size > 0) {
        memcpy(response_buffer_ptr + M2M_HIF_HDR_OFFSET, payload_ptr, payload_size);
    }
    
    winc_creg_write(WIFI_HOST_RCV_CTRL_1, winc_hif_response_address);

    uint32_t reg_val = (u16Length << 2) | (1 << 0);
    winc_creg_write(WIFI_HOST_RCV_CTRL_0, reg_val);

    gpio_put(22, 0);

    char log_msg_response[SIM_LOG_MESSAGE_STRING_LEN];
    snprintf(log_msg_response, sizeof(log_msg_response), "[HIF] Sent response GID: %u, Opcode: 0x%02X, Length: %u", u8Gid, u8Opcode, u16Length);
    SIM_LOG(SIM_LOG_TYPE_COMMAND, log_msg_response, 0, 0);
}

static void hif_rx_done_callback(uint32_t address, uint32_t value) {
    if (value & (1 << 1)) {
        gpio_put(22, 1);
        SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Host signaled RX done. Interrupt de-asserted.", address, value);

        if (g_hif_state == HIF_STATE_PENDING_DHCP) {
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Sending dummy DHCP response.", 0, 0);
            tstrM2MIPConfig ip_config;
            memset(&ip_config, 0, sizeof(tstrM2MIPConfig));
            
            ip_config.u32StaticIP = SWAP_32(0xC0A80164);
            ip_config.u32Gateway = SWAP_32(0xC0A80101);
            ip_config.u32SubnetMask = SWAP_32(0xFFFFFF00);
            ip_config.u32DNS = SWAP_32(0xC0A80101);
            ip_config.u32DhcpLeaseTime = 86400;

            uint16_t payload_size = sizeof(tstrM2MIPConfig);
            uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
            uint16_t aligned_length = WORD_ALIGN(logical_length);

            winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_REQ_DHCP_CONF, aligned_length, (uint8_t*)&ip_config, payload_size);
            
            g_hif_state = HIF_STATE_IDLE;
        } else if (g_hif_state == HIF_STATE_PENDING_HTTP_RESP) {
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Sending dummy HTTP response.", 0, 0);

            const char* http_response = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello World!";
            uint16_t http_response_len = strlen(http_response);

            uint16_t recv_reply_size = sizeof(tstrRecvReply);
            uint16_t payload_size = recv_reply_size + http_response_len;
            uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
            uint16_t aligned_length = WORD_ALIGN(logical_length);

            uint8_t temp_payload[payload_size];
            
            tstrRecvReply recv_reply;
            memset(&recv_reply, 0, recv_reply_size);
            recv_reply.sock = g_http_socket;
            recv_reply.u16SessionID = g_http_session_id;
            recv_reply.s16RecvStatus = http_response_len; // The driver expects this to be LE
            recv_reply.u16DataOffset = recv_reply_size; // The offset from start of this payload to the data

            memcpy(temp_payload, &recv_reply, recv_reply_size);
            memcpy(temp_payload + recv_reply_size, http_response, http_response_len);

            winc_hif_send_response(M2M_REQ_GROUP_IP, SOCKET_CMD_RECV, aligned_length, temp_payload, payload_size);

            g_hif_state = HIF_STATE_IDLE;
        }
    }
}

static void hif_request_callback(uint32_t address, uint32_t value) {
    uint32_t dma_addr = value >> 2;
    uint8_t* hif_packet_ptr = get_memory_ptr(dma_addr, sizeof(tstrHifHdr));

    if (hif_packet_ptr) {
        tstrHifHdr* header = (tstrHifHdr*)hif_packet_ptr;
        uint16_t length = header->u16Length;

        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "[HIF] GID: %u, Opcode: 0x%02X, Length: %u", header->u8Gid, header->u8Opcode, length);
        SIM_LOG(SIM_LOG_TYPE_COMMAND, log_msg, 0, 0);

        if (header->u8Gid == M2M_REQ_GROUP_WIFI && header->u8Opcode == M2M_WIFI_REQ_SCAN) {
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Responding to M2M_WIFI_REQ_SCAN with SCAN_DONE.", 0, 0);
            tstrM2mScanDone scan_done_resp;
            scan_done_resp.u8NumofCh = 1;
            scan_done_resp.s8ScanState = M2M_SUCCESS;
            uint16_t payload_size = sizeof(tstrM2mScanDone);
            uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
            uint16_t aligned_length = WORD_ALIGN(logical_length);
            winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_RESP_SCAN_DONE, aligned_length, (uint8_t*)&scan_done_resp, payload_size);
        } else if (header->u8Gid == M2M_REQ_GROUP_WIFI && header->u8Opcode == M2M_WIFI_REQ_SCAN_RESULT) {
            tstrM2mReqScanResult* req_scan_result = (tstrM2mReqScanResult*)(hif_packet_ptr + M2M_HIF_HDR_OFFSET);
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
            tstrM2mWifiStateChanged con_state;
            con_state.u8CurrState = M2M_WIFI_CONNECTED;
            con_state.u8ErrCode = M2M_SUCCESS;
            uint16_t payload_size = sizeof(tstrM2mWifiStateChanged);
            uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
            uint16_t aligned_length = WORD_ALIGN(logical_length);
            winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_RESP_CON_STATE_CHANGED, aligned_length, (uint8_t*)&con_state, payload_size);
            g_hif_state = HIF_STATE_PENDING_DHCP;
        } else if (header->u8Gid == M2M_REQ_GROUP_IP && header->u8Opcode == 0xca) { // Driver sends 0xCA for SOCKET_CMD_SEND
            
            tstrSendCmd* send_cmd = (tstrSendCmd*)(hif_packet_ptr + M2M_HIF_HDR_OFFSET);
            uint16_t data_size = SWAP_16(send_cmd->u16DataSize);

            // Log the received HTTP request.
            // Offset is 8 (HIF) + 80 (TCP/IP) = 88 bytes.
            char* http_request = (char*)(hif_packet_ptr + 88);
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Received HTTP Request:", 0, 0);
            SIM_LOG(SIM_LOG_TYPE_INFO, http_request, 0, 0);


            // Store socket info for the response
            g_http_socket = send_cmd->sock;
            g_http_session_id = send_cmd->u16SessionID;

            // 1. Acknowledge the send request immediately.
            tstrSendReply send_reply;
            send_reply.sock = send_cmd->sock;
            send_reply.s16SentBytes = SWAP_16(data_size); // Report all bytes as "sent"
            send_reply.u16SessionID = send_cmd->u16SessionID;

            uint16_t reply_payload_size = sizeof(tstrSendReply);
            uint16_t reply_logical_length = M2M_HIF_HDR_OFFSET + reply_payload_size;
            uint16_t reply_aligned_length = WORD_ALIGN(reply_logical_length);

            winc_hif_send_response(M2M_REQ_GROUP_IP, SOCKET_CMD_SEND, reply_aligned_length, (uint8_t*)&send_reply, reply_payload_size);

            // 2. Set state to send the actual HTTP response later.
            g_hif_state = HIF_STATE_PENDING_HTTP_RESP;

        } else if (header->u8Gid == M2M_REQ_GROUP_WIFI && header->u8Opcode == 0x12) { // Undocumented DNS request?
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
        } else if (header->u8Gid == M2M_REQ_GROUP_IP && header->u8Opcode == SOCKET_CMD_CONNECT) {
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Responding to SOCKET_CMD_CONNECT with dummy success.", 0, 0);

            tstrConnectCmd* conn_cmd = (tstrConnectCmd*)(hif_packet_ptr + M2M_HIF_HDR_OFFSET);

            tstrConnectReply conn_reply;
            memset(&conn_reply, 0, sizeof(tstrConnectReply));
            conn_reply.sock = conn_cmd->sock;
            conn_reply.s8Error = SOCK_ERR_NO_ERROR;
            // From socket.c, the data offset for TCP is TCP_TX_PACKET_OFFSET, which includes other headers.
            // Let's provide a reasonable offset. The driver will subtract M2M_HIF_HDR_OFFSET from it.
            // From socket.c: #define TCP_TX_PACKET_OFFSET (IP_PACKET_OFFSET + TCP_IP_HEADER_LENGTH) -> 80
            conn_reply.u16AppDataOffset = SWAP_16(80 + M2M_HIF_HDR_OFFSET);

            uint16_t payload_size = sizeof(tstrConnectReply);
            uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
            uint16_t aligned_length = WORD_ALIGN(logical_length);

            winc_hif_send_response(M2M_REQ_GROUP_IP, SOCKET_CMD_CONNECT, aligned_length, (uint8_t*)&conn_reply, payload_size);
        }
    } else {
        SIM_LOG(SIM_LOG_TYPE_ERROR, "[HIF] Invalid DMA address received", dma_addr, 0);
    }
}

void winc_hif_init(void) {
    winc_creg_register_callback(0x106c, hif_request_callback);
    winc_creg_register_callback(WIFI_HOST_RCV_CTRL_0, hif_rx_done_callback);
    SIM_LOG(SIM_LOG_TYPE_NONE, "[HIF] Initialized and callbacks registered.", 0, 0);
}
