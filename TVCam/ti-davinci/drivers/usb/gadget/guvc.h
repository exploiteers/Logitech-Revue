/*
 *      guvc.h -- Header for USB Video Class (UVC) gadget driver
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      author: Ajay Kumar Gupta (ajay.gupta@ti.com)
 */
#ifndef __LINUX_USB_VIDEO_H
#define __LINUX_USB_VIDEO_H

#define UVC_YUV 	0

#if UVC_YUV
#undef DUAL_STREAM
#else
#define DUAL_STREAM
#endif

//#define BIDIR_STREAM
#define UAC  
#define EN_TIMESTAMP
#define UVC_CHAR_DEVNUM 	217
#define UVC_CHAR_DEVNAME 	"/dev/uvc"

#define T_BUF_SIZE_VID 		(1024)
#define UVC_NUM_BUFS 		2
#define UVC_BUFFER_LEN 		64*1024

#ifdef EN_TIMESTAMP
	#define URB_HEAD_SIZE	(0x0C)
	#define EN_SCR_PTS		(0x0C)
#else
	#define URB_HEAD_SIZE	(0x02)
	#define EN_SCR_PTS		(0x00)
#endif
#define MAX_UVC_URB 250

/* IOCTLs  */
#define UVC_GET_BUF	0xa5
#define UVC_READY	0x1
#define UVC_SUBMIT	0x2
#define UVC_GET_FORMAT	0x3
#define UVC_USE_MMAP	0x4
#define UVC_SET_STREAM	0x5

#define MJPEG_HEADER_SZ URB_HEAD_SIZE
/* Masks for MJPEG payload header */
#define MJPEG_H_FID			(0x01)
#define MJPEG_H_EOF			(0x02)
#define MJPEG_H_PTS			(0x04)
#define MJPEG_H_SCR			(0x08)
#define MJPEG_H_STI			(0x20)
#define MJPEG_H_ERR			(0x40)
#define MJPEG_H_EOH			(0x80)

/* MJPEG Payload headr BFH */
#define MJPEG_H_BFH 	(((MJPEG_H_EOF) | (MJPEG_H_PTS) | (MJPEG_H_SCR) | (MJPEG_H_EOH)) & (~MJPEG_H_FID))

struct uvc_buf {
	int index;      		/* index number, 0 -> N-1 */
	unsigned int offset;    /* physical address of the buffer used in the mmap() */
	unsigned int kvaddr;    /* kernel virtual address */
	unsigned int size;      /* size of the buffer */
};
struct uvc_input_info {
	void *virtAddr;
	void *phyAddr;
	unsigned int framesize; 	/* size of the current frame */
};

typedef enum uvc_ctrl_cmd_type {
	UVC_CMD_CHANGE_RESOLUTION = 1,
	UVC_CMD_DUMMY = 0xFF,
}uvc_ctrl_command;

struct uvc_command_ds {
	uvc_ctrl_command cmdidx; /* Command Index */
	unsigned char params; /* Params to be passed to or from application */
};
struct uvc_stream_header {
	__u8  bHeaderLength;
	__u8  bmHeaderInfo;
	__u32 dwPresentationTime;
	__u64 scrSourceClock:48;
}__attribute__ ((packed));

#define UVC_MJPEG_STREAM_HEADER_SIZE 12

/* ---------------------------------------------------------- */
/* Video Device Class Codes */
#define USB_CC_VIDEO                   0x0e

/* Video Interface Subclass Codes */
#define USB_SC_UNDEFINED               0x00
#define USB_SC_VIDEOCONTROL            0x01
#define USB_SC_VIDEOSTREAMING          0x02
#define USB_SC_INTERFACE_COLLECTION    0x03

/* Video Interface Protocol Codes */
#define USB_PROTOCOL_UNDEFINED         0x00

/* Video Class-Specific Descriptor Types */
#define USB_CS_UNDEFINED               0x20
#define USB_CS_DEVICE                  0x21
#define USB_CS_CONFIGURATION           0x22
#define USB_CS_STRING                  0x23
#define USB_CS_INTERFACE               0x24
#define USB_CS_ENDPOINT                0x25

/* Video Class-Specific VC Interface Descriptor Subtypes */
#define USB_VC_DESCRIPTOR_UNDEFINED    0x00
#define USB_VC_HEADER                  0x01
#define USB_VC_INPUT_TERMINAL          0x02
#define USB_VC_OUTPUT_TERMINAL         0x03
#define USB_VC_SELECTOR_UNIT           0x04
#define USB_VC_PROCESSING_UNIT         0x05
#define USB_VC_EXTERNSION_UNIT         0x06

