/*
 * 	guvc.c -- USB Video Class (UVC) gadget driver
 *      
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *      author: Ajay Kumar Gupta (ajay.gupta@ti.com) 
 */

#include <linux/device.h>
#include <linux/usb_ch9.h>
#include <linux/usb_gadget.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include "guvc.h" 

#define	MEDIA_CHAR_DEVNUM	217
#define USB_CLASS_MISC 	0xEF
struct buf_info {
	int paddr[2];
	int vaddr[2];
};

struct format_info {
        unsigned char   format;
        unsigned char   resolution;
};

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"

/* declare required descriptor */
DECLARE_USB_VC_HEADER_DESCRIPTOR(VC_HEADER_NR_VS);
DECLARE_USB_SELECTOR_UNIT_DESCRIPTOR(VC_SEL_UNIT_NR_IN_PINS);
DECLARE_USB_EXTENSION_UNIT_DESCRIPTOR(VC_EXT_UNIT_NR_IN_PINS,VC_EXT_UNIT_CONTROL_SIZE);
DECLARE_USB_CAM_TERMINAL_DESCRIPTOR(VC_CAM_TERM_CONTROL_SIZE);
DECLARE_USB_VS_INPUT_HEADER_DESCRIPTOR(INPUT_HEADER_NR_FORMART,INPUT_HEADER_CONTROL_SIZE);
DECLARE_USB_PROCESSING_UNIT_DESCRIPTOR(VC_PRO_UNIT_CONTROL_SIZE);
#ifdef BIDIR_STREAM
DECLARE_USB_VS_OUTPUT_HEADER_DESCRIPTOR(1,1);
#endif
/*-------------------------------------------------------------------------*/
#define DRIVER_VERSION 		"v1.0.0"
#define DRIVER_AUTHOR 		"Texas Instruments India Pvt Ltd"
#define DRIVER_DESC 		"Media gadget driver"
#define DRIVER_VENDOR_NUM	0x041E //0x0451 /* with TI's ID XP is not calling SET_CONFIG */
#define DRIVER_PRODUCT_NUM	0x4057
#define STRING_MANUFACTURER	1
#define STRING_PRODUCT		2
#define STRING_SERIAL		3

#define EP0_BUFSIZE 1024
#define DELAYED_STATUS	(EP0_BUFSIZE + 999)

// #define ISOC_IN_FS_PKT_SIZE 0x400 

static const char shortname [] = "g_media";
static const char longname [] = "Media gadget (UVC/UAC)";
static const char manufacturer[] = "Texas Instruments India Pvt Ltd";
static const char serial[] = "0.0.1";

#ifdef UAC
#include "guac.h"
#include "uac_mike.h"
#endif


#define UAVC_CLASS_REQ_SUPPORT

struct media_g_dev *gb_media;
struct media_g_dev *gb_mediauvcuac;

struct common_ctrl {
	u8 info;
	u8 len;
	u16 min;
	u16 max;
	u16 res;
	u16 def;
	u16 cur;
};

struct pu_controls {
	struct common_ctrl brgt;
	struct common_ctrl cnst;
	struct common_ctrl hue;
	struct common_ctrl satr;
   struct common_ctrl wbt;
   struct common_ctrl wbt_auto;
   struct common_ctrl blc;  //blacklight compensation
   struct common_ctrl gain;
   struct common_ctrl gamma;
   struct common_ctrl anti_flicker; 
};
struct ct_controls{
   struct common_ctrl exposure_auto;
	struct common_ctrl exposure_absolute;
	struct common_ctrl exposure_relative;
	struct common_ctrl zoom;
	struct common_ctrl focus;
};

struct uvc_stream {
	struct usb_ep           *isoc_in;
#ifdef BIDIR_STREAM
	struct usb_ep           *isoc_out;
#endif
	struct uvc_streaming_control cur_vs_state;
	struct uvc_streaming_control uvc_prob_comm_ds;
	struct uvc_streaming_control uvc_prob_comm_ds_min;
	struct uvc_streaming_control uvc_prob_comm_ds_max;
	int req_done;
	int eof_flag;
	unsigned char *vmalloc_area;
	unsigned char *vmalloc_phys_area;
	int u_size_current;
	int u_size_remaining;
	int tLen;
	size_t urb_data_size;
	u8 tBuf[4 + T_BUF_SIZE_VID + 4 + 4];
	struct uvc_buf uvc_buffer[UVC_NUM_BUFS];
	struct buf_info bufinfo;
	struct list_head urblist;
	struct timeval scr_timeinfo;
	struct timeval pts_timeinfo;
	u8		freereqcnt;
	struct format_info formatinfo;

	struct uvc_stream_header 	uvc_mjpeg_header;   /* Note: When YUV is enabled, this field is actually uvc_yuv_header */ 
	struct uvc_stream_header 	uvc_mpeg2ts_header;
};

struct media_g_dev {
	spinlock_t lock;
	u8			config;
	struct usb_gadget	*gadget;
	struct usb_ep		*ep0;
	struct usb_request	*ep0req;

	/* uvc control interface */
	struct usb_ep           *int_in;
	struct pu_controls pu_ctrl;
   struct ct_controls ct_ctrl; 

	/* using mmap buffer from uvc driver ? */
#ifdef DUAL_STREAM
	u8	use_mmap[2];
#else
	u8	use_mmap[1];
#endif
	u8	cur_strm;

#if defined(DUAL_STREAM) && defined(BIDIR_STREAM)
	struct	uvc_stream	s[3];
#elif defined(DUAL_STREAM) || defined(BIDIR_STREAM)
	struct	uvc_stream	s[2];
#else
	struct	uvc_stream	s[1];
#endif

#ifdef UAC
	struct uac_g_dev g_uac_dev;
#endif

#ifdef UAVC_CLASS_REQ_SUPPORT
	#define CLASS_REQ_MAX_SIZE 32
	struct semaphore read_sem;
	struct semaphore class_req_sem;
	u32 class_req[CLASS_REQ_MAX_SIZE];
	bool flag_release;
#endif

};

static struct usb_device_descriptor
device_desc = {
	.bLength =		sizeof device_desc,
	.bDescriptorType =	USB_DT_DEVICE,
	.bcdUSB =		__constant_cpu_to_le16 (0x0200),
	.bDeviceClass =		USB_CLASS_MISC,
	.bDeviceSubClass =	USB_MISC_SC_COMMON,
	.bDeviceProtocol =	USB_PROTOCOL_ASSOC_DESC,
	.idVendor =		__constant_cpu_to_le16 (DRIVER_VENDOR_NUM),
	.idProduct =		__constant_cpu_to_le16 (DRIVER_PRODUCT_NUM),
	.bcdDevice = 		__constant_cpu_to_le16 (0x0100),
	.iManufacturer =	STRING_MANUFACTURER,
	.iProduct =		STRING_PRODUCT,
	.iSerialNumber =	STRING_SERIAL,
	.bNumConfigurations =	UVC_DEVICE_NR_CONFIG,
};

static struct usb_config_descriptor
config_desc = {
	.bLength =		sizeof config_desc,
	.bDescriptorType =	USB_DT_CONFIG,
	.bNumInterfaces =	UVC_DEVICE_NR_INTF,
	.bConfigurationValue =	1,
	.bmAttributes =		USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower =		0xFA, 
};

/* interface association descriptor */
static struct usb_interface_assoc_descriptor
interface_assoc_descriptor = {
	.bLength =		sizeof interface_assoc_descriptor,
	.bDescriptorType =	USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface =	0x0,
#ifdef UAC
	.bInterfaceCount =	UVC_DEVICE_NR_INTF - 2,
#else
	.bInterfaceCount =	UVC_DEVICE_NR_INTF,
#endif
	.bFunctionClass =	USB_CLASS_VIDEO,
	.bFunctionSubClass =	USB_SC_INTERFACE_COLLECTION,
	.bFunctionProtocol =	USB_PROTOCOL_UNDEFINED,
	.iFunction =	STRING_PRODUCT,
};

static struct usb_interface_descriptor
interface_descriptor_0_0 = {
	.bLength =		sizeof interface_descriptor_0_0,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bInterfaceNumber =	VC_INTERFACE_NUMBER,
	.bAlternateSetting =	0x0,
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_VIDEO,
	.bInterfaceSubClass =	USB_SC_VIDEOCONTROL,
	.bInterfaceProtocol =	USB_PROTOCOL_UNDEFINED,
	.iInterface = STRING_PRODUCT,
};

static struct usb_vc_header_descriptor
vc_header_descriptor = {
	.bLength = sizeof vc_header_descriptor,
	.bDescriptorType = USB_CS_INTERFACE,
	.bDescriptorSubType = USB_VC_HEADER, 

	.bcdUVC = VC_HEADER_bcdUVC,
	.dwClockFrequency = VC_HEADER_CLK_FREQ,
	.bInCollection = VC_HEADER_NR_VS,
};

static struct usb_cam_terminal_descriptor 
cam_terminal_descriptor = {
        .bLength = sizeof cam_terminal_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VC_INPUT_TERMINAL,
        .bTerminalID = VC_CAM_TERM_TID,
        .wTerminalType = USB_ITT_CAMERA,
        .bAssocTerminal = VC_CAM_TERM_ASSOC_TERM,
        .iTerminal = VC_CAM_TERM_TERMINAL,
        .wObjectFocalLengthMin = VC_CAM_TERM_OBJ_FOCAL_LEN_MIN,
        .wObjectFocalLengthMax = VC_CAM_TERM_OBJ_FOCAL_LEN_MAX,
        .wOcularFocalLength = VC_CAM_TERM_OCL_FOCAL_LEN,
        .bControlSize = VC_CAM_TERM_CONTROL_SIZE,
};

static struct usb_output_terminal_descriptor
output_terminal_descriptor = {
        .bLength = sizeof output_terminal_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VC_OUTPUT_TERMINAL,
        .bTerminalID = VC_OUTPUT_TERM_TID,
        .wTerminalType = USB_TT_STREAMING,
        .bAssocTerminal = VC_OUTPUT_TERM_ASSOC_TERM,
        .bSourceID = VC_OUTPUT_TERM_SOURCE_ID,
        .iTerminal = VC_OUTPUT_TERM_TERMINAL,
};

static struct usb_selector_unit_descriptor
selector_unit_descriptor = {
        .bLength = sizeof selector_unit_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VC_SELECTOR_UNIT,
	.bUnitID = VC_SEL_UNIT_UID,
	.bNrInPins = VC_SEL_UNIT_NR_IN_PINS,
	.iSelector = VC_SEL_UNIT_SELECTOR,
};

static struct usb_processing_unit_descriptor
processing_unit_descriptor = {
        .bLength = sizeof processing_unit_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VC_PROCESSING_UNIT,
        .bUnitID = VC_PRO_UNIT_UID,
        .bSourceID = VC_PRO_UNIT_SOURCE_ID,
        .wMaxMultiplier = VC_PRO_UNIT_MAX_MULT,
        .bControlSize = VC_PRO_UNIT_CONTROL_SIZE, 
        .iProcessing = VC_PRO_UNIT_PROCESSING,
};

static struct usb_extension_unit_desciptor
extension_unit_desciptor = {
        .bLength = sizeof extension_unit_desciptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VC_EXTERNSION_UNIT,
        .bUnitID = VC_EXT_UNIT_UID,
	.bNumControls = VC_EXT_UNIT_NR_CONTROLS,
	.bNrInPins = VC_EXT_UNIT_NR_IN_PINS,
        .bControlSize = VC_EXT_UNIT_CONTROL_SIZE,
        .iExtension = VC_EXT_UNIT_EXTENSION,
};

static struct usb_endpoint_descriptor
int_in_ep_descriptor = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize = INT_IN_PACKET_SIZE,
	.bInterval = INT_IN_INTERVAL,
};

static struct usb_vc_interrupt_endpoint_descriptor
vc_int_in_ep_descriptor = {
        .bLength = sizeof vc_int_in_ep_descriptor,
        .bDescriptorType = USB_CS_ENDPOINT,
        .bDescriptorSubType = USB_EP_INTERRUPT,
        .wMaxTransferSize = VC_INT_EP_MAX_TRF_SIZE,
};

static struct usb_interface_descriptor
interface_descriptor_1_0 = {
	.bLength =		sizeof interface_descriptor_1_0,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bInterfaceNumber =	VS_ISOC_IN_INTERFACE,
	.bAlternateSetting =	0x0,
	.bNumEndpoints =	0x0,
	.bInterfaceClass =	USB_CLASS_VIDEO,
	.bInterfaceSubClass =	USB_SC_VIDEOSTREAMING,
	.bInterfaceProtocol =	USB_PROTOCOL_UNDEFINED,
	.iInterface = 0x0,
};

static struct usb_vs_input_header_descriptor
input_header_descriptor = {
        .bLength = sizeof input_header_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VS_INPUT_HEADER,
        .bNumFormats = INPUT_HEADER_NR_FORMART,
        .bEndpointAddress = USB_DIR_IN,
        .bmInfo = INPUT_HEADER_BM_INFO,
        .bTerminalLink = INPUT_HEADER_TERM_LINK,
        .bStillCaptureMethod = INPUT_HEADER_STILL_CAP_METHOD,
        .bTriggerSupport = INPUT_HEADER_TRIGGER_SUPPORT,
        .bTriggerUsage = INPUT_HEADER_TRIGGER_USAGE,
        .bControlSize = INPUT_HEADER_CONTROL_SIZE,
};

static struct usb_vs_format_mjpeg_descriptor
format_mjpeg_descriptor = {
        .bLength = sizeof format_mjpeg_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VS_FORMAT_MJPEG,
        .bFormatIndex = MJPEG_FORMAT_INDEX,
        .bNumFrameDescriptors = VS_F_NR_DESC,
        .bmFlags = VS_F_BM_FLAGS,
        .bDefaultFrameIndex = VS_F_DEF_F_INDEX,
        .bAspectRatioX = VS_F_ASPECT_RATIO_X,
        .bAspectRatioY = VS_F_ASPECT_RATIO_Y,
        .bmInterfaceFlags = VS_F_INTF_FLAGS,
        .bCopyProtect = VS_F_COPY_PROTECT,
};

static struct usb_vs_frame_mjpeg_descriptor
frame_mjpeg_descriptor[VS_F_NR_DESC];

static struct usb_vs_format_mpeg2ts_descriptor
format_mpeg2ts_descriptor = {
	.bLength = 7,
	.bDescriptorType = USB_CS_INTERFACE,
	.bDescriptorSubType = USB_VS_FORMAT_MPEG2TS,

	.bFormatIndex = MPEG2TS_FORMAT_INDEX,
	.bDataOffset = 0,
	.bPacketLength = 188,
	.bStrideLength = 188,
	.guidStrideFormat[0] = 0,
	.guidStrideFormat[1] = 0,
	.guidStrideFormat[2] = 0,
	.guidStrideFormat[3] = 0,
};


