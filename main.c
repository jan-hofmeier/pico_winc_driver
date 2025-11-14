#include <stdio.h>
#include "pico/stdlib.h"

#include "driver/include/m2m_wifi.h"
#include "nmdrv.h"
#include "bsp/include/nm_bsp.h"

int main() {
    stdio_init_all();

    tstrWifiInitParam param;
    param.pfAppWifiCb = NULL;

    int8_t ret = nm_bsp_init();
    if (ret != M2M_SUCCESS) {
        printf("Error initializing BSP\n");
        while(1);
    }

    ret = m2m_wifi_init(&param);
    if (ret != M2M_SUCCESS) {
        printf("Error initializing WiFi\n");
        while(1);
    }

    printf("WINC1500 Initialized\n");

    while (true) {
        m2m_wifi_handle_events(NULL);
        sleep_ms(10);
    }
}