/* Video Class-Specific VS Interface Descriptor Subtype */
#define USB_VS_UNDEFINED               0x00
#define USB_VS_INPUT_HEADER            0x01
#define USB_VS_OUTPUT_HEADER           0x02
#define USB_VS_STILL_IMAGE_FRAME       0x03
#define USB_VS_FORMAT_UNCOMPRESSED     0x04
#define USB_VS_FRAME_UNCOMPRESSED      0x05
#define USB_VS_FORMAT_MJPEG            0x06
#define USB_VS_FRAME_MJPEG             0x07
/* Reserved 0x08 */
/* Reserved 0x09 */
#define USB_VS_FORMAT_MPEG2TS          0x0a
/* Reserved 0x0B */
#define USB_VS_FORMAT_DV               0x0c
#define USB_VS_COLORFORMAT             0x0d
/* Reserved 0x0e */
/* Reserved 0x0f */
#define USB_VS_FORMAT_FRAME_BASED      0x10
#define USB_VS_FRAME_FRAME_BASED       0x11
#define USB_VS_FORMAT_STREAM_BASED     0x12

/* Video Class-Specific Endpoint Descriptor Subtype */
#define USB_EP_UNDEFINED               0x00
#define USB_EP_GENERAL                 0x01
#define USB_EP_ENDPOINT                0x02
#define USB_EP_INTERRUPT               0x03

/* Video Class-Specific Request Codes */
#define RC_UNDEFINED					0x00
#define SET_CUR						0x01
#define SET_MIN						0x02
#define SET_MAX						0x03
#define SET_RES						0x04
#define GET_CUR						0x81
#define GET_MIN						0x82
#define GET_MAX						0x83
#define GET_RES						0x84
#define GET_LEN						0x85
#define GET_INFO					0x86
#define GET_DEF						0x87

/* VideoControl Interface Control Selectors */
#define VC_CONTROL_UNDEFINED            0x00
#define VC_VIDEO_POWER_MODE_CONTROL     0x01
#define VC_REQUEST_ERROR_CODE_CONTROL   0x02
/* Reserved 0x03 */

/* Terminal Control Selectors */
#define USB_TE_CONTROL_UNDEFINED       0x00

/* Selector Unit Control Selectors */
#define SU_CONTROL_UNDEFINED       0x00
#define SU_INPUT_SELECT_CONTROL       0x01

/* Camera Terminal Controls Selectors */
#define CT_CONTROL_UNDEFINED            		0x00
#define CT_SCANNING_MODE_CONTROL        		0x01
#define CT_AE_MODE_CONTROL              		0x02
#define CT_AE_PRIORITY_CONTROL          		0x03
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL		0x04
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL		0x05
#define CT_FOCUS_ABSOLUTE_CONTROL       		0x06
#define CT_FOCUS_RELATIVE_CONTROL       		0x07
#define CT_FOCUS_AUTO_CONTROL           		0x08
#define CT_IRIS_ABSOLUTE_CONTROL        		0x09
#define CT_IRIS_RELATIVE_CONTROL        		0x0a
#define CT_ZOOM_ABSOLUTE_CONTROL        		0x0b
#define CT_ZOOM_RELATIVE_CONTROL        		0x0c
#define CT_PANTILT_ABSOLUTE_CONTROL     		0x0d
#define CT_PANTILT_RELATIVE_CONTROL     		0x0e
#define CT_ROLL_ABSOLUTE_CONTROL        		0x0f
#define CT_ROLL_RELATIVE_CONTROL        		0x10
#define CT_PRIVACY_CONTROL              		0x11

/* Processing Unit Control Selectors */
#define PU_CONTROL_UNDEFINED            		0x00
#define PU_BACKLIGHT_COMPENSATION_CONTROL		0x01
#define PU_BRIGHTNESS_CONTROL           		0x02
#define PU_CONTRAST_CONTROL             		0x03
#define PU_GAIN_CONTROL                 		0x04
#define PU_POWER_LINE_FREQUENCY_CONTROL 		0x05
#define PU_HUE_CONTROL                  		0x06
#define PU_SATURATION_CONTROL					0x07
#define PU_SHARPNESS_CONTROL					0x08
#define PU_GAMMA_CONTROL						0x09
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL	0x0a
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL	0x0b
#define PU_WHITE_BALANCE_COMPONENT_CONTROL		0x0c
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL	0x0d
#define PU_DIGITAL_MULTIPLIER_CONTROL			0x0e
#define PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL		0x0f
#define PU_HUE_AUTO_CONTROL				0x10
#define PU_ANALOG_VIDEO_STANDARD_CONTROL		0x11
#define PU_ANALOG_LOCK_STATUS_CONTROL			0x12