#if UVC_YUV
static const struct usb_vs_format_uncompressed_desc format_yuv_descriptor = {
	.bLength		= sizeof format_yuv_descriptor,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= USB_VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex		= 1,
	.bNumFrameDescriptors	= 1,
	.guidFormat		=
		{ 'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00,
		 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
	.bBitsPerPixel		= 16,
	.bDefaultFrameIndex	= 1,
	.bAspectRatioX		= 0,
	.bAspectRatioY		= 0,
	.bmInterfaceFlags	= 0,
	.bCopyProtect		= 0,
};

#if 0
static const struct usb_vs_frame_uncompressed_desc frame_yuv_720p_descriptor = {
	.bLength		= sizeof frame_yuv_720p_descriptor,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= USB_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(1280),
	.wHeight		= cpu_to_le16(720),
	.dwMinBitRate		= cpu_to_le32(29491200),
	.dwMaxBitRate		= cpu_to_le32(29491200),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(1843200),
	.dwDefaultFrameInterval	= cpu_to_le32(5000000),
	.bFrameIntervalType	= 1,
	.dwFrameInterval0	= cpu_to_le32(5000000),
};
#else

/* This is actually 360p */ 
static const struct usb_vs_frame_uncompressed_desc frame_yuv_720p_descriptor = {
	.bLength		= sizeof frame_yuv_720p_descriptor,
	.bDescriptorType	= USB_DT_CS_INTERFACE,
	.bDescriptorSubType	= USB_VS_FRAME_UNCOMPRESSED,
	.bFrameIndex		= 1,
	.bmCapabilities		= 0,
	.wWidth			= cpu_to_le16(640),
	.wHeight		= cpu_to_le16(360),
	.dwMinBitRate		= cpu_to_le32(18432000),
	.dwMaxBitRate		= cpu_to_le32(55296000),
	.dwMaxVideoFrameBufferSize	= cpu_to_le32(460800),
	.dwDefaultFrameInterval	= cpu_to_le32(666666),
	.bFrameIntervalType	= 3,
	.dwFrameInterval0	= cpu_to_le32(666666),
	.dwFrameInterval1	= cpu_to_le32(1000000),
	.dwFrameInterval2	= cpu_to_le32(5000000),
};
#endif
#endif


static struct usb_color_matching_descriptor
color_matching_descriptor = {
        .bLength = sizeof color_matching_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VS_COLORFORMAT,
        .bColorPrimaries = VS_COLOR_PRIMARIES,
        .bTransferCharacteristics = VS_COLOR_XFER_CHARS,
        .bMatrixCoefficients = VS_COLOR_MATRIX_COEFF,
};

static struct usb_interface_descriptor
interface_descriptor_1_1 = {
        .bLength =              sizeof interface_descriptor_1_1,
        .bDescriptorType =      USB_DT_INTERFACE,

        .bInterfaceNumber =     VS_ISOC_IN_INTERFACE,
        .bAlternateSetting =    0x1,
        .bNumEndpoints =        0x1,
        .bInterfaceClass =      USB_CLASS_VIDEO,
        .bInterfaceSubClass =   USB_SC_VIDEOSTREAMING,
        .bInterfaceProtocol =   USB_PROTOCOL_UNDEFINED,
        .iInterface = 0x0,
};

static struct usb_endpoint_descriptor
isoc_in_ep_descriptor_1_1 = {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = USB_DIR_IN,
        .bmAttributes = USB_ENDPOINT_XFER_ISOC,
        .wMaxPacketSize = ISOC_IN_PKT_SIZE, 
        .bInterval = ISOC_IN_INTERVAL,
};

#ifdef DUAL_STREAM

static struct usb_output_terminal_descriptor
ds_output_terminal_descriptor = {
        .bLength = sizeof ds_output_terminal_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VC_OUTPUT_TERMINAL,
        .bTerminalID = VC_DS_OUTPUT_TERM_ID,
        .wTerminalType = USB_TT_STREAMING,
        .bAssocTerminal = VC_OUTPUT_TERM_ASSOC_TERM,
        .bSourceID = VC_OUTPUT_TERM_SOURCE_ID,
        .iTerminal = VC_OUTPUT_TERM_TERMINAL,
};
static struct usb_interface_descriptor
ds_interface_descriptor_as_0 = {
	.bLength =		sizeof ds_interface_descriptor_as_0,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bInterfaceNumber =	VS_ISOC_IN_INTERFACE2,
	.bAlternateSetting =	0x0,
	.bNumEndpoints =	0x0,
	.bInterfaceClass =	USB_CLASS_VIDEO,
	.bInterfaceSubClass =	USB_SC_VIDEOSTREAMING,
	.bInterfaceProtocol =	USB_PROTOCOL_UNDEFINED,
	.iInterface = 0x0,
};

static struct usb_vs_input_header_descriptor
ds_input_header_descriptor = {
	.bLength = sizeof ds_input_header_descriptor,
	.bDescriptorType = USB_CS_INTERFACE,
	.bDescriptorSubType = USB_VS_INPUT_HEADER,
	.bNumFormats = INPUT_HEADER_NR_FORMART,
	.bEndpointAddress = USB_DIR_IN,
	.bmInfo = INPUT_HEADER_BM_INFO,
	.bTerminalLink = VC_DS_OUTPUT_TERM_ID,
	.bStillCaptureMethod = INPUT_HEADER_STILL_CAP_METHOD,
	.bTriggerSupport = INPUT_HEADER_TRIGGER_SUPPORT,
	.bTriggerUsage = INPUT_HEADER_TRIGGER_USAGE,
	.bControlSize = INPUT_HEADER_CONTROL_SIZE,
};

static struct usb_vs_format_mjpeg_descriptor
ds_format_mjpeg_descriptor = {
	.bLength = sizeof ds_format_mjpeg_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VS_FORMAT_MJPEG,
        .bFormatIndex = MJPEG_FORMAT_INDEX,
        .bNumFrameDescriptors = VS_F_NR_DESC,
        .bmFlags = VS_F_BM_FLAGS,
        .bDefaultFrameIndex = VS_F_DEF_F_INDEX,
        .bAspectRatioX = VS_F_ASPECT_RATIO_X,
        .bAspectRatioY = VS_F_ASPECT_RATIO_Y,
        .bmInterfaceFlags = VS_F_INTF_FLAGS,
        .bCopyProtect = VS_F_COPY_PROTECT,
};

static struct usb_vs_frame_mjpeg_descriptor
ds_frame_mjpeg_descriptor[VS_F_NR_DESC];

static struct usb_vs_format_mpeg2ts_descriptor
ds_format_mpeg2ts_descriptor = {
	.bLength = 7,
	.bDescriptorType = USB_CS_INTERFACE,
	.bDescriptorSubType = USB_VS_FORMAT_MPEG2TS,

	.bFormatIndex = MPEG2TS_FORMAT_INDEX,
	.bDataOffset = 0,
	.bPacketLength = 188,
	.bStrideLength = 188,
	.guidStrideFormat[0] = 0,
	.guidStrideFormat[1] = 0,
	.guidStrideFormat[2] = 0,
	.guidStrideFormat[3] = 0,
};

static struct usb_color_matching_descriptor
ds_color_matching_descriptor = {
        .bLength = sizeof ds_color_matching_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VS_COLORFORMAT,
        .bColorPrimaries = VS_COLOR_PRIMARIES,
        .bTransferCharacteristics = VS_COLOR_XFER_CHARS,
        .bMatrixCoefficients = VS_COLOR_MATRIX_COEFF,
};

static struct usb_interface_descriptor
ds_interface_descriptor_as_1 = {
        .bLength =              sizeof ds_interface_descriptor_as_1,
        .bDescriptorType =      USB_DT_INTERFACE,

        .bInterfaceNumber =     VS_ISOC_IN_INTERFACE2,
        .bAlternateSetting =    0x1,
        .bNumEndpoints =        0x1,
        .bInterfaceClass =      USB_CLASS_VIDEO,
        .bInterfaceSubClass =   USB_SC_VIDEOSTREAMING,
        .bInterfaceProtocol =   USB_PROTOCOL_UNDEFINED,
        .iInterface = 0x0,
};

static struct usb_endpoint_descriptor
ds_isoc_in_ep_descriptor = {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = USB_DIR_IN,
        .bmAttributes = USB_ENDPOINT_XFER_ISOC,
        .wMaxPacketSize = ISOC_IN_PKT_SIZE, 
        .bInterval = ISOC_IN_INTERVAL,
};
#endif

#ifdef BIDIR_STREAM
static struct usb_input_terminal_descriptor
input_terminal_descriptor = {
        .bLength = sizeof input_terminal_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VC_INPUT_TERMINAL,
        .bTerminalID = VC_INPUT_TERM_TID,
        .wTerminalType = USB_TT_STREAMING,
        .bAssocTerminal = 0,
        .iTerminal = 0,
};

static struct usb_output_terminal_descriptor
lcd_terminal_descriptor = {
        .bLength = sizeof lcd_terminal_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VC_OUTPUT_TERMINAL,
        .bTerminalID = VC_BDS_OUTPUT_TERM_TID,
        .wTerminalType = USB_OTT_DISPLAY,
        .bAssocTerminal = 0,
        .bSourceID = VC_INPUT_TERM_TID,
        .iTerminal = 0,
};

static struct usb_interface_descriptor
bds_interface_descriptor_as_0 = {
	.bLength =		sizeof bds_interface_descriptor_as_0,
	.bDescriptorType =	USB_DT_INTERFACE,

	.bInterfaceNumber =	VS_ISOC_OUT_INTERFACE,
	.bAlternateSetting =	0x0,
	.bNumEndpoints =	0x0,
	.bInterfaceClass =	USB_CLASS_VIDEO,
	.bInterfaceSubClass =	USB_SC_VIDEOSTREAMING,
	.bInterfaceProtocol =	USB_PROTOCOL_UNDEFINED,
	.iInterface = 0x0,
};

static struct usb_vs_output_header_descriptor
bds_output_header_descriptor = {
	.bLength = 8,
	.bDescriptorType = USB_CS_INTERFACE,
	.bDescriptorSubType = USB_VS_OUTPUT_HEADER,
	.bNumFormats = 1,
	.bEndpointAddress = USB_DIR_OUT,
	.bTerminalLink = VC_INPUT_TERM_TID,
};
static struct usb_vs_format_mjpeg_descriptor
bds_format_mjpeg_descriptor = {
	.bLength = sizeof bds_format_mjpeg_descriptor,
        .bDescriptorType = USB_CS_INTERFACE,
        .bDescriptorSubType = USB_VS_FORMAT_MJPEG,
        .bFormatIndex = 1,
        .bNumFrameDescriptors = 1,
        .bmFlags = VS_F_BM_FLAGS,
        .bDefaultFrameIndex = 1,
        .bAspectRatioX = VS_F_ASPECT_RATIO_X,
        .bAspectRatioY = VS_F_ASPECT_RATIO_Y,
        .bmInterfaceFlags = VS_F_INTF_FLAGS,
        .bCopyProtect = VS_F_COPY_PROTECT,
};

static struct usb_vs_frame_mjpeg_descriptor
bds_frame_mjpeg_descriptor[1];

static struct usb_interface_descriptor
bds_interface_descriptor_as_1 = {
        .bLength =              sizeof bds_interface_descriptor_as_1,
        .bDescriptorType =      USB_DT_INTERFACE,

        .bInterfaceNumber =     VS_ISOC_OUT_INTERFACE,
        .bAlternateSetting =    0x1,
        .bNumEndpoints =        0x1,
        .bInterfaceClass =      USB_CLASS_VIDEO,
        .bInterfaceSubClass =   USB_SC_VIDEOSTREAMING,
        .bInterfaceProtocol =   USB_PROTOCOL_UNDEFINED,
        .iInterface = 0x0,
};


static struct usb_endpoint_descriptor
bds_isoc_out_ep_descriptor = {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = USB_DIR_OUT,
        .bmAttributes = USB_ENDPOINT_XFER_ISOC,
        .wMaxPacketSize = cpu_to_le16(ISOC_IN_PKT_SIZE), 
        .bInterval = ISOC_IN_INTERVAL,
};
#endif

/*
 * USB 2.0 devices need to expose both high speed and full speed
 * descriptors, unless they only run at full speed.
 *
 * That means alternate endpoint descriptors (bigger packets)
 * and a "device qualifier" ... plus more construction options
 * for the config descriptor.
 */
static struct usb_qualifier_descriptor
dev_qualifier = {
	.bLength =		sizeof dev_qualifier,
	.bDescriptorType =	USB_DT_DEVICE_QUALIFIER,
	.bcdUSB =		__constant_cpu_to_le16(0x0200),
	.bDeviceClass =		USB_CLASS_PER_INTERFACE,
	.bNumConfigurations =	1,
};

static const struct usb_descriptor_header *media_function[] = {
	(struct usb_descriptor_header *)&interface_assoc_descriptor,
	(struct usb_descriptor_header *)&interface_descriptor_0_0,
	(struct usb_descriptor_header *)&vc_header_descriptor,
	(struct usb_descriptor_header *)&cam_terminal_descriptor,
	(struct usb_descriptor_header *)&output_terminal_descriptor,
	(struct usb_descriptor_header *)&selector_unit_descriptor,
	(struct usb_descriptor_header *)&processing_unit_descriptor,
	(struct usb_descriptor_header *)&extension_unit_desciptor,

#ifdef DUAL_STREAM
	(struct usb_descriptor_header *)&ds_output_terminal_descriptor,
#endif

#ifdef BIDIR_STREAM
	(struct usb_descriptor_header *)&input_terminal_descriptor,
	(struct usb_descriptor_header *)&lcd_terminal_descriptor,
#endif

	(struct usb_descriptor_header *)&int_in_ep_descriptor,
	(struct usb_descriptor_header *)&vc_int_in_ep_descriptor,
	(struct usb_descriptor_header *)&interface_descriptor_1_0,
	(struct usb_descriptor_header *)&input_header_descriptor,

#if UVC_YUV
	(struct usb_descriptor_header *)&format_yuv_descriptor,
	(struct usb_descriptor_header *)&frame_yuv_720p_descriptor,
#else
	(struct usb_descriptor_header *)&format_mjpeg_descriptor,
	(struct usb_descriptor_header *)&frame_mjpeg_descriptor[0],
	(struct usb_descriptor_header *)&frame_mjpeg_descriptor[1],
	(struct usb_descriptor_header *)&frame_mjpeg_descriptor[2],
	(struct usb_descriptor_header *)&frame_mjpeg_descriptor[3],
#endif
//(struct usb_descriptor_header *)&format_mpeg2ts_descriptor,
	//(struct usb_descriptor_header *)&color_matching_descriptor,
	(struct usb_descriptor_header *)&interface_descriptor_1_1,
	(struct usb_descriptor_header *)&isoc_in_ep_descriptor_1_1,

#ifdef DUAL_STREAM
        (struct usb_descriptor_header *)&ds_interface_descriptor_as_0,
        (struct usb_descriptor_header *)&ds_input_header_descriptor,
	#if 0
        (struct usb_descriptor_header *)&ds_format_mjpeg_descriptor,
        (struct usb_descriptor_header *)&ds_frame_mjpeg_descriptor[0],
        (struct usb_descriptor_header *)&ds_frame_mjpeg_descriptor[1],
        (struct usb_descriptor_header *)&ds_frame_mjpeg_descriptor[2],
        (struct usb_descriptor_header *)&ds_frame_mjpeg_descriptor[3],
	#endif
        (struct usb_descriptor_header *)&ds_format_mpeg2ts_descriptor,
        //(struct usb_descriptor_header *)&ds_color_matching_descriptor,
        (struct usb_descriptor_header *)&ds_interface_descriptor_as_1,
        (struct usb_descriptor_header *)&ds_isoc_in_ep_descriptor,
#endif

#ifdef BIDIR_STREAM
        (struct usb_descriptor_header *)&bds_interface_descriptor_as_0,
        (struct usb_descriptor_header *)&bds_output_header_descriptor,
        (struct usb_descriptor_header *)&bds_format_mjpeg_descriptor,
        (struct usb_descriptor_header *)&bds_frame_mjpeg_descriptor[0],
        (struct usb_descriptor_header *)&bds_interface_descriptor_as_1,
        (struct usb_descriptor_header *)&bds_isoc_out_ep_descriptor,
#endif

#ifdef UAC
	(struct usb_descriptor_header *)&uac_aic_iad,
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
#endif
	NULL,
};

/* Static strings, in UTF-8 (for simplicity we use only ASCII characters) */
static struct usb_string		strings[] = {
	{STRING_MANUFACTURER,	manufacturer},
	{STRING_PRODUCT,	longname},
	{STRING_SERIAL,		serial},
	{}
};

static struct usb_gadget_strings	stringtab = {
	.language	= 0x0409,		// en-us
	.strings	= strings,
};

static void init_streaming_control(struct media_g_dev *media, u8 sn)
{
	/* init default value */
        media->s[sn].uvc_prob_comm_ds.bmHint = 0x0000; /* set by host and RO for device */
        media->s[sn].uvc_prob_comm_ds.bFormatIndex = 0x01; /* set by host */
        media->s[sn].uvc_prob_comm_ds.bFrameIndex = 0x01; /* set by host */
        media->s[sn].uvc_prob_comm_ds.dwFrameInterval = 0x00051615;
        media->s[sn].uvc_prob_comm_ds.wKeyFrameRate = 0x0000;
        media->s[sn].uvc_prob_comm_ds.wPFrameRate = 0x0000;
        media->s[sn].uvc_prob_comm_ds.wCompQuality = 0x0000;
        media->s[sn].uvc_prob_comm_ds.wCompWindowSize = 0x0000;
        media->s[sn].uvc_prob_comm_ds.wDelay = 0x0000; /* set by device and RO for host */
        media->s[sn].uvc_prob_comm_ds.dwMaxVideoFrameSize = 0x80000; /* set by device and RO for host */
        /* REVISIT: this should be 3K for high bandwidth isoc trasnfer */
        media->s[sn].uvc_prob_comm_ds.dwMaxPayloadTransferSize = ISOC_IN_PKT_SIZE; /* set by device and RO for host */
#ifdef UVC_1_1
        media->s[sn].uvc_prob_comm_ds.dwClockFrequency = 0x1017DF80; /* set by device and RO for host */
        media->s[sn].uvc_prob_comm_ds.bmFramingInfo = 0x80;
        media->s[sn].uvc_prob_comm_ds.bPreferedVersion = 0x01;
        media->s[sn].uvc_prob_comm_ds.bMinVersion = 0x01;
        media->s[sn].uvc_prob_comm_ds.bMaxVersion = 0x01;
#endif

	/* init min value */
        media->s[sn].uvc_prob_comm_ds_min.bmHint = 0x0000;
        media->s[sn].uvc_prob_comm_ds_min.bFormatIndex = 0x01;
        media->s[sn].uvc_prob_comm_ds_min.bFrameIndex = 0x01;
        //.dwFrameInterval = 0x000A2C2A;
        media->s[sn].uvc_prob_comm_ds_min.dwFrameInterval = 0x0001B207;
        media->s[sn].uvc_prob_comm_ds_min.wKeyFrameRate = 0x0000;
        media->s[sn].uvc_prob_comm_ds_min.wPFrameRate = 0x0000;
        media->s[sn].uvc_prob_comm_ds_min.wCompQuality = 0x0000;
        media->s[sn].uvc_prob_comm_ds_min.wCompWindowSize = 0x0000;
        media->s[sn].uvc_prob_comm_ds_min.wDelay = 0x0000;
        media->s[sn].uvc_prob_comm_ds_min.dwMaxVideoFrameSize = 0x400;
        media->s[sn].uvc_prob_comm_ds_min.dwMaxPayloadTransferSize = ISOC_IN_PKT_SIZE;
#ifdef UVC_1_1
        media->s[sn].uvc_prob_comm_ds_min.dwClockFrequency = 0x1017DF80;
        media->s[sn].uvc_prob_comm_ds_min.bmFramingInfo = 0x80;
        media->s[sn].uvc_prob_comm_ds_min.bPreferedVersion = 0x01;
        media->s[sn].uvc_prob_comm_ds_min.bMinVersion = 0x01;
        media->s[sn].uvc_prob_comm_ds_min.bMaxVersion = 0x01;
#endif

	/* init max value */
        media->s[sn].uvc_prob_comm_ds_max.bmHint = 0x0000;
        media->s[sn].uvc_prob_comm_ds_max.bFormatIndex = 0x01;
        media->s[sn].uvc_prob_comm_ds_max.bFrameIndex = 0x03;
        //.dwFrameInterval = 0x000A2C2A;
        media->s[sn].uvc_prob_comm_ds_max.dwFrameInterval = 0x000A2C2A;
        media->s[sn].uvc_prob_comm_ds_max.wKeyFrameRate = 0x0000;
        media->s[sn].uvc_prob_comm_ds_max.wPFrameRate = 0x0000;
        media->s[sn].uvc_prob_comm_ds_max.wCompQuality = 0x0000;
        media->s[sn].uvc_prob_comm_ds_max.wCompWindowSize = 0x0000;
        media->s[sn].uvc_prob_comm_ds_max.wDelay = 0x0000;
        media->s[sn].uvc_prob_comm_ds_max.dwMaxVideoFrameSize = 0x00300000;
        media->s[sn].uvc_prob_comm_ds_max.dwMaxPayloadTransferSize = ISOC_IN_PKT_SIZE;
#ifdef UVC_1_1
        media->s[sn].uvc_prob_comm_ds_max.dwClockFrequency = 0x00009600;
        media->s[sn].uvc_prob_comm_ds_max.bmFramingInfo = 0x80;
        media->s[sn].uvc_prob_comm_ds_max.bPreferedVersion = 0x01;
        media->s[sn].uvc_prob_comm_ds_max.bMinVersion = 0x01;
        media->s[sn].uvc_prob_comm_ds_max.bMaxVersion = 0x01;
#endif

}
static u8 get_stream_number(struct media_g_dev *media, struct usb_ep *ep)
{
	if (!strcmp(media->s[0].isoc_in->name, ep->name))
			return 0;
	else
			return 1;
}

static int populate_config_buf(struct usb_gadget *gadget,
		u8 *buf, u8 type, unsigned index)
{
	int					len;
	const struct usb_descriptor_header	**func;

	if (index > 0)
		return -EINVAL;

	func = media_function;

	len = usb_gadget_config_buf(&config_desc, buf, EP0_BUFSIZE, func);
	((struct usb_config_descriptor *) buf)->bDescriptorType = type;
	return len;
}

static int ep0_queue(struct media_g_dev *media)
{
	int	rc;

	rc = usb_ep_queue(media->ep0, media->ep0req, GFP_ATOMIC);
	if (rc != 0 && rc != -ESHUTDOWN) {
		media->ep0req->status = 0;
		/* We can't do much more than wait for a reset */
		printk("error in submission: %s --> %d\n",
				media->ep0->name, rc);
	}
	return rc;
}
#ifdef UAC
void audio_set_desc_data(void *desc, u8 *data, u8 nelem, u8 offs)
{
        int i;
        u8 *pDesc = (u8 *)desc;

        for(i=0; i < nelem; ++i)
                pDesc[offs+i] = data[i];
}
static struct usb_request *uac_g_alloc_req(struct usb_ep *ep, unsigned int len, int kmalloc_flags)
{
        struct usb_request *req;

        if (ep == NULL)
                return NULL;

        req = usb_ep_alloc_request(ep, kmalloc_flags);

        if (req != NULL) {
                req->length = len;
/*              req->buf = kmalloc(len, kmalloc_flags);
                if (req->buf == NULL) {
                        printk("error: unable to alloc req\n");
                        usb_ep_free_request(ep, req);
                        return NULL;
                }
*/
        }

        return req;
}

static void uac_g_free_req(struct usb_ep *ep, struct usb_request *req)
{
        if (ep != NULL && req != NULL) {
                //kfree(req->buf);
                usb_ep_free_request(ep, req);
        }
}
static void uac_tx_submit_complete(struct usb_ep *ep, struct usb_request *req)
{
//        struct uac_g_dev *dev = &gb_media->g_uac_dev;
		int status=0;
        struct media_g_dev	*media = ep->driver_data;
		struct uac_g_dev *dev = &media->g_uac_dev;

        unsigned long flags;

//		printk("uac_tx_submit_complete called case=%d ep=%s media=%p uac_dev=%p\n", req->status, ep->name,media,dev);
        spin_lock_irqsave(&dev->lock, flags);

        switch(req->status) {
        case 0:
req_queue:
                /* for simulate */
                req->buf = dev->aud_data;
                req->length = dev->bfLen;
                req->complete = uac_tx_submit_complete;
				dev->req_done1 = 0;
				if (req->length > 0) {
			       status = usb_ep_queue(dev->iso_in, req, GFP_ATOMIC);
				   if (status) {
 	  					printk("usb_ep_queue submit error BUF addr=%p len=%d \n", req->buf, req->length);
						//usb_ep_set_halt (dev->iso_in);
				   }
				} else {
					dev->req_done1 = 1; // used for calling uac_tx_submit based on new frame from app
			//		printk("uac_tx_submit_complete: req_done1 set \n");
					//free_ep_req(dev->iso_in, req);
				}
				

//				printk("uac_tx_submit_complete BUF addr=%p len=%d \n", req->buf, req->length);
                dev->eof_flag = 1;
                dev->req_done = 0;
				dev->bfLen  = 0; //clear this as transfer is submitted
                dev->len_remaining = req->actual;
                break;

        case -ESHUTDOWN:
                /* disconnect */
                printk("uac_g_complete: shutdown\n");
//LK                uac_g_free_req(ep, req);
                break;

        case -ECONNRESET:
                /* dequeued */
                printk("uac_g_complete: dequeued\n");
                break;

        default:
                /* unexpected */
                printk(KERN_ERR
                "uac_g_complete: unexpected status error, status=%d\n",
                        req->status);
                goto req_queue;
                break;


        }

        spin_unlock_irqrestore(&dev->lock, flags);
}

static void uac_do_write(struct uac_g_dev *dev, u8 * buf, int bflen)
{
        struct usb_request *req = dev->req;
        unsigned long flags;

        spin_lock_irqsave(&dev->lock, flags);
		printk("uac_do_write called buf = %p, len = %d\n", buf, bflen);
        req = uac_g_alloc_req(dev->iso_in, dev->bfLen, GFP_ATOMIC);
        if (req == NULL) {
                printk("req is not allocated!!\r\n");
        }
        req->buf = dev->aud_data;
        req->length = dev->bfLen;
        req->complete = uac_tx_submit_complete;
        usb_ep_queue(dev->iso_in, req, GFP_ATOMIC);
        dev->eof_flag = 1;
        dev->req_done = 0;
//		printk("uac_do_write exit eof_flag = %d\n", dev->eof_flag);
        spin_unlock_irqrestore(&dev->lock, flags);
}

static int
uac_class_setup_req(struct uac_g_dev *dev, const struct usb_ctrlrequest *ctrl)
{
        struct usb_request *req = dev->ep0req;
        int value = -EOPNOTSUPP;
        int bControl,bChannel,bUintId,bIntfId,i,j;
        u16 wIndex = le16_to_cpu(ctrl->wIndex);
        u16 wValue = le16_to_cpu(ctrl->wValue);
        u16 wLength = le16_to_cpu(ctrl->wLength);
        u8 *pbuf;

        dev->cmd = 0;
        dev->ep0req->context = NULL;

        bControl = (u8)(wValue >> 8);
        bChannel = (u8)(wValue & 0xff);
        bUintId = (u8)(wIndex >> 8);
        bIntfId = (u8)(wIndex & 0xff);

        printk("\n\nbControl=%x,bChannel=%x,bUintId=%x,bIntfId=%x\n",bControl,bChannel,bUintId,bIntfId);
        /* UAC Specific Requests */
        switch (ctrl->bRequest) {

        case RC_UNDEFINED:
                printk("RC_UNDEFINED:Type:%x wVal:%x Idx:%x\r\n",
                       ctrl->bRequestType, wValue, wIndex);
                break;

   /* UAC Specific Requests */
        case GET_MAX:
        case GET_CUR:
        case GET_MIN:
        case GET_RES:
                printk("GET_MIN,MAX,RES,CUR:Type:%x wVal:%x Idx:%x\r\n", ctrl->bRequestType,
                       wValue, wIndex);

           switch( bControl )
           {
                case AC_FU_MUTE_CONTROL :
                if( ctrl->bRequest == GET_CUR )
                {
                        value = min(wLength, (u16) 2);
                       for(i=0; i<MAX_FU_UNITS; ++i)
                       {
                                if( bChannel == 0xFF ){
                                        pbuf = req->buf;
                                        pbuf[i] = dev->fu_ctrl[i].mute;
//                                             *(u8 *)&req->buf[i] = dev->fu_ctrl[i].mute;
                                }else{
                                       if( dev->fu_ctrl[i].bUnitId == bUintId )
                                       {
                                                pbuf = req->buf;
                                                pbuf[0] = dev->fu_ctrl[i].mute;
//                                              *(u8 *)&req->buf[0] = dev->fu_ctrl[i].mute;
                                                break;
                                       }
                                }
                        }
                 }
                 break;
               case AC_FU_VOLUME_CONTROL :
                for(i=0,j=0; i<MAX_FU_UNITS; ++i)
                {
                        if( bChannel != 0xFF && dev->fu_ctrl[i].bUnitId != bUintId )
                                continue;
                        value = min(wLength, (u16) 2);
                        switch( ctrl->bRequest )
                        {
                        case GET_CUR :
                             pbuf = req->buf;
                             *(u16 *)&pbuf[2*j] = dev->fu_ctrl[i].vol_cur;
                             //*(u16 *)&req->buf[2*j] = dev->fu_ctrl[i].vol_cur;
                             j++;
                             break;
                        case GET_MAX :
                             pbuf = req->buf;
                             *(u16 *)&pbuf[2*j] = dev->fu_ctrl[i].vol_max;
                             //*(u16 *)&req->buf[2*j] = dev->fu_ctrl[i].vol_max;
                             j++;
                             break;
                        case GET_MIN :
                             pbuf = req->buf;
                             *(u16 *)&pbuf[2*j] = dev->fu_ctrl[i].vol_min;
                             //*(u16 *)&req->buf[2*j] = dev->fu_ctrl[i].vol_min;
                             j++;
                             break;
                        case GET_RES :
                             pbuf = req->buf;
                             *(u16 *)&pbuf[2*j] = dev->fu_ctrl[i].vol_res;
                             //*(u16 *)&req->buf[2*j] = dev->fu_ctrl[i].vol_res;
                             j++;
                             break;
                        }
                        if( bChannel != 0xFF)
                              break;
                }
                break;

                default :
                        break;
        }
        break;
        case SET_MIN:           /* ! Set MIN,MAX and RES not supported */
        case SET_MAX:
        case SET_RES:
        case SET_CUR:
                printk("SET_MIN,MAX,RES,CUR:Type:%x wVal:%x Idx:%x\r\n", ctrl->bRequestType,
                       wValue, wIndex);
                value = wLength;
                dev->cmd = ctrl->bRequest;
                dev->wIndex = wIndex;
                dev->wValue = wValue;
                dev->wLength = wLength;
                dev->ep0req->context = (void *)dev;
                break;
        default:
      printk("Unknown Ctrl Request%02x.%02x v%04x i%04x l%d\r\n",
             ctrl->bRequestType, ctrl->bRequest, wValue, wIndex,
             wLength);

      break;
       }

        return value;
}
static void uac_g_start_stream(struct uac_g_dev *uac_g)
{
        unsigned long flags;

        spin_lock_irqsave(&uac_g->lock, flags);

        printk("starting the stream\n");
		uac_g->strm_started1 = 1;
        /* isoc-in enqueue*/
        usb_ep_enable(uac_g->iso_in, &as_iso_in_ep_desc0);

        spin_unlock_irqrestore(&uac_g->lock, flags);

}

static void alloc_uac_audio_buffer(struct uac_g_dev *g_uac_dev)
{
        int i;
        printk("alloc_uac_audio_buffer; dev=%p\n", g_uac_dev);

        g_uac_dev->uac_buffer[0].kvaddr = (unsigned int)kmalloc(2 * UAC_FRAME_LEN, GFP_KERNEL);
        g_uac_dev->uac_buffer[0].offset = virt_to_phys((void*)g_uac_dev->uac_buffer[0].kvaddr);

        printk("kmalloc: va=%x,pas=%x\n",g_uac_dev->uac_buffer[0].kvaddr,g_uac_dev->uac_buffer[0].offset);
        g_uac_dev->uac_buffer[1].kvaddr = g_uac_dev->uac_buffer[0].kvaddr + UAC_FRAME_LEN;
        g_uac_dev->uac_buffer[1].offset = g_uac_dev->uac_buffer[0].offset + UAC_FRAME_LEN;

        g_uac_dev->aud_buf[0] = (void *)g_uac_dev->uac_buffer[0].kvaddr;
        g_uac_dev->aud_buf[1] = (void *)g_uac_dev->uac_buffer[1].kvaddr;
        g_uac_dev->aud_data  =  g_uac_dev->aud_buf[0];
        g_uac_dev->bfLen = 0;
		g_uac_dev->req_done1 = 0;
		g_uac_dev->strm_started1 = 0;

        for (i=0 ; i <UAC_NUM_BUFS ;i++) {
                g_uac_dev->uac_buffer[i].size = UAC_FRAME_LEN;
                g_uac_dev->uac_buffer[i].index = i;
        }
}


#endif



static void free_all_req(struct media_g_dev *media)
{
}

#if 0
static void print_streaming_control(struct uvc_streaming_control *c)
{
	printk("\nbmHint = 0x%x\nbFormatIndex = %d\nbFrameIndex = %d \
		\ndwFrameInterval = 0x%x\nwKeyFrameRate = %d\nwPFrameRate = %d \
		\nwCompQuality = %d\nwCompWindowSize = %d\nwDelay = %d \
		\ndwMaxVideoFrameSize = 0x%x\ndwMaxPayloadTransferSize = %d",\
		c->bmHint,c->bFormatIndex,c->bFrameIndex,c->dwFrameInterval, \
		c->wKeyFrameRate,c->wPFrameRate,c->wCompQuality,c->wCompWindowSize, \
		c->wDelay,c->dwMaxVideoFrameSize,c->dwMaxPayloadTransferSize);
	
}
#endif

static int uvc_cmd_get_cur(struct media_g_dev *media, u16 wIndex, u16 wValue, u16 wLength) {

	struct usb_request *req = media->ep0req;
	int value = -EOPNOTSUPP;
	unsigned long flags;
	u8 bifIdx = (u8)(wIndex);
	u8 benIdx = (wIndex>>8);
	u8 bcsIdx = (wValue>>8);
	//printk("get_cur if:%x ent:%x cs:%x\n", bifIdx, benIdx, bcsIdx);
	switch (bifIdx) {
		case VC_INTERFACE_NUMBER:
			switch(benIdx) {
				case VC_INTERFACE_NUMBER:
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						case VC_REQUEST_ERROR_CODE_CONTROL:
							*(u8 *) req->buf = 0x06;	// Return 'Not Supported'
							value = 1;
							break;
						default:
							printk("get_cur inetrface 0 CS:%x\n", bcsIdx);
							break;
					}
				break;
				
				case VC_CAM_TERM_TID:
					switch (bcsIdx) {
						case CT_SCANNING_MODE_CONTROL:break;
						case CT_AE_MODE_CONTROL:
                     printk("-------uvc_cmd_get_cur():CT_AE_MODE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_auto.cur, 2);
							value = 1;
							break; 
						case CT_AE_PRIORITY_CONTROL: break;	
						case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
                     printk("-------uvc_cmd_get_cur():CT_EXPOSURE_TIME_ABSOLUTE_CONTROL-------\n");   	
							memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_absolute.cur,2);
							value = 2;
                        break;
						case CT_EXPOSURE_TIME_RELATIVE_CONTROL: break;						
						case CT_FOCUS_ABSOLUTE_CONTROL: break;						
						case CT_FOCUS_RELATIVE_CONTROL: break;						
						case CT_FOCUS_AUTO_CONTROL:	break;
						case CT_IRIS_ABSOLUTE_CONTROL: break;
						case CT_IRIS_RELATIVE_CONTROL: break;						
						case CT_ZOOM_ABSOLUTE_CONTROL: break;						
						case CT_ZOOM_RELATIVE_CONTROL: break;						
						case CT_PANTILT_ABSOLUTE_CONTROL: break;						
						case CT_PANTILT_RELATIVE_CONTROL: break;						
						case CT_ROLL_ABSOLUTE_CONTROL: break;						
						case CT_ROLL_RELATIVE_CONTROL: break;						
						case CT_PRIVACY_CONTROL: break;						
						default:
							break;
					}
					break;
	
				case VC_OUTPUT_TERM_TID:		
					switch (bcsIdx) {
						default:
							break;
					}
					break;
				
				case VC_SEL_UNIT_UID:		
					switch (bcsIdx) {
						case SU_CONTROL_UNDEFINED: break;
						case SU_INPUT_SELECT_CONTROL: break;	
						default:
							break;
					}
					break;
				
				case VC_PRO_UNIT_UID:
					switch (bcsIdx) {
						case PU_CONTROL_UNDEFINED: break;
						case PU_BACKLIGHT_COMPENSATION_CONTROL:
							printk("-------uvc_cmd_get_cur():PU_BACKLIGHT_COMPENSATION_CONTROL-------\n");   	
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.blc.cur, 2);
							value = 2;
							 break;  	
						case PU_BRIGHTNESS_CONTROL:
							printk("-------uvc_cmd_get_cur():PU_BRIGHTNESS_CONTROL-------\n");   	
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.brgt.cur, 2);
							value = 2;
							 break;           	
						case PU_CONTRAST_CONTROL:
							printk("-------uvc_cmd_get_cur():PU_CONTRAST_CONTROL-------\n");   	
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.cnst.cur, 2);
							value = 2;
							 break;             	
						case PU_GAIN_CONTROL: 
							printk("-------uvc_cmd_get_cur():PU_GAIN_CONTROL-------\n");   	
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gain.cur, 2);
							value = 2;
							 break;                 	
						case PU_POWER_LINE_FREQUENCY_CONTROL: 
                     printk("-------uvc_cmd_get_cur():PU_POWER_LINE_FREQUENCY_CONTROL-------\n");   
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.anti_flicker.cur, 2);
							value = 0;
							break;   		
						case PU_HUE_CONTROL:
							printk("-------uvc_cmd_get_cur():PU_HUE_CONTROL-------\n");   
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.hue.cur, 2);
							value = 2;
							break;                  	
						case PU_SATURATION_CONTROL: 
							printk("-------uvc_cmd_get_cur():PU_SATURATION_CONTROL-------\n");   
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.satr.cur, 2);
							value = 2;
							break;				
						case PU_SHARPNESS_CONTROL: break;				
						case PU_GAMMA_CONTROL: 
							printk("-------uvc_cmd_get_cur():PU_GAMMA_CONTROL-------\n");   
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gamma.cur, 2);
							value = 2;
							break;					
						case PU_WHITE_BALANCE_TEMPERATURE_CONTROL: 
                     printk("-------uvc_cmd_get_cur():PU_WB_TEMPLA_CONTROL-------\n");   
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt.cur, 2);
							value = 2;
							break;
						case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL: 
                     printk("-------uvc_cmd_get_cur():PU_WB_TEMPLA_AUTO_CONTROL-------\n");   
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt_auto.cur, 1);
							value = 0;
							break;
						case PU_WHITE_BALANCE_COMPONENT_CONTROL: break;		
						case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL: break;	
						case PU_DIGITAL_MULTIPLIER_CONTROL: break;			
						case PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL: break;		
						case PU_HUE_AUTO_CONTROL: break;						
						case PU_ANALOG_VIDEO_STANDARD_CONTROL: break;		
						case PU_ANALOG_LOCK_STATUS_CONTROL: break;	
				
						default:
							break;
					}
					break;

				case VC_EXT_UNIT_UID:		
					switch (bcsIdx) {
						case XU_CONTROL_UNDEFINED: break;
						default:
							break;
					}
					break;
				
				default:
					printk("get_cur on VCI, no entity specified\n");
					break;
			}
			break;

		case VS_ISOC_IN_INTERFACE:
#ifdef DUAL_STREAM
		case VS_ISOC_IN_INTERFACE2:
#endif
#ifdef BIDIR_STREAM
		case VS_ISOC_OUT_INTERFACE:
#endif
			switch (bcsIdx) { 
			case VS_CONTROL_UNDEFINED:
				break;
			case VS_STREAM_ERROR_CODE_CONTROL:
				*(u8 *) req->buf = 0x06;
				value = 1;
				break;
			case VS_PROBE_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_streaming_control));
				memcpy(req->buf, &media->s[bifIdx-1].uvc_prob_comm_ds, value);
				//print_streaming_control(&uvc_prob_comm_ds);
				break;
			case VS_COMMIT_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_streaming_control));
				spin_lock_irqsave(&media->lock, flags);
				memcpy(req->buf, &media->s[bifIdx - 1].cur_vs_state, value);
				spin_unlock_irqrestore(&media->lock, flags);
				break;
	#ifdef EN_STILL_CAP
			case VS_STILL_PROBE_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_stillimg_control));
				memcpy(req->buf, &uvc_prob_comm_ds_still, value);
				break;
			
			case VS_STILL_COMMIT_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_stillimg_control));
				//memcpy(req->buf, &media->if_stm_state, value);
				break;
		
			case VS_STILL_IMAGE_TRIGGER_CONTROL:
				//*(u8 *)req->buf = media->en_sti;
				value = 1;
				break;
	#endif
			default:
				printk("Req for VStreaming, value:%x\n", (wValue >> 8));
				break;
			}
			break;

		default:
			printk("GET_CUR for Idx:%x, value:%x\n", wIndex, (wValue >> 8));
			break;
		}
	return value;
}

int uvc_cmd_get_info(struct media_g_dev *media, u16 wIndex, u16 wValue, u16 wLength) {

	struct usb_request *req = media->ep0req;
	int value = -EOPNOTSUPP;
	u8 bifIdx = (u8)(wIndex);
	u8 benIdx = (wIndex>>8);
	u8 bcsIdx = (wValue>>8);
	
	switch (bifIdx) {
		case VC_INTERFACE_NUMBER:{
			switch(benIdx) {
				case VC_INTERFACE_NUMBER:
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						case VC_REQUEST_ERROR_CODE_CONTROL:
							printk("VC_REQUEST_ERROR_CODE_CONTROL\r\n");
							*(u8 *) req->buf = 0x06;
							value = 1;
							break;
						default:
							printk("get_cur inetrface 0 CS:%x\n", bcsIdx);
							break;
					}
					break;
					
				case VC_CAM_TERM_TID:
					switch (bcsIdx) {
						  case CT_AE_MODE_CONTROL:
                       printk("-------uvc_cmd_get_info():CT_AE_MODE_CONTROL-------\n"); 
						  	  memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_auto.info, 2);
							  value = 1;
							  break;
                    case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
							  printk("-------uvc_cmd_get_info():CT_EXPOSURE_TIME_ABSOLUTE_CONTROL-------\n");   
							  memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_absolute.info, 1);
							  value = 1;
							break;
						default:
							break;
					}
					break;
	
				case VC_OUTPUT_TERM_TID:		
					switch (bcsIdx) {
						default:
							break;
					}
					break;

                                case VC_SEL_UNIT_UID:
                                        switch (bcsIdx) {
                                                case SU_CONTROL_UNDEFINED: break;
                                                case SU_INPUT_SELECT_CONTROL:
							*(u8 *)(req->buf) = 0x01;
							value = 1;
							break;
                                                default:
                                                        break;
                                        }
                                        break;
				
				case VC_PRO_UNIT_UID:
					switch (bcsIdx) {
						case PU_CONTROL_UNDEFINED: break;
						case PU_BACKLIGHT_COMPENSATION_CONTROL: 
							printk("-------uvc_cmd_get_info():PU_BACKLIGHT_COMPENSATION_CONTROL-------\n");   
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.blc.info, 1);
							value = 1;
							 break;	
						case PU_BRIGHTNESS_CONTROL:
							printk("-------uvc_cmd_get_info():PU_BRIGHTNESS_CONTROL-------\n");   
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.brgt.info, 1);
							value = 1;
							 break;           	
						case PU_CONTRAST_CONTROL:
							printk("-------uvc_cmd_get_info():PU_CONTRAST_CONTROL-------\n");   
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.cnst.info, 1);
							value = 1;
							 break;             	
						case PU_GAIN_CONTROL: 
							printk("-------uvc_cmd_get_info():PU_GAIN_CONTROL-------\n");   
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gain.info, 1);
							value = 1;
							 break;                  	
						case PU_POWER_LINE_FREQUENCY_CONTROL: 
                      printk("-------uvc_cmd_get_info():PU_POWER_LINE_FREQUENCY_CONTROL-------\n"); 
                      memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.anti_flicker.info, 1);
                      value=1;
                      break;		
						case PU_HUE_CONTROL: 
							printk("-------uvc_cmd_get_info():PU_HUE_CONTROL-------\n");  
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.hue.info, 1);
							value = 1;
							break;                  	
						case PU_SATURATION_CONTROL: 
							printk("-------uvc_cmd_get_info():PU_SATURATION_CONTROL-------\n");  
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.satr.info, 1);
							value = 1;
							break;				
						case PU_SHARPNESS_CONTROL: break;				
						case PU_GAMMA_CONTROL: 
							printk("-------uvc_cmd_get_info():PU_GAMMA_CONTROL-------\n");  
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gamma.info, 1);
							value = 1;
							break;					
						case PU_WHITE_BALANCE_TEMPERATURE_CONTROL: 
	                  printk("-------uvc_cmd_get_info():PU_WB_TEMPERATURE_CONTROL-------\n");  
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt.info, 1);
							value = 1;
							break;	
						case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL: 
                     printk("-------uvc_cmd_get_info():PU_WB_TEMPERATURE_AUTO_CONTROL-------\n");  
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt_auto.info, 1);
							value = 1;
							break;	
						case PU_WHITE_BALANCE_COMPONENT_CONTROL: break;		
						case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL: break;	
						case PU_DIGITAL_MULTIPLIER_CONTROL: break;			
						case PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL: break;		
						case PU_HUE_AUTO_CONTROL: break;						
						case PU_ANALOG_VIDEO_STANDARD_CONTROL: break;		
						case PU_ANALOG_LOCK_STATUS_CONTROL: break;	
				
						default:
							break;
					}
					break;

                                case VC_EXT_UNIT_UID:
                                        switch (bcsIdx) {
                                                case XU_CONTROL_UNDEFINED: break;
                                                default:
                                                        break;
                                        }
                                        break;

				default:
					printk("get_cur on VCI, no entity specified\n");
					break;
				}
			}
			break;

		case VS_ISOC_IN_INTERFACE:
			switch (bcsIdx) {
			case VS_CONTROL_UNDEFINED:
				break;
			case VS_STREAM_ERROR_CODE_CONTROL:
				*(u8 *) req->buf = 0x06;
				value = 1;
				break;
			case VS_PROBE_CONTROL:
				*(u8 *) req->buf = 0x03;
				value = 1;
				break;
			case VS_COMMIT_CONTROL:
				*(u8 *) req->buf = 0x03;
				value = 1;
				break;
	#ifdef EN_STILL_CAP	
			case VS_STILL_IMAGE_TRIGGER_CONTROL:
				//*(u8 *)req->buf = media->en_sti;
				value = 1;
				break;	
	#endif
			default:
				break;
			}
			break;

		default:
			break;
		}
			
	return value;
}
/* TODO: This is to avoid repeated ep0 completion call which is corrupting the
   format setting for stream-0 */
