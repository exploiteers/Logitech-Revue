/*
 *      guac.h -- Header for USB Audio Class (UVC) gadget driver
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      author: Ravi babu (ravibabu@ti.com), Ajay Kumar Gupta (ajay.gupta@ti.com)
 */
#ifndef __LINUX_USB_AUDIO_H
#define __LINUX_USB_AUDIO_H

#define UAC_CHAR_DEVNUM 	217 // uac major number
#define UAC_CHAR_DEVNAME 	"/dev/uac_g"

/* Audio  Device Class Codes */
#define USB_CC_AUDIO                   0x01

/* Audio Interface Subclass Codes */
#define USB_SC_UNDEFINED               0x00
#define USB_SC_AUDIOCONTROL            0x01
#define USB_SC_AUDIOTREAMING           0x02
#define USB_SC_INTERFACE_COLLECTION    0x03

/* Audio Interface Protocol Codes */
#define USB_PROTOCOL_UNDEFINED         0x00

/* Audio Class-Specific Descriptor Types */
#define USB_CS_UNDEFINED               0x20
#define USB_CS_DEVICE                  0x21
#define USB_CS_CONFIGURATION           0x22
#define USB_CS_STRING                  0x23
#define USB_CS_INTERFACE               0x24
#define USB_CS_ENDPOINT                0x25

/* Audio Class-Specific AC Interface Descriptor Subtypes */
#define USB_AC_DESCRIPTOR_UNDEFINED    0x00
#define USB_AC_HEADER                  0x01
#define USB_AC_INPUT_TERMINAL          0x02
#define USB_AC_OUTPUT_TERMINAL         0x03
#define USB_AC_MIXER_UNIT	       0x04
#define USB_AC_SELECTOR_UNIT           0x05
#define USB_AC_FEATURE_UNIT	       0x06
#define USB_AC_PROCESSING_UNIT         0x07
#define USB_AC_EXTERNSION_UNIT         0x08

/* Audio Class-Specific AS Interface Descriptor Subtype */
#define USB_AS_UNDEFINED               0x00
#define USB_AS_GENERAL    	       0x01
#define USB_AS_FORMAT_TYPE             0x02
#define USB_AS_FORMAT_SPECIFIC         0x03

/* Audio Processing Unit Process Types */
#define USB_AC_PROCESS_UNDEFINED       0x00
#define USB_AC_UPDOWN_PROCESS	       0x01
#define USB_AC_DOLBY_PROLOGIC_PROCESS  0x02
#define USB_AC_3D_STREO_EXTD		0x03
#define USB_AC_REVERB_PROCESS		0x04
#define USB_AC_CHOROUS_PROCESS		0x05
#define USB_AC_DYN_RANGE_COMP_PROCESS	0x06

/* Audio Class-Specific Endpoint Descriptor Subtype */
#define USB_EP_UNDEFINED               0x00
#define USB_EP_GENERAL                 0x01
#define USB_EP_ENDPOINT                0x02
#define USB_EP_INTERRUPT               0x03

/* Audio Class-Specific Request Codes */
#define RC_UNDEFINED                                    0x00
#define SET_CUR                                         0x01
#define SET_MIN                                         0x02
#define SET_MAX                                         0x03
#define SET_RES                                         0x04
#define SET_MEM						0x05
#define GET_CUR                                         0x81
#define GET_MIN                                         0x82
#define GET_MAX                                         0x83
#define GET_RES                                         0x84
#define GET_MEM                                         0x85
#define GET_STAT                                        0xff


/* Audio Control Interface Control Selectors */
#define VC_CONTROL_UNDEFINED            0x00
#define VC_VIDEO_POWER_MODE_CONTROL     0x01
#define VC_REQUEST_ERROR_CODE_CONTROL   0x02
/* Reserved 0x03 */

/* Audio Terminal Control Selectors */
#define USB_TE_CONTROL_UNDEFINED        0x00
#define USB_COPY_PROTECT_CONTROL	0x01

/* Audio Feature Unit Control Selectors */
#define	AC_FU_CONTROL_UNDEFINED		0x00
#define AC_FU_MUTE_CONTROL		0x01
#define AC_FU_VOLUME_CONTROL		0x02
#define AC_FU_BASS_CONTROL		0x03
#define AC_FU_MID_CONTROL		0x04
#define AC_FU_TREBLE_CONTROL		0x05
#define AC_FU_GRAPHIC_EQLZR_CONTROL	0x06
#define AC_FU_AUTO_GAIN_CONTROL		0x07
#define AC_FU_DELAY_CONTROL		0x08
#define AC_FU_BASS_BOOST_CONTROL	0x09
#define AC_FU_LOUDNESS_CONTROL		0x0A

