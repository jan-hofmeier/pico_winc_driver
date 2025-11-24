#include "winc_hif.h"
#include "winc_callback_manager.h"
#include "winc_simulator_app.h"
#include "sim_log.h"
#include "driver/source/m2m_hif.h"
#include "m2m_wifi.h"
#include "m2m_socket_host_if.h"
#include "wifi_credentials.h"
#include "winc_wifi_drv.h"
#include <string.h>
#include "hardware/gpio.h"

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

// Map WINC sockets to Driver sockets
#define MAX_SOCKETS 7
static int g_winc_socket_map[MAX_SOCKETS];
static uint16_t g_winc_session_id[MAX_SOCKETS];

void winc_hif_sockets_init() {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        g_winc_socket_map[i] = -1;
    }
}

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
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Sending DHCP response (using lwIP info).", 0, 0);

            tstrM2MIPConfig ip_config;
            memset(&ip_config, 0, sizeof(tstrM2MIPConfig));
            
            winc_drv_get_ip_config(&ip_config.u32StaticIP, &ip_config.u32Gateway, &ip_config.u32SubnetMask);
            ip_config.u32DNS = ip_config.u32Gateway;
            ip_config.u32DhcpLeaseTime = 86400;

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
    uint8_t* hif_packet_ptr = get_memory_ptr(dma_addr, sizeof(tstrHifHdr));

    if (hif_packet_ptr) {
        tstrHifHdr* header = (tstrHifHdr*)hif_packet_ptr;
        uint16_t length = header->u16Length;

        char log_msg[100];
        snprintf(log_msg, sizeof(log_msg), "[HIF] GID: %u, Opcode: 0x%02X, Length: %u", header->u8Gid, header->u8Opcode, length);
        SIM_LOG(SIM_LOG_TYPE_COMMAND, log_msg, 0, 0);

        if (header->u8Gid == M2M_REQ_GROUP_WIFI && header->u8Opcode == M2M_WIFI_REQ_SCAN) {
             SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Starting Wi-Fi Scan...", 0, 0);
             if (winc_drv_scan_start() == 0) {
                 // Wait for scan or poll?
                 // We are in ISR context or close to it.
                 // But let's rely on main loop polling for completion if we could...
                 // but we need to respond.
                 // Let's wait.
                 while (winc_drv_scan_active()) {
                     winc_drv_poll();
                     // Busy wait... bad but necessary for now
                 }

                 tstrM2mScanDone scan_done_resp;
                 scan_done_resp.u8NumofCh = 20; // Just report max or something? No, we have to count.
                 // Actually winc_drv_scan_get_results count?
                 // The driver doesn't expose count directly in header, but get_results returns fail if out of bounds.
                 // Let's loop to count.
                 int count = 0;
                 winc_drv_scan_result_t res;
                 while(winc_drv_scan_get_results(count, &res) == 0) {
                     count++;
                 }

                 scan_done_resp.u8NumofCh = count;
                 scan_done_resp.s8ScanState = M2M_SUCCESS;
                 uint16_t payload_size = sizeof(tstrM2mScanDone);
                 uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
                 uint16_t aligned_length = WORD_ALIGN(logical_length);
                 winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_RESP_SCAN_DONE, aligned_length, (uint8_t*)&scan_done_resp, payload_size);
             } else {
                 SIM_LOG(SIM_LOG_TYPE_ERROR, "[HIF] Scan failed start", 0, 0);
             }
        } else if (header->u8Gid == M2M_REQ_GROUP_WIFI && header->u8Opcode == M2M_WIFI_REQ_SCAN_RESULT) {
            tstrM2mReqScanResult* req_scan_result = (tstrM2mReqScanResult*)(hif_packet_ptr + M2M_HIF_HDR_OFFSET);
            uint8_t idx = req_scan_result->u8Index;

            winc_drv_scan_result_t res;
            if (winc_drv_scan_get_results(idx, &res) == 0) {
                 tstrM2mWifiscanResult scan_result;
                 memset(&scan_result, 0, sizeof(tstrM2mWifiscanResult));
                 scan_result.u8index = idx;
                 scan_result.s8rssi = res.rssi;
                 scan_result.u8AuthType = res.auth_type;
                 scan_result.u8ch = res.channel;
                 memcpy(scan_result.au8BSSID, res.bssid, 6);
                 strncpy((char*)scan_result.au8SSID, res.ssid, M2M_MAX_SSID_LEN);

                 uint16_t payload_size = sizeof(tstrM2mWifiscanResult);
                 uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
                 uint16_t aligned_length = WORD_ALIGN(logical_length);
                 winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_RESP_SCAN_RESULT, aligned_length, (uint8_t*)&scan_result, payload_size);
            }
        } else if (header->u8Gid == M2M_REQ_GROUP_WIFI && header->u8Opcode == M2M_WIFI_REQ_CONNECT) {
            tstrM2mWifiConnect* connect_req = (tstrM2mWifiConnect*)(hif_packet_ptr + M2M_HIF_HDR_OFFSET);
            char ssid[M2M_MAX_SSID_LEN + 1] = {0};
            strncpy(ssid, (char*)connect_req->au8SSID, M2M_MAX_SSID_LEN);
            char *password = (char*)connect_req->strSec.uniAuth.au8PSK;

            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Connecting to SSID:", 0, 0);
            SIM_LOG(SIM_LOG_TYPE_INFO, ssid, 0, 0);
            
            int err = winc_drv_connect(ssid, password, connect_req->strSec.u8SecType);

            if (err == 0) {
                 SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Connected!", 0, 0);
                 tstrM2mWifiStateChanged con_state;
                 con_state.u8CurrState = M2M_WIFI_CONNECTED;
                 con_state.u8ErrCode = M2M_SUCCESS;
                 uint16_t payload_size = sizeof(tstrM2mWifiStateChanged);
                 uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
                 uint16_t aligned_length = WORD_ALIGN(logical_length);
                 winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_RESP_CON_STATE_CHANGED, aligned_length, (uint8_t*)&con_state, payload_size);
                 g_hif_state = HIF_STATE_PENDING_DHCP;
            } else {
                 SIM_LOG(SIM_LOG_TYPE_ERROR, "[HIF] Connection failed", err, 0);
                 tstrM2mWifiStateChanged con_state;
                 con_state.u8CurrState = M2M_WIFI_DISCONNECTED;
                 con_state.u8ErrCode = M2M_ERR_CONN_INPROGRESS;
                 uint16_t payload_size = sizeof(tstrM2mWifiStateChanged);
                 uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
                 uint16_t aligned_length = WORD_ALIGN(logical_length);
                 winc_hif_send_response(M2M_REQ_GROUP_WIFI, M2M_WIFI_RESP_CON_STATE_CHANGED, aligned_length, (uint8_t*)&con_state, payload_size);
            }

        } else if (header->u8Gid == M2M_REQ_GROUP_IP && header->u8Opcode == SOCKET_CMD_CONNECT) {
             tstrConnectCmd* conn_cmd = (tstrConnectCmd*)(hif_packet_ptr + M2M_HIF_HDR_OFFSET);

             uint32_t ip = conn_cmd->strAddr.u32IPAddr;
             uint16_t port = conn_cmd->strAddr.u16Port;

             SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Socket Connect request", 0, 0);

             int drv_sock = winc_drv_sock_create(WINC_SOCK_TCP); // Assume TCP for now from CONNECT command context
             int winc_sock = conn_cmd->sock;

             int err = -1;
             if (drv_sock >= 0) {
                 err = winc_drv_sock_connect(drv_sock, ip, port);
                 if (err == 0) {
                     if (winc_sock >= 0 && winc_sock < MAX_SOCKETS) {
                         g_winc_socket_map[winc_sock] = drv_sock;
                         g_winc_session_id[winc_sock] = conn_cmd->u16SessionID;
                     }
                 } else {
                     winc_drv_sock_close(drv_sock);
                 }
             }

             tstrConnectReply conn_reply;
             memset(&conn_reply, 0, sizeof(tstrConnectReply));
             conn_reply.sock = conn_cmd->sock;
             conn_reply.u16AppDataOffset = SWAP_16(80 + M2M_HIF_HDR_OFFSET);

             if (err == 0) {
                 SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Socket Connected", 0, 0);
                 conn_reply.s8Error = SOCK_ERR_NO_ERROR;
             } else {
                 SIM_LOG(SIM_LOG_TYPE_ERROR, "[HIF] Socket Connection Failed", err, 0);
                 conn_reply.s8Error = SOCK_ERR_CONN_ABORTED;
             }

             uint16_t payload_size = sizeof(tstrConnectReply);
             uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
             uint16_t aligned_length = WORD_ALIGN(logical_length);

             winc_hif_send_response(M2M_REQ_GROUP_IP, SOCKET_CMD_CONNECT, aligned_length, (uint8_t*)&conn_reply, payload_size);

        } else if (header->u8Gid == M2M_REQ_GROUP_IP && header->u8Opcode == 0xca) { // SOCKET_CMD_SEND
            tstrSendCmd* send_cmd = (tstrSendCmd*)(hif_packet_ptr + M2M_HIF_HDR_OFFSET);
            uint16_t data_size = SWAP_16(send_cmd->u16DataSize);

            // Data offset 88 bytes from start of packet usually
            const uint8_t* data_ptr = (hif_packet_ptr + 88);

            int winc_sock = send_cmd->sock;
            int sent_bytes = -1;

            if (winc_sock >= 0 && winc_sock < MAX_SOCKETS && g_winc_socket_map[winc_sock] >= 0) {
                sent_bytes = winc_drv_sock_send(g_winc_socket_map[winc_sock], data_ptr, data_size);
            }

            tstrSendReply send_reply;
            send_reply.sock = send_cmd->sock;
            send_reply.s16SentBytes = SWAP_16((int16_t)sent_bytes);
            send_reply.u16SessionID = send_cmd->u16SessionID;

            uint16_t reply_payload_size = sizeof(tstrSendReply);
            uint16_t reply_logical_length = M2M_HIF_HDR_OFFSET + reply_payload_size;
            uint16_t aligned_length = WORD_ALIGN(reply_logical_length);

            winc_hif_send_response(M2M_REQ_GROUP_IP, SOCKET_CMD_SEND, aligned_length, (uint8_t*)&send_reply, reply_payload_size);

        } else if (header->u8Gid == M2M_REQ_GROUP_IP && header->u8Opcode == SOCKET_CMD_RECV) {
            tstrRecvCmd *recv_cmd = (tstrRecvCmd*)(hif_packet_ptr + M2M_HIF_HDR_OFFSET);
            int winc_sock = recv_cmd->sock;
            if (winc_sock >= 0 && winc_sock < MAX_SOCKETS) {
                g_winc_session_id[winc_sock] = recv_cmd->u16SessionID;
            }
        } else if (header->u8Gid == M2M_REQ_GROUP_IP && header->u8Opcode == SOCKET_CMD_DNS_RESOLVE) {
            char* hostname = (char*)(hif_packet_ptr + M2M_HIF_HDR_OFFSET);
            SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Resolving DNS:", 0, 0);
            SIM_LOG(SIM_LOG_TYPE_INFO, hostname, 0, 0);

            tstrDnsReply dns_reply;
            memset(&dns_reply, 0, sizeof(tstrDnsReply));
            strncpy(dns_reply.acHostName, hostname, HOSTNAME_MAX_SIZE);

            if (winc_drv_resolve_dns(hostname, &dns_reply.u32HostIP) != 0) {
                 dns_reply.u32HostIP = 0;
            }

            uint16_t payload_size = sizeof(tstrDnsReply);
            uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
            uint16_t aligned_length = WORD_ALIGN(logical_length);
    
            winc_hif_send_response(M2M_REQ_GROUP_IP, SOCKET_CMD_DNS_RESOLVE, aligned_length, (uint8_t*)&dns_reply, payload_size);
        }
    } else {
        SIM_LOG(SIM_LOG_TYPE_ERROR, "[HIF] Invalid DMA address received", dma_addr, 0);
    }
}

