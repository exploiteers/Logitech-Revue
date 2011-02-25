/*
 *      uac_mike -- USB Audio class input device(mike) descriptors.
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      author: Ravi.B ( ravibabu@ti.com)
 */
#include "guvc.h"
#if 0
static struct usb_device_descriptor
device_desc = {
        .bLength =              sizeof device_desc,
        .bDescriptorType =      USB_DT_DEVICE,
        .bcdUSB =               __constant_cpu_to_le16 (0x0200),
        .bDeviceClass =         0,   /* for audio device clas bDeviceClass is zero */
        .bDeviceSubClass =      0x0, /* class info is found at interface level */
        .bDeviceClass =         0x0,
        .idVendor =             __constant_cpu_to_le16 (DRIVER_VENDOR_NUM),
        .idProduct =            __constant_cpu_to_le16 (DRIVER_PRODUCT_NUM),
        .bcdDevice =            0x0100,
        .iManufacturer =        STRING_MANUFACTURER,
        .iProduct =             STRING_PRODUCT,
        .iSerialNumber =        0,//STRING_SERIAL,
        .bNumConfigurations =   1,
};
static struct usb_config_descriptor
config_desc = {
        .bLength =              sizeof config_desc,
        .bDescriptorType =      USB_DT_CONFIG,
        /* wTotalLength computed by usb_gadget_config_buf() */
        .bNumInterfaces =       2, /* AC and AS interfaces*/
        .bConfigurationValue =  1,
        .bmAttributes =         USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
        .bMaxPower =            1,      // self-powered
};
/* USB_DT_INTERFACE_ASSOCIATION: groups interfaces */

struct usb_interface_assoc_descriptor {
	u8  bLength;
	u8  bDescriptorType;

	u8  bFirstInterface;
	u8  bInterfaceCount;
	u8  bFunctionClass;
	u8  bFunctionSubClass;
	u8  bFunctionProtocol;
	u8  iFunction;
} __attribute__ ((packed));
#endif

static const struct usb_interface_assoc_descriptor
uac_aic_iad = {
	.bLength = 0x8,		/* sizeof uac_aic_iad,                          */
	.bDescriptorType = 0x0b, //DT_INTERFACE_ASSOCIATION,	/* 0x0B                                                         */
	.bFirstInterface = AC_INTERFACE_NUMBER,
	.bInterfaceCount = 0x02,
	.bFunctionClass = 1,	/* Audio Interface Class Value 0x01     */
	.bFunctionSubClass = 2,	/*0->2LK: Audio Interface Collection       */
	.bFunctionProtocol = 0x00,	/* PC_PROTOCOL_UNDEFINED                        */
	.iFunction = 0,	/* 0x0                                                          */
};

static struct usb_interface_descriptor
ac_std_intf_desc0 = {
        .bLength =              sizeof ac_std_intf_desc0,
        .bDescriptorType =      USB_DT_INTERFACE,

        .bInterfaceNumber =     AC_INTERFACE_NUMBER, //0x0, /* interface number is 0 */
        .bAlternateSetting =    0x0,
        .bNumEndpoints =        0,
        .bInterfaceClass =      USB_CLASS_AUDIO,
        .bInterfaceSubClass =   0x1, /* USB_SUBCLASS_AUDIO_CONTROL */
        .bInterfaceProtocol =   0x0, /* PROTOCOL UNDEFINED */
};


DECLARE_AC_CS_HEADER_DESC(ac_cs_intf_header, 1, 0x001E, AS_ISOC_IN_INTERFACE);
static DECLARE_AC_INPUT_TERM_DESC (ac_input_term_desc0, 1, 0x201, 0,1,0,0,0);
//static DECLARE_AC_OUTPUT_TERM_DESC (ac_output_term_desc0, 2, 0x0101, 1, 1, 0);
//assocTerminal =1
static DECLARE_AC_OUTPUT_TERM_DESC (ac_output_term_desc0, 2, 0x0101, 0, 1, 0);
static struct usb_interface_descriptor
as_std_intf_desc0_alt0 = {
        .bLength =              sizeof as_std_intf_desc0_alt0,
        .bDescriptorType =      USB_DT_INTERFACE,

        .bInterfaceNumber =     AS_ISOC_IN_INTERFACE, //0x1, /* interface number is 0 */
        .bAlternateSetting =    0x0,
        .bNumEndpoints =        0,
        .bInterfaceClass =      USB_CLASS_AUDIO,
        .bInterfaceSubClass =   0x2, /* USB_SUBCLASS_AUDIO_STREAMING SUBCLASS */
        .bInterfaceProtocol =   0x0, /* PROTOCOL UNDEFINED */
};

