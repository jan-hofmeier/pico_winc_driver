/**
 *
 * \file
 *
 * \brief This module contains M2M host interface APIs implementation.
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

#include "driver/source/m2m_hif.h"
#include "driver/source/nmbus.h"
#include "bsp/include/nm_bsp.h"
#include "conf_winc.h"
#include "m2m_types.h"
#include "nmasic.h"
#include "m2m_periph.h"
#include "m2m_wifi.h"
#include "nm_bsp.h"
#include "bus_wrapper/include/nm_bus_wrapper.h"
#include "m2m_ota.h"
#include "socket/include/socket.h"
#include "memory.h"
#include "nmi2c.h"
#include <stdlib.h>


#if (defined NM_EDGE_INTERRUPT)||(defined NM_LEVEL_INTERRUPT)
#if !((defined NM_EDGE_INTERRUPT)&&(defined NM_LEVEL_INTERRUPT))
#else
#error "only one type of interrupt NM_EDGE_INTERRUPT,NM_LEVEL_INTERRUPT"
#endif
#else
#error "define interrupt type NM_EDGE_INTERRUPT,NM_LEVEL_INTERRUPT"
#endif

#define WIFI_HOST_RCV_CTRL_0	(0x1070)
#define WIFI_HOST_RCV_CTRL_1	(0x1084)

#define M2M_HIF_WAKE_VALUE				(0xF0)
#define M2M_HIF_WAKE_ADDR				(0x01)

#define M2M_HIF_ACK_VALUE				(0x0F)
#define M2M_HIF_ACK_ADDR				(0x02)

#define M2M_HIF_CHIP_SLEEP_VALUE		(0x0F)
#define M2M_HIF_CHIP_SLEEP_ADDR			(0x03)

#define M2M_HIF_CHIP_WAKE_VALUE			(0xF0)
#define M2M_HIF_CHIP_WAKE_ADDR			(0x03)

#define M2M_HIF_HAVE_INTR_VALUE			(0x0F)
#define M2M_HIF_HAVE_INTR_ADDR			(0x04)

#define M2M_HIF_IST_REG_ADDR			(0xfa)
#define M2M_HIF_REPLY_INTR_ADDR			(0xfc)

#ifdef ARDUINO
#define NM_BUS_MAX_TRX_SZ	256
#else
#define NM_BUS_MAX_TRX_SZ	4096
#endif

typedef struct {
	uint8 u8ChipMode;
	uint8 u8ChipSleep;
	uint8 u8HifRXDone;
	uint8 u8HifTxDone;
	uint8 u8Interrupt;
	uint8 u8Yield;
	uint8 u8PsMode;
}tstrHifContext;

static volatile tstrHifContext gstrHifCxt;
static tpfHifCallBack pfWifiCb = NULL;
static tpfHifCallBack pfIpCb = NULL;
static tpfHifCallBack pfOtaCb = NULL;
static tpfHifCallBack pfSigmaCb = NULL;
static tpfHifCallBack pfHifCb = NULL;
static tpfHifCallBack pfCryptoCb = NULL;
static tpfHifCallBack pfSslCb = NULL;

static void isr(void);

static sint8 hif_set_rx_done(void)
{
	gstrHifCxt.u8Interrupt = 0;
	gstrHifCxt.u8HifRXDone = 1;
	return M2M_SUCCESS;
}
/**
*	@fn		hif_handle_isr(void)
*	@brief	Handle interrupt received from NMC1500 firmware.
*   @return
			The function SHALL return 0 for success and a negative value otherwise.
*/
NMI_API sint8 hif_handle_isr(void)
{
	sint8 ret = M2M_SUCCESS;
	uint32 tmp;
	tstrHifHdr strHif;

	if (gstrHifCxt.u8Interrupt) {
		ret = nm_read_reg_with_ret(M2M_HIF_IST_REG_ADDR, (uint32 *)&tmp);
		if (ret != M2M_SUCCESS) {
			M2M_ERR("bus error\n");
			return ret;
		}

		if (tmp & NBIT0) { /* data pkt */
			ret = nm_read_block(WIFI_HOST_RCV_CTRL_0, (uint8*)&strHif, sizeof(tstrHifHdr));
			if (ret != M2M_SUCCESS) {
				M2M_ERR("bus error\n");
				return ret;
			}
			strHif.u16Length = NM_BSP_B_L_16(strHif.u16Length);
			if (strHif.u16Length > NM_BUS_MAX_TRX_SZ) {
				M2M_ERR("bus error\n");
				return M2M_ERR_BUS_FAIL;
			}
			if (strHif.u16Length > 0) {
				if (pfWifiCb)
					pfWifiCb(strHif.u8Opcode, strHif.u16Length, WIFI_HOST_RCV_CTRL_0 + sizeof(tstrHifHdr));
			}

			ret = nm_read_reg_with_ret(WIFI_HOST_RCV_CTRL_0, &tmp);
			if (ret != M2M_SUCCESS) {
				M2M_ERR("bus error\n");
				return ret;
			}

			tmp &= ~NBIT0;
			ret = nm_write_reg(WIFI_HOST_RCV_CTRL_0, tmp);
			if (ret != M2M_SUCCESS) {
				M2M_ERR("bus error\n");
				return ret;
			}
		}

		if (tmp & NBIT1) { /* data pkt */
			ret = nm_read_block(WIFI_HOST_RCV_CTRL_1, (uint8*)&strHif, sizeof(tstrHifHdr));
			if (ret != M2M_SUCCESS) {
				M2M_ERR("bus error\n");
				return ret;
			}
			strHif.u16Length = NM_BSP_B_L_16(strHif.u16Length);
			if (strHif.u16Length > NM_BUS_MAX_TRX_SZ) {
				M2M_ERR("bus error\n");
				return M2M_ERR_BUS_FAIL;
			}
			if (strHif.u16Length > 0) {
				if (pfWifiCb)
					pfWifiCb(strHif.u8Opcode, strHif.u16Length, WIFI_HOST_RCV_CTRL_1 + sizeof(tstrHifHdr));
			}

			ret = nm_read_reg_with_ret(WIFI_HOST_RCV_CTRL_1, &tmp);
			if (ret != M2M_SUCCESS) {
				M2M_ERR("bus error\n");
				return ret;
			}

			tmp &= ~NBIT0;
			ret = nm_write_reg(WIFI_HOST_RCV_CTRL_1, tmp);
			if (ret != M2M_SUCCESS) {
				M2M_ERR("bus error\n");
				return ret;
			}
		}

		if (tmp & NBIT2) {
			uint32 reg;
			ret = nm_read_reg_with_ret(0x10a0, &reg);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
			reg |= (1ul << 3);
			ret = nm_write_reg(0x10a0, reg);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
			reg &= ~(1ul << 3);
			ret = nm_write_reg(0x10a0, reg);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
		}

		if (tmp & NBIT3) { /* control */
			uint16 u16len = 0;
			uint32 address = 0;

			ret = nm_read_block(M2M_HIF_REPLY_INTR_ADDR, (uint8*)&strHif, sizeof(tstrHifHdr));
			if (ret != M2M_SUCCESS) {
				M2M_ERR("bus error\n");
				return ret;
			}
			u16len = NM_BSP_B_L_16(strHif.u16Length);
			address = M2M_HIF_REPLY_INTR_ADDR + sizeof(tstrHifHdr);

			switch(strHif.u8Gid)
			{
			case M2M_REQ_GROUP_WIFI:
				if(pfWifiCb)
					pfWifiCb(strHif.u8Opcode,u16len,address);
				break;
			case M2M_REQ_GROUP_IP:
				if(pfIpCb)
					pfIpCb(strHif.u8Opcode,u16len,address);
				break;
			case M2M_REQ_GROUP_OTA:
				if(pfOtaCb)
					pfOtaCb(strHif.u8Opcode,u16len,address);
				break;
			case M2M_REQ_GROUP_HIF:
				if(pfHifCb)
					pfHifCb(strHif.u8Opcode,u16len,address);
				break;
			case M2M_REQ_GROUP_CRYPTO:
				if(pfCryptoCb)
					pfCryptoCb(strHif.u8Opcode,u16len,address);
				break;
			case M2M_REQ_GROUP_SSL:
				if(pfSslCb)
					pfSslCb(strHif.u8Opcode,u16len,address);
				break;
			default:
				M2M_ERR("GRp not defined\n");
				break;
			}

			ret = nm_read_reg_with_ret(M2M_HIF_IST_REG_ADDR, (uint32 *)&tmp);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
			tmp &= ~NBIT3;
			ret = nm_write_reg(M2M_HIF_IST_REG_ADDR, tmp);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
		}

		if (tmp & NBIT4) { // Other interrupts
			ret = nm_read_reg_with_ret(M2M_HIF_IST_REG_ADDR, (uint32 *)&tmp);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
			tmp &= ~NBIT4;
			ret = nm_write_reg(M2M_HIF_IST_REG_ADDR, tmp);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
		}
		if (tmp & NBIT5) { // Other interrupts
			ret = nm_read_reg_with_ret(M2M_HIF_IST_REG_ADDR, (uint32 *)&tmp);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
			tmp &= ~NBIT5;
			ret = nm_write_reg(M2M_HIF_IST_REG_ADDR, tmp);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
		}
		if (tmp & NBIT6) { // Other interrupts
			ret = nm_read_reg_with_ret(M2M_HIF_IST_REG_ADDR, (uint32 *)&tmp);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
			tmp &= ~NBIT6;
			ret = nm_write_reg(M2M_HIF_IST_REG_ADDR, tmp);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
		}
		if (tmp & NBIT7) { // Other interrupts
			ret = nm_read_reg_with_ret(M2M_HIF_IST_REG_ADDR, (uint32 *)&tmp);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
			tmp &= ~NBIT7;
			ret = nm_write_reg(M2M_HIF_IST_REG_ADDR, tmp);
			if(M2M_SUCCESS != ret) {
				M2M_ERR("BUS-Read-Error\n");
				return ret;
			}
		}
		gstrHifCxt.u8Interrupt = 0;
	}

	return ret;
}