void winc_hif_handle_socket_events(void) {
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (g_winc_socket_map[i] >= 0) {
            #define RECV_BUF_SIZE 1400
            uint8_t data_buf[RECV_BUF_SIZE];
            int len = winc_drv_sock_recv(g_winc_socket_map[i], data_buf, RECV_BUF_SIZE);

            if (len > 0) {
                SIM_LOG(SIM_LOG_TYPE_INFO, "[HIF] Socket Data Received", len, 0);

                uint16_t recv_reply_size = sizeof(tstrRecvReply);
                uint16_t payload_size = recv_reply_size + len;
                uint16_t logical_length = M2M_HIF_HDR_OFFSET + payload_size;
                uint16_t aligned_length = WORD_ALIGN(logical_length);

                uint8_t temp_payload[payload_size];
                tstrRecvReply recv_reply;
                memset(&recv_reply, 0, recv_reply_size);
                recv_reply.sock = i;
                recv_reply.u16SessionID = g_winc_session_id[i];
                recv_reply.s16RecvStatus = SWAP_16((int16_t)len);
                recv_reply.u16DataOffset = SWAP_16(recv_reply_size); // Offset from start of payload

                memcpy(temp_payload, &recv_reply, recv_reply_size);
                memcpy(temp_payload + recv_reply_size, data_buf, len);

                winc_hif_send_response(M2M_REQ_GROUP_IP, SOCKET_CMD_RECV, aligned_length, temp_payload, payload_size);
                break;
            }
        }
    }
}

void winc_hif_init(void) {
    winc_creg_register_callback(0x106c, hif_request_callback);
    winc_creg_register_callback(WIFI_HOST_RCV_CTRL_0, hif_rx_done_callback);
    winc_hif_sockets_init();
    SIM_LOG(SIM_LOG_TYPE_NONE, "[HIF] Initialized and callbacks registered.", 0, 0);
}