/* Extension Unit Control Selectors */
#define XU_CONTROL_UNDEFINED       0x00
#define XU_FACEDETECTION_ENABLE_CONTROL   0x01
#define XU_FACEDETECTION_DISABLE_CONTROL   0x02

/* VideoStream Interface Control Selectors */
#define VS_CONTROL_UNDEFINED		0x00
#define VS_PROBE_CONTROL		0x01
#define VS_COMMIT_CONTROL		0x02
#define VS_STILL_PROBE_CONTROL		0x03
#define VS_STILL_COMMIT_CONTROL		0x04
#define VS_STILL_IMAGE_TRIGGER_CONTROL	0x05
#define VS_STREAM_ERROR_CODE_CONTROL	0x06
#define VS_GENERATE_KEY_FRAME_CONTROL	0x07
#define VS_UPDATE_FRAME_SEGMENT_CONTROL	0x08
#define VS_SYNC_DELAY_CONTROL		0x09

/* USB Terminal Types */
#define USB_TT_VENDOR_SPECIFIC         0x0100
#define USB_TT_STREAMING               0x0101

/* Input Terminal Types */
#define USB_ITT_VENDOR_SPECIFIC        0x0200
#define USB_ITT_CAMERA                 0x0201
#define SUB_ITT_MEDIA_TRANSP_INPUT     0x0202

/* Output Terminal Types */
#define USB_OTT_VENDOR_SPECIFIC        0x0300
#define USB_OTT_DISPLAY                0x0301
#define USB_OTT_MEDIA_TRANSP_OUTPUT    0x0302

/* External Terminal Types */
#define USB_EXTERNAL_VENDOR_SPECIFIC   0x0400
#define USB_COMPOSITE_CONNECTOR        0x0401
#define USB_SVIDEO_CONNECTOR           0x0402
#define USB_COMPONENT_CONNECTOR        0x0403

/* Class-specific VC Interface Header Descriptor */
#define USB_VC_HEADER_SIZE(n)          (12+(n))

#define DECLARE_USB_VC_HEADER_DESCRIPTOR(n)                    \
struct usb_vc_header_descriptor {	                       \
       __u8  bLength;                                          \
       __u8  bDescriptorType;                                  \
       __u8  bDescriptorSubType;                               \
       __le16 bcdUVC;                                          \
       __le16 wTotalLength;                                    \
       __le32 dwClockFrequency;                                \
       __u8  bInCollection;                                    \
       __u8  baInterfaceNr[n];                                 \
} __attribute__ ((packed))

/* Input Terminal Descriptor */
struct usb_input_terminal_descriptor {
       __u8  bLength;                  /* 8  bytes */
       __u8  bDescriptorType;          /* CS_INTERFACE */

       __u8  bDescriptorSubType;       /* VC_INPUT_TERMINAL */
       __u8  bTerminalID;
       __le16 wTerminalType;
       __u8  bAssocTerminal;
       __u8  iTerminal;

/* Depending on the Terminal Type, certain Input Terminal descriptors have
 * additional fields. The descriptors for these special Terminal types are
 * described in separate sections specific to those Terminals, and in
 * accompanying documents.
 */
} __attribute__ ((packed));

#define USB_INPUT_TERMINAL_SIZE        8

/* Output Terminal Descriptor */
struct usb_output_terminal_descriptor {
       __u8  bLength;                  /* 9  bytes */
       __u8  bDescriptorType;          /* CS_INTERFACE */

       __u8  bDescriptorSubType;       /* VC_OUTPUT_TERMINAL */
       __u8  bTerminalID;
       __le16 wTerminalType;
       __u8  bAssocTerminal;
       __u8  bSourceID;
       __u8  iTerminal;

/* Depending on the Terminal Type, certain Output Terminal descriptors have
 * additional fields. The descriptors for these special Terminal types are
 * described in separate sections specific to those Terminals, and in
 * accompanying documents.
 */
} __attribute__ ((packed));

#define USB_OUTPUT_TERMINAL_SIZE       9

/* Camera Terminal Descriptor */
#define USB_CAM_TERMINAL_SIZE(n)       (15+(n))

#define DECLARE_USB_CAM_TERMINAL_DESCRIPTOR(n)                 \
struct usb_cam_terminal_descriptor { 		               \
       __u8  bLength;                                          \
       __u8  bDescriptorType;                                  \
       __u8  bDescriptorSubType;                               \
       __u8  bTerminalID;                                      \
       __le16 wTerminalType;                                   \
       __u8  bAssocTerminal;                                   \
       __u8  iTerminal;                                        \
       __le16 wObjectFocalLengthMin;                           \
       __le16 wObjectFocalLengthMax;                           \
       __le16 wOcularFocalLength;                              \
       __u8  bControlSize;                                     \
       __u8  bmControls[n];                                    \
} __attribute__ ((packed))