static void isr(void)
{
	gstrHifCxt.u8Interrupt++;
#ifdef CORTUS_APP
	if(gstrHifCxt.u8Yield == 1)
	{
		hif_yield();
	}
#endif
}

/**
*   @fn			hif_init
*   @brief
				To initialize HIF layer.
*   @param [in]	arg
*				Pointer to the arguments.
*   @return
			    The function shall return ZERO for successful operation and a negative value otherwise.
*/
sint8 hif_init(void * arg)
{
	m2m_memset((uint8*)&gstrHifCxt,0,sizeof(tstrHifContext));
	nm_bsp_register_isr(isr);
	hif_set_sleep_mode(M2M_NO_POWERSAVE);
	return M2M_SUCCESS;
}
/**
*	@fn		hif_deinit
*	@brief	De-initialize HIF layer
*	@author
*	@date
*	@version	1.0
*/
sint8 hif_deinit(void * arg)
{
	sint8 ret = M2M_SUCCESS;
	ret = nm_bus_deinit();
	return ret;
}
/**
*	@fn			hif_register_cb
*	@brief
				To set Callback function for every  Component.

*	@param [in]	u8Grp
*				Group to which the Callback function should be set.

*	@param [in]	fn
*				function to be set to the specified group.
*   @return
			    The function shall return ZERO for successful operation and a negative value otherwise.
*/
sint8 hif_register_cb(uint8 u8Grp,tpfHifCallBack fn)
{
	sint8 ret = M2M_SUCCESS;
	switch(u8Grp)
	{
		case M2M_REQ_GROUP_WIFI:
			pfWifiCb = fn;
			break;
		case M2M_REQ_GROUP_IP:
			pfIpCb = fn;
			break;
		case M2M_REQ_GROUP_OTA:
			pfOtaCb = fn;
			break;
		case M2M_REQ_GROUP_HIF:
			pfHifCb = fn;
			break;
		case M2M_REQ_GROUP_CRYPTO:
			pfCryptoCb = fn;
			break;
		case M2M_REQ_GROUP_SIGMA:
			pfSigmaCb = fn;
			break;
		case M2M_REQ_GROUP_SSL:
			pfSslCb = fn;
			break;
		default:
			ret = M2M_ERR_FAIL;
			break;
	}
	return ret;
}
/**
*	@fn		hif_send
*	@brief	Send packet using host interface
*	@param [in]	u8Gid
*				Group ID
*	@param [in]	u8Opcode
*				Operation ID
*	@param [in]	pu8CtrlBuf
*				Pointer to the Control buffer
*	@param [in]	u16CtrlBufSize
				Control buffer size
*	@param [in]	pu8DataBuf
*				Packet buffer Allocated by the caller
*	@param [in]	u16DataSize
*				Packet buffer size (including the HIF header)
*	@param [in]	u16DataOffset
*				Packet Data offset
*   @return
		The function shall return ZERO for successful operation and a negative value otherwise.
*/
sint8 hif_send(uint8 u8Gid,uint8 u8Opcode,uint8 *pu8CtrlBuf,uint16 u16CtrlBufSize,
			   uint8 *pu8DataBuf,uint16 u16DataSize, uint16 u16DataOffset)
{
	sint8 ret = M2M_SUCCESS;

	tstrHifHdr* pstrHif = (tstrHifHdr*)malloc(sizeof(tstrHifHdr) + u16CtrlBufSize + u16DataSize);
	if (NULL != pstrHif) {
		pstrHif->u8Gid = u8Gid;
		pstrHif->u8Opcode = u8Opcode;
		pstrHif->u16Length = NM_BSP_B_L_16(u16CtrlBufSize+u16DataSize);

		if(u16CtrlBufSize > 0)
			m2m_memcpy(((uint8*)pstrHif)+sizeof(tstrHifHdr),pu8CtrlBuf,u16CtrlBufSize);

		if(u16DataSize > 0)
			m2m_memcpy(((uint8*)pstrHif)+sizeof(tstrHifHdr)+u16CtrlBufSize,pu8DataBuf,u16DataSize);

		ret = nm_bus_ioctl(NM_BUS_IOCTL_W, pstrHif);
		if(ret != M2M_SUCCESS)
		{
			free(pstrHif);
		}
	} else {
		ret = M2M_ERR_MEM_ALLOC;
	}
	return ret;
}
/**
*	@fn		hif_chip_sleep
*	@brief	put the chip in sleep mode
*	@return
			The function shall return ZERO for successful operation and a negative value otherwise.
*/
sint8 hif_chip_sleep(void)
{
	if(gstrHifCxt.u8ChipSleep == 0)
	{
		sint8 ret;
		gstrHifCxt.u8ChipSleep = 1;
		ret = nm_write_reg(M2M_HIF_CHIP_SLEEP_ADDR, M2M_HIF_CHIP_SLEEP_VALUE);
		return ret;
	}
	return M2M_SUCCESS;
}
/**
*	@fn		hif_chip_wake
*	@brief	Wakeup the chip.
*   @return
			The function shall return ZERO for successful operation and a negative value otherwise.
*/
sint8 hif_chip_wake(void)
{
	sint8 ret = M2M_ERR_FAIL;
	if (gstrHifCxt.u8ChipSleep)
	{
		ret = nm_write_reg(M2M_HIF_CHIP_WAKE_ADDR, M2M_HIF_CHIP_WAKE_VALUE);
		if(ret == M2M_SUCCESS)
			gstrHifCxt.u8ChipSleep = 0;
	}
	return ret;
}