static void setup_complete_null(struct usb_ep *ep, struct usb_request *req)
{
}
static void setup_complete_set_video_probe(struct usb_ep *ep, struct usb_request *req)
{
	struct media_g_dev	*media = ep->driver_data;
	unsigned long flags;

	u32	sn = (u32)req->context;

	spin_lock_irqsave(&media->lock, flags);
	memcpy(&media->s[sn].uvc_prob_comm_ds, (struct uvc_streaming_control *)req->buf, 0x1A);
	spin_unlock_irqrestore(&media->lock, flags);

	req->complete = setup_complete_null;	
	//printk("set_cur_probe--------------------------\n");
	//print_streaming_control(&uvc_prob_comm_ds);
}

static void setup_complete_set_video_commit(struct usb_ep *ep, struct usb_request *req)
{
	struct media_g_dev	*media = ep->driver_data;
	unsigned long flags;

	u32	sn = (u32)req->context;

	spin_lock_irqsave(&media->lock, flags);
	memcpy(&media->s[sn].cur_vs_state, (struct uvc_streaming_control *)req->buf, 0x1A);
	spin_unlock_irqrestore(&media->lock, flags);
	//printk("set_cur_commit--------------------------\n");
	//print_streaming_control(&media->s[0].cur_vs_state);
	req->complete = setup_complete_null;	
}

int uvc_cmd_set_cur(struct media_g_dev *media, u16 wIndex, u16 wValue, u16 wLength) {

	struct usb_request *req = media->ep0req;
	int value = -EOPNOTSUPP;
	u8 bifIdx = (u8)(wIndex);
	u8 benIdx = (wIndex>>8);
	u8 bcsIdx = (wValue>>8);
	
	switch (bifIdx) {
		case VC_INTERFACE_NUMBER:
			switch(benIdx) {
				case VC_INTERFACE_NUMBER:
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						case VC_REQUEST_ERROR_CODE_CONTROL:
							*(u8 *) req->buf = 0x06;
							value = 1;
							break;
						default:
							printk("set_cur inetrface 0 CS:%x\n", bcsIdx);
							break;
					}
					break;
			
				case VC_CAM_TERM_TID:	
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						case CT_AE_MODE_CONTROL:
                     printk("-------uvc_cmd_set_cur():CT_AE_MODE_CONTROL-------\n");  
						   memcpy((u8 *)&media->ct_ctrl.exposure_absolute.min,(u8 *)(req->buf), 2); 
							value = 2;
                #ifdef UAVC_CLASS_REQ_SUPPORT
					
							/* Acquire the class request buffer access semaphore */
							down(&media->class_req_sem);
							/* Fill the class request data.*/
                     media->class_req[0] = VC_CAM_TERM_TID;
							media->class_req[1] = CT_AE_MODE_CONTROL;
							media->class_req[2] = le16_to_cpu(*((u16 *)req->buf));
							up(&media->class_req_sem);

							/* Unblock user space reader process */ 							
							up(&media->read_sem);
               #endif

							 break;   
                  case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
                     printk("-------uvc_cmd_set_cur():CT_EXPOSURE_TIME_ABSOLUTE_CONTROL-------\n");  
						   memcpy((u8 *)&media->ct_ctrl.exposure_absolute.min,(u8 *)(req->buf), 2); 
							value = 2;
                #ifdef UAVC_CLASS_REQ_SUPPORT
					
							/* Acquire the class request buffer access semaphore */
							down(&media->class_req_sem);
							/* Fill the class request data.*/
                     media->class_req[0] = VC_CAM_TERM_TID;
							media->class_req[1] = CT_EXPOSURE_TIME_ABSOLUTE_CONTROL;
							media->class_req[2] = le16_to_cpu(*((u16 *)req->buf));
							up(&media->class_req_sem);

							/* Unblock user space reader process */ 							
							up(&media->read_sem);
               #endif

							 break;     
						default:
							break;
					}
					break;
	
				case VC_OUTPUT_TERM_TID:		
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						default:
							break;
					}
					break;

	            case VC_SEL_UNIT_UID:
                    switch (bcsIdx) {
                            case SU_CONTROL_UNDEFINED: break;
                            case SU_INPUT_SELECT_CONTROL: break;
                            default:
                                    break;
                    }
                    break;
				
				case VC_PRO_UNIT_UID:
					switch (bcsIdx) {
						case PU_CONTROL_UNDEFINED: break;
						case PU_BACKLIGHT_COMPENSATION_CONTROL: 
							printk("-------uvc_cmd_set_cur():PU_BACKLIGHT_COMPENSATION_CONTROL-------\n");  
							memcpy((u8 *)&media->pu_ctrl.blc.min,(u8 *)(req->buf), 2); /* ??? shall this update MIN???? */
							value = 2;
#ifdef UAVC_CLASS_REQ_SUPPORT
					
							/* Acquire the class request buffer access semaphore */
							down(&media->class_req_sem);
							/* Fill the class request data.*/
							media->class_req[bcsIdx] = (benIdx<<24)|(bcsIdx<<16)|le16_to_cpu(*((u16 *)req->buf));
							up(&media->class_req_sem);

							/* Unblock user space reader process */ 							
							up(&media->read_sem);
#endif
							 break;  	
						case PU_BRIGHTNESS_CONTROL:
							printk("-------uvc_cmd_set_cur():PU_BRIGHTNESS_CONTROL-------\n");  
							memcpy((u8 *)&media->pu_ctrl.brgt.min,(u8 *)(req->buf), 2); /* ??? shall this update MIN???? */
							value = 2;
#ifdef UAVC_CLASS_REQ_SUPPORT
					
							/* Acquire the class request buffer access semaphore */
							down(&media->class_req_sem);
							/* Fill the class request data.*/
							media->class_req[bcsIdx] = (benIdx<<24)|(bcsIdx<<16)|le16_to_cpu(*((u16 *)req->buf));
							up(&media->class_req_sem);

							/* Unblock user space reader process */ 							
							up(&media->read_sem);
#endif

							 break;           	
						case PU_CONTRAST_CONTROL:  
							printk("-------uvc_cmd_set_cur():PU_CONTRAST_CONTROL-------\n");  
							memcpy((u8 *)&media->pu_ctrl.cnst.min,(u8 *)(req->buf), 2);
							value = 2;
#ifdef UAVC_CLASS_REQ_SUPPORT
					
							/* Acquire the class request buffer access semaphore */
							down(&media->class_req_sem);
							/* Fill the class request data.*/
							media->class_req[bcsIdx] = (benIdx<<24)|(bcsIdx<<16)|le16_to_cpu(*((u16 *)req->buf));
							up(&media->class_req_sem);

							/* Unblock user space reader process */ 							
							up(&media->read_sem);
#endif

							 break;           	
						case PU_GAIN_CONTROL: 
							printk("-------uvc_cmd_set_cur():PU_GAIN_CONTROL-------\n");  
							memcpy((u8 *)&media->pu_ctrl.gain.min,(u8 *)(req->buf), 2);
							value = 2;
#ifdef UAVC_CLASS_REQ_SUPPORT
					
							/* Acquire the class request buffer access semaphore */
							down(&media->class_req_sem);
							/* Fill the class request data.*/
							media->class_req[bcsIdx] = (benIdx<<24)|(bcsIdx<<16)|le16_to_cpu(*((u16 *)req->buf));
							up(&media->class_req_sem);

							/* Unblock user space reader process */ 							
							up(&media->read_sem);
#endif
							break;                	
						case PU_POWER_LINE_FREQUENCY_CONTROL: 
                      printk("-------uvc_cmd_set_cur():PU_POWER_LINE_FREQUENCY_CONTROL-------\n"); 
                      memcpy((u8 *)&media->pu_ctrl.anti_flicker.min,(u8 *)(req->buf), 2);
                      value=2;
#ifdef UAVC_CLASS_REQ_SUPPORT
					
							/* Acquire the class request buffer access semaphore */
							down(&media->class_req_sem);
							/* Fill the class request data.*/
							media->class_req[bcsIdx] = (benIdx<<24)|(bcsIdx<<16)|le16_to_cpu(*((u16 *)req->buf));
							up(&media->class_req_sem);

							/* Unblock user space reader process */ 							
							up(&media->read_sem);
#endif
     
                      break;		
						case PU_HUE_CONTROL: 
							printk("-------uvc_cmd_set_cur():PU_HUE_CONTROL-------\n"); 
							memcpy((u8 *)&media->pu_ctrl.hue.min,(u8 *)(req->buf), 2); /* ??? shall this update MIN???? */
							value = 2;
#ifdef UAVC_CLASS_REQ_SUPPORT
					
							/* Acquire the class request buffer access semaphore */
							down(&media->class_req_sem);
							/* Fill the class request data.*/
							media->class_req[bcsIdx] = (benIdx<<24)|(bcsIdx<<16)|le16_to_cpu(*((u16 *)req->buf));
							up(&media->class_req_sem);

							/* Unblock user space reader process */ 							
							up(&media->read_sem);
#endif
                                           break;                  	
						case PU_SATURATION_CONTROL:
							printk("-------uvc_cmd_set_cur():PU_SATURATION_CONTROL-------\n"); 
							memcpy((u8 *)&media->pu_ctrl.satr.min,(u8 *)(req->buf), 2); /* ??? shall this update MIN???? */
							value = 2;
#ifdef UAVC_CLASS_REQ_SUPPORT
					
							/* Acquire the class request buffer access semaphore */
							down(&media->class_req_sem);
							/* Fill the class request data.*/
							media->class_req[bcsIdx] = (benIdx<<24)|(bcsIdx<<16)|le16_to_cpu(*((u16 *)req->buf));
							up(&media->class_req_sem);

							/* Unblock user space reader process */ 							
							up(&media->read_sem);
#endif
                                           break;    			
						case PU_SHARPNESS_CONTROL: break;				
						case PU_GAMMA_CONTROL: 
							printk("-------uvc_cmd_set_cur():PU_GAMMA_CONTROL-------\n"); 
							memcpy((u8 *)&media->pu_ctrl.gamma.min,(u8 *)(req->buf), 2); /* ??? shall this update MIN???? */
							value = 2;
#ifdef UAVC_CLASS_REQ_SUPPORT
					
							/* Acquire the class request buffer access semaphore */
							down(&media->class_req_sem);
							/* Fill the class request data.*/
							media->class_req[bcsIdx] = (benIdx<<24)|(bcsIdx<<16)|le16_to_cpu(*((u16 *)req->buf));
							up(&media->class_req_sem);

							/* Unblock user space reader process */ 							
							up(&media->read_sem);
#endif
                     break; 				
						case PU_WHITE_BALANCE_TEMPERATURE_CONTROL: 
                     printk("-------uvc_cmd_set_cur():PU_WB_TEMPERATURE_CONTROL-------\n"); 
							memcpy((u8 *)&media->pu_ctrl.wbt.min,(u8 *)(req->buf), 2); /* ??? shall this update MIN???? */
							value = 2;
#ifdef UAVC_CLASS_REQ_SUPPORT
					
							/* Acquire the class request buffer access semaphore */
							down(&media->class_req_sem);
							/* Fill the class request data.*/
							media->class_req[bcsIdx] = (benIdx<<24)|(bcsIdx<<16)|le16_to_cpu(*((u16 *)req->buf));
							up(&media->class_req_sem);

							/* Unblock user space reader process */ 							
							up(&media->read_sem);
#endif
                     break;
						case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL: 
						   printk("-------uvc_cmd_set_cur():PU_WB_TEMPERATURE_AUTO_CONTROL-------\n"); 
							memcpy((u8 *)&media->pu_ctrl.wbt_auto.min,(u8 *)(req->buf), 2); /* ??? shall this update MIN???? */
							value = 2;
#ifdef UAVC_CLASS_REQ_SUPPORT
					
							/* Acquire the class request buffer access semaphore */
							down(&media->class_req_sem);
							/* Fill the class request data.*/
							media->class_req[bcsIdx] = (benIdx<<24)|(bcsIdx<<16)|le16_to_cpu(*((u16 *)req->buf));
							up(&media->class_req_sem);

							/* Unblock user space reader process */ 							
							up(&media->read_sem);
#endif
                                           break;
						case PU_WHITE_BALANCE_COMPONENT_CONTROL: break;		
						case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL: break;	
						case PU_DIGITAL_MULTIPLIER_CONTROL: break;			
						case PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL: break;		
						case PU_HUE_AUTO_CONTROL: break;						
						case PU_ANALOG_VIDEO_STANDARD_CONTROL: break;		
						case PU_ANALOG_LOCK_STATUS_CONTROL: break;	
				
						default:
							break;
					}
					break;

		            case VC_EXT_UNIT_UID:
		                    switch (bcsIdx) {
		                            case XU_CONTROL_UNDEFINED: break;
									case XU_FACEDETECTION_ENABLE_CONTROL:
									case XU_FACEDETECTION_DISABLE_CONTROL:
									
#ifdef UAVC_CLASS_REQ_SUPPORT
										/* Acquire the class request buffer access semaphore */
										down(&media->class_req_sem);
										/* Fill the class request data.*/
										media->class_req[bcsIdx] = (benIdx<<24)|(bcsIdx<<16)|le16_to_cpu(*((u16 *)req->buf));
										up(&media->class_req_sem);

										/* Unblock user space reader process */ 							
										up(&media->read_sem);
#endif										
									break;
		                            default:
	                                    break;
		                    }
		                    break;

				default:
					printk("get_min on VCI, no entity specified\n");
					break;
			}
			break;

		case VS_ISOC_IN_INTERFACE:
#ifdef DUAL_STREAM
		case VS_ISOC_IN_INTERFACE2:
#endif
#ifdef BIDIR_STREAM
		case VS_ISOC_OUT_INTERFACE:
#endif
			switch (bcsIdx) {
			case VS_CONTROL_UNDEFINED:
				break;
			case VS_STREAM_ERROR_CODE_CONTROL:
				*(u8 *) req->buf = 0x06;
				value = 1;
				break;
			case VS_PROBE_CONTROL:
				value = min(wLength, (u16) sizeof(struct uvc_streaming_control));
				req->complete = setup_complete_set_video_probe;
				req->context = (void*)(bifIdx - 1);
				break;
			case VS_COMMIT_CONTROL:
				value = min(wLength, (u16) sizeof(struct uvc_streaming_control));
				req->complete = setup_complete_set_video_commit;
				req->context = (void*)(bifIdx - 1);
				break;
	#ifdef EN_STILL_CAP	
			case VS_STILL_PROBE_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_stillimg_control));
				memcpy(&uvc_prob_comm_ds_still, req->buf, value);
				break;
			
			case VS_STILL_COMMIT_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_stillimg_control));
				//memcpy(&dev->if_stm_state, req->buf, value);
				break;
		
			case VS_STILL_IMAGE_TRIGGER_CONTROL:
				//printk("Request for still capture:%d\n", *(u8 *)req->buf);
				/*  if((*(u8 *)req->buf))
					media->en_sti = 1;
				else
					media->en_sti = 0;
				*/
				break;
	#endif
			default:
				break;
			}
			break;

		default:
			break;
		}	
	
	return value;
}

int uvc_cmd_get_min(struct media_g_dev *media, u16 wIndex, u16 wValue, u16 wLength) {

	struct usb_request *req = media->ep0req;
	int value = -EOPNOTSUPP;
	u8 bifIdx = (u8)(wIndex);
	u8 benIdx = (wIndex>>8);
	u8 bcsIdx = (wValue>>8);
	
	switch (bifIdx) {
		case VC_INTERFACE_NUMBER:
			switch(benIdx) {
				case VC_INTERFACE_NUMBER:
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						case VC_REQUEST_ERROR_CODE_CONTROL:
							*(u8 *) req->buf = 0x06;
							value = 1;
							break;
						default:
							printk("get_cur inetrface 0 CS:%x\n", bcsIdx);
							break;
					}
					break;
			
				case VC_CAM_TERM_TID:	
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						  case CT_AE_MODE_CONTROL:
                       printk("-------uvc_cmd_get_min():CT_AE_MODE_CONTROL-------\n"); 
						  	  memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_auto.min, 2);
							  value = 2;
							  break;
                     case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
                       printk("-------uvc_cmd_get_min():CT_EXPOSURE_TIME_ABSOLUTE_CONTROL-------\n"); 
							  memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_absolute.min, 2);
							  value = 2;
							  break;
						default:
							break;
					}
					break;
	
				case VC_OUTPUT_TERM_TID:		
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						default:
							break;
					}
					break;

                                case VC_SEL_UNIT_UID:
                                        switch (bcsIdx) {
                                                case SU_CONTROL_UNDEFINED: break;
                                                case SU_INPUT_SELECT_CONTROL: break;
                                                default:
                                                        break;
                                        }
                                        break;
				
				case VC_PRO_UNIT_UID:
					switch (bcsIdx) {
						case PU_CONTROL_UNDEFINED: break;
						case PU_BACKLIGHT_COMPENSATION_CONTROL: 
							printk("-------uvc_cmd_get_min():PU_BACKLIGHT_COMPENSATION_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.blc.min, 2);
							value = 2;
							 break;	
						case PU_BRIGHTNESS_CONTROL:
							printk("-------uvc_cmd_get_min():PU_BRIGHTNESS_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.brgt.min, 2);
							value = 2;
							 break;           	
						case PU_CONTRAST_CONTROL:   
							printk("-------uvc_cmd_get_min():PU_CONTRAST_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.cnst.min, 2);
							value = 2;
							 break;           	
						case PU_GAIN_CONTROL: 
							printk("-------uvc_cmd_get_min():PU_GAIN_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gain.min, 2);
							value = 2;
							 break;                	
						case PU_POWER_LINE_FREQUENCY_CONTROL: 
                      printk("-------uvc_cmd_get_min():PU_POWER_LINE_FREQUENCY_CONTROL-------\n"); 
                      memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.anti_flicker.min, 2);
                      value=2;
                      break;	
						case PU_HUE_CONTROL: 
							printk("-------uvc_cmd_get_min():PU_HUE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.hue.min, 2);
							value = 2;
							break;                  	
						case PU_SATURATION_CONTROL: 
							printk("-------uvc_cmd_get_min():PU_SATURATION_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.satr.min, 2);
							value = 2;
							break;    
						case PU_SHARPNESS_CONTROL: break;				
						case PU_GAMMA_CONTROL: 
							printk("-------uvc_cmd_get_min():PU_GAMMA_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gamma.min, 2);
							value = 2;
							break; 					
						case PU_WHITE_BALANCE_TEMPERATURE_CONTROL: 
	                  printk("-------uvc_cmd_get_min():PU_WB_TEMPERATURE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt.min, 2);
							value = 2;
							break;
						case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL: 
	                  printk("-------uvc_cmd_get_min():PU_WB_TEMPERATURE_AUTO_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt_auto.min, 2);
							value = 2;
							break;
						case PU_WHITE_BALANCE_COMPONENT_CONTROL: break;		
						case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL: break;	
						case PU_DIGITAL_MULTIPLIER_CONTROL: break;			
						case PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL: break;		
						case PU_HUE_AUTO_CONTROL: break;						
						case PU_ANALOG_VIDEO_STANDARD_CONTROL: break;		
						case PU_ANALOG_LOCK_STATUS_CONTROL: break;	
				
						default:
							break;
					}
					break;

                                case VC_EXT_UNIT_UID:
                                        switch (bcsIdx) {
                                                case XU_CONTROL_UNDEFINED: break;
                                                default:
                                                        break;
                                        }
                                        break;

				default:
					printk("get_min on VCI, no entity specified\n");
					break;
			}
			break;

		case VS_ISOC_IN_INTERFACE:
#ifdef DUAL_STREAM
		case VS_ISOC_IN_INTERFACE2:
#endif
#ifdef BIDIR_STREAM
		case VS_ISOC_OUT_INTERFACE:
#endif
			switch (bcsIdx) {
			case VS_CONTROL_UNDEFINED:
				break;
			case VS_STREAM_ERROR_CODE_CONTROL:
				*(u8 *) req->buf = 0x06;
				value = 1;
				break;
			case VS_PROBE_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_streaming_control));
				memcpy(req->buf, (struct uvc_streaming_control *)&media->s[bifIdx-1].uvc_prob_comm_ds_min, value);
				//printk("get_min--------------------------\n");
				//print_streaming_control(&uvc_prob_comm_ds_min);
				break;
			case VS_COMMIT_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_streaming_control));
				memcpy(req->buf, (struct uvc_streaming_control *)&media->s[bifIdx-1].uvc_prob_comm_ds_min, value);
				break;
	#ifdef EN_STILL_CAP
			case VS_STILL_PROBE_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_stillimg_control));
				memcpy(req->buf, &uvc_prob_comm_ds_still, value);
				break;
			
			case VS_STILL_COMMIT_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_stillimg_control));
				//memcpy(req->buf, &dev->if_stm_state, value);
				break;
	#endif
			default:
				break;
			}
		
		default:
			break;
		}	
	
	return value;
}

