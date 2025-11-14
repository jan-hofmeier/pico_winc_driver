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
    default:
        break;
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);
    printf("Starting WINC driver initialization...\n");

    tstrWifiInitParam param;
    memset(&param, 0, sizeof(param));
    param.pfAppWifiCb = wifi_callback;

    if (m2m_wifi_init(&param) != M2M_SUCCESS)
    {
        printf("Failed to initialize WINC driver\n");
        while (1);
    }

    printf("WINC driver initialized\n");

    while (true)
    {
        m2m_wifi_handle_events(NULL);
        printf("looping\n");
        sleep_ms(1000);
    }

    m2m_wifi_connect(WIFI_SSID, sizeof(WIFI_SSID), M2M_WIFI_SEC_WPA_PSK, WIFI_PASSWORD, M2M_WIFI_CH_ALL);

    return 0;
}