/**
*	@fn		hif_check_wake(void)
*	@brief	Check of the chip is awake.
*   @return
			The function shall return ZERO for successful operation and a negative value otherwise.
*/
static sint8 hif_check_wake(void)
{
	sint8 ret = M2M_ERR_FAIL;
	uint8 tmp;

	ret = nm_read_reg_with_ret(M2M_HIF_HAVE_INTR_ADDR, (uint32 *)&tmp);
	if (ret == M2M_SUCCESS) {
		if (tmp == M2M_HIF_HAVE_INTR_VALUE) {
			gstrHifCxt.u8ChipSleep = 0;
		}
	}

	return ret;
}
/**
*	@fn		hif_chip_sleep_sc
*	@brief	clear the chip count only but keep the chip wake
*	@return
			The function shall return ZERO for successful operation and a negative value otherwise.
*/
sint8 hif_chip_sleep_sc(void)
{
	if(gstrHifCxt.u8ChipSleep == 0)
	{
		gstrHifCxt.u8ChipSleep = 1;
	}
	return M2M_SUCCESS;
}

/**
*	@fn		hif_set_sleep_mode(void)
*	@brief	Set the sleep mode of the HIF layer.
*	@param[IN] u8Pstype
*				Sleep mode.
*	@return
			The function SHALL return 0 for success and a negative value otherwise.
*/
void hif_set_sleep_mode(uint8 u8Pstype)
{
	gstrHifCxt.u8PsMode = u8Pstype;
}
/**
*	@fn		hif_get_sleep_mode(void)
*	@brief	Get the sleep mode of the HIF layer.
*	@return
			The function SHALL return the sleep mode of the HIF layer.
*/
uint8 hif_get_sleep_mode(void)
{
	return gstrHifCxt.u8PsMode;
}
#ifdef CORTUS_APP
void hif_yield(void)
{
	gstrHifCxt.u8Yield = 1;
	while(gstrHifCxt.u8Yield) nm_bsp_yield();
}
/**
*	@fn		hif_Resp_handler(uint8 *pu8Buffer, uint16 u16BufferSize)
*	@brief	Response handler for HIF layer
*	@param [in]	pu8Buffer
*				Pointer to the buffer
*	@param [in]	u16BufferSize
*				Buffer size
*   @return
		The function SHALL return 0 for success and a negative value otherwise.
*/
sint8 hif_Resp_handler(uint8 *pu8Buffer, uint16 u16BufferSize)
{
	sint8 ret = M2M_SUCCESS;
	tstrHifHdr	*pstrHif = (tstrHifHdr*)pu8Buffer;

	pstrHif->u16Length = NM_BSP_B_L_16(pstrHif->u16Length);

	switch(pstrHif->u8Gid)
	{
	case M2M_REQ_GROUP_WIFI:
		if(pfWifiCb)
			pfWifiCb(pstrHif->u8Opcode,pstrHif->u16Length - M2M_HIF_HDR_OFFSET, (uint32)pstrHif+M2M_HIF_HDR_OFFSET);
		break;
	case M2M_REQ_GROUP_IP:
		if(pfIpCb)
			pfIpCb(pstrHif->u8Opcode,pstrHif->u16Length - M2M_HIF_HDR_OFFSET, (uint32)pstrHif+M2M_HIF_HDR_OFFSET);
		break;
	case M2M_REQ_GROUP_OTA:
		if(pfOtaCb)
			pfOtaCb(pstrHif->u8Opcode,pstrHif->u16Length - M2M_HIF_HDR_OFFSET, (uint32)pstrHif+M2M_HIF_HDR_OFFSET);
		break;
	case M2M_REQ_GROUP_SIGMA:
		if(pfSigmaCb)
			pfSigmaCb(pstrHif->u8Opcode, pstrHif->u16Length, (uint32)pstrHif+M2M_HIF_HDR_OFFSET);
		break;
	case M2M_REQ_GROUP_HIF:
		if(pfHifCb)
			pfHifCb(pstrHif->u8Opcode, pstrHif->u16Length, (uint32)pstrHif+M2M_HIF_HDR_OFFSET);
		break;
	case M2M_REQ_GROUP_CRYPTO:
		if(pfCryptoCb)
			pfCryptoCb(pstrHif->u8Opcode, pstrHif->u16Length, (uint32)pstrHif+M2M_HIF_HDR_OFFSET);
		break;
	case M2M_REQ_GROUP_SSL:
		if(pfSslCb)
			pfSslCb(pstrHif->u8Opcode,pstrHif->u16Length, (uint32)pstrHif+M2M_HIF_HDR_OFFSET);
		break;
	}
	gstrHifCxt.u8Yield = 0;
	return ret;
}
#endif

