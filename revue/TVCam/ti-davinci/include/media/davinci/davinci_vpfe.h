/*
 * Copyright (C) 2007-2009 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _DAVINCI_VPFE_H
#define _DAVINCI_VPFE_H

#ifdef __KERNEL__

#include <linux/videodev.h>
#include <media/video-buf.h>
#include <media/davinci/vid_decoder_if.h>
#include <media/davinci/ccdc_hw_if.h>
#include <asm/arch/imp_common.h>

#define VPFE_CAPTURE_NUM_DECODERS        5

/* Macros */
#define VPFE_MAJOR_RELEASE              0
#define VPFE_MINOR_RELEASE              0
#define VPFE_BUILD                      1
#define VPFE_CAPTURE_VERSION_CODE       ((VPFE_MAJOR_RELEASE<<16) | \
	(VPFE_MINOR_RELEASE<<8)  | \
	VPFE_BUILD)

#define VPFE_VALID_FIELD(field)  ((V4L2_FIELD_ANY == field) || \
	(V4L2_FIELD_NONE == field) || \
	(V4L2_FIELD_INTERLACED == field) || \
	(V4L2_FIELD_SEQ_TB == field))

#define VPFE_VALID_BUFFER_TYPE(buftype)	{ \
			(V4L2_BUF_TYPE_VIDEO_CAPTURE == buftype) }

#define VPFE_CAPTURE_MAX_DEVICES	1
#define VPFE_MAX_DECODER_STD		50
#define VPFE_TIMER_COUNT		5
#define VPFE_SLICED_BUF_SIZE		256
#define VPFE_SLICED_MAX_SERVICES	3
#define VPFE_HBI_INDEX			2
#define VPFE_VBI_INDEX			1
#define VPFE_VIDEO_INDEX		0

/* Define 10 private CIDs for SoC CCDC */
#define VPFE_CCDC_CID_START		V4L2_CID_PRIVATE_BASE
#define VPFE_CCDC_CID_END		(V4L2_CID_PRIVATE_BASE + 10)

/* Define for device type to be passed in init */
#define 	MT9T001	0
#define		TVP5146	1
#define 	MT9T031	2
#define 	MT9P031	3
#define 	TVP7002	4

#define VPFE_NUMBER_OF_OBJECTS		1

/* Macros */
#define ISALIGNED(a)    (0 == (a%32))
#define ISEXTERNALCMD(cmd)	((VPFE_CMD_S_DECODER_PARAMS == cmd) || \
	(VPFE_CMD_G_DECODER_PARAMS == cmd) || \
	(VPFE_CMD_S_CCDC_PARAMS == cmd) || \
	(VPFE_CMD_G_CCDC_PARAMS == cmd) || \
	(VPFE_CMD_CONFIG_CCDC_YCBCR == cmd) || \
	(VPFE_CMD_CONFIG_CCDC_RAW == cmd) || \
	(VPFE_CMD_CONFIG_TVP5146 == cmd) || \
	(VPFE_CMD_S_MT9T001_PARAMS == cmd) || \
	(VPFE_CMD_G_MT9T001_PARAMS == cmd))

#include <media/v4l2-dev.h>
/* Buffer size defines for TVP5146 and MT9T001 */
#define VPFE_TVP5146_MAX_FRAME_WIDTH      768	/* for PAL Sqpixel mode */
#define VPFE_TVP5146_MAX_FRAME_HEIGHT     576	/* for PAL              */
/* 4:2:2 data */
#define VPFE_TVP5146_MAX_FBUF_SIZE      \
		 (VPFE_TVP5146_MAX_FRAME_WIDTH*VPFE_TVP5146_MAX_FRAME_HEIGHT*2)

#define VPFE_MT9T001_MAX_FRAME_WIDTH     (1920)
#define VPFE_MT9T001_MAX_FRAME_HEIGHT    (1080)

/* 2 BYTE FOR EACH PIXEL */
#define VPFE_MT9T001_MAX_FBUF_SIZE       \
		(VPFE_MT9T001_MAX_FRAME_WIDTH*VPFE_MT9T001_MAX_FRAME_HEIGHT*2)

#define VPFE_MAX_SECOND_RESOLUTION_SIZE  (640*480*2)

#define VPFE_PIXELASPECT_NTSC 		{11, 10}
#define VPFE_PIXELASPECT_PAL  		{54, 59}
#define VPFE_PIXELASPECT_NTSC_SP    	{1, 1}
#define VPFE_PIXELASPECT_PAL_SP     	{1, 1}
#define VPFE_PIXELASPECT_DEFAULT    	{1, 1}

#define ROUND32(x)	((((x) + 31) >> 5) << 5)