int uvc_cmd_get_max(struct media_g_dev *media, u16 wIndex, u16 wValue, u16 wLength) {

	struct usb_request *req = media->ep0req;
	int value = -EOPNOTSUPP;
	u8 bifIdx = (u8)(wIndex);
	u8 benIdx = (wIndex>>8);
	u8 bcsIdx = (wValue>>8);

	switch (bifIdx) {
		case VC_INTERFACE_NUMBER:
			switch(benIdx) {
				case VC_INTERFACE_NUMBER:
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						case VC_REQUEST_ERROR_CODE_CONTROL:
							*(u8 *) req->buf = 0x06;
							value = 1;
							break;
						default:
							printk("get_cur inetrface 0 CS:%x\n", bcsIdx);
							break;
					}
					break;
				case VC_CAM_TERM_TID:	
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						case CT_AE_MODE_CONTROL:
                       printk("-------uvc_cmd_get_max():CT_AE_MODE_CONTROL-------\n"); 
						  	  memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_auto.max, 2);
							  value = 2;
							  break;
                  case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
                       printk("-------uvc_cmd_get_max():CT_EXPOSURE_TIME_ABSOLUTE_CONTROL-------\n"); 
				           memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_absolute.max, 2);
				           value = 2;
				           break;
						default:
							break;
					}
					break;
	
				case VC_OUTPUT_TERM_TID:		
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						default:
							break;
					}
					break;

                                case VC_SEL_UNIT_UID:
                                        switch (bcsIdx) {
                                                case SU_CONTROL_UNDEFINED: break;
                                                case SU_INPUT_SELECT_CONTROL: break;
                                                default:
                                                        break;
                                        }
                                        break;
				
				case VC_PRO_UNIT_UID:
					switch (bcsIdx) {
						case PU_CONTROL_UNDEFINED: break;
						case PU_BACKLIGHT_COMPENSATION_CONTROL: 
							printk("-------uvc_cmd_get_max():PU_BACKLIGHT_COMPENSATION_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.blc.max, 2);
							value = 2;
							 break;	
						case PU_BRIGHTNESS_CONTROL:    
							printk("-------uvc_cmd_get_max():PU_BRIGHTNESS_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.brgt.max, 2);
							value = 2;
							 break;           	
						case PU_CONTRAST_CONTROL:      
							printk("-------uvc_cmd_get_max():PU_CONTRAST_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.cnst.max, 2);
							value = 2;
							 break;           	
						case PU_GAIN_CONTROL: 
							printk("-------uvc_cmd_get_max():PU_GAIN_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gain.max, 2);
							value = 2;
							 break;                 	
						case PU_POWER_LINE_FREQUENCY_CONTROL: 
							printk("-------uvc_cmd_get_max():PU_POWER_LINE_FREQUENCY_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.anti_flicker.max, 2);
							value = 2;
							break;          	
						case PU_HUE_CONTROL:
							printk("-------uvc_cmd_get_max():PU_HUE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.hue.max, 2);
							value = 2;
							break;                  	
						case PU_SATURATION_CONTROL: 
							printk("-------uvc_cmd_get_max():PU_SATURATION_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.satr.max, 2);
							value = 2;
							break;				
						case PU_SHARPNESS_CONTROL: break;				
						case PU_GAMMA_CONTROL: 
							printk("-------uvc_cmd_get_max():PU_GAMMA_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gamma.max, 2);
							value = 2;
							break;					
						case PU_WHITE_BALANCE_TEMPERATURE_CONTROL: 
	                  printk("-------uvc_cmd_get_max():PU_WB_TEMPERATURE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt.max, 2);
							value = 2;
							break;		
						case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL: 
	                  printk("-------uvc_cmd_get_max():PU_WB_TEMPERATURE_AUTO_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt_auto.max, 2);
							value = 2;
							break;
						case PU_WHITE_BALANCE_COMPONENT_CONTROL: break;		
						case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL: break;	
						case PU_DIGITAL_MULTIPLIER_CONTROL: break;			
						case PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL: break;		
						case PU_HUE_AUTO_CONTROL: break;						
						case PU_ANALOG_VIDEO_STANDARD_CONTROL: break;		
						case PU_ANALOG_LOCK_STATUS_CONTROL: break;	
				
						default:
							break;
					}
					break;

                                case VC_EXT_UNIT_UID:
                                        switch (bcsIdx) {
                                                case XU_CONTROL_UNDEFINED: break;
                                                default:
                                                        break;
                                        }
                                        break;

				default:
					printk("get_max on VCI, no entity specified\n");
					break;
			}
			break;

		case VS_ISOC_IN_INTERFACE:
#ifdef DUAL_STREAM
		case VS_ISOC_IN_INTERFACE2:
#endif
#ifdef BIDIR_STREAM
		case VS_ISOC_OUT_INTERFACE:
#endif
			switch (bcsIdx) {
			case VS_CONTROL_UNDEFINED:
				break;
			case VS_STREAM_ERROR_CODE_CONTROL:
				*(u8 *) req->buf = 0x06;
				value = 1;
				break;
			case VS_PROBE_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_streaming_control));
				memcpy(req->buf, (struct uvc_streaming_control *)&media->s[bifIdx-1].uvc_prob_comm_ds_max, value);
				//printk("get_max--------------------------\n");
				//print_streaming_control(&uvc_prob_comm_ds_max);
				break;
			case VS_COMMIT_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_streaming_control));
				memcpy(req->buf, (struct uvc_streaming_control *)&media->s[bifIdx-1].uvc_prob_comm_ds_max, value);
				break; 
	#ifdef EN_STILL_CAP
			case VS_STILL_PROBE_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_stillimg_control));
				memcpy(req->buf, &uvc_prob_comm_ds_still, value);
				break;
			
			case VS_STILL_COMMIT_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_stillimg_control));
				//memcpy(req->buf, &dev->if_stm_state, value);
				break;
	#endif
			default:
				break;
			}
			break;

		default:
			break;
		}
	
	return value;
}

