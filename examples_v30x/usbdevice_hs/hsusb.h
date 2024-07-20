#ifndef _HSUSB_H
#define _HSUSB_H

/* High speed USB infrastructure for CH32V30x.
   Based off of the official USB stack and the current CH32X035 FS implementation
*/

#include <stdint.h>
#include "ch32v003fun.h"
#include "usb_defines.h"
#include "usb_config.h"

struct _USBState
{
	// Setup Request
	uint8_t  USBHS_SetupReqCode;
	uint8_t  USBHS_SetupReqType;
	uint16_t USBHS_SetupReqLen;   // Used for tracking place along send.
	uint32_t USBHS_IndexValue;

	// USB Device Status
	uint16_t  USBHS_DevConfig;
	uint16_t  USBHS_DevAddr;
	uint8_t  USBHS_DevSleepStatus;
	uint8_t  USBHS_DevEnumStatus;

	uint8_t  *  pCtrlPayloadPtr;

	uint8_t ENDPOINTS[HUSB_CONFIG_EPS][64];

	#define CTRL0BUFF					(HSUSBCTX.ENDPOINTS[0])
	#define pUSBHS_SetupReqPak			((tusb_control_request_t*)CTRL0BUFF)

#if HUSB_HID_INTERFACES > 0
	uint8_t USBHS_HidIdle[HUSB_HID_INTERFACES];
	uint8_t USBHS_HidProtocol[HUSB_HID_INTERFACES];
#endif
	volatile uint8_t  USBHS_Endp_Busy[HUSB_CONFIG_EPS];
};

// Provided functions:
int HSUSBSetup();
uint8_t USBHS_Endp_DataUp(uint8_t endp, const uint8_t *pbuf, uint16_t len, uint8_t mod);

// Implement the following:
#if HUSB_HID_USER_REPORTS
int HandleHidUserGetReportSetup( struct _USBState * ctx, tusb_control_request_t * req );
int HandleHidUserSetReportSetup( struct _USBState * ctx, tusb_control_request_t * req );
void HandleHidUserReportDataOut( struct _USBState * ctx, uint8_t * data, int len );
int HandleHidUserReportDataIn( struct _USBState * ctx, uint8_t * data, int len );
void HandleHidUserReportOutComplete( struct _USBState * ctx );
#endif

extern struct _USBState HSUSBCTX;

#include "hsusb.c"

#endif