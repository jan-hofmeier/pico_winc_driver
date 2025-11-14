/**
 *
 * \file
 *
 * \brief This module contains Pico BSP APIs implementation.
 *
 */

#include "bsp/include/nm_bsp.h"
#include "bsp/include/nm_bsp_internal.h"
#include "common/include/nm_common.h"
#include "conf_winc.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

static tpfNmBspIsr gpfIsr;

static void chip_isr(uint gpio, uint32_t events)
{
	if (gpfIsr) {
		gpfIsr();
	}
}

/*
 *	@fn		nm_bsp_init
 *	@brief	Initialize BSP
 *	@return	0 in case of success and -1 in case of failure
 */
sint8 nm_bsp_init(void)
{
	gpfIsr = NULL;

	// Initialize chip IOs.
	gpio_init(EN_PIN);
    gpio_set_dir(EN_PIN, GPIO_OUT);

    gpio_init(RESET_PIN);
    gpio_set_dir(RESET_PIN, GPIO_OUT);

	gpio_init(WAKE_PIN);
    gpio_set_dir(WAKE_PIN, GPIO_OUT);

	gpio_put(EN_PIN, 0);
	gpio_put(RESET_PIN, 0);

	//
	gpio_init(IRQ_PIN);
	gpio_set_dir(IRQ_PIN, GPIO_IN);
	gpio_pull_up(IRQ_PIN);


	return M2M_SUCCESS;
}

/**
 *	@fn		nm_bsp_deinit
 *	@brief	De-iInitialize BSP
 *	@return	0 in case of success and -1 in case of failure
 */
sint8 nm_bsp_deinit(void)
{
	gpio_deinit(EN_PIN);
	gpio_deinit(RESET_PIN);
	gpio_deinit(WAKE_PIN);
	gpio_deinit(IRQ_PIN);

	return M2M_SUCCESS;
}

/**
 *	@fn		nm_bsp_reset
 *	@brief	Reset NMC1500 SoC by setting CHIP_EN and RESET_N signals low,
 *           CHIP_EN high then RESET_N high
 */
void nm_bsp_reset(void)
{
	gpio_put(EN_PIN, 0);
	gpio_put(RESET_PIN, 0);
	nm_bsp_sleep(1);
	gpio_put(EN_PIN, 1);
	nm_bsp_sleep(10);
	gpio_put(RESET_PIN, 1);
}

/*
 *	@fn		nm_bsp_sleep
 *	@brief	Sleep in units of mSec
 *	@param[IN]	u32TimeMsec
 *				Time in milliseconds
 */
void nm_bsp_sleep(uint32 u32TimeMsec)
{
	sleep_ms(u32TimeMsec);
}

/*
 *	@fn		nm_bsp_register_isr
 *	@brief	Register interrupt service routine
 *	@param[IN]	pfIsr
 *				Pointer to ISR handler
 */
void nm_bsp_register_isr(tpfNmBspIsr pfIsr)
{
	gpfIsr = pfIsr;

    gpio_set_irq_enabled_with_callback(IRQ_PIN, GPIO_IRQ_EDGE_FALL, true, &chip_isr);
}

/*
 *	@fn		nm_bsp_interrupt_ctrl
 *	@brief	Enable/Disable interrupts
 *	@param[IN]	u8Enable
 *				'0' disable interrupts. '1' enable interrupts
 */
void nm_bsp_interrupt_ctrl(uint8 u8Enable)
{
	if (u8Enable) {
		gpio_set_irq_enabled(IRQ_PIN, GPIO_IRQ_EDGE_FALL, true);
	} else {
		gpio_set_irq_enabled(IRQ_PIN, GPIO_IRQ_EDGE_FALL, false);
	}
}