/* Feature Unit Control Seclectors */
#define UNDEFINED_CONTROL 		0x00
#define MUTE_CONTROL   			0x0100
#define VOLUME_CONTROL 			0x0200

#define VOL_MIN      0x8000
#define VOL_MAX      0x7FFF
#define VOL_RES      0x000A

/* Audio Procesing Unit control */

/* Audio Procesing Unit UP/DOWN control */

/* Selector Unit Control Selectors */
#define SU_CONTROL_UNDEFINED       0x00
#define SU_INPUT_SELECT_CONTROL       0x01

/* Processing Unit Control Selectors */

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


/*
 * Interface/Terminal/Unit IDs, update the descriptor with these values
 */
// #define AC_INTERFACE_NUMBER     0 // Control Interface
// #define AS_INTERFACE_NUMBER     1 // streaming Interface


/* audio class descriptor declartions */
/* 4.3.1 std ac interface descriptor */
/* 4.3.2 class specific ac interface descriptor */
#define DECLARE_AC_CS_HEADER_DESC(name, nbInCollection, totlen, baIntfNr) \
struct s_##name { \
       __u8  bLength;	\
        __u8  bDescriptorType;	\
        __u8  bDescriptorSubType;	\
        __u16 bcdADC;	\
        __u16  wTotalLength; /* total nbytes for class specific AC intf descriptor */	\
        __u8  bInCollection; /* num of AS and MIDI Streaming intfs in AIC to which
                                AC intf belongs */	\
        __u8  baInterfaceNr[nbInCollection];	\
} __attribute__ ((packed)); \
struct s_##name name = { \
        .bLength = sizeof name,\
        .bDescriptorType = 0x24, /* CS_INTERFACE */ \
        .bDescriptorSubType = USB_AC_HEADER, /* ac header type type */ \
	.bcdADC = 0x100, /* Audio Device Class Spec release 1.00 */	\
	.wTotalLength = totlen, \
	.bInCollection = nbInCollection,	\
	.baInterfaceNr[0] = baIntfNr,	\
};
/* 4.3.2.1 audio input terminal descriptor */
struct ac_input_terminal_desc {
        __u8  bLength;
        __u8  bDescriptorType;    /* CS_INTERFACE desc type */
        __u8  bDescriptorSubType; /* IT descriptor sub type */
        __u8  bTerminalID;     /* unique ID for IT within audio function */
        __u16 wTerminalType;
        __u8  bAssocTerminal;  /* USB OUT/IN terminal type */
        __u8  bNrChannel;      /* Nr logical o/p chnl to which IT is associated */
        __u16 wChannelConfig;  /* bitmap: describes spatial loc of log channels */
        __u8  iChannelNames;   /* index of a string desc, name of 1st log channel */
        __u8  iTerminal;
} __attribute__ ((packed));

#define DECLARE_AC_INPUT_TERM_DESC(name, bTermId, bTermType, bAsocTerm,bNrChnls, wChanCfg,iChnlNames,iTermial) \
struct ac_input_terminal_desc name = { \
	.bLength = sizeof name,	\
        .bDescriptorType = 0x24,  /* CS_INTERFACE */	\
        .bDescriptorSubType = USB_AC_INPUT_TERMINAL, /* INPUT_TERMINAL desc type */	\
        .bTerminalID = bTermId, /* Unique terminal identifier */	\
        .wTerminalType = bTermType, /* audio streaming */ 	\
        .bAssocTerminal = bAsocTerm,  /* association to output terminal */	\
        .bNrChannel = bNrChnls,    /* Nr logical o/p chnl to which IT is associated */	\
        .wChannelConfig = wChanCfg,  /* bitmap: describes spatial loc of log channels */	\
        .iChannelNames = iChnlNames,   /* index of a string desc, name of 1st log channel */	\
        .iTerminal = iTermial	\
};	
/* 4.3.2.2 audio output terminal descriptors */
struct ac_output_terminal_desc {
        __u8  bLength;
        __u8  bDescriptorType;  /* CS_INTERFACE desc type */
        __u8  bDescriptorSubType; /* IT descriptor sub type */
        __u8  bTerminalID;
        __u16 wTerminalType;
        __u8  bAssocTerminal;
        __u8  bSourceID;
        __u8  iTerminal;
} __attribute__ ((packed));