sint8 hif_receive(uint32 u32Addr, uint8 *pu8Buf, uint16 u16Sz, uint8 isDone)
{
	uint32 address, reg;
	uint16 size;
	sint8 ret = M2M_SUCCESS;

	if(u32Addr == 0 ||pu8Buf == NULL || u16Sz == 0)
	{
		if(isDone)
		{
			/* set RX done */
			ret = hif_set_rx_done();
		}
		else
		{
			ret = M2M_ERR_FAIL;
			M2M_ERR(" hif_receive: Invalid argument\n");
		}
		goto ERR1;
	}

	ret = nm_read_reg_with_ret(WIFI_HOST_RCV_CTRL_0,&reg);
	if(ret != M2M_SUCCESS)goto ERR1;


	size = (uint16)((reg >> 2) & 0xfff);
	ret = nm_read_reg_with_ret(WIFI_HOST_RCV_CTRL_1,&address);
	if(ret != M2M_SUCCESS)goto ERR1;


	if(u16Sz > size)
	{
		ret = M2M_ERR_FAIL;
		M2M_ERR("APP Requested Size is larger than the recived buffer size <%d><%d>\n",u16Sz, size);
		goto ERR1;
	}
	if((u32Addr < address)||((u32Addr + u16Sz)>(address+size)))
	{
		ret = M2M_ERR_FAIL;
		M2M_ERR("APP Requested Address beyond the recived buffer address and length\n");
		goto ERR1;
	}
	
	/* Receive the payload */
	ret = nm_read_block(u32Addr, pu8Buf, u16Sz);
	if(ret != M2M_SUCCESS)goto ERR1;

	/* check if this is the last packet */
	if((((address + size) - (u32Addr + u16Sz)) <= 0) || isDone)
	{
		/* set RX done */
		ret = hif_set_rx_done();
	}

ERR1:
	return ret;
}