/* Selector Unit Descriptor */
#define USB_SELECTOR_UNIT_SIZE(p)      (6+(p))

#define DECLARE_USB_SELECTOR_UNIT_DESCRIPTOR(p)                \
struct usb_selector_unit_descriptor { 		               \
       __u8  bLength;                  /* 6+p bytes */         \
       __u8  bDescriptorType;          /* CS_INTERFACE */      \
                                                               \
       __u8  bDescriptorSubType;       /* VC_SELECTOR_UNIT */  \
       __u8  bUnitID;                                          \
       __u8  bNrInPins;                /* p */                 \
       __u8  baSourceID[p];                                    \
       __u8 iSelector;                                         \
} __attribute__ ((packed))

/* Processing Unit Descriptor */
#define USB_PROCESSING_UNIT_SIZE(n)    (10+(n))

#define DECLARE_USB_PROCESSING_UNIT_DESCRIPTOR(n)              \
struct usb_processing_unit_descriptor {		               \
       __u8  bLength;                  /* 10+n bytes */        \
       __u8  bDescriptorType;          /* CS_INTERFACE */      \
                                                               \
       __u8  bDescriptorSubType;       /* VC_PROCESSING_UNIT */\
       __u8  bUnitID;                                          \
       __u8  bSourceID;                                        \
       __le16 wMaxMultiplier;                                  \
       __u8  bControlSize;             /* n */                 \
       __u8  bmControls[n];                                    \
       __u8  iProcessing;                                      \
} __attribute__ ((packed))

/* Extension Unit Descriptor */
#define USB_EXTENSION_UNIT_SIZE(p,n)   (24+(p)+(n))

#define DECLARE_USB_EXTENSION_UNIT_DESCRIPTOR(p,n)             \
struct usb_extension_unit_desciptor {                  	       \
       __u8  bLength;                  /* 24+p+n bytes */      \
       __u8  bDescriptorType;          /* CS_INTERFACE */      \
                                                               \
       __u8  bDescriptorSubType;       /* VC_EXTENSION_UNIT */ \
       __u8  bUnitID;                                          \
       __u32  guidExtensionCode[4];                            \
       __u8  bNumControls;                                     \
       __u8  bNrInPins;                /* p */                 \
       __u8  baSourceID[p];                                     \
       __u8  bControlSize;             /* n */                 \
       __u8  bmControls[n];                                    \
       __u8  iExtension;                                       \
} __attribute__ ((packed))

/* Class-specific VC Interrupt Endpoint Descriptor */
struct usb_vc_interrupt_endpoint_descriptor {
       __u8  bLength;                  /* 5 bytes */
       __u8  bDescriptorType;          /* CS_ENDPOINT */

       __u8  bDescriptorSubType;       /* EP_INTERRUPT */
       __le16 wMaxTransferSize;
} __attribute__ ((packed));

#define USB_VC_INTERRUPT_ENDPOINT_SIZE 5

/* Class-specific VS Interface Input Header Descriptor */
#define USB_VS_INPUT_HEADER_SIZE(p,n)  (13+((p)*(n)))

#define DECLARE_USB_VS_INPUT_HEADER_DESCRIPTOR(p,n)            \
struct usb_vs_input_header_descriptor {		               \
       __u8  bLength;                  /* 13+(p*n) bytes */    \
       __u8  bDescriptorType;          /* CS_INTERFACE */      \
                                                               \
       __u8  bDescriptorSubType;       /* VS_INPUT_HEADER */   \
       __u8  bNumFormats;              /* p */                 \
       __le16 wTotalLength;                                    \
       __u8  bEndpointAddress;                                 \
       __u8  bmInfo;                                           \
       __u8  bTerminalLink;                                    \
       __u8  bStillCaptureMethod;                              \
       __u8  bTriggerSupport;                                  \
       __u8  bTriggerUsage;                                    \
       __u8  bControlSize;             /* n */                 \
       __u8  bmaControls[p][n];         /* [p][n] */            \
} __attribute__ ((packed))

/* Class-specific VS Interface Output Header Descriptor */
#define USB_VS_OUTPUT_HEADER_SIZE(p,n) (9+((p)*(n)))

