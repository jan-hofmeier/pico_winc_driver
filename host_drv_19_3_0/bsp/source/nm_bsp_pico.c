#include "bsp/include/nm_bsp.h"
#include "bsp/include/nm_bsp_internal.h"
#include "common/include/nm_common.h"
#include "conf_winc.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

static tpfNmBspIsr gpfIsr;

static void chip_isr(uint gpio, uint32_t events)
{
    if (gpfIsr)
    {
        gpfIsr();
    }
}

static void init_chip_pins(void)
{
    gpio_init(CONF_WINC_PIN_CHIP_ENABLE);
    gpio_set_dir(CONF_WINC_PIN_CHIP_ENABLE, GPIO_OUT);

    gpio_init(CONF_WINC_PIN_RESET);
    gpio_set_dir(CONF_WINC_PIN_RESET, GPIO_OUT);

    gpio_init(CONF_WINC_PIN_WAKE);
    gpio_set_dir(CONF_WINC_PIN_WAKE, GPIO_OUT);

    gpio_put(CONF_WINC_PIN_CHIP_ENABLE, 0);
    gpio_put(CONF_WINC_PIN_RESET, 0);
}

sint8 nm_bsp_init(void)
{
    printf("nm_bsp_init\n");
    gpfIsr = NULL;

    init_chip_pins();

    return M2M_SUCCESS;
}

sint8 nm_bsp_deinit(void)
{
    gpio_deinit(CONF_WINC_PIN_CHIP_ENABLE);
    gpio_deinit(CONF_WINC_PIN_RESET);
    gpio_deinit(CONF_WINC_PIN_WAKE);
    gpio_deinit(CONF_WINC_SPI_INT_PIN);

    return M2M_SUCCESS;
}

void nm_bsp_reset(void)
{
    gpio_put(CONF_WINC_PIN_CHIP_ENABLE, 0);
    gpio_put(CONF_WINC_PIN_RESET, 0);
    nm_bsp_sleep(1);
    gpio_put(CONF_WINC_PIN_CHIP_ENABLE, 1);
    nm_bsp_sleep(10);
    gpio_put(CONF_WINC_PIN_RESET, 1);
}

void nm_bsp_sleep(uint32 u32TimeMsec)
{
    sleep_ms(u32TimeMsec);
}

void nm_bsp_register_isr(tpfNmBspIsr pfIsr)
{
    gpfIsr = pfIsr;

    gpio_init(CONF_WINC_SPI_INT_PIN);
    gpio_set_dir(CONF_WINC_SPI_INT_PIN, GPIO_IN);
    gpio_pull_up(CONF_WINC_SPI_INT_PIN);

    gpio_set_irq_enabled_with_callback(CONF_WINC_SPI_INT_PIN, GPIO_IRQ_EDGE_FALL, true, &chip_isr);
}

void nm_bsp_interrupt_ctrl(uint8 u8Enable)
{
    if (u8Enable)
    {
        gpio_set_irq_enabled(CONF_WINC_SPI_INT_PIN, GPIO_IRQ_EDGE_FALL, true);
    }
    else
    {
        gpio_set_irq_enabled(CONF_WINC_SPI_INT_PIN, GPIO_IRQ_EDGE_FALL, false);
    }
}
