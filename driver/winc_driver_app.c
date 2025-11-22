#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>

#include "../config/conf_winc.h"
#include "m2m_wifi.h"
#include "socket.h"
#include "m2m_types.h"
#include "iot/http/http_client.h" // New include for HTTP client
#include "wifi_credentials.h"
#include "winc_driver_app.h"

#define MAIN_HTTP_CLIENT_URL "httpbin.org"
#define MAIN_HTTP_CLIENT_PATH "/anything"
#define MAIN_HTTP_CLIENT_PORT 80
#define MAIN_WIFI_M2M_BUFFER_SIZE 4096 // Increased buffer size

// HTTP client related variables

#define MAIN_HTTP_CLIENT_USER_AGENT "PicoWINCClient/1.0"

static uint32_t host_ip = 0; // Placeholder for host IP
static SOCKET tcp_client_socket = -1;
static uint8_t gau8RxBuffer[MAIN_WIFI_M2M_BUFFER_SIZE];
static struct http_client_module http_client_instance;
volatile bool is_connected = false;

static uint8_t u8NumFoundAPs = 0;
static uint8_t u8ScanResultIdx = 0;

static void wifi_callback(uint8_t u8MsgType, void *pvMsg)
{
    switch (u8MsgType)
    {
    case M2M_WIFI_RESP_CON_STATE_CHANGED:
    {
        tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
        if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED)
        {
            printf("Wi-Fi Connected\n");
            //m2m_wifi_request_dhcp_client(); // DHCP is requested automatically
        }
        else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED)
        {
            printf("Wi-Fi Disconnected\n");
            // Re-scan or re-connect logic can go here if needed
            // For now, let's just re-initiate a scan
            m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
        } else {
            printf("Wifi state: %d\n", pstrWifiState->u8CurrState);
        }
        break;
    }
    case M2M_WIFI_REQ_DHCP_CONF:
    {
        uint8 *pu8IPAddress = (uint8 *)pvMsg;
        printf("Wi-Fi IP Address is %u.%u.%u.%u\n", pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
        host_ip = 1; // Indicate that we have an IP address
        break;
    }
    case M2M_WIFI_RESP_SCAN_DONE:
    {
        tstrM2mScanDone *pstrInfo = (tstrM2mScanDone *)pvMsg;
        printf("Scan Done: Number of APs found: %d\n", pstrInfo->u8NumofCh);
        u8NumFoundAPs = pstrInfo->u8NumofCh;
        u8ScanResultIdx = 0;
        if (u8NumFoundAPs > 0)
        {
            m2m_wifi_req_scan_result(u8ScanResultIdx);
        }
        else
        {
            printf("No APs found, re-scanning...\n");
            m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
        }
        break;
    }
    case M2M_WIFI_RESP_SCAN_RESULT:
    {
        tstrM2mWifiscanResult *pstrScanResult = (tstrM2mWifiscanResult *)pvMsg;
        printf("SSID: %s, RSSI: %d, Auth: %d, Channel: %d\n",
               pstrScanResult->au8SSID, pstrScanResult->s8rssi,
               pstrScanResult->u8AuthType, pstrScanResult->u8ch);

        if (strcmp((const char*)pstrScanResult->au8SSID, WIFI_SSID) == 0)
        {
            printf("Found desired AP: %s, connecting...\n", WIFI_SSID);
            m2m_wifi_connect((char *)WIFI_SSID, strlen(WIFI_SSID),
                             M2M_WIFI_SEC_WPA_PSK, (void *)WIFI_PASSWORD, M2M_WIFI_CH_ALL);
        }
        else
        {
            u8ScanResultIdx++;
            if (u8ScanResultIdx < u8NumFoundAPs)
            {
                m2m_wifi_req_scan_result(u8ScanResultIdx);
            }
            else
            {
                printf("Desired AP not found, re-scanning...\n");
                m2m_wifi_request_scan(M2M_WIFI_CH_ALL);
            }
        }
        break;
    }
    default:
        printf("Wifi Callback Message %d\n", u8MsgType);
    }
}