enum vpfe_irq_use_type {
	VPFE_USE_CCDC_IRQ,
	VPFE_USE_IMP_IRQ,
	VPFE_NO_IRQ
};

/* enumerated data types */
/* Enumerated data type to give id to each device per channel */
enum vpfe_channel_id {
	/* Channel0 Video */
	VPFE_CHANNEL0_VIDEO,
	/* Channel1 Video */
	VPFE_CHANNEL1_VIDEO
};

/* structures */
/* Table to keep track of the standards supported in all the decoders */
struct vpfe_decoder_std_tbl {
	u8 dec_idx;
	u8 std_idx;
	v4l2_std_id std;
};

#define VPFE_MAX_CAMERA_CAPT_PIX_FMTS	2
#define VPFE_MAX_YUV_CAPT_PIX_FMTS	2
#define VPFE_MAX_PREV_CAPT_PIX_FMTS	2
#define VPFE_MAX_PREV_RSZ_CAPT_PIX_FMTS	2

enum output_src {
	VPFE_CCDC_OUT,
	VPFE_IMP_PREV_OUT,
	VPFE_IMP_RSZ_OUT
};

struct vpfe_pixel_formats {
	unsigned int pix_fmt;
	char *desc;
	enum imp_pix_formats hw_fmt;
};

struct video_obj {
	/* Currently selected or default standard */
	v4l2_std_id std;
	enum v4l2_field buf_field;
	/* indicate whether to return most recent displayed frame only */
	u32 latest_only;
	/*Keeps track of the information about the standard */
	struct ccdc_std_info std_info;
	/* This is to track the last input that is passed
	 * to applicISALIGNEDation
	 */
	u32 input_idx;
	struct vpfe_decoder_std_tbl std_tbl[VPFE_MAX_DECODER_STD];
	int count_std;
};

struct vbi_obj {
	/* Counter to synchronize access to the timer */
	u8 timer_counter;
	/* An object of tasklets structure which is used for
	 * read  sliced vbi data from decoders
	 */
	struct tasklet_struct vbi_tasklet;
	u8 num_services;
};

struct common_obj {
	/* Buffer specific parameters */
	/* List of buffer pointers for storing frames */
	u8 *fbuffers[VIDEO_MAX_FRAME];
	/* number of buffers in fbuffers */
	u32 numbuffers;
	/* Pointer pointing to current
	 * v4l2_buffer
	 */
	struct videobuf_buffer *curFrm;
	/* Pointer pointing to next v4l2_buffer */
	struct videobuf_buffer *nextFrm;
	/* This field keeps track of type of buffer exchange mechanism
	 * user has selected
	 */
	enum v4l2_memory memory;
	/* Used to store pixel format */
	struct v4l2_format fmt;
	/* Buffer queue used in video-buf */
	struct videobuf_queue buffer_queue;
	/* Queue of filled frames */
	struct list_head dma_queue;
	/* Used in video-buf */
	spinlock_t irqlock;
	/* channel specifc parameters */
	/* lock used to access this structure */
	struct mutex lock;
	/* number of users performing IO */
	u32 io_usrs;
	/* Indicates whether streaming started */
	u8 started;
	/* offset where second field starts from the starting of
	 * the buffer for field seperated YCbCr formats
	 */
	u32 field_off;
	/* offset where second buffer starts from the starting of
	 * the buffer. This is for storing the second IPIPE resizer
	 * output
	 */
	u32 second_off;
	/* Indicates width of the image data or width of the vbi data */
	u32 width;
	/* Indicates height of the image data or height of the vbi data */
	u32 height;
	/* used when IMP is chained to store the crop window which
	 * is different from the image window
	 */
	struct v4l2_rect crop;
	/* To track if we need to attach IPIPE IRQ or CCDC IRQ */
	enum vpfe_irq_use_type irq_type;
	/* IMP update and dma interrupts */
	unsigned int ipipe_update_irq;
	unsigned int ipipe_dma_irq;
	/* Previewer is always present if IMP is chained */
	unsigned char imp_chained;
	/* Resizer is chained at the output of previewer */
	unsigned char rsz_present;
	/* if second resolution output is present */
	unsigned char second_output;
	/* Size of second output image */
	int second_out_img_sz;
	/* output from CCDC or IPIPE */
	enum output_src out_from;
	/* table for output pix fmts supported */
	struct vpfe_pixel_formats *pix_fmt_table;
	/* Max no. of pix format supported */
	int max_pix_fmts;
};