#define DECLARE_USB_VS_OUTPUT_HEADER_DESCRIPTOR(p,n)           \
struct usb_vs_output_header_descriptor {	               \
       __u8  bLength;                  /* 9+(p*n) bytes */     \
       __u8  bDescriptorType;          /* CS_INTERFACE */      \
                                                               \
       __u8  bDescriptorSubType;       /* VS_OUTPUT_HEADER */  \
       __u8  bNumFormats;              /* p */                 \
       __le16 wTotalLength;                                    \
       __u8  bEndpointAddress;                                 \
       __u8  bTerminalLink;                                    \
       __u8  bControlSize;             /* n */                 \
       __u8  bmaControls[p*n];         /* [p][n] */            \
} __attribute__ ((packed))

/* Still Image Frame Descriptor */
#define USB_STILL_IMAGE_FRAME_SIZE(n,m)        (5+(4*(n))+(m))

struct usb_image_pattern {
       __le16  wWidth;
       __le16  wHeight;
};

#define DECLARE_USB_STILL_IMAGE_FRAME_DESCRIPTOR(n,m)          \
struct usb_still_image_frame_descriptor {	               \
       __u8  bLength;          /* 10+(4*n)-4+m-1 bytes */      \
       __u8  bDescriptorType;          /* CS_INTERFACE */      \
                                                               \
       __u8  bDescriptorSubType; /* VS_STILL_IMAGE_FRAME */    \
       __u8  bNumImageSizePatterns;    /* n */                 \
       struct usb_image_pattern p[n];  /* [n] */               \
       __u8  bNumCompressionPattern;   /* m */                 \
       __u8  bCompression[m];          /* [m] */               \
} __attribute__ ((packed))

/* Color Matching Descriptor */
struct usb_color_matching_descriptor {
       __u8  bLength;                  /* 6 bytes */
       __u8  bDescriptorType;          /* CS_INTERFACE */

       __u8  bDescriptorSubType;       /* VS_COLORFORMAT */
       __u8  bColorPrimaries;
       __u8  bTransferCharacteristics;
       __u8  bMatrixCoefficients;
} __attribute__ ((packed));

#define USB_COLOR_MATCHING_SIZE        6

/* USB_VS_FORMAT_MPEG2-TS */
struct usb_vs_format_mpeg2ts_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFormatIndex;
	__u8  bDataOffset;
	__u8  bPacketLength;
	__u8  bStrideLength;
	__u32 guidStrideFormat[4];
} __attribute__ ((packed));

/* USB_VS_FORMAT_MJPEG_INTERFACE_DESCRIPTOR */
struct usb_vs_format_mjpeg_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFormatIndex;
	__u8  bNumFrameDescriptors;
	__u8  bmFlags;
	__u8  bDefaultFrameIndex;
	__u8  bAspectRatioX;
	__u8  bAspectRatioY;
	__u8  bmInterfaceFlags;
	__u8  bCopyProtect;
} __attribute__ ((packed));

/* USB_VS_FRAME_MJPEG_INTERFACE_DESCRIPTOR */
struct usb_vs_frame_mjpeg_descriptor {
	__u8  bLength;
	__u8  bDescriptorType;
	__u8  bDescriptorSubType;
	__u8  bFrameIndex;
	__u8  bmCapabilities;
	__u16 wWidth;
	__u16 wHeight;
	__u32 dwMinBitRate;
	__u32 dwMaxBitRate;
	__u32 dwMaxVideoFrameBufferSize;
	__u32 dwDeafultFrameInterval;
	__u8  bFrameIntervalType;
	__u32 dwMinFrameInterval;
	__u32 dwMaxFrameInterval;
	__u32 dwFrameIntervalStep;
} __attribute__ ((packed));

/* USB_VS_FORMAT_UNCOMPRESSED_DESCRIPTOR */
struct usb_vs_format_uncompressed_desc {
        __u8  bLength;
        __u8  bDescriptorType;
        __u8  bDescriptorSubType;
        __u8  bFormatIndex;
        __u8  bNumFrameDescriptors;
#if 0
        __u32 guidFormat0;
        __u32 guidFormat1;
        __u32 guidFormat2;
        __u32 guidFormat3;
#else
		__u8  guidFormat[16];

#endif
        __u8  bBitsPerPixel;
        __u8  bDefaultFrameIndex;
        __u8  bAspectRatioX;
        __u8  bAspectRatioY;
        __u8  bmInterfaceFlags;
        __u8  bCopyProtect;
} __attribute__ ((packed));

