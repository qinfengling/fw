/*
 * sdk-usb.c - Interface to USB stack in Bouffalo's SDK
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#include "usbd_core.h"

#include "../../sys/debug.h"
#include "../../sdk-hal.h"

#include "sdk-usb.h"


/* --- Descriptors --------------------------------------------------------- */

/*
 * Based on Bouffalo's hid_keyboard_template.c
 */

#define	USB_CONFIG_DESC_SIZE	(9 + 9)

// http://www.baiheee.com/Documents/090518/090518112619/USB_LANGIDs.pdf
#define	USBD_LANGID_STRING	0x409	// English (United States)

static const uint8_t device_descriptor[] = {
	USB_DEVICE_DESCRIPTOR_INIT(
		USB_2_0,	// bcdUSB
		0,		// bDeviceClass (at interface)
		0,		// bDeviceSubClass
		0,		// bDeviceProtocol
		USB_VENDOR,	// idVendor
		USB_PRODUCT,	// idProduct
		2,		// bcdDevice
		1),		// bNumConfigurations
	USB_CONFIG_DESCRIPTOR_INIT(
		USB_CONFIG_DESC_SIZE, // wTotalLength
		1,		// bNumInterfaces
		1,		// bConfigurationValue
		USB_CONFIG_BUS_POWERED, // bmAttributes
		USB_MAX_POWER),	// bMaxPower
	/* Interface #0 */
		9,		// bLength
		USB_DESCRIPTOR_TYPE_INTERFACE,
		0,		// bInterfaceNumber
		0,		// bAlternateSetting
		0,		// bNumEndpoints
		0xff,		// bInterfaceClass (vendor-specific)
		0,		// bInterfaceSubClass
		0xff,		// bInterfaceProtocol
		0,		// iInterface,
	/* String #0 */
		USB_LANGID_INIT(USBD_LANGID_STRING),
	/* String #1 */
		14,
		USB_DESCRIPTOR_TYPE_STRING,
		'S', 0,
		'u', 0,
		'n', 0,
		'e', 0,
		'l', 0,
		'a', 0,
	/* String #2 */
		26,
		USB_DESCRIPTOR_TYPE_STRING,
		'S', 0,
		'u', 0,
		'n', 0,
		'e', 0,
		'l', 0,
		'a', 0,
		' ', 0,
		'B', 0,
		'L', 0,
		'6', 0,
		'1', 0,
		'8', 0,
};


void usbd_event_handler(uint8_t event)
{
	switch (event) {
	case USBD_EVENT_RESET:
		debug("RESET\n");
		break;
	case USBD_EVENT_CONNECTED:
		debug("CONNECTED\n");
		break;
	case USBD_EVENT_DISCONNECTED:
		debug("DISCONNECTED\n");
		break;
	case USBD_EVENT_RESUME:
		debug("RESUME\n");
		break;
	case USBD_EVENT_SUSPEND:
		debug("SUSPEND\n");
		break;
	case USBD_EVENT_CONFIGURED:
		debug("CONFIGURED\n");
		break;
	case USBD_EVENT_SET_REMOTE_WAKEUP:
		debug("SET WAKEUP\n");
		break;
	case USBD_EVENT_CLR_REMOTE_WAKEUP:
		debug("CLR WAKEUP\n");
		break;
	default:
		break;
	}
}


static int ep0_handler(struct usb_setup_packet *setup, uint8_t **data,
    uint32_t *len)
{
	debug("EP0 bmReqT 0x%2x bReq 0x%02x wInd %u\r\n",
	    setup->bmRequestType, setup->bRequest, setup->wIndex);
	switch (setup->bmRequestType) {
	case FROM_DEV:
		if (!usb_query(setup->bRequest, data, len))
			return -1;
		asm volatile ("fence.i" ::: "memory");
		return 0;
	case TO_DEV:
		if (usb_arrival(setup->bRequest, *data, *len))
			return 0;
		return -1;
	default:
		return -1;
	}
}


static struct usbd_interface intf;


void sunela_usb_init(void)
{
	usbd_desc_register(device_descriptor);
	intf.class_interface_handler = NULL;
	intf.class_endpoint_handler = NULL;
	intf.vendor_handler = ep0_handler;
	intf.notify_handler = NULL;
	usbd_add_interface(&intf);
	usbd_initialize();
}