struct channel_obj {
	/* V4l2 specific parameters */
	/* Identifies video device for this channel */
	struct video_device *video_dev;
	/* Used to keep track of state of the priority */
	struct v4l2_prio_state prio;
	/* number of open instances of the channel */
	u32 usrs;
	/* Indicates id of the field which is being captured */
	u32 field_id;
	/* First full frame being captured for interlaced capture*/
	u8 first_frame;
	/* field one received */
	u8 fld_one_recvd;
	/* skip frame count */
	u8 skip_frame_count;
	/* skip frame count init value */
	u8 skip_frame_count_init;
	/* time per frame for skipping */
	struct v4l2_fract timeperframe;
	/* flag to indicate whether decoder is initialized */
	u8 initialized;
	/* Identifies channel */
	enum vpfe_channel_id channel_id;
	/* pointer to decoder device
	 *  structures for this channel
	 */
	struct decoder_device *decoder[VPFE_CAPTURE_NUM_DECODERS];
	/* indicates number of decoders registered to VPIF-V4L2 */
	u8 numdecoders;
	/* Index of the currently selected decoder */
	u8 current_decoder;
	/* Index of the defalut decoder, set as per boot args at
	 * channel creation time
	 */
	u8 default_decoder;
	struct common_obj common[VPFE_NUMBER_OF_OBJECTS];
	struct video_obj video;
	struct vbi_obj vbi;
};

/* File handle structure */
struct vpfe_fh {
	/* pointer to channel object for opened device */
	struct channel_obj *channel;
	/* Indicates whether this file handle is doing IO */
	u8 io_allowed[VPFE_NUMBER_OF_OBJECTS];
	/* Used to keep track priority of this instance */
	enum v4l2_priority prio;
	/* Used to indicate channel is initialize or not */
	u8 initialized;
};

/* vpfe device structure */
struct vpfe_device {
	struct channel_obj *dev[CCDC_CAPTURE_NUM_CHANNELS];
};

struct vpfe_config_params {
	u8 min_numbuffers;
	u8 numbuffers[CCDC_CAPTURE_NUM_CHANNELS];
	s8 device_type;
	u32 min_bufsize[CCDC_CAPTURE_NUM_CHANNELS];
	u32 channel_bufsize[CCDC_CAPTURE_NUM_CHANNELS];
	u8 default_device[CCDC_CAPTURE_NUM_CHANNELS];
	u8 max_device_type;
};
/* Struct which keeps track of the line numbers for the sliced vbi service */
struct vpfe_service_line {
	u16 service_id;
	u16 service_line[2];
};
#endif				/* End of __KERNEL__ */

/* Private CIDs for V4L2 control IOCTLs */
#define VPFE_CCDC_CID_R_GAIN		(V4L2_CID_PRIVATE_BASE + 0)
#define VPFE_CCDC_CID_GR_GAIN		(V4L2_CID_PRIVATE_BASE + 1)
#define VPFE_CCDC_CID_GB_GAIN		(V4L2_CID_PRIVATE_BASE + 2)
#define VPFE_CCDC_CID_B_GAIN		(V4L2_CID_PRIVATE_BASE + 3)
#define VPFE_CCDC_CID_OFFSET		(V4L2_CID_PRIVATE_BASE + 4)

/* IOCTLs */
#define VPFE_CMD_LATEST_FRM_ONLY \
			_IOW('V', BASE_VIDIOC_PRIVATE + 2, int)
#define VPFE_CMD_CONFIG_TVP5146 \
			_IOW('V', BASE_VIDIOC_PRIVATE + 3, void *)
#define VPFE_CMD_S_MT9T001_PARAMS \
			_IOW('V', BASE_VIDIOC_PRIVATE + 5, void *)
#define VPFE_CMD_G_MT9T001_PARAMS \
			_IOR('V', BASE_VIDIOC_PRIVATE + 6, void *)
#define VPFE_CMD_CONFIG_CCDC_YCBCR \
			_IOW('V', BASE_VIDIOC_PRIVATE + 1, void *)
#define VPFE_CMD_CONFIG_CCDC_RAW \
			_IOW('V', BASE_VIDIOC_PRIVATE + 4, void *)
#define VPFE_CMD_S_DECODER_PARAMS _IOW('V', BASE_VIDIOC_PRIVATE + 9, \
					void *)
#define VPFE_CMD_G_DECODER_PARAMS _IOR('V', BASE_VIDIOC_PRIVATE + 8, \
					void *)
#define VPFE_CMD_S_CCDC_PARAMS _IOW('V', BASE_VIDIOC_PRIVATE + 10, \
					void *)
#define VPFE_CMD_G_CCDC_PARAMS _IOW('V', BASE_VIDIOC_PRIVATE + 11, \
					void *)

#endif				/* _DAVINCI_VPFE_H */
