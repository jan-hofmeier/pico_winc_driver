/**
 *
 * \file
 *
 * \brief This module contains NMC1000 bus wrapper APIs implementation.
 *
 * Copyright (c) 2016-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
#ifndef _NM_BUS_WRAPPER_H_
#define _NM_BUS_WRAPPER_H_

#include "common/include/nm_common.h"

/**
	@defgroup	BUSWRAPPER	Bus Wrapper
	@brief
*/
/**@{*/
#if (defined __cplusplus)
extern "C" {
#endif

/**
*	@struct	tstrNmBusCapabilities
*	@brief	Structure holding bus capabilities information
*	@sa		nm_bus_get_capabilities
*/ 
typedef struct
{
	uint16	u16MaxTrxSz;	/*!< Maximum Transfer size */
}tstrNmBusCapabilities;

#define NM_BUS_IOCTL_R			0	/*!< Read register */
#define NM_BUS_IOCTL_W			1	/*!< Write register */
#define NM_BUS_IOCTL_RW			2	/*!< Read/Write register */
#define NM_BUS_IOCTL_R_BLOCK	3
#define NM_BUS_IOCTL_W_BLOCK	4
#define NM_BUS_IOCTL_RW_BLOCK	5

typedef struct
{
	uint8*	pu8Buf1;
	uint8*	pu8Buf2;
	uint16	u16Sz1;
	uint16	u16Sz2;
}tstrNmI2cSpecial;

typedef struct
{
	uint8*	pu8OutBuf;
	uint8*	pu8InBuf;
	uint16	u16Sz;
}tstrNmSpiRw;

/**
*	@fn		nm_bus_init
*	@brief	Initialize the bus wrapper
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*/ 
sint8 nm_bus_init(void *);

/**
*	@fn		nm_bus_ioctl
*	@brief	send/receive from the bus
*	@param[IN]	u8Cmd
*					IOCTL command for the operation
*	@param[IN]	pvParameter
*					Arbitrary parameter depending on IOCTL
*	@return	M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*	@note	For SPI only, it's important to be able to send/receive at the same time
*/
sint8 nm_bus_ioctl(uint8 u8Cmd, void* pvParameter);

/**
*	@fn		nm_bus_deinit
*	@brief	De-initialize the bus wrapper
*/
sint8 nm_bus_deinit(void);

/**
*	@fn		nm_bus_get_capabilities
*	@brief	Get the bus capabilities notes
*	@return	a constant pointer to the tstrNmBusCapabilities structure which holds the bus capabilities
*/
tstrNmBusCapabilities* nm_bus_get_capabilities(void);


/**
*	@fn			nm_bus_reinit
*	@brief		re-initialize the bus wrapper
*	@param [in]	void *config
*					re-init configuration data
*	@return		M2M_SUCCESS in case of success and M2M_ERR_BUS_FAIL in case of failure
*/
sint8 nm_bus_reinit(void * config);

/**
 *  @fn         nm_bus_speed
 *  @brief      Either set the bus speed to default (HIGH) or a reduced speed (LOW)
				to increase stability during WINC wakeup
 *  @param [in] uint8 level
 *                  HIGH(1) or LOW(0)
 *  @return     M2M_SUCCESS in case of success and M2M_ERR_INVALID_ARG in case of an 
				incorrect parameter
 */
sint8 nm_bus_speed(uint8 level);

sint8 nm_spi_rw(uint8* pu8Mosi, uint8* pu8Miso, uint16 u16Sz);

extern tstrNmBusCapabilities egstrNmBusCapabilities;

#if (defined __cplusplus)
}
#endif
/**@}*/
#endif /* _NM_BUS_WRAPPER_H_ */