/* USB_VS_FRAME_UNCOMPRESSED_DESCRIPTOR */
struct usb_vs_frame_uncompressed_desc {
        __u8  bLength;
        __u8  bDescriptorType;
        __u8  bDescriptorSubType;
        __u8  bFrameIndex;
        __u8  bmCapabilities;
        __u16 wWidth;
        __u16 wHeight;
        __u32 dwMinBitRate;
        __u32 dwMaxBitRate;
        __u32 dwMaxVideoFrameBufferSize;
        __u32 dwDefaultFrameInterval;
        __u8  bFrameIntervalType;
        __u32 dwFrameInterval0;
        __u32 dwFrameInterval1;
        __u32 dwFrameInterval2;
} __attribute__ ((packed));

/* UVC Streaming control, Probe/Commit */
struct uvc_streaming_control {
	__u16 bmHint;
	__u8  bFormatIndex;
	__u8  bFrameIndex;
	__u32 dwFrameInterval;
	__u16 wKeyFrameRate;
	__u16 wPFrameRate;
	__u16 wCompQuality;
	__u16 wCompWindowSize;
	__u16 wDelay;
	__u32 dwMaxVideoFrameSize;
	__u32 dwMaxPayloadTransferSize;
#ifdef UVC_1_1
	__u32 dwClockFrequency;
	__u8  bmFramingInfo;
	__u8  bPreferedVersion;
	__u8  bMinVersion;
	__u8  bMaxVersion;
#endif
}__attribute__ ((packed));

#ifdef UVC_1_1
	#define UVC_PROB_COMM_DST_SIZE 34
#else
	#define UVC_PROB_COMM_DST_SIZE 26
#endif

#ifdef EN_STILL_CAP
/* -------------UVC Still Image Probe/Commit------------------*/
struct uvc_stillimg_control {
	__u8  bFormatIndex;
	__u8  bFrameIndex;
	__u8  bCompressionIndex;
	__u32 dwMaxVideoFrameSize;
	__u32 dwMaxPayloadTransferSize;
}__attribute__ ((packed));

#define UVC_PROB_COMM_STM_SIZE 11 /* Still Image */

static struct uvc_stillimg_control 
uvc_prob_comm_ds_still = {
	.bFormatIndex = 0x01,
	.bFrameIndex = 0x01,
	.bCompressionIndex = 0x01,
	.dwMaxVideoFrameSize = 0x1E8480, //(0x000F4240 * 2), //0x00300000,
	.dwMaxPayloadTransferSize = 0x00300000,
};
#endif

/* USB class definitions */
#define USB_MISC_SC_COMMON	0x2
#define	USB_PROTOCOL_ASSOC_DESC	0x1
/*
 * Interface/Terminal/Unit IDs, update the descriptor with these values
 */
#define VC_INTERFACE_NUMBER	0
#define VS_ISOC_IN_INTERFACE	VC_INTERFACE_NUMBER + 1

/* UID and TIDs */
#define VC_CAM_TERM_TID		0x1
#define VC_OUTPUT_TERM_TID	VC_CAM_TERM_TID + 1
#define VC_SEL_UNIT_UID         VC_OUTPUT_TERM_TID + 1
#define VC_PRO_UNIT_UID		VC_SEL_UNIT_UID + 1
#define VC_EXT_UNIT_UID		VC_PRO_UNIT_UID + 1

#if defined(DUAL_STREAM) && defined(BIDIR_STREAM) && defined(UAC)
	#define VS_ISOC_IN_INTERFACE2	VS_ISOC_IN_INTERFACE + 1
	#define VS_ISOC_OUT_INTERFACE	VS_ISOC_IN_INTERFACE2 + 1
	#define AC_INTERFACE_NUMBER	VS_ISOC_OUT_INTERFACE + 1
	#define AS_ISOC_IN_INTERFACE	AC_INTERFACE_NUMBER + 1
	#define UVC_DEVICE_NR_INTF	AS_ISOC_IN_INTERFACE + 1
	u8	vc_header_intf_nr[3] = {0x1, 0x2, 0x3};
	#define VC_DS_OUTPUT_TERM_ID	VC_EXT_UNIT_UID + 1
	#define VC_INPUT_TERM_TID       VC_DS_OUTPUT_TERM_ID + 1
	#define VC_BDS_OUTPUT_TERM_TID  VC_INPUT_TERM_TID + 1
#elif defined(DUAL_STREAM) && defined(UAC) && !defined(BIDIR_STREAM)
	#define VS_ISOC_IN_INTERFACE2	VS_ISOC_IN_INTERFACE + 1
	#define AC_INTERFACE_NUMBER	VS_ISOC_IN_INTERFACE2 + 1
	#define AS_ISOC_IN_INTERFACE	AC_INTERFACE_NUMBER + 1
	#define UVC_DEVICE_NR_INTF	AS_ISOC_IN_INTERFACE + 1
	u8	vc_header_intf_nr[2] = {0x1, 0x2};
	#define VC_DS_OUTPUT_TERM_ID	VC_EXT_UNIT_UID + 1
