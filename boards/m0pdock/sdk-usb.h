/*
 * sdk-usb.h - Interface to USB stack in Bouffalo's SDK
 *
 * This work is licensed under the terms of the MIT License.
 * A copy of the license can be found in the file LICENSE.MIT
 */

#ifndef USB_H
#define	USB_H

#define	USB_VENDOR	0x20b7	/* Qi Hardware */
#define	USB_PRODUCT	0xae72	/* Sunela (AnELok, device 2) */

#define	USB_MAX_POWER	500	/* 500 mA */

/*
 * bmRequestType:
 *
 * D7 D6..5 D4...0
 * |  |     |
 * direction (0 = host->dev)
 *    type (2 = vendor)
 *          recipient (0 = device)
 */

#ifndef	USB_TYPE_VENDOR
#define	USB_TYPE_VENDOR	0x40
#endif

#ifndef	USB_DIR_IN
#define	USB_DIR_IN	0x80
#endif

#ifndef	USB_DIR_OUT
#define	USB_DIR_OUT	0x00
#endif

#define	FROM_DEV	(USB_TYPE_VENDOR | USB_DIR_IN)
#define	TO_DEV		(USB_TYPE_VENDOR | USB_DIR_OUT)


enum sunela_requests {
	/* 1 was SUNELA_TIME */
	SUNELA_QUERY	= 2,
	SUNELA_DEMO	= 3,
	SUNELA_RMT	= 4,
};

#endif /* !USB_H */
