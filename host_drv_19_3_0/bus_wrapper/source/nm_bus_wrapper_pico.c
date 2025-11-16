#include "bsp/include/nm_bsp.h"
#include "common/include/nm_common.h"
#include "bus_wrapper/include/nm_bus_wrapper.h"
#include "conf_winc.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdio.h>

#define NM_BUS_MAX_TRX_SZ 256

tstrNmBusCapabilities egstrNmBusCapabilities = {
    NM_BUS_MAX_TRX_SZ};

sint8 nm_spi_rw(uint8 *pu8Mosi, uint8 *pu8Miso, uint16 u16Sz)
{
    gpio_put(CONF_WINC_SPI_CS_PIN, 0);
    sleep_us(1);

    if (pu8Mosi == NULL && pu8Miso == NULL)
    {
        gpio_put(CONF_WINC_SPI_CS_PIN, 1);
        return M2M_ERR_BUS_FAIL;
    }
    else if (pu8Mosi == NULL)
    {
        spi_read_blocking(CONF_WINC_SPI_PORT, 0, pu8Miso, u16Sz);
    }
    else if (pu8Miso == NULL)
    {
        spi_write_blocking(CONF_WINC_SPI_PORT, pu8Mosi, u16Sz);
    }
    else
    {
        spi_write_read_blocking(CONF_WINC_SPI_PORT, pu8Mosi, pu8Miso, u16Sz);
    }

    sleep_us(1);
    gpio_put(CONF_WINC_SPI_CS_PIN, 1);

    return M2M_SUCCESS;
}

sint8 nm_bus_init(void *pvinit)
{
    printf("nm_bus_init\n");
    spi_init(CONF_WINC_SPI_PORT, 4 * 1000 * 1000);
    spi_set_format(CONF_WINC_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(CONF_WINC_SPI_MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(CONF_WINC_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(CONF_WINC_SPI_MOSI_PIN, GPIO_FUNC_SPI);

    gpio_init(CONF_WINC_SPI_CS_PIN);
    gpio_set_dir(CONF_WINC_SPI_CS_PIN, GPIO_OUT);
    gpio_put(CONF_WINC_SPI_CS_PIN, 1);

    nm_bsp_reset();

    return M2M_SUCCESS;
}

sint8 nm_bus_ioctl(uint8 u8Cmd, void *pvParameter)
{
    sint8 s8Ret = 0;
    switch (u8Cmd)
    {
    case NM_BUS_IOCTL_RW:
    {
        tstrNmSpiRw *pstrParam = (tstrNmSpiRw *)pvParameter;
        s8Ret = nm_spi_rw(pstrParam->pu8InBuf, pstrParam->pu8OutBuf, pstrParam->u16Sz);
    }
    break;
    default:
        s8Ret = -1;
        M2M_ERR("invalid ioclt cmd\n");
        break;
    }

    return s8Ret;
}

sint8 nm_bus_deinit(void)
{
    spi_deinit(CONF_WINC_SPI_PORT);
    return M2M_SUCCESS;
}

sint8 nm_bus_reinit(void *config)
{
    return M2M_SUCCESS;
}

sint8 nm_bus_speed(uint8 level)
{
    return M2M_SUCCESS;
}