static struct usb_interface_descriptor
as_std_intf_desc0_alt1 = {
        .bLength =              sizeof as_std_intf_desc0_alt1,
        .bDescriptorType =      USB_DT_INTERFACE,

        .bInterfaceNumber =     AS_ISOC_IN_INTERFACE, //0x1, /* interface number is 0 */
        .bAlternateSetting =    0x1,
        .bNumEndpoints =        1,
        .bInterfaceClass =      USB_CLASS_AUDIO,
        .bInterfaceSubClass =   0x2, /* USB_SUBCLASS_AUDIO_STREAMING SUBCLASS */
        .bInterfaceProtocol =   0x0, /* PROTOCOL UNDEFINED */
};
struct usb_as_intf_desc
as_cs_intf_desc0 = {
        .bLength = sizeof as_cs_intf_desc0,
        .bDescriptorType = 0x24,
        .bDescriptorSubType = 0x1,
        .bTerminalLink = 2,
        .bDelay = 0x01,
        .wFormatTag = 0x1, // 0x0005, /* PCM->Mu-law:LK:0001->0005 */
};
struct usb_as_format_intf_desc
as_intf_format_desc0 = {
#if 1 //working with -f S16_LE
        .bLength = 0x0b, //sizeof as_intf_format_desc0,
        .bDescriptorType = 0x24,
        .bDescriptorSubType = 0x02,
        .bFormatType = 0x1,
        .bNrChannels = 0x1,
        .bSubframeSize = 0x02,
        .bBitResolution = 0x10,
        .bSampFreqType = 0x01,
        .SamFreq0 = 0x3e80, //0x01f40, (8Khz) fa00-64khz
#else
		.bLength = 0x0b, //sizeof as_intf_format_desc0,
		.bDescriptorType = 0x24,
		.bDescriptorSubType = 0x02,
		.bFormatType = 0x1,
		.bNrChannels = 0x1,
		.bSubframeSize = 0x02,
		.bBitResolution = 0x08,
		.bSampFreqType = 0x01,
		.SamFreq0 = 0x1f40, //0x01f40, (8Khz) fa00-64khz
#endif
};

struct usb_endpoint_descriptor
as_iso_in_ep_desc0 = {
        .bLength = 0x09, //USB_DT_ENDPOINT_SIZE+2,
        .bDescriptorType = 0x05,

//        .bEndpointAddress = USB_DIR_IN, /* IN endpoint ep number -1 */
		.bEndpointAddress = USB_DIR_IN, /* IN endpoint ep number -1 */
        .bmAttributes = 0x05, /*1->5LK transfer=ISO, SYNC=adaptive, usage-DATA EP*/
        .wMaxPacketSize = 0x40, // 0x0010,
        .bInterval = 0x4, // 0x01,
};

struct usb_as_iso_ep_desc
as_cs_data_ep_desc0 = {
        .bLength = sizeof as_cs_data_ep_desc0,
        .bDescriptorType = 0x25,
        .bDescriptorSubType = 0x01,
        .bmAttributes = 0x0, /* D0:SampFreq,D1:Pitch,D6:2-Resrvd,D7:MaxPackets only */
        .bLockDelayUints = 0x0,
        .wLockDelay = 0x0,
};

#if 0
static const struct usb_descriptor_header *function[] = {
        (struct usb_descriptor_header *)&ac_std_intf_desc0,
        (struct usb_descriptor_header *)&ac_cs_intf_header,
        (struct usb_descriptor_header *)&ac_input_term_desc0,
        (struct usb_descriptor_header *)&ac_output_term_desc0,
        (struct usb_descriptor_header *)&as_std_intf_desc0_alt0,
        (struct usb_descriptor_header *)&as_std_intf_desc0_alt1,
        (struct usb_descriptor_header *)&as_cs_intf_desc0,
        (struct usb_descriptor_header *)&as_intf_format_desc0,
        (struct usb_descriptor_header *)&as_iso_in_ep_desc0,
        (struct usb_descriptor_header *)&as_cs_data_ep_desc0,
        NULL,
};
#endif