int uvc_cmd_get_res(struct media_g_dev *media, u16 wIndex, u16 wValue, u16 wLength) {

	struct usb_request *req = media->ep0req;
	int value = -EOPNOTSUPP;
	u8 bifIdx = (u8)(wIndex);
	u8 benIdx = (wIndex>>8);
	u8 bcsIdx = (wValue>>8);

	switch (bifIdx) {
		case VC_INTERFACE_NUMBER:
			switch(benIdx) {
				case VC_INTERFACE_NUMBER:
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						case VC_REQUEST_ERROR_CODE_CONTROL:
							*(u8 *) req->buf = 0x06;
							value = 1;
							break;
						default:
							printk("get_cur inetrface 0 CS:%x\n", bcsIdx);
							break;
					}
					break;
					
				case VC_CAM_TERM_TID:	
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						case CT_AE_MODE_CONTROL:
                       printk("-------uvc_cmd_get_res():CT_AE_MODE_CONTROL-------\n"); 
						  	  memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_auto.res, 2);
							  value = 2;
							  break;
                  case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
                     printk("-------uvc_cmd_get_res():CT_EXPOSURE_TIME_ABSOLUTE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_absolute.res, 2);
							value = 2;
							break;
						default:
							break;
					}
					break;
	
				case VC_OUTPUT_TERM_TID:		
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						default:
							break;
					}
					break;

                                case VC_SEL_UNIT_UID:
                                        switch (bcsIdx) {
                                                case SU_CONTROL_UNDEFINED: break;
                                                case SU_INPUT_SELECT_CONTROL: break;
                                                default:
                                                        break;
                                        }
                                        break;
				
				case VC_PRO_UNIT_UID:
					switch (bcsIdx) {
						case PU_CONTROL_UNDEFINED: break;
						case PU_BACKLIGHT_COMPENSATION_CONTROL: 
							printk("-------uvc_cmd_get_res():PU_BACKLIGHT_COMPENSATION_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.blc.res, 2);
							value = 2;
							break;   	
						case PU_BRIGHTNESS_CONTROL:  
						       printk("-------uvc_cmd_get_res():PU_BRIGHTNESS_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.brgt.res, 2);
							value = 2;
							 break;           	
						case PU_CONTRAST_CONTROL:  
							printk("-------uvc_cmd_get_res():PU_CONTRAST_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.cnst.res, 2);
							value = 2;
							 break;           	
						case PU_GAIN_CONTROL: 
 							printk("-------uvc_cmd_get_res():PU_GAIN_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gain.res, 2);
							value = 2;
							break;              	
						case PU_POWER_LINE_FREQUENCY_CONTROL: 
                      printk("-------uvc_cmd_get_res():PU_POWER_LINE_FREQUENCY_CONTROL-------\n"); 
                      memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.anti_flicker.res, 2);
                      value=2;
                      break;	
						case PU_HUE_CONTROL: 
							printk("-------uvc_cmd_get_res():PU_HUE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.hue.res, 2);
							value = 2;
							break;                  	
						case PU_SATURATION_CONTROL:
							printk("-------uvc_cmd_get_res():PU_SATURATION_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.satr.res, 2);
							value = 2;
  							break;				
						case PU_SHARPNESS_CONTROL: break;				
						case PU_GAMMA_CONTROL: 
							printk("-------uvc_cmd_get_res():PU_GAMMA_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gamma.res, 2);
							value = 2;
  							break;						
						case PU_WHITE_BALANCE_TEMPERATURE_CONTROL: 
	                  printk("-------uvc_cmd_get_res():PU_WB_TEMPERATURE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt.res, 2);
							value = 2;
							break;	
						case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL: 
                     printk("-------uvc_cmd_get_res():PU_WB_TEMPERATURE_AUTO_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt_auto.res, 2);
							value = 2;
							break;	
						case PU_WHITE_BALANCE_COMPONENT_CONTROL: break;		
						case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL: break;	
						case PU_DIGITAL_MULTIPLIER_CONTROL: break;			
						case PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL: break;		
						case PU_HUE_AUTO_CONTROL: break;						
						case PU_ANALOG_VIDEO_STANDARD_CONTROL: break;		
						case PU_ANALOG_LOCK_STATUS_CONTROL: break;	
				
						default:
							break;
					}
					break;

                                case VC_EXT_UNIT_UID:
                                        switch (bcsIdx) {
                                                case XU_CONTROL_UNDEFINED: break;
                                                default:
                                                        break;
                                        }
                                        break;

				default:
					printk("get_res on VCI, no entity specified\n");
					break;
			}
			break;

		case VS_ISOC_IN_INTERFACE:
			switch (bcsIdx) {
			case VS_CONTROL_UNDEFINED:
				break;
			case VS_STREAM_ERROR_CODE_CONTROL:
				*(u8 *) req->buf = 0x06;
				value = 1;
				break;
			case VS_PROBE_CONTROL:
				break;
			case VS_COMMIT_CONTROL:
				break;
			default:
				break;
			}
			break;

		default:
			break;
		}
		
	return value;
}

int uvc_cmd_get_len(struct media_g_dev *media, u16 wIndex, u16 wValue, u16 wLength) {

	struct usb_request *req = media->ep0req;
	int value = -EOPNOTSUPP;
	u8 bifIdx = (u8)(wIndex);
	u8 benIdx = (wIndex>>8);
	u8 bcsIdx = (wValue>>8);

	switch (bifIdx) {
		case VC_INTERFACE_NUMBER:
			switch(benIdx) {
				case VC_INTERFACE_NUMBER:
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						case VC_REQUEST_ERROR_CODE_CONTROL:
							*(u8 *) req->buf = 0x06;
							value = 1;
							break;
						default:
							printk("get_cur inetrface 0 CS:%x\n", bcsIdx);
							break;
					}
					break;
				case VC_CAM_TERM_TID:	
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						  case CT_AE_MODE_CONTROL:
                       printk("-------uvc_cmd_get_len():CT_AE_MODE_CONTROL-------\n"); 
						  	  memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_auto.len, 2);
							  value = 2;
							  break;
                    case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
                       printk("-------uvc_cmd_get_len():CT_EXPOSURE_TIME_ABSOLUTE_CONTROL-------\n"); 
							  memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_absolute.len, 2);
							  value = 2;
						   	break;
						default:
							break;
					}
					break;
	
				case VC_OUTPUT_TERM_TID:		
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						default:
							break;
					}
					break;

                                case VC_SEL_UNIT_UID:
                                        switch (bcsIdx) {
                                                case SU_CONTROL_UNDEFINED: break;
                                                case SU_INPUT_SELECT_CONTROL: break;
                                                default:
                                                        break;
                                        }
                                        break;
				
				case VC_PRO_UNIT_UID:
					switch (bcsIdx) {
						case PU_CONTROL_UNDEFINED: break;
						case PU_BACKLIGHT_COMPENSATION_CONTROL: 
							printk("-------uvc_cmd_get_len():PU_BACKLIGHT_COMPENSATION_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.blc.len, 1);
							value = 1;
							 break;	
						case PU_BRIGHTNESS_CONTROL:  
							printk("-------uvc_cmd_get_len():PU_BRIGHTNESS_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.brgt.len, 1);
							value = 1;
							 break;           	
						case PU_CONTRAST_CONTROL:	
							printk("-------uvc_cmd_get_len():PU_CONTRAST_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.cnst.len, 1);
							value = 1;
							 break;           	
						case PU_GAIN_CONTROL: 
							printk("-------uvc_cmd_get_len():PU_GAIN_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gain.len, 1);
							value = 1;
							 break;                	
						case PU_POWER_LINE_FREQUENCY_CONTROL: 
                      printk("-------uvc_cmd_get_len():PU_POWER_LINE_FREQUENCY_CONTROL-------\n"); 
                      memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.anti_flicker.len, 1);
                      value=1;
                      break;		
						case PU_HUE_CONTROL: 
							printk("-------uvc_cmd_get_len():PU_HUE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.hue.len, 1);
							value = 1;
							break;                  	
						case PU_SATURATION_CONTROL:
							printk("-------uvc_cmd_get_len():PU_SATURATION_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.satr.len, 1);
							value = 1;
    						break;				
						case PU_SHARPNESS_CONTROL: break;				
						case PU_GAMMA_CONTROL: 
							printk("-------uvc_cmd_get_len():PU_GAMMA_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gamma.len, 1);
							value = 1;
    						break;					
						case PU_WHITE_BALANCE_TEMPERATURE_CONTROL: 
                     printk("-------uvc_cmd_get_len():PU_WB_TEMPERATURE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt.len, 1);
							value = 1;
    						break;	
						case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL: break;
                     printk("-------uvc_cmd_get_len():PU_WB_TEMPERATURE_AUTO_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt_auto.len, 1);
							value = 1;
    						break;	
						case PU_WHITE_BALANCE_COMPONENT_CONTROL: break;		
						case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL: break;	
						case PU_DIGITAL_MULTIPLIER_CONTROL: break;			
						case PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL: break;		
						case PU_HUE_AUTO_CONTROL: break;						
						case PU_ANALOG_VIDEO_STANDARD_CONTROL: break;		
						case PU_ANALOG_LOCK_STATUS_CONTROL: break;	
				
						default:
							break;
					}
					break;

                                case VC_EXT_UNIT_UID:
                                        switch (bcsIdx) {
                                                case XU_CONTROL_UNDEFINED: break;
                                                default:
                                                        break;
                                        }
                                        break;

				default:
					printk("get_len on VCI, no entity specified\n");
					break;
			}
			break;

		case VS_ISOC_IN_INTERFACE:
			switch (bcsIdx) {
			case VS_CONTROL_UNDEFINED:
				break;
			case VS_STREAM_ERROR_CODE_CONTROL:
				*(u8 *) req->buf = 0x06;
				value = 1;
				break;
			case VS_PROBE_CONTROL:
				*(u8 *) req->buf = 0x1A;
				value = 1;
				break;
			case VS_COMMIT_CONTROL:
				*(u8 *) req->buf = 0x1A;
				value = 1;
				break;
	#ifdef EN_STILL_CAP			
			case VS_STILL_PROBE_CONTROL:
				*(u8 *) req->buf = 0x0B;
				value = 1;
				break;
		
			case VS_STILL_COMMIT_CONTROL:
				*(u8 *) req->buf = 0x0B;
				value = 1;
				break;
	#endif			
			default:
				break;
			}
			break;

		default:
			break;
		}
	
	return value;
}

int uvc_cmd_get_def(struct media_g_dev *media, u16 wIndex, u16 wValue, u16 wLength) {

	struct usb_request *req = media->ep0req;
	int value = -EOPNOTSUPP;
	u8 bifIdx = (u8)(wIndex);
	u8 benIdx = (wIndex>>8);
	u8 bcsIdx = (wValue>>8);
	
	switch (bifIdx) {
		case VC_INTERFACE_NUMBER:
			switch(benIdx) {
				case VC_INTERFACE_NUMBER:
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						case VC_REQUEST_ERROR_CODE_CONTROL:
							*(u8 *) req->buf = 0x06;
							value = 1;
							break;
						default:
							printk("get_cur inetrface 0 CS:%x\n", bcsIdx);
							break;
					}
					break;
					
				case VC_CAM_TERM_TID:	
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
                 case CT_AE_MODE_CONTROL:
                     printk("-------uvc_cmd_get_def():CT_AE_MODE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_auto.def, 2);
							value = 1;
							break; 
                 case CT_EXPOSURE_TIME_ABSOLUTE_CONTROL:
                     printk("-------uvc_cmd_get_def():CT_EXPOSURE_TIME_ABSOLUTE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->ct_ctrl.exposure_absolute.def, 2);
							value = 2;
							break;
						default:
							break;
					}
					break;
	
				case VC_OUTPUT_TERM_TID:		
					switch (bcsIdx) {
						case VC_CONTROL_UNDEFINED:
							break;
						default:
							break;
					}
					break;

                                case VC_SEL_UNIT_UID:
                                        switch (bcsIdx) {
                                                case SU_CONTROL_UNDEFINED: break;
                                                case SU_INPUT_SELECT_CONTROL: break;
                                                default:
                                                        break;
                                        }
                                        break;
				
				case VC_PRO_UNIT_UID:
					switch (bcsIdx) {
						case PU_CONTROL_UNDEFINED: break;
						case PU_BACKLIGHT_COMPENSATION_CONTROL: 
                     printk("-------uvc_cmd_get_def():PU_BACKLIGHT_COMPENSATION_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.blc.def, 2);
							value = 2;
							break;
						case PU_BRIGHTNESS_CONTROL:
							printk("-------uvc_cmd_get_def():PU_BRIGHTNESS_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.brgt.def, 2);
							value = 2;
							break;
						case PU_CONTRAST_CONTROL:
							printk("-------uvc_cmd_get_def():PU_CONTRAST_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.cnst.def, 2);
							value = 2;
							break;             	
						case PU_GAIN_CONTROL: 
                     printk("-------uvc_cmd_get_def():PU_GAIN_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gain.def, 2);
							value = 2;
							break;                	
						case PU_POWER_LINE_FREQUENCY_CONTROL: 
                      printk("-------uvc_cmd_get_def():PU_POWER_LINE_FREQUENCY_CONTROL-------\n"); 
                      memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.anti_flicker.def, 2);
                      value=2;
                      break;		
						case PU_HUE_CONTROL:
							printk("-------uvc_cmd_get_def():PU_HUE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.hue.def, 2);
							value = 2;
							break;                  	
						case PU_SATURATION_CONTROL: 
							printk("-------uvc_cmd_get_def():PU_SATURATION_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.satr.def, 2);
							value = 2;
							break;				
						case PU_SHARPNESS_CONTROL: break;				
						case PU_GAMMA_CONTROL: 
							printk("-------uvc_cmd_get_def():PU_GAMMA_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.gamma.def, 2);
							value = 2;
							break;						
						case PU_WHITE_BALANCE_TEMPERATURE_CONTROL: 
                     printk("-------uvc_cmd_get_def():PU_WB_TEMPERATURE_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt.def, 2);
							value = 2;
							break;
						case PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL: 
                     printk("-------uvc_cmd_get_def():PU_WB_TEMPERATURE_AUTO_CONTROL-------\n"); 
							memcpy((u8 *)(req->buf), (u8 *)&media->pu_ctrl.wbt_auto.def, 2);
							value = 2;
							break;
						case PU_WHITE_BALANCE_COMPONENT_CONTROL: break;		
						case PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL: break;	
						case PU_DIGITAL_MULTIPLIER_CONTROL: break;			
						case PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL: break;		
						case PU_HUE_AUTO_CONTROL: break;						
						case PU_ANALOG_VIDEO_STANDARD_CONTROL: break;		
						case PU_ANALOG_LOCK_STATUS_CONTROL: break;	
				
						default:
							break;
					}
					break;

                                case VC_EXT_UNIT_UID:
                                        switch (bcsIdx) {
                                                case XU_CONTROL_UNDEFINED: break;
                                                default:
                                                        break;
                                        }
                                        break;

				default:
					printk("get_def on VCI, no entity specified\n");
					break;
			}
			break;

		case VS_ISOC_IN_INTERFACE:
#ifdef DUAL_STREAM
		case VS_ISOC_IN_INTERFACE2:
#endif
#ifdef BIDIR_STREAM
		case VS_ISOC_OUT_INTERFACE:
#endif
			switch (bcsIdx) {
			case VS_CONTROL_UNDEFINED:
				break;
			case VS_STREAM_ERROR_CODE_CONTROL:
				*(u8 *) req->buf = 0x06;
				value = 1;
				break;
			case VS_PROBE_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_streaming_control));
				memcpy(req->buf, (struct uvc_streaming_control *)&media->s[bifIdx-1].uvc_prob_comm_ds, value);
				//printk("get_def--------------------------\n");
				//print_streaming_control(&uvc_prob_comm_ds);
				break;
			case VS_COMMIT_CONTROL:
				value =
				    min(wLength,
					(u16) sizeof(struct
						     uvc_streaming_control));
				//memcpy(req->buf, (struct uvc_streaming_control *)&dev->cur_vs_state, value);
			default:
				break;
			}
			break;

		default:
			break;
		}
	
	return value;
}

static int uvc_class_setup_req(struct media_g_dev *media,
		const struct usb_ctrlrequest *ctrl)
{
	int			value = -EOPNOTSUPP;
	u16 wIndex = le16_to_cpu(ctrl->wIndex);
	u16 wValue = le16_to_cpu(ctrl->wValue);
	u16 wLength = le16_to_cpu(ctrl->wLength);

	switch (ctrl->bRequest) {
		case RC_UNDEFINED:
			printk("RC_UNDEFINED:Type:%x wVal:%x Idx:%x\r\n",
		       		ctrl->bRequestType, wValue, wIndex);
			break;

		case GET_CUR:
			value = uvc_cmd_get_cur(media, wIndex, wValue, wLength);
			break;

		case GET_INFO:
			value = uvc_cmd_get_info(media, wIndex, wValue, wLength);
			break;

		case SET_CUR:
			value = uvc_cmd_set_cur(media, wIndex, wValue, wLength);
			break;

		case GET_MIN:
			value = uvc_cmd_get_min(media, wIndex, wValue, wLength);
			break;

		case GET_MAX:
			value = uvc_cmd_get_max(media, wIndex, wValue, wLength);
			break;

		case GET_RES:
			value = uvc_cmd_get_res(media, wIndex, wValue, wLength);
			break;

		case GET_LEN:
			value = uvc_cmd_get_len(media, wIndex, wValue, wLength);
			break;

		case GET_DEF:
			value = uvc_cmd_get_def(media, wIndex, wValue, wLength);
			break;

		default:
			printk("invalid uvc control req%02x.%02x v%04x i%04x l%d\n",
					ctrl->bRequestType, ctrl->bRequest,
						wValue, wIndex, wLength);
			usb_ep_set_halt(media->gadget->ep0);
      			break;
	}

	return value;
}

static inline struct usb_request *get_request(struct media_g_dev *gb, u8 sn)
{
	struct list_head *queue = (struct list_head *)&gb->s[sn].urblist;
	struct usb_request	*req;
	if (list_empty(queue))
		return NULL;

	gb->s[sn].freereqcnt--;
	req = (struct usb_request *) container_of(queue->next, 
			struct usb_request, list);
	list_del_init(&req->list);
	return req;
}

static inline void put_request (struct media_g_dev *gb, struct usb_request *req, u8 sn)
{
	struct list_head *queue = &gb->s[sn].urblist;

	if(!req)
		return;

	gb->s[sn].freereqcnt++;
	list_add_tail(&req->list, queue);
}

static void uvc_tx_submit_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct media_g_dev	*media = ep->driver_data;
	unsigned long flags;
	char 	*buf1 = NULL;
	u8	loopflg = 0;
	u8	sn;
	u32	length;

#ifdef EN_TIMESTAMP
	__u64 temp3 = 0;
	unsigned int scr_tv_usec = 0, pts_tv_usec = 0, scr_tv_sec = 0, pts_tv_sec = 0;
	u64 cur_frame;
#endif
	spin_lock_irqsave(&media->lock, flags);

	sn = get_stream_number(media, ep);

	cur_frame = (u64) usb_gadget_frame_number(media->gadget);

	if (req->status) {
		put_request (media,req, sn);
		printk ("Media gadget Error received %x \n", cur_frame);
		printk("req->length = %d\n", req->length);
		printk("ep = %s\n",ep->name);
		spin_unlock_irqrestore(&media->lock, flags);
		return;
	}

#ifdef EN_TIMESTAMP
	cur_frame = (u64) usb_gadget_frame_number(media->gadget);
	if (cur_frame < 0)
		printk(KERN_INFO "cur_frame is neg\r\n");

	do_gettimeofday(&media->s[sn].scr_timeinfo);
	do_gettimeofday(&media->s[sn].pts_timeinfo);
	scr_tv_usec = (unsigned int)media->s[sn].scr_timeinfo.tv_usec;
	scr_tv_sec = (unsigned int)media->s[sn].scr_timeinfo.tv_sec;

	pts_tv_usec = (unsigned int)media->s[sn].pts_timeinfo.tv_usec;
	pts_tv_sec = (unsigned int)media->s[sn].pts_timeinfo.tv_sec;

#if 0
	temp3 = (scr_tv_sec * 1000000) + scr_tv_usec;
	temp3 |= ((cur_frame & 0x7ff)<<32);
#else
	temp3 = ((scr_tv_sec * 1000000) + scr_tv_usec) << 16;
	temp3 |= (cur_frame & 0x7ff);
#endif
	media->s[sn].uvc_mjpeg_header.scrSourceClock = temp3;
#endif
	if ((media->s[sn].eof_flag == 0) && (media->s[sn].u_size_remaining > 0)) {
		buf1 = req->buf + req->length; 
		put_request (media,req, sn);

		/* Batch the completed requests.  More efficient */
		if (media->s[sn].freereqcnt != MAX_UVC_URB)
			goto copyheader;

		media->s[sn].uvc_mjpeg_header.bmHeaderInfo |= EN_SCR_PTS;

		do {
			req = get_request (media, sn);
			if (!req)
				break;
		/* Update the header status if there is only one request needed
		 * to complete the video buffer transfer.
		 */
		if (media->s[sn].u_size_remaining <= media->s[sn].urb_data_size) {
			media->s[sn].urb_data_size = media->s[sn].u_size_remaining;
			if (!loopflg){
				media->s[sn].uvc_mjpeg_header.bmHeaderInfo |= MJPEG_H_EOF;
				media->s[sn].req_done = 1;
			}
		}

		length = media->s[sn].u_size_remaining >= media->s[sn].urb_data_size
				? media->s[sn].urb_data_size : media->s[sn].u_size_remaining;
	
		/* Prepare the USB request information for transfer */	
		if(!media->s[sn].u_size_current) {
			media->s[sn].uvc_mjpeg_header.bmHeaderInfo ^= 0x01;

#if 1 /* kernel virtual address hack */
//			media->s[sn].vmalloc_area = 0xd6000000 +

#if CONFIG_MTD_DM365_NOR
			media->s[sn].vmalloc_area = 0xc7000000 +
			media->s[sn].vmalloc_phys_area - 0x83c00000;
#else
			media->s[sn].vmalloc_area = 0xc5000000 +
			media->s[sn].vmalloc_phys_area - 0x83c00000;
#endif
#endif
			/* For the first transfer copy the UVC Header info
			 * in the 1K free region preceeding the Video data 
			 */
			buf1 = media->s[sn].vmalloc_area;
			media->s[sn].uvc_mjpeg_header.dwPresentationTime = pts_tv_usec;
			if (sn == 0)
				memcpy(buf1, &media->s[sn].uvc_mjpeg_header, URB_HEAD_SIZE);
			else if (sn == 1)
				memcpy(buf1, &media->s[sn].uvc_mpeg2ts_header, URB_HEAD_SIZE);
			
			req->buf = buf1;
			req->dma = (unsigned long)media->s[sn].vmalloc_phys_area;
		} else {
			req->buf = media->s[sn].vmalloc_area + media->s[sn].u_size_current;
			req->dma = (unsigned long)media->s[sn].vmalloc_phys_area + 
					media->s[sn].u_size_current;
		}

		req->length = length + URB_HEAD_SIZE;
		usb_ep_queue(media->s[sn].isoc_in, req, GFP_ATOMIC);
		media->s[sn].u_size_current += length;
		media->s[sn].u_size_remaining = media->s[sn].tLen - media->s[sn].u_size_current;
		loopflg = 1;
		}while (media->s[sn].u_size_remaining);
copyheader:
#if 0
		/* Copy the UVC data for the next transfer into the
		 * 1K preceeding the video buffer
		 */
		if (media->s[sn].u_size_current)
			memcpy(buf1, &media->s[sn].uvc_mjpeg_header, URB_HEAD_SIZE);
#endif
		if (media->s[sn].req_done) {
			/* Complete the current frame transfer */
			media->s[sn].eof_flag = 1;
			media->s[sn].req_done = 0;
			//media->s[sn].uvc_mjpeg_header.bmHeaderInfo ^= 0x01;
			media->s[sn].uvc_mjpeg_header.bmHeaderInfo &= ~MJPEG_H_EOF;
			media->s[sn].urb_data_size = ((T_BUF_SIZE_VID) - (URB_HEAD_SIZE));
		}

	} else if (media->s[sn].freereqcnt == (MAX_UVC_URB - 1)){
		media->s[sn].uvc_mjpeg_header.dwPresentationTime = pts_tv_usec;

		/* Submit the UVC header alone as no video data is available */
		if (sn == 0)
			memcpy(media->s[sn].tBuf, &media->s[sn].uvc_mjpeg_header, URB_HEAD_SIZE);
		else if (sn == 1)
			memcpy(media->s[sn].tBuf, &media->s[sn].uvc_mpeg2ts_header, URB_HEAD_SIZE);

		req->buf = media->s[sn].tBuf;
		req->dma = 0;
		req->length = URB_HEAD_SIZE;
		usb_ep_queue(media->s[sn].isoc_in, req, GFP_ATOMIC);
	} else {
		/* Continue writting the UVC header for the USB requests that
		 * are already submitted.  The Video buffer processing is 
		 * complete from the USB perspective but need to wait intil
		 * the last queued USB request before accepting another frame 
		 * in.  This implementation can be improved further thus
		 * enabling aceptance of addtional frames while the current
		 * frame is being processed by USB subsystem.
		 */
		buf1 = req->buf + req->length - URB_HEAD_SIZE; 
		put_request(media,req, sn);
		if ((MAX_UVC_URB - 1) == media->s[sn].freereqcnt)
			media->s[sn].uvc_mjpeg_header.bmHeaderInfo |= MJPEG_H_EOF;

		media->s[sn].uvc_mjpeg_header.scrSourceClock = temp3;

		if (sn == 0)
			memcpy(buf1, &media->s[sn].uvc_mjpeg_header, URB_HEAD_SIZE);
		else if (sn == 1)
			memcpy(buf1, &media->s[sn].uvc_mpeg2ts_header, URB_HEAD_SIZE);

		if ((MAX_UVC_URB - 1) == media->s[sn].freereqcnt){
			media->s[sn].uvc_mjpeg_header.bmHeaderInfo ^= 0x01;
			media->s[sn].uvc_mjpeg_header.bmHeaderInfo &= ~MJPEG_H_EOF;
			media->s[sn].eof_flag = 1;
			media->s[sn].req_done = 0;
			media->s[sn].urb_data_size = ((T_BUF_SIZE_VID) - (URB_HEAD_SIZE));
		}
	}
		
	spin_unlock_irqrestore(&media->lock, flags);
}

static int media_ioctl(struct inode *inode, struct file *fp, unsigned int cmd,
                     unsigned long arg);

static int uvc_start_stream(struct media_g_dev *media,	u8 sn)
{
	struct usb_request *req;
	struct usb_gadget *app_gadget = media->gadget;
#ifdef EN_TIMESTAMP
	__u64 temp3 = 0;
	unsigned int scr_tv_usec = 0, pts_tv_usec = 0, scr_tv_sec = 0, pts_tv_sec = 0;
	u64 cur_frame;
#endif
	unsigned long flags;
        /* isoc-in enqueue*/
	if (sn == 0)
        	usb_ep_enable(media->s[sn].isoc_in,&isoc_in_ep_descriptor_1_1);
#ifdef DUAL_STREAM
	else if (sn == 1)
        	usb_ep_enable(media->s[sn].isoc_in,&ds_isoc_in_ep_descriptor);
#endif

	spin_lock_irqsave(&media->lock, flags);
	media->s[sn].uvc_mjpeg_header.bHeaderLength = URB_HEAD_SIZE;
	media->s[sn].uvc_mjpeg_header.bmHeaderInfo |= EN_SCR_PTS;

	media->s[sn].eof_flag = 1;

#ifdef EN_TIMESTAMP
	cur_frame = (u64) usb_gadget_frame_number(app_gadget);
	if (cur_frame < 0)
		printk(KERN_INFO "cur_frame is neg\r\n");

	do_gettimeofday(&media->s[sn].scr_timeinfo);
	scr_tv_usec = (unsigned int)media->s[sn].scr_timeinfo.tv_usec;
	scr_tv_sec = (unsigned int)media->s[sn].scr_timeinfo.tv_sec;

	pts_tv_usec = (unsigned int)media->s[sn].pts_timeinfo.tv_usec;
	pts_tv_sec = (unsigned int)media->s[sn].pts_timeinfo.tv_sec;
	temp3 = (scr_tv_sec * 1000000) + scr_tv_usec;
	temp3 |= ((cur_frame & 0x7ff)<<32);

	media->s[sn].uvc_mjpeg_header.scrSourceClock = temp3;
	media->s[sn].uvc_mjpeg_header.dwPresentationTime = pts_tv_usec;
#endif

	/* Start the UVC transfer.  Initiate the Video frame transfer from
	 * complete function.  In this way we centralize the handling of
	 * video frames.
	 */
	req = get_request (media, sn);
	if (sn == 0)
		memcpy(media->s[sn].tBuf, &media->s[sn].uvc_mjpeg_header, URB_HEAD_SIZE);
	else if (sn == 1)
		memcpy(media->s[sn].tBuf, &media->s[sn].uvc_mpeg2ts_header, URB_HEAD_SIZE);
	req->buf = media->s[sn].tBuf;
	req->length = URB_HEAD_SIZE;
	usb_ep_queue(media->s[sn].isoc_in, req, GFP_ATOMIC);
	spin_unlock_irqrestore(&media->lock, flags);
	return 0;
}

static int standard_setup_req(struct media_g_dev *media,
		const struct usb_ctrlrequest *ctrl)
{
	struct usb_request	*req = media->ep0req;
	int			value = -EOPNOTSUPP;
	u16			wIndex = le16_to_cpu(ctrl->wIndex);
	u16			wValue = le16_to_cpu(ctrl->wValue);
	u16			wLength = le16_to_cpu(ctrl->wLength);

	/* Usually this just stores reply data in the pre-allocated ep0 buffer,
	 * but config change events will also reconfigure hardware. */
	switch (ctrl->bRequest) {

	case USB_REQ_GET_DESCRIPTOR:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_STANDARD |
				USB_RECIP_DEVICE))
			break;
		switch (wValue >> 8) {

		case USB_DT_DEVICE:
			value = min(wLength, (u16) sizeof device_desc);
			memcpy(req->buf, &device_desc, value);
			break;
		case USB_DT_DEVICE_QUALIFIER:
			if (!media->gadget->is_dualspeed)
				break;
			value = min(wLength, (u16) sizeof dev_qualifier);
			memcpy(req->buf, &dev_qualifier, value);
			break;

		case USB_DT_OTHER_SPEED_CONFIG:
			if (!media->gadget->is_dualspeed)
				break;
			goto get_config;
		case USB_DT_CONFIG:
		get_config:
			value = populate_config_buf(media->gadget,
					req->buf,
					wValue >> 8,
					wValue & 0xff);
			if (value >= 0) {
				value = min(wLength, (u16) value);
			} else
				printk("error in get config desc(%d)....\n",value);
			break;

		case USB_DT_STRING:
			/* wIndex == language code */
			value = usb_gadget_get_string(&stringtab,
					wValue & 0xff, req->buf);
			if (value >= 0)
				value = min(wLength, (u16) value);
			break;
		}
		break;

	/* One config, two speeds */
	case USB_REQ_SET_CONFIGURATION:
		if (ctrl->bRequestType != (USB_DIR_OUT | USB_TYPE_STANDARD |
				USB_RECIP_DEVICE))
			break;
		value = 0;
		printk("set configuration\n");
		break;
	case USB_REQ_GET_CONFIGURATION:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_STANDARD |
				USB_RECIP_DEVICE))
			break;
		*(u8 *) req->buf = media->config;
		value = min(wLength, (u16) 1);
		break;

	case USB_REQ_SET_INTERFACE:
		if (ctrl->bRequestType != (USB_DIR_OUT| USB_TYPE_STANDARD |
				USB_RECIP_INTERFACE))
			break;
		value = 0;
		printk("set interface: %d/%d\n",ctrl->wIndex,ctrl->wValue);
		if (ctrl->wValue ==1 && ctrl->wIndex == 1) {
			uvc_start_stream(media, 0);
		} else if (wIndex == 1 && wValue == 0){
			usb_ep_disable(media->s[0].isoc_in);
#ifdef DUAL_STREAM
		} else if (ctrl->wValue ==1 && ctrl->wIndex == VS_ISOC_IN_INTERFACE2) {
			uvc_start_stream(media, (u8)(ctrl->wIndex-1));
		} else if (wIndex == VS_ISOC_IN_INTERFACE2 && wValue == 0){
			usb_ep_disable(media->s[(u8)(wIndex-1)].isoc_in);
#endif
#ifdef BIDIR_STREAM
		} else if (ctrl->wValue ==1 && ctrl->wIndex == VS_ISOC_OUT_INTERFACE) {
			usb_ep_enable(media->s[(u8)(ctrl->wIndex-1)].isoc_out, &bds_isoc_out_ep_descriptor);
		} else if (wIndex == VS_ISOC_OUT_INTERFACE && wValue == 0){
			//usb_ep_disable(media->s[(u8)(wIndex-1)].isoc_out);
#endif

#ifdef UAC
		} else if (wIndex == AS_ISOC_IN_INTERFACE && wValue == 1) {
                        uac_g_start_stream(&media->g_uac_dev);
                        uac_do_write(&media->g_uac_dev, media->g_uac_dev.aud_data,
                                            media->g_uac_dev.bfLen);
		} else if (wIndex == AS_ISOC_IN_INTERFACE && wValue == 0) {
			usb_ep_disable(media->g_uac_dev.iso_in);	
#endif
		}
		break;
	case USB_REQ_GET_INTERFACE:
		if (ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_STANDARD |
				USB_RECIP_INTERFACE))
			break;
		if (!media->config)
			break;
		if (wIndex != 0) {
			value = -EDOM;
			break;
		}
		printk("get interface\n");
		*(u8 *) req->buf = 0;
		value = min(wLength, (u16) 1);
		break;

	default:
		printk("unknown control req %02x.%02x v%04x i%04x l%u\n",
			ctrl->bRequestType, ctrl->bRequest,
			wValue, wIndex, wLength);
	}

	return value;
}

/*
 * The setup() callback implements all the ep0 functionality that's
 * not handled lower down, in hardware or the hardware driver (like
 * device and endpoint feature flags, and their status).  It's all
 * housekeeping for the gadget function we're implementing.  Most of
 * the work is in config-specific setup.
 */
static int
media_setup (struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl)
{
	struct media_g_dev		*media = get_gadget_data(gadget);
	int			rc = -1;
	u16			wLength = le16_to_cpu(ctrl->wLength);
    	u16			 wIndex = le16_to_cpu(ctrl->wIndex);
	u8			bmInterface = wIndex & 0xFF;

	media->ep0req->context = NULL;
	media->ep0req->length = 0;
	
	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS) {
#ifdef UAC
		if (bmInterface < (UVC_DEVICE_NR_INTF - 2))
#else
		if (bmInterface < UVC_DEVICE_NR_INTF)
#endif
			rc = uvc_class_setup_req(media, ctrl);
#ifdef UAC
		else if (bmInterface < (UVC_DEVICE_NR_INTF))
			rc = uac_class_setup_req(&media->g_uac_dev, ctrl);
#endif
		else
			printk("Class request for invalid interface\n");	
	} else {
		rc = standard_setup_req(media, ctrl);
	}

	/* Respond with data/status or defer until later? */
	if (rc >= 0 && rc != DELAYED_STATUS) {
		media->ep0req->length = rc;
		media->ep0req->zero = (rc < wLength &&
				(rc % gadget->ep0->maxpacket) == 0);
		rc = ep0_queue(media);
	}

	/* Device either stalls (rc < 0) or reports success */
	return rc;
}


static void
media_disconnect (struct usb_gadget *gadget)
{
	printk("media_disconnect()\n");
}

static void ep0_complete(struct usb_ep *ep, struct usb_request *req)
{
	if (req->status || req->actual != req->length)
		printk("ep0_complete --> %d, %d/%d\n",
				req->status, req->actual, req->length);
}

/*-------------------------------------------------------------------------*/
static void alloc_uvc_video_buffer(struct media_g_dev *media, u8 sn)
{
	int i;
	printk("alloc_uvc_video_buffer\n");

	media->s[sn].uvc_buffer[0].kvaddr = (unsigned int)kmalloc(2 * UVC_BUFFER_LEN, GFP_KERNEL);
	media->s[sn].uvc_buffer[0].offset = virt_to_phys((void*)media->s[sn].uvc_buffer[0].kvaddr);

	printk("kmalloc: va=%x,pas=%x\n",media->s[sn].uvc_buffer[0].kvaddr,media->s[sn].uvc_buffer[0].offset);
	media->s[sn].uvc_buffer[1].kvaddr = media->s[sn].uvc_buffer[0].kvaddr + UVC_BUFFER_LEN;
	media->s[sn].uvc_buffer[1].offset = media->s[sn].uvc_buffer[0].offset + UVC_BUFFER_LEN;

	for (i=0 ; i <UVC_NUM_BUFS ;i++) {
		media->s[sn].uvc_buffer[i].size = UVC_BUFFER_LEN;
		media->s[sn].uvc_buffer[i].index = i;
		/* update bufinfo */
		media->s[sn].bufinfo.vaddr[i] = media->s[sn].uvc_buffer[i].kvaddr;
		media->s[sn].bufinfo.paddr[i] = media->s[sn].uvc_buffer[i].offset;

		if (i == 0) {
                        media->s[sn].vmalloc_area = (unsigned char*)media->s[sn].uvc_buffer[i].kvaddr;
                        media->s[sn].vmalloc_phys_area = (unsigned char*)media->s[sn].uvc_buffer[i].offset;
		}
	}
}
static void free_uvc_video_buffer(struct media_g_dev *media, u8 sn)
{
	/* freeup buffer allocated */
	kfree((void*)media->s[sn].uvc_buffer[0].kvaddr);
}
#ifdef UAC
static void free_uac_audio_buffer(struct media_g_dev *media)
{
        /* freeup buffer allocated */
        kfree((void*)media->g_uac_dev.uac_buffer[0].kvaddr);
}
#endif

static void
media_unbind (struct usb_gadget *gadget)
{
	struct media_g_dev		*media = get_gadget_data (gadget);
	struct usb_request	*req = media->ep0req;
	/* Free the request and buffer for endpoint 0 */
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(media->ep0, req);
	}
	set_gadget_data(gadget, NULL);
	
	free_all_req(media);

#ifdef UAC
	free_uac_audio_buffer(media);
#endif
	if(media)
		kfree(media);
}

static int
media_bind (struct usb_gadget *gadget)
{
	struct media_g_dev		*media;
	struct usb_ep		*ep;
	struct usb_request	*req;
	int i, j;

	/* alloc mem for media */
	media = kzalloc(sizeof *media, GFP_KERNEL);
	if (!media)
		return -ENOMEM;

	/* global uvc-dev init */
	gb_media = media;
	gb_mediauvcuac = media;

	/* init */
	media->use_mmap[0] = 0;
#ifdef DUAL_STREAM
	media->use_mmap[1] = 0;
#endif
	media->cur_strm = 0;
	/* init pu_controls */
	media->pu_ctrl.brgt.info = 7;
	media->pu_ctrl.brgt.len = 2;
	media->pu_ctrl.brgt.min = 0;
	media->pu_ctrl.brgt.max = 100;  
	media->pu_ctrl.brgt.res = 5;
	media->pu_ctrl.brgt.def = 25;
	media->pu_ctrl.brgt.cur = 25;

	media->pu_ctrl.cnst.info = 7;
	media->pu_ctrl.cnst.len = 2;
	media->pu_ctrl.cnst.min = 0;
	media->pu_ctrl.cnst.max = 100;  
	media->pu_ctrl.cnst.res = 5;
	media->pu_ctrl.cnst.def = 25;
	media->pu_ctrl.cnst.cur = 25;

	media->pu_ctrl.hue.info = 7;
	media->pu_ctrl.hue.len = 2;
	media->pu_ctrl.hue.min = 0;
	media->pu_ctrl.hue.max = 360; 
	media->pu_ctrl.hue.res = 40;
	media->pu_ctrl.hue.def = 280;
	media->pu_ctrl.hue.cur = 280;

	media->pu_ctrl.satr.info = 7;
	media->pu_ctrl.satr.len = 2;
	media->pu_ctrl.satr.min = 0;
	media->pu_ctrl.satr.max = 100;  
	media->pu_ctrl.satr.res = 5;
	media->pu_ctrl.satr.def = 50;
	media->pu_ctrl.satr.cur = 50;

   media->pu_ctrl.wbt.info = 7;
	media->pu_ctrl.wbt.len = 2;
	media->pu_ctrl.wbt.min = 1000;
	media->pu_ctrl.wbt.max = 11000;
	media->pu_ctrl.wbt.res = 500;
	media->pu_ctrl.wbt.def = 5000;
	media->pu_ctrl.wbt.cur = 5000;

   media->pu_ctrl.wbt_auto.info = 7;
	media->pu_ctrl.wbt_auto.len = 2;
	media->pu_ctrl.wbt_auto.min = 0;  
	media->pu_ctrl.wbt_auto.max = 1; 
	media->pu_ctrl.wbt_auto.res = 1;
	media->pu_ctrl.wbt_auto.def = 1;
	media->pu_ctrl.wbt_auto.cur = 1;

   media->pu_ctrl.blc.info = 7;  //qvalue
	media->pu_ctrl.blc.len = 2;
	media->pu_ctrl.blc.min = 0;  
	media->pu_ctrl.blc.max = 100; 
	media->pu_ctrl.blc.res = 5;
	media->pu_ctrl.blc.def = 90;
	media->pu_ctrl.blc.cur = 90;
 
   media->pu_ctrl.gain.info =7; //bitrate
	media->pu_ctrl.gain.len = 2;
	media->pu_ctrl.gain.min = 0;   //*1000
	media->pu_ctrl.gain.max = 3000; //*1000
	media->pu_ctrl.gain.res = 100;
	media->pu_ctrl.gain.def = 2000; //*1000
	media->pu_ctrl.gain.cur = 2000; //*1000

   media->pu_ctrl.gamma.info =7; 
	media->pu_ctrl.gamma.len = 2;
	media->pu_ctrl.gamma.min = 1;   
	media->pu_ctrl.gamma.max = 6; 
	media->pu_ctrl.gamma.res = 1;
	media->pu_ctrl.gamma.def = 2; 
	media->pu_ctrl.gamma.cur = 2;

   media->pu_ctrl.anti_flicker.info=7;
	media->pu_ctrl.anti_flicker.len=2;
	media->pu_ctrl.anti_flicker.min=1;
	media->pu_ctrl.anti_flicker.max=2;
	media->pu_ctrl.anti_flicker.res=1;
	media->pu_ctrl.anti_flicker.def=1;
	media->pu_ctrl.anti_flicker.cur=1; 
   /*** init ct_contrls  *****/
   media->ct_ctrl.exposure_auto.info=7;       //exposure auto
	media->ct_ctrl.exposure_auto.len=2;
	media->ct_ctrl.exposure_auto.min=0;         
	media->ct_ctrl.exposure_auto.max=1;       
   media->ct_ctrl.exposure_auto.res=1;
	media->ct_ctrl.exposure_auto.def=1;       
	media->ct_ctrl.exposure_auto.cur=1;

   media->ct_ctrl.exposure_absolute.info=7;       //exposure
	media->ct_ctrl.exposure_absolute.len=2;
	media->ct_ctrl.exposure_absolute.min=10;         //*100
	media->ct_ctrl.exposure_absolute.max=8000;        //*100
   media->ct_ctrl.exposure_absolute.res=1;
	media->ct_ctrl.exposure_absolute.def=1000;         //*100
	media->ct_ctrl.exposure_absolute.cur=1000;

	/* init mjpeg header */
	media->s[0].uvc_mjpeg_header.bHeaderLength = 0x02;
	media->s[0].uvc_mjpeg_header.bmHeaderInfo = 0x80;
	media->s[0].uvc_mjpeg_header.dwPresentationTime = 0x00000000;
	media->s[0].uvc_mjpeg_header.scrSourceClock = 0x000000000000;

	media->s[0].uvc_mpeg2ts_header.bHeaderLength = 0x0C;
	media->s[0].uvc_mpeg2ts_header.bmHeaderInfo = 0x80;
	media->s[0].uvc_mpeg2ts_header.dwPresentationTime = 0x00000000;
	media->s[0].uvc_mpeg2ts_header.scrSourceClock = 0x000000000000;

#ifdef DUAL_STREAM
	media->s[1].uvc_mjpeg_header.bHeaderLength = 0x02;
	media->s[1].uvc_mjpeg_header.bmHeaderInfo = 0x80;
	media->s[1].uvc_mjpeg_header.dwPresentationTime = 0x00000000;
	media->s[1].uvc_mjpeg_header.scrSourceClock = 0x000000000000;

	media->s[1].uvc_mpeg2ts_header.bHeaderLength = 0x0C;
	media->s[1].uvc_mpeg2ts_header.bmHeaderInfo = 0x80;
	media->s[1].uvc_mpeg2ts_header.dwPresentationTime = 0x00000000;
	media->s[1].uvc_mpeg2ts_header.scrSourceClock = 0x000000000000;
#endif

	/* init streaming control structures */
	init_streaming_control(media, 0);
	/* update commit control */
	memcpy(&media->s[0].cur_vs_state, &media->s[0].uvc_prob_comm_ds, 26);
	media->s[0].cur_vs_state.bFrameIndex = media->s[0].uvc_prob_comm_ds.bFrameIndex;

#if defined(DUAL_STREAM) && defined(BIDIR_STREAM)
	init_streaming_control(media, 1);
	memcpy(&media->s[1].cur_vs_state, &media->s[1].uvc_prob_comm_ds, 26);
	media->s[1].cur_vs_state.bFrameIndex = media->s[1].uvc_prob_comm_ds.bFrameIndex;
	init_streaming_control(media, 2);
	memcpy(&media->s[2].cur_vs_state, &media->s[2].uvc_prob_comm_ds, 26);
	media->s[2].cur_vs_state.bFrameIndex = media->s[2].uvc_prob_comm_ds.bFrameIndex;
#elif defined(DUAL_STREAM) || defined(BIDIR_STREAM)
	init_streaming_control(media, 1);
	memcpy(&media->s[1].cur_vs_state, &media->s[1].uvc_prob_comm_ds, 26);
	media->s[1].cur_vs_state.bFrameIndex = media->s[1].uvc_prob_comm_ds.bFrameIndex;
#endif
	/* lock init */
	spin_lock_init(&media->lock);

#ifdef UAC
	 spin_lock_init(&media->g_uac_dev.lock);

	/* allocate buffers */
	 alloc_uac_audio_buffer(&media->g_uac_dev);

	media->g_uac_dev.len_remaining = 0;
        media->g_uac_dev.eof_flag = 1;

#endif

#ifdef UAVC_CLASS_REQ_SUPPORT
	init_MUTEX_LOCKED(&media->read_sem);
	init_MUTEX(&media->class_req_sem);
	memset(media->class_req, 0xAB, sizeof(media->class_req));
#endif

	media->s[0].req_done = 0;
	media->s[0].u_size_remaining = media->s[0].tLen = 0;
	media->s[0].u_size_current = 0;
	media->s[0].eof_flag = 0;
	media->s[0].urb_data_size = ((T_BUF_SIZE_VID) - (URB_HEAD_SIZE));

	media->s[1].req_done = 0;
	media->s[1].u_size_remaining = media->s[1].tLen = 0;
	media->s[1].u_size_current = 0;
	media->s[1].eof_flag = 0;
	media->s[1].urb_data_size = ((T_BUF_SIZE_VID) - (URB_HEAD_SIZE));

	/* identifier for video buffer */
    	media->s[0].tBuf[0] = 2;
    	media->s[1].tBuf[0] = 2;

	media->gadget = gadget;
	set_gadget_data(gadget, media);
	media->ep0 = gadget->ep0;
	media->ep0->driver_data = media;
	
	/* update vc_header_descriptor */
	for (i=0; i < VC_HEADER_NR_VS ; i++)
		vc_header_descriptor.baInterfaceNr[i] = vc_header_intf_nr[i];
	vc_header_descriptor.wTotalLength = sizeof vc_header_descriptor
						+ sizeof cam_terminal_descriptor
						+ sizeof output_terminal_descriptor
#ifdef DUAL_STREAM
						+ sizeof ds_output_terminal_descriptor
#endif
						+ sizeof selector_unit_descriptor
						+ sizeof processing_unit_descriptor
#ifdef BIDIR_STREAM
						+ sizeof input_terminal_descriptor
						+ sizeof lcd_terminal_descriptor
#endif
						+ sizeof extension_unit_desciptor;
	/* update cam_terminal_descriptor */
	for (i=0; i < VC_CAM_TERM_CONTROL_SIZE ; i++)
		cam_terminal_descriptor.bmControls[i] = vc_cam_term_control[i];
	/* update selector_unit_descriptor */
	for (i=0; i < VC_SEL_UNIT_NR_IN_PINS ; i++)
		selector_unit_descriptor.baSourceID[i] = vc_sel_unit_src_id[i];
	/* update processing_unit_descriptor */
	for (i=0; i < VC_PRO_UNIT_CONTROL_SIZE ; i++){
		printk("--------bmControls[%d] =%d--------\n",i,processing_unit_descriptor.bmControls[i]);
		processing_unit_descriptor.bmControls[i] = vc_pro_unit_control[i];}
	/* update extension_unit_desciptor */
	for (i=0; i < VC_EXT_UNIT_NR_IN_PINS ; i++)
		extension_unit_desciptor.baSourceID[i] = vc_ext_unit_src_id[i];
	for (i=0; i < VC_EXT_UNIT_CONTROL_SIZE ; i++)
		extension_unit_desciptor.bmControls[i] = vc_ext_unit_control[i];
	for (i=0; i < 4 ; i++)
		extension_unit_desciptor.guidExtensionCode[i] = vc_ext_unit_guid[i];
	/* update input_header_descriptor */
	for (i=0; i < INPUT_HEADER_NR_FORMART ; i++)
		for (j=0; j < INPUT_HEADER_CONTROL_SIZE ; j++)
			input_header_descriptor.bmaControls[i][j] = input_header_control[i][j];

#if UVC_YUV
	input_header_descriptor.wTotalLength = sizeof input_header_descriptor
						+ sizeof format_yuv_descriptor 
						+ sizeof frame_yuv_720p_descriptor; 


#else
	input_header_descriptor.wTotalLength = sizeof input_header_descriptor
						+ sizeof format_mjpeg_descriptor 
						+ sizeof frame_mjpeg_descriptor[0] 
						+ sizeof frame_mjpeg_descriptor[1] 
						+ sizeof frame_mjpeg_descriptor[2]
						+ sizeof frame_mjpeg_descriptor[3];
						//+ sizeof format_mpeg2ts_descriptor
						//+ sizeof color_matching_descriptor;

	/* form usb_vs_frame_mjpeg_descriptor */
	for (i=0; i < VS_F_NR_DESC ; i++) {
		frame_mjpeg_descriptor[i].bLength = sizeof (struct usb_vs_frame_mjpeg_descriptor);
		frame_mjpeg_descriptor[i].bDescriptorType = USB_CS_INTERFACE;
		frame_mjpeg_descriptor[i].bDescriptorSubType = USB_VS_FRAME_MJPEG;
		frame_mjpeg_descriptor[i].bFrameIndex = vs_frame_mjpeg_frame_index[i];
		frame_mjpeg_descriptor[i].bmCapabilities = vs_frame_mpjeg_capabilities[i];
		frame_mjpeg_descriptor[i].wWidth = vs_frame_mjpeg_width[i];
		frame_mjpeg_descriptor[i].wHeight = vs_frame_mjpeg_height[i];
		frame_mjpeg_descriptor[i].dwMinBitRate = vs_frame_mjpeg_min_bitrate[i];
		frame_mjpeg_descriptor[i].dwMaxBitRate = vs_frame_mjpeg_max_bitrate[i];
		frame_mjpeg_descriptor[i].dwMaxVideoFrameBufferSize = vs_frame_mjpeg_max_video_frame_bufsize[i];
		frame_mjpeg_descriptor[i].dwDeafultFrameInterval = vs_frame_mjpeg_max_def_frame_interval[i];
		frame_mjpeg_descriptor[i].bFrameIntervalType = vs_frame_mjpeg_frame_interval_type[i];
		frame_mjpeg_descriptor[i].dwMinFrameInterval = vs_frame_mjpeg_min_frame_interval[i];
		frame_mjpeg_descriptor[i].dwMaxFrameInterval = vs_frame_mjpeg_max_frame_interval[i];
		frame_mjpeg_descriptor[i].dwFrameIntervalStep = vs_frame_mjpeg_frame_interval_step[i];
	}
#endif

#ifdef DUAL_STREAM
        /* update ds_input_header_descriptor */
        for (i=0; i < INPUT_HEADER_NR_FORMART ; i++)
                for (j=0; j < INPUT_HEADER_CONTROL_SIZE ; j++)
                        ds_input_header_descriptor.bmaControls[i][j] = input_header_control[i][j];
        ds_input_header_descriptor.wTotalLength = sizeof ds_input_header_descriptor
					#if 0
                                                + sizeof ds_format_mjpeg_descriptor
                                                + sizeof ds_frame_mjpeg_descriptor[0]
                                                + sizeof ds_frame_mjpeg_descriptor[1]
                                                + sizeof ds_frame_mjpeg_descriptor[2]
                                                + sizeof ds_frame_mjpeg_descriptor[3]
					#endif
                                                + sizeof ds_format_mpeg2ts_descriptor;
                                                //+ sizeof ds_color_matching_descriptor;

	/* form usb_vs_frame_mjpeg_descriptor */
	for (i=0; i < VS_F_NR_DESC ; i++) {
		ds_frame_mjpeg_descriptor[i].bLength = sizeof (struct usb_vs_frame_mjpeg_descriptor);
		ds_frame_mjpeg_descriptor[i].bDescriptorType = USB_CS_INTERFACE;
		ds_frame_mjpeg_descriptor[i].bDescriptorSubType = USB_VS_FRAME_MJPEG;
		ds_frame_mjpeg_descriptor[i].bFrameIndex = vs_frame_mjpeg_frame_index[i];
		ds_frame_mjpeg_descriptor[i].bmCapabilities = vs_frame_mpjeg_capabilities[i];
		ds_frame_mjpeg_descriptor[i].wWidth = vs_frame_mjpeg_width[i];
		ds_frame_mjpeg_descriptor[i].wHeight = vs_frame_mjpeg_height[i];
		ds_frame_mjpeg_descriptor[i].dwMinBitRate = vs_frame_mjpeg_min_bitrate[i];
		ds_frame_mjpeg_descriptor[i].dwMaxBitRate = vs_frame_mjpeg_max_bitrate[i];
		ds_frame_mjpeg_descriptor[i].dwMaxVideoFrameBufferSize = vs_frame_mjpeg_max_video_frame_bufsize[i];
		ds_frame_mjpeg_descriptor[i].dwDeafultFrameInterval = vs_frame_mjpeg_max_def_frame_interval[i];
		ds_frame_mjpeg_descriptor[i].bFrameIntervalType = vs_frame_mjpeg_frame_interval_type[i];
		ds_frame_mjpeg_descriptor[i].dwMinFrameInterval = vs_frame_mjpeg_min_frame_interval[i];
		ds_frame_mjpeg_descriptor[i].dwMaxFrameInterval = vs_frame_mjpeg_max_frame_interval[i];
		ds_frame_mjpeg_descriptor[i].dwFrameIntervalStep = vs_frame_mjpeg_frame_interval_step[i];
	}
#endif

#ifdef BIDIR_STREAM
	bds_output_header_descriptor.bmaControls[0] = 0x0;
	bds_output_header_descriptor.wTotalLength = sizeof bds_output_header_descriptor
						+ sizeof bds_format_mjpeg_descriptor
						+ sizeof bds_frame_mjpeg_descriptor[0];
	/* form usb_vs_frame_mjpeg_descriptor */
	for (i=0; i < 1 ; i++) {
		bds_frame_mjpeg_descriptor[i].bLength = sizeof (struct usb_vs_frame_mjpeg_descriptor);
		bds_frame_mjpeg_descriptor[i].bDescriptorType = USB_CS_INTERFACE;
		bds_frame_mjpeg_descriptor[i].bDescriptorSubType = USB_VS_FRAME_MJPEG;
		bds_frame_mjpeg_descriptor[i].bFrameIndex = vs_frame_mjpeg_frame_index[i];
		bds_frame_mjpeg_descriptor[i].bmCapabilities = vs_frame_mpjeg_capabilities[i];
		bds_frame_mjpeg_descriptor[i].wWidth = 320;
		bds_frame_mjpeg_descriptor[i].wHeight = 240;
		bds_frame_mjpeg_descriptor[i].dwMinBitRate = vs_frame_mjpeg_min_bitrate[i];
		bds_frame_mjpeg_descriptor[i].dwMaxBitRate = vs_frame_mjpeg_max_bitrate[i];
		bds_frame_mjpeg_descriptor[i].dwMaxVideoFrameBufferSize = vs_frame_mjpeg_max_video_frame_bufsize[i];
		bds_frame_mjpeg_descriptor[i].dwDeafultFrameInterval = vs_frame_mjpeg_max_def_frame_interval[i];
		bds_frame_mjpeg_descriptor[i].bFrameIntervalType = vs_frame_mjpeg_frame_interval_type[i];
		bds_frame_mjpeg_descriptor[i].dwMinFrameInterval = vs_frame_mjpeg_min_frame_interval[i];
		bds_frame_mjpeg_descriptor[i].dwMaxFrameInterval = vs_frame_mjpeg_max_frame_interval[i];
		bds_frame_mjpeg_descriptor[i].dwFrameIntervalStep = vs_frame_mjpeg_frame_interval_step[i];
	}
#endif

	/* Find all the endpoints we will use */
	usb_ep_autoconfig_reset(gadget);

	/* config for interrupt in endpoint */
	ep = usb_ep_autoconfig(gadget, &int_in_ep_descriptor);
	if (!ep)
		goto autoconf_fail;
	ep->driver_data = media;
	media->int_in = ep;

	/* config for isoc in endpoint */
	ep = usb_ep_autoconfig(gadget, &isoc_in_ep_descriptor_1_1);
	if (!ep)
		goto autoconf_fail;
	ep->driver_data = media;
	media->s[0].isoc_in = ep;
	input_header_descriptor.bEndpointAddress = isoc_in_ep_descriptor_1_1.bEndpointAddress;

#ifdef DUAL_STREAM
	/* config for isoc in endpoint */
	ep = usb_ep_autoconfig(gadget, &ds_isoc_in_ep_descriptor);
	if (!ep)
		goto autoconf_fail;
	ep->driver_data = media;
	media->s[1].isoc_in = ep;
	ds_input_header_descriptor.bEndpointAddress =
		 ds_isoc_in_ep_descriptor.bEndpointAddress;
#endif
#ifdef BIDIR_STREAM
	/* config for isoc out endpoint */
	ep = usb_ep_autoconfig(gadget, &bds_isoc_out_ep_descriptor);
	if (!ep)
		goto autoconf_fail;
	ep->driver_data = media;
	media->s[0].isoc_out = ep;
	bds_output_header_descriptor.bEndpointAddress =
		 bds_isoc_out_ep_descriptor.bEndpointAddress;
#endif
#ifdef UAC
	ep = usb_ep_autoconfig(gadget, &as_iso_in_ep_desc0);
        if (!ep)
                goto autoconf_fail;
        ep->driver_data = media;
        media->g_uac_dev.iso_in = ep;
	ac_cs_intf_header.wTotalLength = sizeof ac_input_term_desc0
					+ sizeof ac_output_term_desc0
					+ sizeof ac_cs_intf_header;
	ep->driver_data = media;	
#endif
	/* Fix up the descriptors */
	device_desc.bMaxPacketSize0 = media->ep0->maxpacket;

	/* Assume ep0 uses the same maxpacket value for both speeds */
	dev_qualifier.bMaxPacketSize0 = media->ep0->maxpacket;

	/* Allocate the request and buffer for endpoint 0 */
	media->ep0req = req = usb_ep_alloc_request(media->ep0, GFP_KERNEL);
	if (!req)
		goto out;
	req->buf = kmalloc(EP0_BUFSIZE, GFP_KERNEL);
	if (!req->buf)
		goto out;
	req->complete = ep0_complete;

	/* This should reflect the actual gadget power source */
	usb_gadget_set_selfpowered(gadget);

	
	/* Allocate the USB requests */
	INIT_LIST_HEAD(&media->s[0].urblist);

	do {
		req = usb_ep_alloc_request(media->s[0].isoc_in, GFP_ATOMIC);
		if (!req) {
			printk(KERN_WARNING "req %d is not allocated!!\r\n",
				media->s[0].freereqcnt);
			return -ENOMEM;
		}

		req->complete = uvc_tx_submit_complete;
		req->dma = (unsigned long)NULL;
		put_request (media,req, 0);
	} while (media->s[0].freereqcnt < MAX_UVC_URB);

#ifdef DUAL_STREAM
	INIT_LIST_HEAD(&media->s[1].urblist);

	do {
		req = usb_ep_alloc_request(media->s[1].isoc_in, GFP_ATOMIC);
		if (!req) {
			printk(KERN_WARNING "req %d is not allocated!!\r\n",
				media->s[1].freereqcnt);
			return -ENOMEM;
		}

		req->complete = uvc_tx_submit_complete;
		req->dma = (unsigned long)NULL;
		put_request (media,req, 1);
	} while (media->s[1].freereqcnt < MAX_UVC_URB);
#endif
	printk("media_bind()\n");
	return 0;
	
autoconf_fail:
	printk("unable to autoconfigure all endpoints\n");
	return -ENOTSUPP;
out:
	media_unbind(gadget);
	return -ENOMEM;
}
#ifdef UAC
int usb_audio_submit(struct uac_g_dev *g_uac_dev,u8 *buf, int len)
{
	struct usb_request *req = g_uac_dev->req;

    /*This function does the handling of UAC_SUBMIT */
    g_uac_dev->eof_flag = 0;
    g_uac_dev->aud_data = buf;
    g_uac_dev->bfLen = len;

	if((g_uac_dev->req_done1 == 1)&&(g_uac_dev->strm_started1 ==1)){
		/* reinitiate uac_tx_submit_complete to send next frame */
		if (req == NULL) {
//			printk("usb_audio_submit:req is not allocated!!\r\n");
			
			req = uac_g_alloc_req(g_uac_dev->iso_in, g_uac_dev->bfLen, GFP_ATOMIC);
			if (req == NULL) {
					printk("req is not allocated!!\r\n");
			}
		}
		req->status = 0;
		uac_tx_submit_complete(g_uac_dev->iso_in, req);
	}

	// below is just debug code
	if (g_uac_dev->req_done !=0) {
//		printk("UAC_SUBMIT previous transfer not done: increase URB req's \n");
	} else {
		g_uac_dev->req_done = 1;
	}		

    return 0;
}

#endif

static void
media_suspend (struct usb_gadget *gadget)
{
}

static void
media_resume (struct usb_gadget *gadget)
{
}

/* media char device file operations functions */
static int media_mmap(struct file *fp, struct vm_area_struct *vma)
{
	struct media_g_dev *media = fp->private_data;

	if (media->cur_strm == 0 && media->use_mmap[0] == 0)
		return -EINVAL;
#ifdef DUAL_STREAM
	if (media->cur_strm == 1 && media->use_mmap[1] == 0)
		return -EINVAL;
#endif
        if (remap_pfn_range(vma, vma->vm_start, media->s[media->cur_strm].uvc_buffer[0].offset >> PAGE_SHIFT,
                         vma->vm_end - vma->vm_start, vma->vm_page_prot))
                return -EAGAIN;

	return	0;
}
static int media_open(struct inode *inode, struct file *fp)
{
	struct media_g_dev *media = gb_mediauvcuac;

	if (gb_mediauvcuac==NULL) {
		printk("UVC/UAC ERROR: media instance NULL in media_open\n");
	}


	fp->private_data = media;

#ifdef UAVC_CLASS_REQ_SUPPORT
	media->flag_release = false;
#endif

	return 0;
}
static int media_read(struct file *fp, char *buff, size_t len, loff_t * off)
{
#ifdef UAVC_CLASS_REQ_SUPPORT
	struct media_g_dev *media = fp->private_data;

	/* Block here until a UAC/UVC class request available */
	down(&media->read_sem);
	printk(KERN_INFO "Class request available for read\n");

	/* Acquire the class request buffer access semaphore */
	down(&media->class_req_sem);
	copy_to_user(buff, media->class_req, sizeof(media->class_req));	
	up(&media->class_req_sem);

	if(media->flag_release)
		return -1;
	else
		return (sizeof(media->class_req));
#else
	return 0;
#endif
}

static int media_write(struct file *fd, const char __user *buf, size_t length, loff_t *ptr)
{
	return 0;
}
static int media_release(struct inode *inode, struct file *fp)
{
	struct media_g_dev *media = fp->private_data;

	if (media->use_mmap[0]) {
		free_uvc_video_buffer(media, 0);
		media->use_mmap[0] = 0;
	}
#ifdef DUAL_STREAM
	if (media->use_mmap[1]) {
		free_uvc_video_buffer(media, 1);
		media->use_mmap[1] = 0;
	}
#endif

#ifdef UAVC_CLASS_REQ_SUPPORT
	media->flag_release = true;
	/* Unblock user space reader process */ 							
	up(&media->read_sem);	
#endif
	return 0;
}


static int media_ioctl(struct inode *inode, struct file *fp, unsigned int cmd,
		     unsigned long arg)
{
	static struct uvc_input_info uvc_input;
	unsigned long flags;
	struct media_g_dev *media = fp->private_data;
	void __user *user_arg = (void __user *)arg;
#ifdef UAC
	unsigned int addr;
	int bufIdx;
	struct uac_g_dev *dev = &media->g_uac_dev;
#endif
	/* active video streaming interface as set from app */
	u8	sn = media->cur_strm;

	switch (cmd) {
		case UVC_SET_STREAM:
				spin_lock_irqsave(&media->lock, flags);
				if (copy_from_user(&media->cur_strm, (u8*)user_arg, 1)) {
					printk("copy_from_user failed\n");
					//return -1;
				}
				spin_unlock_irqrestore(&media->lock, flags);
		
				break;
		case UVC_GET_BUF: /* temporary and need to remove */
				spin_lock_irqsave(&media->lock, flags);
				if (copy_to_user(user_arg, &media->s[sn].bufinfo, sizeof(struct buf_info)))
					printk("failed in copy_to_user\n");
				spin_unlock_irqrestore(&media->lock, flags);
				break;
		case UVC_GET_FORMAT:
				spin_lock_irqsave(&media->lock, flags);
				media->s[sn].formatinfo.format = media->s[sn].cur_vs_state.bFormatIndex;
				media->s[sn].formatinfo.resolution = media->s[sn].cur_vs_state.bFrameIndex;
				if (copy_to_user(user_arg, &media->s[sn].formatinfo, sizeof(struct format_info)))
					printk("failed in copy_to_user\n");
				spin_unlock_irqrestore(&media->lock, flags);
				break;
		case UVC_READY:
				spin_lock_irqsave(&media->lock, flags);
				if (copy_to_user(user_arg, &media->s[sn].eof_flag, 1))
					printk("failed in copy_to_user\n");
				spin_unlock_irqrestore(&media->lock, flags);
				break;
		case UVC_USE_MMAP:
				spin_lock_irqsave(&media->lock, flags);
				if (copy_from_user(&media->use_mmap[sn], (u8*)user_arg, 1)) {
					printk("copy_from_user failed\n");
					//return -1;
				}
				spin_unlock_irqrestore(&media->lock, flags);
				if (media->use_mmap[sn])
					alloc_uvc_video_buffer(media, sn);
		
				break;
		case UVC_SUBMIT:
				if ((media->s[sn].eof_flag) /* && (uvc_ent_time > 34000)*/ ) {
					if (copy_from_user(&uvc_input, (struct uvc_input_info *)(user_arg),
							sizeof(struct uvc_input_info))) {
						printk("copy_from_user failed\n");
						//return -1;
					}
					spin_lock_irqsave(&media->lock, flags);
					media->s[sn].tLen = uvc_input.framesize;
					media->s[sn].u_size_remaining = media->s[sn].tLen;
					media->s[sn].u_size_current = 0;
					if(media->s[sn].u_size_remaining <= 0) {
						printk("!!size remaining is:%d\r\n", media->s[sn].u_size_remaining);
						return -1;
					}
					media->s[sn].vmalloc_area = (unsigned char*) uvc_input.virtAddr;
					media->s[sn].vmalloc_phys_area = (unsigned char*) uvc_input.phyAddr;
					media->s[sn].eof_flag = 0;
		#ifdef EN_TIMESTAMP
					do_gettimeofday(&media->s[sn].pts_timeinfo);
		#endif
					spin_unlock_irqrestore(&media->lock, flags);

#if 0 /* This is just to simulate a class request is received from USB host so that we can test user space control */ 
{
		static unsigned int count = 0;
		static unsigned short contrast = 0;
		count++;
		
		if ((count % 100) == 0)
		{
			printk("** about to send a class request from kernel ****** \n");
			contrast+=1000;
			/* Acquire the class request buffer access semaphore */
			down(&media->class_req_sem);
			/* Fill the class request data.*/
			media->class_req[0] = VC_EXT_UNIT_UID;
			media->class_req[1] = 1; // contrast; /* le16_to_cpu(*((u16 *)req->buf)); */
			up(&media->class_req_sem);
			
			/* Unblock user space reader process */ 							
			up(&media->read_sem);


		}



}

#endif
		
					return 0;
				} else {
          				/* printk ("WARNING: recieved a UVC submit while TX-ing\n"); */
          				return -1; /* usb tx is busy */
				}
				break;
#ifdef UAC
        case UAC_SUBMIT:{

                spin_lock_irqsave(&dev->lock, flags);
                memset(&g_audio_buf, 0, sizeof(struct uac_buf));
                if(  copy_from_user(&addr, (u32 *)(arg), sizeof(u32)) )
                        printk(" copy from user failed \n");

                if(  copy_from_user(&g_audio_buf, (struct uac_inp_buf *)(addr), sizeof(struct uac_inp_buf)) )
                        printk(" copy from user failed \n");

 #if 1
        		bufIdx = g_audio_buf.bufIndex;
                /* copy audio buf from user space to kernel space buffer : NEED to be OPTIMIZED*/
                if(  copy_from_user(dev->aud_buf[bufIdx], (u32 *)(g_audio_buf.addr), g_audio_buf.len) )
                        printk(" copy from user failed \n");
//				printk("UAC_SUBMIT aud_buf[%d]=%p len =%d\n", bufIdx, dev->aud_buf[bufIdx], g_audio_buf.len);
                usb_audio_submit(dev,dev->aud_buf[bufIdx],g_audio_buf.len);
#else
				usb_audio_submit(struct uac_g_dev * g_uac_dev,u8 * buf,int len)(dev,g_audio_buf.addr,g_audio_buf.len);
#endif

                spin_unlock_irqrestore(&dev->lock, flags);
                }
                break;

        case UAC_BUF_READY:{
                spin_lock_irqsave(&dev->lock, flags);
                if (copy_to_user(user_arg, &dev->eof_flag, 1))
                      printk("failed in copy_to_user\n");
                spin_unlock_irqrestore(&dev->lock, flags);

                }
                break;

#endif
		default:
				printk("Unhandled IOCTL in UVC Test Dev module:%u\r\n", cmd);
				return -1;
	}
	return 0;
}
static struct usb_gadget_driver media_gadget_driver = {
	.speed		= USB_SPEED_HIGH,
	.function	= (char *) longname,
	.bind		= media_bind,
	.unbind		= media_unbind,

	.setup		= media_setup,
	.disconnect	= media_disconnect,

	.suspend	= media_suspend,
	.resume		= media_resume,

	.driver 	= {
		.name		= (char *) shortname,
		.owner          = THIS_MODULE,
	},
};

struct file_operations media_fops = {
        .read = media_read,
        .write = media_write,
        .open = media_open,
        .release = media_release,
        .ioctl = media_ioctl,
        .mmap = media_mmap,
      owner:THIS_MODULE,
};

MODULE_AUTHOR (DRIVER_AUTHOR);
MODULE_LICENSE ("GPL");

static int __init media_gadget_init (void)
{
	int rc;

	rc= usb_gadget_register_driver (&media_gadget_driver);
	if(rc!=0){
		printk("gadget driver registration failed \n");
		return rc;
	}

	if (register_chrdev(MEDIA_CHAR_DEVNUM, "media", &media_fops) < 0)
		printk("register_chrdev: failed\n");
		
	printk("media_gadget_init()\n");
	return 0;
}
module_init (media_gadget_init);

static void __exit media_gadget_cleanup (void)
{
	unregister_chrdev(MEDIA_CHAR_DEVNUM, "media");
	usb_gadget_unregister_driver (&media_gadget_driver);
}
module_exit (media_gadget_cleanup);