// HTTP client callback
static void http_client_callback(struct http_client_module *module, int type, union http_client_data *data)
{
    switch (type)
    {
    case HTTP_CLIENT_CALLBACK_SOCK_CONNECTED:
        printf("HTTP_CLIENT_CALLBACK_SOCK_CONNECTED\n");
        break;

    case HTTP_CLIENT_CALLBACK_REQUESTED:
        printf("HTTP_CLIENT_CALLBACK_REQUESTED\n");
        break;

    case HTTP_CLIENT_CALLBACK_RECV_RESPONSE:
        printf("HTTP_CLIENT_CALLBACK_RECV_RESPONSE: Status Code: %d, Content Length: %lu\n", data->recv_response.response_code, data->recv_response.content_length);
        if (data->recv_response.content != NULL && data->recv_response.content_length > 0) {
            printf("Received Content:\n%.*s\n", (int)data->recv_response.content_length, data->recv_response.content);
        }
        break;

    case HTTP_CLIENT_CALLBACK_RECV_BODY_DATA:
        printf("HTTP_CLIENT_CALLBACK_RECV_BODY_DATA: %.*s\n", data->recv_body_data.length, data->recv_body_data.buffer);
        break;

    case HTTP_CLIENT_CALLBACK_DISCONNECTED:
        printf("HTTP_CLIENT_CALLBACK_DISCONNECTED\n");
        break;

    case HTTP_CLIENT_CALLBACK_RECV_CHUNKED_DATA:
        printf("HTTP_CLIENT_CALLBACK_RECV_CHUNKED_DATA: %.*s\n", data->recv_chunked_data.length, data->recv_chunked_data.data);
        break;

    case HTTP_CLIENT_CALLBACK_REQUEST_TIMEOUT:
        printf("HTTP_CLIENT_CALLBACK_REQUEST_TIMEOUT\n");
        break;

    default:
        printf("Unhandled HTTP event type:%d\n", type);
    }


}

static void wrapped_socket_event_handler(SOCKET sock, uint8_t msg_type, void *msg_data) {
    http_client_socket_event_handler(sock, msg_type, msg_data);
}

static void wrapped_socket_resolve_handler(uint8_t *doamin_name, uint32_t server_ip) {
    http_client_socket_resolve_handler(doamin_name, server_ip);
}

int winc_driver_app_main()
{
    stdio_init_all();
    sleep_ms(2000); // Increased sleep for serial capture
    printf("Starting WINC driver initialization...\n");

    nm_bsp_init();

    tstrWifiInitParam param;
    memset(&param, 0, sizeof(param));
    param.pfAppWifiCb = wifi_callback;

    printf("Calling nm_drv_init...\n");
        if (m2m_wifi_init(&param) != M2M_SUCCESS)
        {
            printf("Failed to initialize WINC driver\n");
            while (1);
        }
    printf("nm_drv_init returned successfully.\n");
    
    socketInit(); // Explicitly call socketInit()

    printf("WINC driver initialized\n");
    
    // Initialize the HTTP client module.
    struct http_client_config client_config;
    http_client_get_config_defaults(&client_config);
    client_config.recv_buffer = gau8RxBuffer;
    client_config.recv_buffer_size = sizeof(gau8RxBuffer);
    client_config.user_agent = MAIN_HTTP_CLIENT_USER_AGENT;
    client_config.port = MAIN_HTTP_CLIENT_PORT; // Set the port
    client_config.tls = 0; // Disable TLS
    http_client_init(&http_client_instance, &client_config);

    http_client_register_callback(&http_client_instance, http_client_callback);

    // Register the HTTP client's socket event and DNS resolution handlers
    registerSocketCallback(wrapped_socket_event_handler, wrapped_socket_resolve_handler);

    // Start Wi-Fi scan
    printf("Starting Wi-Fi scan...\n");
    m2m_wifi_request_scan(M2M_WIFI_CH_ALL);

    while (true)
    {
        m2m_wifi_handle_events(NULL);



        if (host_ip != 0) {
            char full_url[256]; // Assuming a max URL length of 256
            snprintf(full_url, sizeof(full_url), "%s%s", MAIN_HTTP_CLIENT_URL, MAIN_HTTP_CLIENT_PATH);
            printf("Attempting to send HTTP request to %s\n", full_url);
            sint8 ret = http_client_send_request(&http_client_instance, full_url, HTTP_METHOD_GET, NULL, NULL);
            if (ret == M2M_SUCCESS) {
                printf("HTTP request sent successfully.\n");
            } else {
                printf("Failed to send HTTP request, error: %d\n", ret);
            }
            host_ip = 0; // Reset host_ip to prevent repeated requests
        }
        sleep_ms(1);
    }

    return 0;
}