#define DECLARE_AC_OUTPUT_TERM_DESC(name, bTermId, bTermType, bAsocTerm,bSrcID,iTermial) \
struct ac_output_terminal_desc name = { \
        .bLength = sizeof name, \
        .bDescriptorType = 0x24,  /* CS_INTERFACE */    \
        .bDescriptorSubType = USB_AC_OUTPUT_TERMINAL, /* OUTPUT_TERMINAL desc type */       \
        .bTerminalID = bTermId, /* Unique terminal identifier */        \
        .wTerminalType = bTermType, /* audio streaming */       \
        .bAssocTerminal = bAsocTerm,  /* association to input terminal */   \
	.bSourceID = bSrcID,	\
	.iTerminal = iTermial   \
};

/* 4.3.2.3 Mixer Unit Descriptor */
#define AC_MIXER_UNIT_SRCID_OFFS 	5
#define DECLARE_AC_MIXER_DESC(name, unitId, nbNrInPins, bNrChan, wChnlCfg,iChnlNames,bmCtrl,iMxer) \
struct s_##name { \
        __u8  bLength;	\
        __u8  bDescriptorType;	\
        __u8  bDescriptorSubType;	\
        __u8  bUnitID;	\
        __u8  bNrInPins;	\
        __u8  baSourceID[nbNrInPins];	\
        __u8  bNrChannels;	\
        __u16 wChannelConfig;	\
        __u8  iChannelNames;	\
        __u8  bmControls;	\
        __u8  iMixer;	\
} __attribute__ ((packed));	\
struct s_##name name = { \
        .bLength = sizeof name,\
        .bDescriptorType = 0x24, /* CS_INTERFACE */ \
        .bDescriptorSubType = USB_AC_MIXER_UNIT, /* mixer Unit desc type */ \
        .bUnitID = unitId, \
	.bNrInPins = nbNrInPins, 	\
	.bNrChannels = bNrChan, 	\
	.wChannelConfig = wChnlCfg,	\
	.iChannelNames = iChnlNames,	\
	.bmControls = bmCtrl,	\
	.iMixer = iMxer,	\
};
/* 4.3.2.4 Selector Unit Descriptor */
#define AC_SEL_UNIT_SRCID_OFFS 	5
#define DECLARE_AC_SELECTOR_DESC(name, unitId, nbNrInPins,iSel) \
struct s_##name { \
        __u8  bLength;  \
        __u8  bDescriptorType;  \
        __u8  bDescriptorSubType;       \
        __u8  bUnitID;  \
	__u8  bNrInPins;	\
        __u8  baSourceID[nbNrInPins];	\
        __u8  iSelector;	\
} __attribute__ ((packed));     \
struct s_##name name = { \
        .bLength = sizeof name,\
        .bDescriptorType = 0x24, /* CS_INTERFACE */ \
        .bDescriptorSubType = USB_AC_SELECTOR_UNIT, /* selector Unit desc type */ \
        .bUnitID = unitId, \
        .bNrInPins = nbNrInPins,        \
	.iSelector = iSel, \
};

/* 4.3.2.5 feature Unit Descriptor */
#define AC_FU_BMCTRLS_OFFS	6	
#define DECLARE_AUDIO_FU_DESC(name, unitId, srcId, ctrlSize,ch) \
struct s_##name { \
        __u8  bLength;  \
        __u8  bDescriptorType;   /* CS_INTERFACE desc type */           \
        __u8  bDescriptorSubType; /* IT descriptor sub type */                  \
        __u8  bUnitID;            /* unique Unit ID wihtin audio fn */  \
        __u8  bSourceID;          /* ID of Unit/Termnl to which feature unit belongs*/  \
        __u8  bControlSize;       /* size in bytes of an element of the bmaControls */ \
        __u8  bmaControls[ctrlSize*ch];   \
        __u8  iFeature;  \
} __attribute__ ((packed)); \
struct s_##name name = { \
        .bLength = sizeof name,\
        .bDescriptorType = 0x24, /* CS_INTERFACE */ \
        .bDescriptorSubType = USB_AC_FEATURE_UNIT, /* Feature Unit desc type */ \
        .bUnitID = unitId, \
        .bSourceID = srcId, \
        .bControlSize = ctrlSize, \
        .iFeature = 0\
};