#elif defined(DUAL_STREAM) && !defined(UAC) && !defined(BIDIR_STREAM)
	#define VS_ISOC_IN_INTERFACE2	VS_ISOC_IN_INTERFACE + 1
	#define UVC_DEVICE_NR_INTF	VS_ISOC_IN_INTERFACE2 + 1
		u8	vc_header_intf_nr[2] = {0x1, 0x2};
	#define VC_DS_OUTPUT_TERM_ID	VC_EXT_UNIT_UID + 1
#elif !defined(DUAL_STREAM) && defined(UAC) && defined(BIDIR_STREAM)
	#define VS_ISOC_OUT_INTERFACE	VS_ISOC_IN_INTERFACE + 1
	#define AC_INTERFACE_NUMBER	VS_ISOC_OUT_INTERFACE + 1
	#define AS_ISOC_IN_INTERFACE	AC_INTERFACE_NUMBER + 1
	#define UVC_DEVICE_NR_INTF	AS_ISOC_IN_INTERFACE + 1
	u8	vc_header_intf_nr[2] = {0x1, 0x2};
	#define VC_INPUT_TERM_TID       VC_EXT_UNIT_UID + 1
	#define VC_BDS_OUTPUT_TERM_TID  VC_INPUT_TERM_TID + 1
#elif defined(UAC)
	#define AC_INTERFACE_NUMBER	VS_ISOC_IN_INTERFACE + 1
	#define AS_ISOC_IN_INTERFACE	AC_INTERFACE_NUMBER + 1
	#define UVC_DEVICE_NR_INTF	AS_ISOC_IN_INTERFACE + 1
	u8	vc_header_intf_nr[1] = {0x1};
#else
	#define UVC_DEVICE_NR_INTF	VS_ISOC_IN_INTERFACE + 1
	u8	vc_header_intf_nr[1] = {0x1};
#endif


/* UVC descriptor forming details */
#define	UVC_DEVICE_NR_CONFIG		1
#ifdef UAC
#define	VC_HEADER_NR_VS		UVC_DEVICE_NR_INTF - 1 - 2 /* audio uses two*/
#else
#define	VC_HEADER_NR_VS		UVC_DEVICE_NR_INTF - 1
#endif
#define	VC_CAM_TERM_CONTROL_SIZE	2
#define VC_SEL_UNIT_NR_IN_PINS		1
#define	VC_PRO_UNIT_CONTROL_SIZE	2
#define VC_EXT_UNIT_NR_IN_PINS		1
#define VC_EXT_UNIT_CONTROL_SIZE	2
#define INPUT_HEADER_NR_FORMART		1
#define INPUT_HEADER_CONTROL_SIZE	1

/* endpoint configurations */
#define VC_INT_EP_MAX_TRF_SIZE		0x8
#define INT_IN_PACKET_SIZE		0x8
#define INT_IN_INTERVAL		0xF
#define ISOC_IN_PKT_SIZE	0x400
#define ISOC_IN_INTERVAL	1

/* Data for usb_vc_header_descriptor */
#define VC_HEADER_bcdUVC	0x100
#define VC_HEADER_TOTAL_LEN	0x54
#define VC_HEADER_CLK_FREQ	0x1C9C380

/* Data for usb_cam_terminal_descriptor */
#define VC_CAM_TERM_ASSOC_TERM	0x0
#define VC_CAM_TERM_TERMINAL	0x0
#define VC_CAM_TERM_OBJ_FOCAL_LEN_MIN	0x0
#define VC_CAM_TERM_OBJ_FOCAL_LEN_MAX	0x0
#define VC_CAM_TERM_OCL_FOCAL_LEN	0x0
u8	vc_cam_term_control[VC_CAM_TERM_CONTROL_SIZE] = {0x0a,0x0};

/* Data for usb_output_terminal_descriptor */
#define VC_OUTPUT_TERM_ASSOC_TERM	0x0
#define VC_OUTPUT_TERM_SOURCE_ID	VC_EXT_UNIT_UID
#define VC_OUTPUT_TERM_TERMINAL		0x0

/* Data for usb_selector_unit_descriptor */
#define VC_SEL_UNIT_SELECTOR	0x0
u8	vc_sel_unit_src_id[VC_SEL_UNIT_NR_IN_PINS] = {VC_CAM_TERM_TID,};

