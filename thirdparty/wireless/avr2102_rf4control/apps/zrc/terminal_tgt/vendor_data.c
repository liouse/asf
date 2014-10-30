/**
 * @file vendor_data.c
 *
 * @brief Implementation of vendor-specific data handling
 *
 * Copyright (c) 2013 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

/*
 * Copyright (c) 2013, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> EULA.txt
 */

#ifdef VENDOR_DATA

/* === INCLUDES ============================================================ */

#include <stdio.h>

#include "rf4ce.h"
#include "zrc.h"
#include "vendor_data.h"
#include "pal.h"

/* === MACROS ============================================================== */

/* === EXTERNALS =========================================================== */

FLASH_EXTERN(uint16_t VendorIdentifier);
#ifdef RF4CE_TARGET
extern void vendor_data_confirm(nwk_enum_t Status, uint8_t PairingRef,
		profile_id_t ProfileId,
		uint8_t Handle
		);
static uint16_t get_batmon_voltage(void);

#else /* RF4CE_TARGET */
extern void nlme_rx_enable_confirm(nwk_enum_t Status);
static void vendor_data_confirm(nwk_enum_t Status, uint8_t PairingRef,
		profile_id_t ProfileId,
		uint8_t Handle
		);
void vendor_data_ind(uint8_t PairingRef, uint16_t VendorId,
		uint8_t nsduLength, uint8_t *nsdu, uint8_t RxLinkQuality,
		uint8_t RxFlags);

#endif /* RF4CE_TARGET */

/* === IMPLEMENTATION ====================================================== */

bool vendor_data_request(uint8_t PairingRef, profile_id_t ProfileId,
		uint16_t VendorId, uint8_t nsduLength, uint8_t *nsdu,
		uint8_t TxOptions)
{
	/* Keep compiler happy */
	ProfileId = ProfileId;

	return nlde_data_request(PairingRef, PROFILE_ID_ZRC, VendorId,
			nsduLength, nsdu, TxOptions,
			1,
			(FUNC_PTR)vendor_data_confirm
			);
}

#ifndef RF4CE_TARGET
void vendor_data_ind(uint8_t PairingRef, uint16_t VendorId,
		uint8_t nsduLength, uint8_t *nsdu, uint8_t RxLinkQuality,
		uint8_t RxFlags)
{
	/* Check if vendor id matches.
	 * Handle here only vendor data from same vendor */
	uint16_t v_id = PGM_READ_WORD(&VendorIdentifier);
	if ((VendorId == v_id) && (RxFlags & RX_FLAG_WITH_SEC)) {
		switch (nsdu[0]) { /* vendor-specific command id */
		case BATTERY_STATUS_REQ:
		{
			uint16_t voltage = get_batmon_voltage();
			nsdu[0] = BATTERY_STATUS_RESP;
			nsdu[1] = (uint8_t)voltage; /* LSB */
			nsdu[2] = (uint8_t)(voltage >> 8); /* MSB */
			nsduLength = 3;
		}
		break;

		case ALIVE_REQ: /* Alive request */
			vendor_app_alive_req();
			/* Send alive response */
			nsdu[0] = ALIVE_RESP;
			nsduLength = 1;
			break;

		case FW_VERSION_REQ:
		{
			/* Send alive response */
			nsdu[0] = FW_VERSION_RESP;
			nsdu[1] = FW_VERSION_MAJOR; /* major version number */
			nsdu[2] = FW_VERSION_MINOR; /* minor version number */
			nsdu[3] = FW_VERSION_REV; /* revision version number */
			nsduLength = 4;
		}
		break;

		case RX_ON_REQ:
		{
			uint32_t duration = 0;

			memcpy(&duration, &nsdu[1], 3);
			if (!nlme_rx_enable_request(duration,
					(FUNC_PTR)nlme_rx_enable_confirm
					)) {
				/*
				 * RX enable could not been added to the queue.
				 * Therefore do not send response message.
				 */
				return;
			}

			/* Send response */
			nsdu[0] = RX_ON_RESP;
			nsduLength = 1;
		}
		break;

		default:
		{
			/* Send response */
			nsdu[0] = FW_DATA_RESP;
			nsdu[1] = VD_NOT_SUPPORTED_ATTRIBUTE;
			nsduLength = 2;
		}
		break;
		}

		/* Transmit response message */

		nlde_data_request(PairingRef, PROFILE_ID_ZRC, VendorId,
				nsduLength, nsdu,
				TXO_UNICAST | TXO_DST_ADDR_NET | TXO_ACK_REQ | TXO_SEC_REQ | TXO_MULTI_CH | TXO_CH_NOT_SPEC | TXO_VEND_SPEC,
				1,
				(FUNC_PTR)vendor_data_confirm
				);
		/* Keep compiler happy */
		RxLinkQuality = RxLinkQuality;
		RxFlags = RxFlags;
	}
}

#endif  /* #ifndef RF4CE_TARGET */

#ifndef RF4CE_TARGET
static void vendor_data_confirm(nwk_enum_t Status, uint8_t PairingRef,
		profile_id_t ProfileId,
		uint8_t Handle
		)
{
	/* Keep compiler happy */
	Status = Status;
	PairingRef = PairingRef;

	Handle = Handle;
	ProfileId = ProfileId;
}

#endif
static uint16_t get_batmon_voltage(void)
{
	uint16_t voltage;
#ifdef TFA_BAT_MON
	voltage = tfa_get_batmon_voltage();
#elif CUSTOM_BOARD
	/*User can get the battery voltage for his custom board*/
#else
	/*USB powered hence giving vcc as bat voltage*/
	voltage = 0xCE4;
#endif
	return voltage;
}

#endif  /* #ifdef VENDOR_DATA */