/* processing unit descriptors */ 
struct ac_processing_unit_desc {
        __u8  bLength;
        __u8  bDescriptorType;
        __u8  bDescriptorSubType;
        __u8  bUnitID;
        __u8  bSourceID;
        __u8  wProcessType;
        __u8  bNrInPins;
        __u8  baSourceID0;
        __u8  bNrChannels;
        __u8  wChannelConfig;
        __u8  iChannelNames;
        __u8  bControlSize;
        __u8  bmControls;
        __u8  iProcessing;
} __attribute__ ((packed));


/* USB_CS_AUDIO STREAM INTERFACE_DESCRIPTOR */
struct usb_as_intf_desc {
        __u8  bLength;
        __u8  bDescriptorType;
        __u8  bDescriptorSubType;
        __u8  bTerminalLink;
        __u8  bDelay;
        __u16  wFormatTag;
}__attribute__ ((packed));

/* USB_CS_AUDIO STREAM ISO AUDIO DATA EP DESCRIPTOR */
struct usb_as_iso_ep_desc {
        __u8  bLength;
        __u8  bDescriptorType;
        __u8  bDescriptorSubType;
        __u8  bmAttributes; /* D0:SampFreq,D1:Pitch,D6:2-Resrvd,D7:MaxPackets only */
        __u8  bLockDelayUints;
        __u16  wLockDelay;
}__attribute__ ((packed));

/* USB_CS_INT_EP_INTERFACE_DESCRIPTOR */
struct usb_ep_int_desc {
        __u8  bLength;
        __u8  bDescriptorType;
        __u8  bDescriptorSubType;
        __u16 wMaxTransferSize;
} __attribute__ ((packed));

/* USB_CS_AUDIO STREAM FORMAT INTERFACE DESCRIPTOR */
struct usb_as_format_intf_desc {
        __u8  bLength;
        __u8  bDescriptorType;
        __u8  bDescriptorSubType;
        __u8  bFormatType;
        __u8  bNrChannels;
        __u8  bSubframeSize;
        __u8  bBitResolution;
        __u8  bSampFreqType;
        __u16 SamFreq0;
        __u16 SamFreq1;
        __u16 SamFreq2;
}__attribute__ ((packed));


#define UAC_TEST_START          0x10
#define UAC_BUF_READY           0x11
#define UAC_SUBMIT              0x12
#define UAC_FRAME_LEN               4096
#define UAC_NUM_BUFS            2
#define MAX_FU_UNITS            3
#define         WAV_FILE_SIZE           14592
#define         AUD_DATA_SIZE           3072

struct audio_fu_unit {
        void *desc;
        //struct usb_ac_feature_unit_desc *desc;
        u8  bUnitId;
        u16 vol_min;
        u16 vol_max;
        u16 vol_cur;
        u16 vol_res;
        u8  mute;
};
struct uac_inp_buf {
        int bufIndex;
        int len;
        unsigned int addr;
};
struct uac_inp_buf g_audio_buf;

struct uac_buf {
        int index;                      /* index number, 0 -> N-1 */
        unsigned int offset;    /* physical address of the buffer used in the mmap() */
        unsigned int kvaddr;    /* kernel virtual address */
        unsigned int size;      /* size of the buffer */
};

struct uac_g_dev {
        spinlock_t lock;
        u8                      config;
        struct usb_gadget       *gadget;
        struct usb_ep           *ep0;
        struct usb_request      *ep0req;
        struct usb_ep           *iso_out;
        struct usb_ep           *iso_in;

        struct usb_request *req;        /* for control responses */
        //struct timer_list resume;
        struct audio_fu_unit fu_ctrl[MAX_FU_UNITS];
        int cmd;
        u16 wIndex, wValue,wLength;
        int mute;
        int volume;
        u16 vol_min;
        u16 vol_max;
        u16 vol_res;
        u8 start;

        int req_done;
		int req_done1;
		int strm_started1;
		int eof_flag;
        int bfLen;
        int len_remaining;
        u8  *aud_buf[2];
        u8 *aud_data;

        int uac_buf_count;
        struct uac_buf uac_buffer[UAC_NUM_BUFS];
        struct usb_request *isocin_req[8];
        struct usb_request *intin_req[8];

};



#endif
