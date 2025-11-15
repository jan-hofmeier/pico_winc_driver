#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>

#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"

#define WIFI_SSID "MY_SSID"
#define WIFI_PASSWORD "MY_PASSWORD"

static void wifi_callback(uint8_t u8MsgType, void *pvMsg)
{
    switch (u8MsgType)
    {
    case M2M_WIFI_RESP_CON_STATE_CHANGED:
    {
        tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
        if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED)
        {
            printf("Wi-Fi connected\n");
        }
        else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED)
        {
            printf("Wi-Fi disconnected\n");
        }
    }
    break;
    case M2M_WIFI_RESP_SCAN_DONE:
    {
        tstrM2mScanDone *pstrScanDone = (tstrM2mScanDone *)pvMsg;
        printf("Scan done, found %d APs\n", pstrScanDone->u8NumofCh);
        if (pstrScanDone->u8NumofCh > 0)
        {
            m2m_wifi_req_scan_result(0);
        }
    }
    break;
    case M2M_WIFI_RESP_SCAN_RESULT:
    {
        tstrM2mWifiscanResult *pstrScanResult = (tstrM2mWifiscanResult *)pvMsg;
        printf("AP: %s, RSSI: %d\n", pstrScanResult->au8SSID, pstrScanResult->s8rssi);
        static uint8_t scan_index = 1;
        if (scan_index < m2m_wifi_get_num_ap_found())
        {
            m2m_wifi_req_scan_result(scan_index++);
        }
        else
        {
            scan_index = 1;
        }
    }
    break;
    default:
        break;
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);
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

    printf("WINC driver initialized\n");

    m2m_wifi_request_scan(M2M_WIFI_CH_ALL);

    while (true)
    {
        m2m_wifi_handle_events(NULL);
        sleep_ms(100);
    }

    return 0;
}