/* Data for usb_processing_unit_descriptor */
#define	VC_PRO_UNIT_SOURCE_ID	VC_SEL_UNIT_UID
#define VC_PRO_UNIT_MAX_MULT	0x0
#define VC_PRO_UNIT_PROCESSING	0x0
#define VC_PRO_UNIT_VIDEO_STD	0x0
/* get supported control from pu descriptor, 0x3 => brightness and contrast */
u8	vc_pro_unit_control[VC_PRO_UNIT_CONTROL_SIZE] = {0x6f,0x13};//{0x03,0x0};

/* Data for usb_extension_unit_desciptor */
#define VC_EXT_UNIT_NR_CONTROLS		0x10
#define VC_EXT_UNIT_EXTENSION		0x0
u8	vc_ext_unit_src_id[VC_EXT_UNIT_NR_IN_PINS] = {VC_PRO_UNIT_UID};
u8	vc_ext_unit_control[VC_EXT_UNIT_CONTROL_SIZE] = {0x0, 0x0};
u32	vc_ext_unit_guid[4] = {0xA7974C56, 0x4B90A77E, 0x711CBF8C, 0x003030EC};


/* Data for usb_vs_input_header_descriptor */
#define INPUT_HEADER_TOTAL_LENGTH	0x45
#define INPUT_HEADER_BM_INFO		0x0
#define	INPUT_HEADER_TERM_LINK		VC_OUTPUT_TERM_TID
#define INPUT_HEADER_STILL_CAP_METHOD	0x0
#define INPUT_HEADER_TRIGGER_SUPPORT	0x0
#define INPUT_HEADER_TRIGGER_USAGE	0x0
u8	input_header_control[INPUT_HEADER_NR_FORMART][INPUT_HEADER_CONTROL_SIZE] = 
			{{0x0,},};

/* Data for usb_vs_format_mjpeg_descriptor */
#define MJPEG_FORMAT_INDEX	0x1
#define MPEG2TS_FORMAT_INDEX	0x1
#define VS_F_NR_DESC		4
#define VS_F_BM_FLAGS		1
#define VS_F_DEF_F_INDEX	3
#define VS_F_ASPECT_RATIO_X	0
#define VS_F_ASPECT_RATIO_Y	0
#define VS_F_INTF_FLAGS		0
#define	VS_F_COPY_PROTECT	0

/* Data for usb_vs_frame_mjpeg_descriptor */
u8	vs_frame_mjpeg_frame_index[VS_F_NR_DESC] = {1, 2, 3, 4};
u8	vs_frame_mpjeg_capabilities[VS_F_NR_DESC] = {0x0, 0x0, 0x0, 0x0};
u16	vs_frame_mjpeg_width[VS_F_NR_DESC] = {0x500, 0x280, 0x140, 0xA0}; /* 1280, 640, 320, 160 */
u16	vs_frame_mjpeg_height[VS_F_NR_DESC] = {0x2D0, 0x1E0, 0x0F0, 0x78}; /*  720, 480, 240, 120 */
u32	vs_frame_mjpeg_min_bitrate[VS_F_NR_DESC] = {0xDEC00, 0xDEC00, 0xDEC00, 0xDEC00};
u32	vs_frame_mjpeg_max_bitrate[VS_F_NR_DESC] = {0xDEC00, 0xDEC00, 0xDEC00, 0xDEC00};
u32	vs_frame_mjpeg_max_video_frame_bufsize[VS_F_NR_DESC] = {0x3DFD240, 0xE1000, 0x38400, 0xE100};
u32	vs_frame_mjpeg_max_def_frame_interval[VS_F_NR_DESC] = {0x51615, 0x51615, 0x51615, 0x51615};
u8	vs_frame_mjpeg_frame_interval_type[VS_F_NR_DESC] = {0x3, 0x3, 0x3, 0x3};
u32	vs_frame_mjpeg_min_frame_interval[VS_F_NR_DESC] = {0x51615, 0x51615, 0x51615, 0x51615};
u32	vs_frame_mjpeg_max_frame_interval[VS_F_NR_DESC] = {0xA2C2A, 0xA2C2A, 0xA2C2A, 0xA2C2A};
u32	vs_frame_mjpeg_frame_interval_step[VS_F_NR_DESC] = {0xF4240, 0xF4240, 0xF4240, 0xF4240};

/* Data for usb_color_matching_descriptor */
#define VS_COLOR_PRIMARIES	0
#define VS_COLOR_XFER_CHARS	0
#define VS_COLOR_MATRIX_COEFF	0

#endif /* __LINUX_USB_VIDEO_H */
