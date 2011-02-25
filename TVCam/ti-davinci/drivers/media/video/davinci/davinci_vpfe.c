/*
 * Copyright (C) 2006-2009 Texas Instruments Inc
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <asm/page.h>
#include <media/v4l2-dev.h>
#include <media/davinci/davinci_vpfe.h>
#include <asm/arch/imp_hw_if.h>
#include <asm/arch/vpss.h>
#if 0
#define DEBUG
#endif

static u32 device_type = TVP5146;
static u32 channel0_numbuffers = 3;
static u32 channel0_bufsize = (1920 * 1080 * 2) + (640 * 480 * 2);
module_param(device_type, uint, S_IRUGO);
module_param(channel0_numbuffers, uint, S_IRUGO);
module_param(channel0_bufsize, uint, S_IRUGO);

static struct vpfe_config_params config_params = {
	.min_numbuffers = 3,
	.numbuffers[0] = 3,
	.min_bufsize[0] = 720 * 480 * 2,
	/* Max buffer size is for one full resolution output
	 * and 1 Max VGA size output (thru previewer/resizer)
	 */
	.channel_bufsize[0] = ((1920 * 1080 * 2) + (640 * 480 * 2)),
	.default_device[0] = 0,
	.max_device_type = 3,
	.device_type = -1
};

static int vpfe_nr[] = { 0 };

/* CCDC hardware interface */
static struct ccdc_hw_interface *ccdc_hw_if;
/*  hardware interface */
static struct imp_hw_interface *imp_hw_if;

/* global variables */
static struct vpfe_device vpfe_obj = { {NULL} };

static struct device *vpfe_dev;

static struct v4l2_capability vpfe_videocap = {
	.driver = "ccdc capture",
	.card = "DaVinci EVM",
	.bus_info = "Platform",
	.version = VPFE_CAPTURE_VERSION_CODE,
	.capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING
};

/* Used when raw Bayer image from ccdc is directly captured to SDRAM */
static struct vpfe_pixel_formats
	vpfe_camera_capt_pix_fmts[VPFE_MAX_CAMERA_CAPT_PIX_FMTS] = {
	{
		.pix_fmt = V4L2_PIX_FMT_SBGGR8,
		.desc = "Raw Mode - Bayer Pattern GrRBGb - 8bit",
		.hw_fmt = IMP_BAYER_8BIT_PACK
	},
	{
		.pix_fmt = V4L2_PIX_FMT_SBGGR16,
		.desc = "Raw Mode - Bayer Pattern GrRBGb - 16bit",
		.hw_fmt = IMP_BAYER
	}
};

/* Used when raw YUV image from ccdc is directly captured to SDRAM */
static struct vpfe_pixel_formats
	vpfe_yuv_capt_pix_fmts[VPFE_MAX_YUV_CAPT_PIX_FMTS] = {
	{
		.pix_fmt = V4L2_PIX_FMT_UYVY,
		.desc = "YCbCr 4:2:2 Interleaved UYVY",
		/* Doesn't matter since ccdc doesn't use this */
		.hw_fmt = IMP_UYVY
	},
	{
		.pix_fmt = V4L2_PIX_FMT_YUYV,
		.desc = "YCbCr 4:2:2 Interleaved YUYV",
		/* Doesn't matter since ccdc doesn't use this */
		.hw_fmt = IMP_UYVY
	}
};

/* Used when previewer is chained at CCDC output */
static struct vpfe_pixel_formats vpfe_prev_capt_pix_fmts[] = {
	{
		.pix_fmt = V4L2_PIX_FMT_SBGGR16,
		.desc = "Raw Mode -Bayer Pattern GrRBGb - 16bit",
		.hw_fmt = IMP_BAYER
	},
	{
		.pix_fmt = V4L2_PIX_FMT_UYVY,
		.desc = "YCbCr 4:2:2 Interleaved UYVY",
		.hw_fmt = IMP_UYVY
	}
};

/* Used when previewer & resizer is chained at CCDC output */
static struct vpfe_pixel_formats vpfe_prev_rsz_capt_pix_fmts[] = {
	{
		.pix_fmt = V4L2_PIX_FMT_UYVY,
		.desc = "YCbCr 4:2:2 Interleaved UYVY",
		.hw_fmt = IMP_UYVY
	},
	{
		.pix_fmt = V4L2_PIX_FMT_NV12,
		.desc = "YCbCr 4:2:0, 2 plane - NV12",
		.hw_fmt = IMP_YUV420SP
	}
};

static int vpfe_enum_pix_format(struct v4l2_fmtdesc *fmtdesc,
				vid_capture_interface_type if_type)
{
	struct common_obj *common = NULL;
	struct channel_obj *channel = NULL;
	channel = vpfe_obj.dev[0];
	common = &(channel->common[VPFE_VIDEO_INDEX]);

	fmtdesc->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (fmtdesc->index < common->max_pix_fmts) {
		strcpy(fmtdesc->description,
		       common->pix_fmt_table[fmtdesc->index].desc);
		fmtdesc->pixelformat =
		    common->pix_fmt_table[fmtdesc->index].pix_fmt;
	} else
		return -1;
	return 0;
}

static struct vpfe_pixel_formats *vpfe_match_format(unsigned int pixfmt,
						    vid_capture_interface_type
						    if_type)
{
	struct v4l2_fmtdesc fmtdesc;
	struct channel_obj *channel = vpfe_obj.dev[0];
	struct common_obj *common = &(channel->common[VPFE_VIDEO_INDEX]);
	int err;
	fmtdesc.index = 0;

	while (!(err = vpfe_enum_pix_format(&fmtdesc, if_type))) {
		if (fmtdesc.pixelformat == pixfmt)
			break;
		fmtdesc.index++;
	}
	if (!err)
		return &common->pix_fmt_table[fmtdesc.index];
	return NULL;
}

static int vpfe_set_hw_type(vid_capture_interface_type if_type)
{
	enum ccdc_hw_if_type vpfe_if = CCDC_BT656;
	struct ccdc_hw_if_param hw_params;
	int ret = 0;

	switch (if_type) {
	case INTERFACE_TYPE_BT656:
		vpfe_if = CCDC_BT656;
		break;
	case INTERFACE_TYPE_BT1120:
		vpfe_if = CCDC_BT1120;
		break;
	case INTERFACE_TYPE_RAW:
		vpfe_if = CCDC_RAW_BAYER;
		break;
	case INTERFACE_TYPE_YCBCR_SYNC_8:
		vpfe_if = CCDC_YCBCR_SYNC_8;
		break;
	case INTERFACE_TYPE_YCBCR_SYNC_16:
		vpfe_if = CCDC_YCBCR_SYNC_16;
		break;
	case INTERFACE_TYPE_BT656_10BIT:
		vpfe_if = CCDC_BT656_10BIT;
		break;
	default:
		dev_err(vpfe_dev, "Unknown decoder interface\n");
		return -1;
	}
	ret = ccdc_hw_if->set_hw_if_type(vpfe_if);
	if (cpu_is_davinci_dm365()) {
		/* update the if parameters to imp hw interface */
		ret |= ccdc_hw_if->get_hw_if_params(&hw_params);
		if (imp_hw_if != NULL)
			ret |= imp_hw_if->set_hw_if_param(&hw_params);
	}
	return ret;
}

static void vpfe_update_pix_table_info(vid_capture_interface_type if_type,
				       struct common_obj *common)
{
	if (common->out_from == VPFE_CCDC_OUT) {
		if (if_type & INTERFACE_TYPE_RAW) {
			common->pix_fmt_table = vpfe_camera_capt_pix_fmts;
			common->max_pix_fmts = VPFE_MAX_CAMERA_CAPT_PIX_FMTS;
		} else {
			common->pix_fmt_table = vpfe_yuv_capt_pix_fmts;
			common->max_pix_fmts = VPFE_MAX_YUV_CAPT_PIX_FMTS;
		}
	} else {
		if (common->out_from == VPFE_IMP_PREV_OUT) {
			common->pix_fmt_table = vpfe_prev_capt_pix_fmts;
			common->max_pix_fmts = VPFE_MAX_PREV_CAPT_PIX_FMTS;
		} else if (common->out_from == VPFE_IMP_RSZ_OUT) {
			common->pix_fmt_table = vpfe_prev_rsz_capt_pix_fmts;
			if (cpu_is_davinci_dm365())
				common->max_pix_fmts =
				    VPFE_MAX_PREV_RSZ_CAPT_PIX_FMTS;
			else
				common->max_pix_fmts =
				    VPFE_MAX_PREV_RSZ_CAPT_PIX_FMTS - 1;
		} else {
			dev_err(vpfe_dev,
				"Unexpected! common->out_from"
				" not initialized\n");
		}
	}
}

/* vpfe_alloc_buffer : Allocate memory for buffers
 */
static inline unsigned long vpfe_alloc_buffer(unsigned int buf_size)
{
	void *mem = 0;
	u32 size = PAGE_SIZE << (get_order(buf_size));

	mem = (void *)__get_free_pages(GFP_KERNEL | GFP_DMA,
				       get_order(buf_size));
	if (mem) {
		unsigned long adr = (unsigned long)mem;
		while (size > 0) {
			SetPageReserved(virt_to_page(adr));
			adr += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
	}
	return (unsigned long)mem;
}

/* vpfe_free_buffer :  Free memory for buffers
 */
static inline void vpfe_free_buffer(unsigned long addr, unsigned int buf_size)
{
	unsigned int size, adr;

	if (!addr)
		return;
	adr = addr;
	size = PAGE_SIZE << (get_order(buf_size));
	while (size > 0) {
		ClearPageReserved(virt_to_page(adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	free_pages(addr, get_order(buf_size));
}

/*  vpfe_uservirt_to_phys : This inline function is used to
 *  convert user space virtual address to physical address.
 */
static inline u32 vpfe_uservirt_to_phys(u32 virtp)
{
	unsigned long physp = 0;
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	/* For kernel direct-mapped memory, take the easy way */
	if (virtp >= PAGE_OFFSET) {
		physp = virt_to_phys((void *)virtp);
	} else {
		vma = find_vma(mm, virtp);
		if (vma && (vma->vm_flags & VM_IO) &&
		   (vma->vm_pgoff)) {
			/* this will catch, kernel-allocated,
			mmaped-to-usermode addresses
			*/
			physp = (vma->vm_pgoff << PAGE_SHIFT) +
				(virtp - vma->vm_start);
		} else {
			/* otherwise, use get_user_pages() for
			general userland pages */
			int res, nr_pages = 1;
			struct page *pages;
			down_read(&current->mm->mmap_sem);

			res = get_user_pages(current, current->mm,
				     virtp, nr_pages, 1, 0, &pages, NULL);
			up_read(&current->mm->mmap_sem);

			if (res == nr_pages)
				physp = __pa(page_address(&pages[0]) +
				(virtp & ~PAGE_MASK));
			else {
				dev_err(vpfe_dev, "get_user_pages failed\n");
				return 0;
			}
		}
	}
	return physp;
}

/* buffer_prepare :  This is the callback function called from
 * videobuf_qbuf() function the buffer is prepared and user space
 * virtual address is converted into  physical address
 */
static int vpfe_buffer_prepare(struct videobuf_queue *q,
			       struct videobuf_buffer *vb,
			       enum v4l2_field field)
{
	/* Get the file handle object and channel object */
	struct vpfe_fh *fh = q->priv_data;
	struct channel_obj *channel = fh->channel;
	struct common_obj *common;
	unsigned long addr;

	dev_dbg(vpfe_dev, "<vpfe_buffer_prepare>\n");

	common = &(channel->common[VPFE_VIDEO_INDEX]);

	/* If buffer is not initialized, initialize it */
	if (STATE_NEEDS_INIT == vb->state) {
		vb->width = common->width;
		vb->height = common->height;
		/* TBD, how this size is used */
		if (common->imp_chained && common->second_output) {
			dev_info(vpfe_dev, "Second output present\n");
			vb->size =
			    (vb->width * vb->height * 2) +
			    VPFE_MAX_SECOND_RESOLUTION_SIZE;
		} else
			vb->size = vb->width * vb->height * 2;
		vb->field = field;
	}
	vb->state = STATE_PREPARED;
	/* if user pointer memory mechanism is used, get the physical
	 * address of the buffer
	 */
	if (V4L2_MEMORY_USERPTR == common->memory) {
		if (0 == vb->baddr) {
			dev_err(vpfe_dev, "buffer address is 0\n");
			return -EINVAL;
		}
		vb->boff = vpfe_uservirt_to_phys(vb->baddr);
		if (!ISALIGNED(vb->boff)) {
			dev_err(vpfe_dev, "buffer_prepare:offset is \
					not aligned to 32 bytes\n");
			return -EINVAL;
		}
	}
	addr = vb->boff;
	if (q->streaming) {
		if (!ISALIGNED((addr + common->second_off)) ||
		    !ISALIGNED((addr + common->field_off))) {
			dev_err(vpfe_dev, "buffer_prepare:offset is \
					not aligned to 32 bytes\n");
			return -EINVAL;
		}
	}

	dev_dbg(vpfe_dev, "</vpfe_buffer_prepare>\n");
	return 0;
}

/*  vpfe_buffer_config : This function is responsible to
 *  responsible for buffer's  physical address
 */
static void vpfe_buffer_config(struct videobuf_queue *q, unsigned int count)
{
	/* Get the file handle object and channel object */
	struct vpfe_fh *fh = q->priv_data;
	struct channel_obj *channel = fh->channel;
	struct common_obj *common;
	int i;
	dev_dbg(vpfe_dev, "<vpfe_buffer_config>\n");

	common = &(channel->common[VPFE_VIDEO_INDEX]);

	/* If memory type is not mmap, return */
	if (V4L2_MEMORY_MMAP != common->memory) {
		dev_dbg(vpfe_dev, "End of buffer config\n");
		return;
	}
	/* Convert kernel space virtual address to physical address */
	for (i = 0; i < count; i++) {
		q->bufs[i]->boff = virt_to_phys((u32 *) common->fbuffers[i]);
		dev_dbg(vpfe_dev, "buffer address: %x\n", q->bufs[i]->boff);
	}
	dev_dbg(vpfe_dev, "</vpfe_buffer_config>\n");
}

/* vpfe_buffer_setup : This function allocates memory for
 * the buffers
 */
static int vpfe_buffer_setup(struct videobuf_queue *q, unsigned int *count,
			     unsigned int *size)
{
	/* Get the file handle object and channel object */
	struct vpfe_fh *fh = q->priv_data;
	struct channel_obj *channel = fh->channel;
	struct common_obj *common;
	int i, startindex = 0;
	dev_dbg(vpfe_dev, "<vpfe_buffer_setup>\n");
	common = &(channel->common[VPFE_VIDEO_INDEX]);

	/* If memory type is not mmap, return */
	if (V4L2_MEMORY_MMAP != common->memory) {
		dev_dbg(vpfe_dev, "End of buffer setup\n");
		return 0;
	}

	/* Calculate the size of the buffer */
	if (V4L2_BUF_TYPE_VIDEO_CAPTURE == q->type) {
		/* Calculate the size of the buffer */
		*size = config_params.channel_bufsize[channel->channel_id];
		startindex = config_params.numbuffers[channel->channel_id];
	}

	for (i = startindex; i < *count; i++) {
		/* Allocate memory for the buffers */
		common->fbuffers[i] = (u8 *) vpfe_alloc_buffer(*size);
		if (!common->fbuffers[i])
			break;
	}
	/* Store number of buffers allocated in numbuffer member */
	*count = common->numbuffers = i;
	dev_dbg(vpfe_dev, "</vpfe_buffer_setup>\n");
	return 0;
}

/* vpfe_buffer_queue : This function adds the buffer to DMA queue
 */
static void vpfe_buffer_queue(struct videobuf_queue *q,
			      struct videobuf_buffer *vb)
{
	/* Get the file handle object and channel object */
	struct vpfe_fh *fh = q->priv_data;
	struct channel_obj *channel = fh->channel;
	struct common_obj *common = NULL;
	dev_dbg(vpfe_dev, "<vpfe_buffer_queue>\n");
	common = &(channel->common[VPFE_VIDEO_INDEX]);

	/* add the buffer to the DMA queue */
	list_add_tail(&vb->queue, &common->dma_queue);
	/* Change state of the buffer */
	vb->state = STATE_QUEUED;
	dev_dbg(vpfe_dev, "</vpfe_buffer_queue>\n");
}

/* vpfe_buffer_release : This function is called from the videobuf
 * layer to free memory allocated to  the buffers
 */
static void vpfe_buffer_release(struct videobuf_queue *q,
				struct videobuf_buffer *vb)
{
	/* Get the file handle object and channel object */
	struct vpfe_fh *fh = q->priv_data;
	struct channel_obj *channel = fh->channel;
	struct common_obj *common = NULL;
	dev_dbg(vpfe_dev, "<vpfe_buffer_release>\n");
	common = &(channel->common[VPFE_VIDEO_INDEX]);

	vb->state = STATE_NEEDS_INIT;

	/* If memory type is not mmap, return */
	if (V4L2_MEMORY_MMAP != common->memory) {
		dev_dbg(vpfe_dev, "End of buffer release\n");
		return;
	}
	dev_dbg(vpfe_dev, "</vpfe_buffer_release>\n");
}

static struct videobuf_queue_ops video_qops = {
	.buf_setup = vpfe_buffer_setup,
	.buf_prepare = vpfe_buffer_prepare,
	.buf_queue = vpfe_buffer_queue,
	.buf_release = vpfe_buffer_release,
	.buf_config = vpfe_buffer_config,
};

/*ISR for VINT0*/
static irqreturn_t vpfe_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	struct timeval timevalue;
	struct channel_obj *channel = NULL;
	struct common_obj *common = NULL;
	struct video_obj *vid_ch = NULL;
	struct vpfe_device *dev = dev_id;
	int fid;
	enum v4l2_field field;
	channel = dev->dev[VPFE_CHANNEL0_VIDEO];
	common = &(channel->common[VPFE_VIDEO_INDEX]);
	vid_ch = &(channel->video);
	field = common->fmt.fmt.pix.field;
	do_gettimeofday(&timevalue);

	dev_dbg(vpfe_dev, "\nStarting vpfe_isr...");

	/* only for 6446 this will be applicable */
	if (!(ISNULL(ccdc_hw_if->reset)))
		ccdc_hw_if->reset();

	if (0 == vid_ch->std_info.frame_format) {
		/* Interlaced. check which field we are in hardware */
		fid = ccdc_hw_if->getfid();
		/* switch the software maintained field id */
		channel->field_id ^= 1;
		dev_dbg(vpfe_dev, "field id = %x:%x.\n", fid,
			channel->field_id);
		if (fid == channel->field_id) {
			/* we are in-sync here,continue */
			if (fid == 0) {
				if (channel->first_frame)
					channel->fld_one_recvd = 1;
				else {
					/* One frame is just being captured.
					 * If the next frame is available,
					 * release the current frame and
					 * move on
					 */
					if (common->curFrm != common->nextFrm) {
						/* Copy frame capture time
						 * value in curFrm->ts
						 */
						common->curFrm->ts = timevalue;
						common->curFrm->state =
							STATE_DONE;
						common->curFrm->field = field;
						common->curFrm->size =
							common->fmt.fmt.
								pix.sizeimage;
						wake_up_interruptible(
							&common->curFrm->done);
						common->curFrm =
							common->nextFrm;
					}
					/* based on whether the two fields are
					 * stored interleavely or separately
					 * in memory, reconfigure the CCDC
					 * memory address
					 */
					if (common->out_from == VPFE_CCDC_OUT) {
						if (field ==
							V4L2_FIELD_SEQ_TB) {
							u32 addr =
							common->curFrm->boff +
							common->field_off;
							ccdc_hw_if->setfbaddr(
							  (unsigned long)addr);
						}
					}
				}
			} else if (fid == 1) {
				/* if one field is just being captured
				 * configure the next frame
				 * get the next frame from the empty queue
				 * if no frame is available
				 * hold on to the current buffer
				 */
				if (channel->first_frame) {
					if (channel->fld_one_recvd) {
						channel->first_frame = 0;
						channel->fld_one_recvd = 0;
					}
				}
				if (!channel->first_frame &&
					common->out_from == VPFE_CCDC_OUT) {
					if (!list_empty(&common->dma_queue) &&
						common->curFrm ==
							common->nextFrm) {
						common->nextFrm =
						    list_entry(common->
							       dma_queue.next,
							       struct
							       videobuf_buffer,
							       queue);
						list_del(&common->nextFrm->
							 queue);
						common->nextFrm->state =
						    STATE_ACTIVE;
						ccdc_hw_if->
						    setfbaddr((unsigned long)
							      common->nextFrm->
							      boff);
					}
				}
			}
		} else if (fid == 0) {
			/* recover from any hardware out-of-sync due to
			 * possible switch of video source
			 * for fid == 0, sync up the two fids
			 * for fid == 1, no action, one bad frame will
			 * go out, but it is not a big deal
			 */
			channel->field_id = fid;
		}
	} else if (1 == vid_ch->std_info.frame_format) {
		dev_dbg(vpfe_dev, "\nframe format is progressive...");
		if (common->curFrm != common->nextFrm) {
			/* Copy frame capture time value in curFrm->ts */
			common->curFrm->ts = timevalue;
			common->curFrm->state = STATE_DONE;
			common->curFrm->field = field;
			common->curFrm->size =
				common->fmt.fmt.pix.sizeimage;
			wake_up_interruptible(&common->curFrm->done);
			common->curFrm = common->nextFrm;
		}
		if (common->imp_chained) {
			channel->skip_frame_count--;
			if (!channel->skip_frame_count) {
				channel->skip_frame_count =
					channel->skip_frame_count_init;
				if (imp_hw_if->enable_resize)
					imp_hw_if->enable_resize(1);
			} else {
				if (imp_hw_if->enable_resize)
					imp_hw_if->enable_resize(0);
			}
		}
	}
	dev_dbg(vpfe_dev, "interrupt returned.\n");
	return IRQ_RETVAL(1);
}

static irqreturn_t vdint1_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	struct channel_obj *channel = NULL;
	struct common_obj *common = NULL;
	struct video_obj *vid_ch = NULL;
	struct vpfe_device *dev = dev_id;
	channel = dev->dev[VPFE_CHANNEL0_VIDEO];
	common = &(channel->common[VPFE_VIDEO_INDEX]);
	vid_ch = &(channel->video);

	dev_dbg(vpfe_dev, "\nInside vdint1_isr...");

	if (1 == vid_ch->std_info.frame_format) {
		if (!list_empty(&common->dma_queue)
		    && common->curFrm == common->nextFrm) {
				common->nextFrm =
					list_entry(common->dma_queue.next,
						struct videobuf_buffer, queue);
				list_del(&common->nextFrm->queue);
				common->nextFrm->state = STATE_ACTIVE;
				ccdc_hw_if->setfbaddr(
					(unsigned long)common->nextFrm->boff);
		}
	}
	return IRQ_RETVAL(1);
}

static irqreturn_t vpfe_imp_dma_isr(int irq, void *dev_id,
				struct pt_regs *regs)
{
	struct channel_obj *channel = NULL;
	struct common_obj *common = NULL;
	struct video_obj *vid_ch = NULL;
	struct vpfe_device *dev = dev_id;
	int fid;

	fid = ccdc_hw_if->getfid();

	channel = dev->dev[VPFE_CHANNEL0_VIDEO];
	common = &(channel->common[VPFE_VIDEO_INDEX]);
	vid_ch = &(channel->video);
	if (1 == vid_ch->std_info.frame_format) {
		if (!list_empty(&common->dma_queue) &&
			common->curFrm == common->nextFrm) {
			common->nextFrm =
			    list_entry(common->dma_queue.next,
				       struct videobuf_buffer, queue);
			list_del(&common->nextFrm->queue);
			common->nextFrm->state = STATE_ACTIVE;
			imp_hw_if->update_outbuf1_address(NULL,
						  common->nextFrm->boff);
			if (common->second_output)
				imp_hw_if->update_outbuf2_address(NULL,
						(common->nextFrm->boff +
						common->second_off));
		}
	} else if (fid == channel->field_id) {
		/* we are in-sync here,continue */
		if (fid == 1 &&
			!list_empty(&common->dma_queue) &&
			common->curFrm == common->nextFrm) {
			common->nextFrm =
			    list_entry(common->dma_queue.next,
					       struct videobuf_buffer,
					       queue);
			list_del(&common->nextFrm->queue);
			common->nextFrm->state = STATE_ACTIVE;
			imp_hw_if->update_outbuf1_address(NULL,
				common->nextFrm->boff);
			if (common->second_output)
				imp_hw_if->update_outbuf2_address
					(NULL,
					(common->nextFrm->boff +
					common->second_off));
		}
	}
	return IRQ_RETVAL(1);
}

static int vpfe_get_decoder_std_info(struct channel_obj *ch,
	struct v4l2_standard *std)
{
	int ret = 0;
	struct decoder_device *dec = ch->decoder[ch->current_decoder];
	struct video_obj *vid_ch = &(ch->video);
	int found = 0;

	if (ISNULL(dec->std_ops)
	    || ISNULL(dec->std_ops->enumstd)) {
		dev_err(vpfe_dev, "vpfe_get_decoder_std_info:No getstd\n");
		ret = -EINVAL;
	} else {
		std->index = 0;
		/* Call getstd function of decoder device */
		do {
			ret = dec->std_ops->enumstd(std, dec);
			if (!ret && (std->id & vid_ch->std)) {
				found = 1;
				break;
			}
			std->index++;
		} while (ret >= 0);
	}
	if (found)
		return 0;
	return ret;
}

static int vpfe_get_std_info(struct channel_obj *ch)
{
	struct video_obj *vid_ch = &(ch->video);
	struct v4l2_standard standard;
	int ret = 0;
	enum ccdc_frmfmt frm_fmt;

	vid_ch->std_info.channel_id = ch->channel_id;

	if (vpfe_get_decoder_std_info(ch, &standard) < 0) {
		dev_err(vpfe_dev, "Unable to get standard from decoder\n");
		return -1;
	}

	strncpy(vid_ch->std_info.name, standard.name,
		sizeof(vid_ch->std_info.name));

	ch->timeperframe = standard.frameperiod;

	/* Get standard information from CCDC */
	ret = ccdc_hw_if->setstd(vid_ch->std_info.name);
	if (ret < 0) {
		dev_err(vpfe_dev,
			"vpfe_get_std_info: Error in set std in decoder\n");
		return -1;
	}
	ret = ccdc_hw_if->getstd_info(&vid_ch->std_info);
	if (ret < 0) {
		dev_err(vpfe_dev,
			"vpfe_get_std_info: Error in get stdinfo from CCDC\n");
		return -1;
	}
	ch->common[VPFE_VIDEO_INDEX].fmt.fmt.pix.width =
	    ch->common[VPFE_VIDEO_INDEX].width = vid_ch->std_info.activepixels;
	ch->common[VPFE_VIDEO_INDEX].fmt.fmt.pix.height =
	    ch->common[VPFE_VIDEO_INDEX].height = vid_ch->std_info.activelines;
	ccdc_hw_if->get_line_length(
		&(ch->common[VPFE_VIDEO_INDEX].fmt.fmt.pix.bytesperline));
	ccdc_hw_if->get_frame_format(&frm_fmt);
	if (frm_fmt == CCDC_FRMFMT_INTERLACED) {
		ch->common[VPFE_VIDEO_INDEX].fmt.fmt.pix.field =
		    V4L2_FIELD_INTERLACED;
	} else {
		ch->common[VPFE_VIDEO_INDEX].fmt.fmt.pix.field =
		    V4L2_FIELD_NONE;
	}
	ch->common[VPFE_VIDEO_INDEX].fmt.fmt.pix.sizeimage =
	    ch->common[VPFE_VIDEO_INDEX].fmt.fmt.pix.bytesperline *
	    ch->common[VPFE_VIDEO_INDEX].fmt.fmt.pix.height;

	/* Save crop info */
	ch->common[VPFE_VIDEO_INDEX].crop.left = 0;
	ch->common[VPFE_VIDEO_INDEX].crop.top = 0;
	ch->common[VPFE_VIDEO_INDEX].crop.width =
	    vid_ch->std_info.activepixels;;
	ch->common[VPFE_VIDEO_INDEX].crop.height = vid_ch->std_info.activelines;
	return ret;
}

/* vpfe_calculate_offsets : This function calculates buffers offset
 *  for top and bottom field
 */
static void vpfe_calculate_offsets(struct channel_obj *channel)
{
	struct video_obj *vid_ch = &(channel->video);
	struct common_obj *common = &(channel->common[VPFE_VIDEO_INDEX]);
	struct v4l2_pix_format *pix = &common->fmt.fmt.pix;
	enum v4l2_field field = pix->field;

	struct v4l2_rect image_win;

	dev_dbg(vpfe_dev, "<vpfe_calculate_offsets>\n");

	if (V4L2_FIELD_ANY == field) {
		if (vid_ch->std_info.frame_format)
			field = V4L2_FIELD_NONE;
		else
			field = V4L2_FIELD_INTERLACED;
	}

	common->second_off = 0;
	common->field_off = 0;

	if (!common->imp_chained) {
		ccdc_hw_if->get_image_window(&image_win);
		common->field_off = (image_win.height - 2) * image_win.width;
		common->field_off = (common->field_off + 31) & ~0x1F;

	} else {
		if (common->second_output)
			common->second_off = pix->sizeimage;

		/* Align the address to 32 byte boundary */
		common->field_off = (common->field_off + 31) & ~0x1F;
		common->second_off = (common->second_off + 31) & ~0x1F;
	}
	dev_dbg(vpfe_dev, "</vpfe_calculate_offsets>\n");
}

static int vpfe_detach_irq(struct channel_obj *channel)
{
	struct common_obj *common = &(channel->common[VPFE_VIDEO_INDEX]);
	enum ccdc_frmfmt frame_format;

	/* First clear irq if already in use */
	switch (common->irq_type) {
	case VPFE_USE_CCDC_IRQ:
		ccdc_hw_if->get_frame_format(&frame_format);
		vpss_free_irq(VPSS_VDINT0, &vpfe_obj);
		if (frame_format == CCDC_FRMFMT_PROGRESSIVE)
			vpss_free_irq(VPSS_VDINT1, &vpfe_obj);
		common->irq_type = VPFE_NO_IRQ;
		break;
	case VPFE_USE_IMP_IRQ:
		vpss_free_irq(VPSS_VDINT0, &vpfe_obj);
		vpss_free_irq(common->ipipe_dma_irq, &vpfe_obj);
		common->irq_type = VPFE_NO_IRQ;
		break;
	case VPFE_NO_IRQ:
		break;
	default:
		return -1;
	}
	return 0;
}

static int vpfe_attach_irq(struct channel_obj *channel,
			   vid_capture_interface_type if_type)
{
	struct common_obj *common = &(channel->common[VPFE_VIDEO_INDEX]);
	enum ccdc_frmfmt frame_format;
	int err = 0;

	common->irq_type = VPFE_USE_CCDC_IRQ;

	if (common->imp_chained)
		common->irq_type = VPFE_USE_IMP_IRQ;

	switch (common->irq_type) {
	case VPFE_USE_CCDC_IRQ:
		{
			ccdc_hw_if->get_frame_format(&frame_format);
			err = vpss_request_irq(VPSS_VDINT0, vpfe_isr,
						SA_INTERRUPT, "vpfe_capture0",
						(void *)&vpfe_obj);

			if (err < 0) {
				dev_err(vpfe_dev, "Error in requesting VINT0 ");
				return -1;
			}
			if (frame_format == CCDC_FRMFMT_PROGRESSIVE) {
				err =
				    vpss_request_irq(VPSS_VDINT1, vdint1_isr,
						     SA_INTERRUPT,
						     "vpfe_capture1",
						     (void *)&vpfe_obj);
				if (err < 0) {
					dev_err(vpfe_dev, "Error in requesting"
							  " VINT1 ");
					vpss_free_irq(VPSS_VDINT0, &vpfe_obj);
					return -1;
				}
			}
		}
		break;
	case VPFE_USE_IMP_IRQ:
		{
			struct irq_numbers irq_info;
			if (common->rsz_present)
				imp_hw_if->get_rsz_irq(&irq_info);
			else
				imp_hw_if->get_preview_irq(&irq_info);

			err = vpss_request_irq(VPSS_VDINT0, vpfe_isr,
						SA_INTERRUPT, "vpfe_capture0",
						(void *)&vpfe_obj);

			if (err < 0) {
				dev_err(vpfe_dev, "Error in requesting VINT0 ");
				return -1;
			}

			common->ipipe_dma_irq = irq_info.sdram;
			err = vpss_request_irq(irq_info.sdram, vpfe_imp_dma_isr,
					       SA_INTERRUPT,
					       "Imp_Sdram_Irq", &vpfe_obj);

			if (err < 0) {
				dev_err(vpfe_dev,
					"Error in requesting SDR write"
					"completion interrupt\n");
				vpss_free_irq(VPSS_VDINT0, &vpfe_obj);
				return -1;
			}
		}
		break;
	default:
		return -1;
	}
	return 0;
}
static int vpfe_config_format(struct channel_obj *channel)
{
	struct decoder_device *dec =
		channel->decoder[channel->current_decoder];
	struct common_obj *common =
		&(channel->common[VPFE_VIDEO_INDEX]);

	common->fmt.fmt.pix.field = V4L2_FIELD_ANY;

	if (config_params.numbuffers[channel->channel_id] == 0)
		common->memory = V4L2_MEMORY_USERPTR;
	else
		common->memory = V4L2_MEMORY_MMAP;

	common->fmt.fmt.pix.sizeimage
	    = common->fmt.fmt.pix.bytesperline * common->fmt.fmt.pix.height;

	if (dec->if_type & INTERFACE_TYPE_RAW)
		common->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR8;
	else
		common->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	if (vpfe_set_hw_type(dec->if_type)) {
		dev_err(vpfe_dev, "Error in setting interface type\n");
		return -1;
	}
	vpfe_update_pix_table_info(dec->if_type, common);
	common->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	common->crop.top = 0;
	common->crop.left = 0;
	return 0;
}

static int vpfe_config_std(struct channel_obj *channel)
{
	int ret = 0;
	struct decoder_device *dec =
		channel->decoder[channel->current_decoder];
	struct video_obj *vid_ch = &(channel->video);
	struct common_obj *common = &(channel->common[VPFE_VIDEO_INDEX]);

	/* Detect the standard from the devide */
	ret = dec->std_ops->getstd(&vid_ch->std, dec);
	if (ret)
		return ret;

	ret = vpfe_get_std_info(channel);

	/* Change the pixel format as per the new decoder */
	if (dec->if_type & INTERFACE_TYPE_RAW)
		common->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SBGGR8;
	else
		common->fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	return ret;
}

static int vpfe_try_format(struct channel_obj *channel,
		     struct v4l2_pix_format *pixfmt, int check)
{
	struct decoder_device *dec =
		channel->decoder[channel->current_decoder];
	struct video_obj *vid_ch = &(channel->video);
	struct common_obj *common = &(channel->common[VPFE_VIDEO_INDEX]);
	struct vpfe_pixel_formats *pix_fmts;
	int sizeimage, hpitch, vpitch, bpp, min_height = 1, min_width = 32,
		max_width, max_height;

	pix_fmts = vpfe_match_format(pixfmt->pixelformat, dec->if_type);
	if (dec->if_type & INTERFACE_TYPE_RAW) {
		if (ISNULL(pix_fmts)) {
			if (check) {
				dev_err(vpfe_dev, "Invalid pixel format\n");
				return -1;
			}
			pixfmt->pixelformat = V4L2_PIX_FMT_SBGGR8;
		}

		if (pixfmt->field == V4L2_FIELD_ANY)
			pixfmt->field = V4L2_FIELD_NONE;

		if (pixfmt->field != V4L2_FIELD_NONE) {
			if (check) {
				dev_err(vpfe_dev, "Invalid field format\n");
				return -1;
			} else
				pixfmt->field = V4L2_FIELD_NONE;
		}
	} else {
		if (ISNULL(pix_fmts)) {
			if (check)
				return -1;
			pixfmt->pixelformat = V4L2_PIX_FMT_UYVY;
		}

		if (pixfmt->field == V4L2_FIELD_ANY)
			pixfmt->field = V4L2_FIELD_INTERLACED;

		if (1 == vid_ch->std_info.frame_format) {
			if (pixfmt->field != V4L2_FIELD_NONE) {
				if (check) {
					dev_err(vpfe_dev, "Invalid field format\n");
					return -1;
				} else
					pixfmt->field = V4L2_FIELD_NONE;
			}
		} else {
			if (pixfmt->field != V4L2_FIELD_INTERLACED &&
			    pixfmt->field != V4L2_FIELD_SEQ_TB) {
				if (check) {
					dev_err(vpfe_dev, "Invalid field format\n");
					return -1;
				} else
					pixfmt->field = V4L2_FIELD_INTERLACED;
			}
			min_height = 2;
		}
	}
	max_width = vid_ch->std_info.activepixels;
	max_height = vid_ch->std_info.activelines;
	if (common->imp_chained) {
		/* check with imp hw for the limits */
		max_width  = imp_hw_if->get_max_output_width(0);
		max_height = imp_hw_if->get_max_output_height(0);
	}
	if ((pixfmt->pixelformat == V4L2_PIX_FMT_SBGGR8) ||
	    (pixfmt->pixelformat == V4L2_PIX_FMT_NV12))
		bpp = 1;
	else
		bpp = 2;

	min_width /= bpp;
	if (V4L2_MEMORY_USERPTR == common->memory) {
		sizeimage = pixfmt->sizeimage;
		/* calculate vpitch based on pixel format. Assume application
		   calculates sizeimage based on pixel format
		 */
		if ((pixfmt->bytesperline == 0) || (sizeimage == 0)) {
			if (check) {
				dev_err(vpfe_dev, "invalid bytesperline\n");
				return -EINVAL;
			}
		}

		/* Check for 32 byte alignment */
		if (pixfmt->bytesperline % 32) {
			if (check) {
				dev_err(vpfe_dev,
					"invalid horizontal pitch alignment\n");
				return -EINVAL;
			}

			/* adjust bytesperline to align with hardware */
			pixfmt->bytesperline =
				((pixfmt->bytesperline + 31) & ~31);
		}
		hpitch = pixfmt->bytesperline/bpp;
		if (common->fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_NV12) {
			/* for NV12 format, sizeimage is y-plane size
			 * + CbCr plane which is half of y-plane
			 */
			vpitch = sizeimage /
				(pixfmt->bytesperline +
				(pixfmt->bytesperline >> 1));
		} else
			vpitch = sizeimage / pixfmt->bytesperline ;
	} else {
		hpitch = pixfmt->width;
		vpitch = pixfmt->height;
	}
	dev_info(vpfe_dev, "hpitch = %d, vpitch = %d, bpp = %d\n", hpitch,
		 vpitch, bpp);
	if (hpitch < min_width) {
		if (check) {
			dev_err(vpfe_dev, "width is less than minimum\n");
			return -EINVAL;
		}
		hpitch = min_width;
	}
	if (vpitch < min_height) {
		if (check) {
			dev_err(vpfe_dev, "height is less than minimum\n");
			return -EINVAL;
		}
		vpitch = min_height;
	}
	/* Check for upper limits of pitch */
	if (hpitch > max_width) {
		if (check) {
			dev_err(vpfe_dev, "width is more than maximum\n");
			return -EINVAL;
		}
		hpitch = max_width;
	}
	if (vpitch > max_height) {
		if (check) {
			dev_err(vpfe_dev, "height is more than maximum\n");
			return -EINVAL;
		}
		vpitch = max_height;
	}
	/* recalculate bytesperline and sizeimage since width
	 * and height might have changed
	 */
	pixfmt->bytesperline = (((hpitch * bpp) + 31) & ~31);
	if (pixfmt->pixelformat == V4L2_PIX_FMT_NV12)
		pixfmt->sizeimage = pixfmt->bytesperline * vpitch +
				((pixfmt->bytesperline * vpitch) >> 1);
	else
		pixfmt->sizeimage = pixfmt->bytesperline * vpitch;
	pixfmt->width = hpitch;
	pixfmt->height = vpitch;
	return 0;
}

static int vpfe_convert_index(struct channel_obj *channel, int *index,
			      int *dec_index)
{
	int i, suminput = 0;
	struct decoder_device *dec;
	*dec_index = 0;
	for (i = 0; i < channel->numdecoders; i++) {
		dec = channel->decoder[i];
		suminput += dec->input_ops->count;
		if ((*index) < suminput) {
			*dec_index = i;
			suminput -= dec->input_ops->count;
			break;
		}
	}
	*index -= suminput;
	if (i == channel->numdecoders)
		return -EINVAL;

	return 0;
}

static void vpfe_get_crop_cap(struct v4l2_cropcap *crop_cap,
			      struct channel_obj *channel)
{
	struct video_obj *vid_ch = &(channel->video);
	crop_cap->bounds.left = 0;
	crop_cap->bounds.top = 0;
	crop_cap->defrect.left = 0;
	crop_cap->defrect.top = 0;
	crop_cap->defrect.height = vid_ch->std_info.activelines;
	crop_cap->bounds.height = vid_ch->std_info.activelines;
	crop_cap->defrect.width = vid_ch->std_info.activepixels;
	crop_cap->bounds.width = vid_ch->std_info.activepixels;
	crop_cap->pixelaspect.numerator =
		vid_ch->std_info.pixaspect.numerator;
	crop_cap->pixelaspect.denominator =
		vid_ch->std_info.pixaspect.denominator;
}

/*  vpfe_doioctl : This function will provide different V4L2 commands.
 *  This function can be used to configure driver or get status of
 *  driver as per command passed  by application
 */
static int vpfe_doioctl(struct inode *inode, struct file *file,
			unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct vpfe_fh *fh = file->private_data;
	struct channel_obj *channel = fh->channel;
	struct video_obj *vid_ch = &(channel->video);
	unsigned int index = 0, i = 0;
	unsigned long addr, flags;
	struct decoder_device *dec =
		channel->decoder[channel->current_decoder];
	dev_dbg(vpfe_dev, "<vpfe_doioctl>\n");

	if (VPFE_CHANNEL0_VIDEO == channel->channel_id) {
		switch (cmd) {
		case VIDIOC_G_INPUT:
		case VIDIOC_S_STD:
		case VIDIOC_S_INPUT:
		case VIDIOC_S_FMT:
		case VIDIOC_REQBUFS:
		case VPFE_CMD_S_CCDC_PARAMS:
		case VPFE_CMD_G_CCDC_PARAMS:
		case VPFE_CMD_CONFIG_CCDC_YCBCR:
		case VPFE_CMD_CONFIG_CCDC_RAW:
		case VPFE_CMD_S_DECODER_PARAMS:
			if (!fh->initialized) {
				dev_dbg(vpfe_dev, "channel busy\n");
				return -EBUSY;
			}
			break;
		default:
			/* nothing */
			;
		}

	}
	/* Check for the priority */
	if (VPFE_CHANNEL0_VIDEO == channel->channel_id) {
		switch (cmd) {
		case VIDIOC_S_STD:
		case VIDIOC_S_INPUT:
		case VIDIOC_S_FMT:
			ret = v4l2_prio_check(&channel->prio, &fh->prio);
			if (0 != ret)
				return ret;
			fh->initialized = 1;
			break;
		default:
			/* nothing */
			;
		}
	}

	/* Check for null value of parameter */
	if (ISNULL((void *)arg)) {
		dev_err(vpfe_dev, "Null pointer\n");
		return -EINVAL;
	}
	/* Switch on the command value */
	switch (cmd) {
	/* If the case is for querying capabilities */
	case VIDIOC_QUERYCAP:
		{
			struct v4l2_capability *cap =
			    (struct v4l2_capability *)arg;
			dev_dbg(vpfe_dev, "VIDIOC_QUERYCAP\n");
			memset(cap, 0, sizeof(*cap));
			if ((VPFE_CHANNEL0_VIDEO == channel->channel_id)) {
				*cap = vpfe_videocap;
				cap->capabilities |= dec->capabilities;
			} else
				ret = -EINVAL;
			break;
		}

		/* If the case is for enumerating inputs */
	case VIDIOC_ENUMINPUT:
		{
			int index, index1;
			struct v4l2_input *input = (struct v4l2_input *)arg;
			dev_dbg(vpfe_dev, "VIDIOC_ENUMINPUT\n");

			index = input->index;
			index1 = input->index;
			/* Map the index to the index of the decoder */
			ret = vpfe_convert_index(channel, &index, &i);
			if (ret)
				break;
			input->index = index;
			/* Call enuminput of the new decoder */
			ret = channel->decoder[i]->input_ops->
			    enuminput(input, channel->decoder[i]);
			input->index = index1;
			break;
		}

		/* If the case is for getting input */
	case VIDIOC_G_INPUT:
		{
			int i;
			int temp_index;
			struct decoder_device *cur_dec;
			int sumindex = 0;
			struct common_obj *common =
			    &(channel->common[VPFE_VIDEO_INDEX]);

			dev_dbg(vpfe_dev, "VIDIOC_G_INPUT\n");
			ret = mutex_lock_interruptible(&common->lock);
			if (ret)
				break;

			if (ISNULL(dec->input_ops)
			    || ISNULL(dec->input_ops->getinput))
				ret = -EINVAL;
			else {
				ret =
				    dec->input_ops->getinput(&temp_index,
							     dec);
				if (!ret) {
					for (i = 0; i < channel->numdecoders;
						i++) {
						cur_dec = channel->decoder[i];
						if (cur_dec->input_ops) {
							if (!strcmp(
							     dec->name,
							     cur_dec->name)) {
								*(int *)arg =
								sumindex +
								temp_index;
								break;
							}
							sumindex +=
								cur_dec->
								input_ops->
								count;
						}
					}
					if (i == channel->numdecoders) {
						/* no matching decoder */
						ret = -EINVAL;
					}
				}
			}
			mutex_unlock(&common->lock);
			break;
		}

		/* If the case is for setting input */
	case VIDIOC_S_INPUT:
		{
			int index = *(int *)arg;
			int index1 = index;
			struct common_obj *common =
			    &(channel->common[VPFE_VIDEO_INDEX]);
			dev_dbg(vpfe_dev, "VIDIOC_S_INPUT\n");
			/* If streaming is started return device busy
			 * error
			 */
			ret = mutex_lock_interruptible(&common->lock);
			if (ret)
				break;
			if (common->started) {
				dev_err(vpfe_dev, "Streaming is on\n");
				ret = -EBUSY;
				mutex_unlock(&common->lock);
				break;
			}

			if (ISNULL(dec->input_ops)
			    || ISNULL(dec->input_ops->setinput) ||
			    ISNULL(dec->deinitialize)
			    || ISNULL(dec->initialize)) {
				dev_err(vpfe_dev, "vpfe_doioctl:No setinput\n");
				mutex_unlock(&common->lock);
				ret = -EINVAL;
				break;
			}

			/* Map the index to the index of the decoder */
			ret = vpfe_convert_index(channel, &index, &i);

			if (ret) {
				mutex_unlock(&common->lock);
				break;
			}
			/* new decoder to be active */
			dec = channel->decoder[channel->current_decoder];
			channel->current_decoder = i;
			/* Deinitialize the previous decoder */
			dec->deinitialize(dec);

			/* Initialize the new decoder */
			dec = channel->decoder[i];
			ret = dec->initialize(dec, DECODER_FULL_INIT_FLAG);

			/* Set the input in the decoder */
			ret |= dec->input_ops->setinput(&index, dec);
			if (ret) {
				mutex_unlock(&common->lock);
				dev_err(vpfe_dev, "vpfe_doioctl:error in"
					" setting input in decoder \n");
				ret = -EINVAL;
				break;
			}

			*(int *)arg = index1;
			if (vpfe_set_hw_type(dec->if_type)) {
				dev_err(vpfe_dev, "vpfe_doioctl:error in"
					" setting interface type \n");
				ret = -EINVAL;
			} else {
				vpfe_update_pix_table_info(dec->if_type,
							   common);
				vpfe_config_std(channel);
			}
			mutex_unlock(&common->lock);
			break;
		}
		/* If the case is for enumerating standards */
	case VIDIOC_ENUMSTD:
		{
			struct v4l2_standard *std = (struct v4l2_standard *)arg;
			dev_dbg(vpfe_dev, "VIDIOC_ENUMSTD\n");
			/* Call enumstd function of decoder device */
			if (ISNULL(dec->std_ops)
			    || ISNULL(dec->std_ops->enumstd)) {
				dev_err(vpfe_dev, "vpfe_doioctl:No enumstd\n");
				ret = -EINVAL;
				break;
			}
			ret = dec->std_ops->enumstd(std, dec);
			break;
		}

		/* If the case is for querying standards */
	case VIDIOC_QUERYSTD:
		{
			v4l2_std_id *std = (v4l2_std_id *) arg;
			struct common_obj *common =
			    &(channel->common[VPFE_VIDEO_INDEX]);
			struct video_obj *vid_ch = &(channel->video);
			dev_dbg(vpfe_dev, "VIDIOC_QUERYSTD\n");
			if (ISNULL(dec->std_ops)
			    || ISNULL(dec->std_ops->querystd)) {
				dev_err(vpfe_dev, "vpfe_doioctl:No querystd\n");
				ret = -EINVAL;
			} else {
				ret = mutex_lock_interruptible(&common->lock);
				if (ret)
					break;
				/* Call querystd function of decoder device */
				ret = dec->std_ops->querystd(std, dec);

				if (!ret) {
					/* Get the information about the
					 * standard from the decoder
					 */
					vid_ch->std = *std;
					ret = vpfe_get_std_info(channel);
				}
				mutex_unlock(&common->lock);
			}
			break;
		}

		/* If the case is for getting standard */
	case VIDIOC_G_STD:
		{
			v4l2_std_id *std = (v4l2_std_id *) arg;
			dev_dbg(vpfe_dev, "VIDIOC_G_STD\n");
			if (ISNULL(dec->std_ops)
			    || ISNULL(dec->std_ops->getstd)) {
				dev_err(vpfe_dev, "vpfe_doioctl:No getstd\n");
				ret = -EINVAL;
			} else {
				/* Call getstd function of decoder device */
				ret = dec->std_ops->getstd(std, dec);
			}
			break;
		}

		/* If the case is for setting standard */
	case VIDIOC_S_STD:
		{
			v4l2_std_id std = *(v4l2_std_id *) arg;
			struct common_obj *common =
			    &(channel->common[VPFE_VIDEO_INDEX]);
			dev_dbg(vpfe_dev, "VIDIOC_S_STD\n");

			/* If streaming is started, return device
			 * busy error
			 */
			if (common->started) {
				dev_err(vpfe_dev, "streaming is started\n");
				ret = -EBUSY;
				break;
			}
			if (ISNULL(dec->std_ops)
			    || ISNULL(dec->std_ops->setstd)) {
				dev_err(vpfe_dev, "vpfe_doioctl:No setstd\n");
				ret = -EINVAL;
				break;
			}
			/* Call decoder driver function to set the
			 * standard
			 */
			ret = mutex_lock_interruptible(&common->lock);
			if (ret)
				break;
			ret = dec->std_ops->setstd(&std, dec);

			/* If it returns error, return error */
			if (ret < 0) {
				mutex_unlock(&common->lock);
				break;
			}
			/* Get the information about the standard from
			 * the decoder
			 */
			vid_ch->std = std;
			ret = vpfe_get_std_info(channel);

			mutex_unlock(&common->lock);
			break;
		}
		/* If the case is for enumerating formats */
	case VIDIOC_ENUM_FMT:
		{
			struct v4l2_fmtdesc *fmt = (struct v4l2_fmtdesc *)arg;
			struct common_obj *common = NULL;
			common = &(channel->common[VPFE_VIDEO_INDEX]);
			dev_dbg(vpfe_dev, "VIDIOC_ENUM_FMT\n");
			/* Fill in the information about format */

			index = fmt->index;
			memset(fmt, 0, sizeof(*fmt));
			fmt->index = index;
			if (vpfe_enum_pix_format(fmt, dec->if_type) < 0)
				ret = -EINVAL;
			dev_dbg(vpfe_dev, "\nEnd of VIDIOC_ENUM_FMT ioctl");
			break;
		}
		/* If the case is for getting formats */
	case VIDIOC_G_FMT:
		{
			struct v4l2_format *fmt = (struct v4l2_format *)arg;
			struct common_obj *common = NULL;
			common = &(channel->common[VPFE_VIDEO_INDEX]);

			dev_dbg(vpfe_dev, "VIDIOC_G_FMT\n");

			if (common->fmt.type != fmt->type) {
				ret = -EINVAL;
				break;
			}
			/* Fill in the information about
			 * format
			 */
			ret = mutex_lock_interruptible(&(common->lock));
			if (!ret) {
				*fmt = common->fmt;
				mutex_unlock(&(common->lock));
			}
			break;
		}

		/* If the case is for setting formats */
	case VIDIOC_S_FMT:
		{
			struct common_obj *common = NULL;
			struct v4l2_format *fmt = (struct v4l2_format *)arg;
			struct v4l2_rect win;
			struct imp_window imp_win;
			enum imp_pix_formats imp_pix;
			int bytesperline;

			dev_dbg(vpfe_dev, "VIDIOC_S_FMT\n");
			common = &(channel->common[VPFE_VIDEO_INDEX]);

			/* If streaming is started, return error */
			if (common->started) {
				dev_err(vpfe_dev, "Streaming is started\n");
				ret = -EBUSY;
				break;
			}
			if (V4L2_BUF_TYPE_VIDEO_CAPTURE == fmt->type) {
				struct v4l2_pix_format *pixfmt = &fmt->fmt.pix;
				/* Check for valid field format */
				ret = vpfe_try_format(channel, pixfmt, 1);

				if (ret)
					break;
			} else {
				dev_err(vpfe_dev,
					"buf type is not supported\n");
				ret = -EINVAL;
				break;
			}

			/* store the pixel format in the channel
			 * object
			 */
			ret = mutex_lock_interruptible(&common->lock);
			if (ret)
				break;

			/* First detach any IRQ if currently attached */
			if (vpfe_detach_irq(channel) < 0) {
				dev_err(vpfe_dev, "Error in detaching IRQ\n");
				mutex_unlock(&common->lock);
				ret = -EFAULT;
				break;
			}

			common->fmt = *fmt;

			/* we are using same variable for setting crop window
			 * at ccdc. For ccdc standalone, this is same as
			 * image window
			 */

			if (!common->imp_chained) {
				/* In this case, image window and crop
				 * window are the same
				 */
				ccdc_hw_if->get_image_window(&win);
				win.width = common->fmt.fmt.pix.width;
				win.height = common->fmt.fmt.pix.height;
				ccdc_hw_if->set_image_window(&win);
				ret = ccdc_hw_if->set_pixelformat(common->fmt.
							    fmt.pix.
							    pixelformat);
				if (ret) {
					dev_err(vpfe_dev, "\n Error setting"
						"pix format at CCDC\n");
					mutex_unlock(&common->lock);
					ret = -EINVAL;
					break;
				}
				ccdc_hw_if->get_line_length(&bytesperline);
				if (bytesperline != common->fmt.fmt.
							pix.bytesperline) {
					dev_err(vpfe_dev, "\n Mismatch"
						"between bytesperline at CCDC"
						"and vpfe\n");
					mutex_unlock(&common->lock);
					ret = -EINVAL;
					break;
				}
				if (common->fmt.fmt.pix.field ==
				    V4L2_FIELD_INTERLACED) {
					ccdc_hw_if->
					    set_buftype
					    (CCDC_BUFTYPE_FLD_INTERLEAVED);
					ccdc_hw_if->
					    set_frame_format
					    (CCDC_FRMFMT_INTERLACED);
				} else if (common->fmt.fmt.pix.field ==
					   V4L2_FIELD_SEQ_TB) {
					ccdc_hw_if->
					    set_buftype
					    (CCDC_BUFTYPE_FLD_SEPARATED);
					ccdc_hw_if->
					    set_frame_format
					    (CCDC_FRMFMT_INTERLACED);
				} else if (common->fmt.fmt.pix.field ==
					   V4L2_FIELD_NONE) {
					ccdc_hw_if->
					    set_frame_format
					    (CCDC_FRMFMT_PROGRESSIVE);
				} else {
					dev_dbg(vpfe_dev, "\n field error!\n");
					mutex_unlock(&common->lock);
					ret = -EINVAL;
					break;
				}
			} else {
				/* Use previewer/resizer to convert the format
				 * and resize check if we have image processor
				 * chained
				 */
				if (dec->if_type & INTERFACE_TYPE_RAW) {
					if (imp_hw_if->
					    set_in_pixel_format
					    (IMP_BAYER) < 0) {
						dev_err(vpfe_dev,
							"Couldn't set pix "
							"format to Bayer at"
							"the IMP\n");
						mutex_unlock(&common->lock);
						ret = -EINVAL;
						break;
					}
				} else {
					if (imp_hw_if->
					    set_in_pixel_format(IMP_UYVY) < 0) {
						dev_err(vpfe_dev,
							"Couldn't set pix"
							"format"
							"at the IMP to UYVY\n");
						mutex_unlock(&common->lock);
						ret = -EINVAL;
						break;
					}
				}

				if (common->fmt.fmt.pix.pixelformat ==
					V4L2_PIX_FMT_SBGGR16)
					imp_pix = IMP_BAYER;
				else if (common->fmt.fmt.pix.pixelformat ==
					V4L2_PIX_FMT_UYVY)
					imp_pix = IMP_UYVY;
				else if (common->fmt.fmt.pix.pixelformat ==
					V4L2_PIX_FMT_NV12)
					imp_pix = IMP_YUV420SP;
				else {
					dev_err(vpfe_dev, "pixel format not"
						"supported at IMP1\n");
					mutex_unlock(&common->lock);
					ret = -EINVAL;
					break;
				}
				if (imp_hw_if->set_out_pixel_format(imp_pix)
							< 0) {
					dev_err(vpfe_dev, "pixel format not"
						"supported at IMP2\n");
					mutex_unlock(&common->lock);
					ret = -EINVAL;
					break;
				}

				if (common->fmt.fmt.pix.field ==
				    V4L2_FIELD_INTERLACED) {
					imp_hw_if->set_buftype(0);
					imp_hw_if->set_frame_format(0);
					ccdc_hw_if->set_frame_format
					    (CCDC_FRMFMT_INTERLACED);
				} else if (common->fmt.fmt.pix.field ==
					   V4L2_FIELD_NONE) {
					imp_hw_if->set_frame_format(1);
					ccdc_hw_if->set_frame_format
					    (CCDC_FRMFMT_PROGRESSIVE);
				} else {
					dev_dbg(vpfe_dev, "\n field error!");
					mutex_unlock(&common->lock);
					ret = -EINVAL;
					break;
				}

				/* Check if we have resizer. Otherwise
				 * don't allow crop size to be different
				 * from image size
				 */
				imp_win.width = common->crop.width;
				imp_win.height = common->crop.height;
				imp_win.hst = common->crop.left;
				/* vst start from 1 */
				imp_win.vst = common->crop.top + 1;
				if (imp_hw_if->set_input_win(&imp_win) < 0) {
					dev_err(vpfe_dev,
						"Error in setting crop window"
						" in IMP\n");
					mutex_unlock(&common->lock);
					ret = -EINVAL;
					break;
				}

				/* Set output */
				imp_win.width = common->fmt.fmt.pix.width;
				imp_win.height = common->fmt.fmt.pix.height;
				imp_win.hst = 0;
				imp_win.vst = 0;
				if (imp_hw_if->set_output_win(&imp_win) < 0) {
					dev_err(vpfe_dev,
						"Error in setting image "
						"windown in IMP\n");
					mutex_unlock(&common->lock);
					ret = -EINVAL;
					break;
				}

				bytesperline = imp_hw_if->get_line_length(0);
				if (bytesperline !=
					common->fmt.fmt.pix.bytesperline) {
					mutex_unlock(&common->lock);
					ret = -EINVAL;
					dev_err(vpfe_dev, "\n Mismatch"
						"between bytesperline at IMP"
						"and vpfe\n");
					break;
				}

				if (imp_hw_if->get_output_state(1)) {
					common->second_out_img_sz =
					    imp_hw_if->get_line_length(1)
					    * imp_hw_if->get_image_height(1);
				}
			}
			mutex_unlock(&common->lock);
			break;
		}
		/* If the case is for trying formats */
	case VIDIOC_TRY_FMT:
		{
			struct common_obj *common = NULL;
			struct v4l2_format *fmt;
			dev_dbg(vpfe_dev, "VIDIOC_TRY_FMT\n");
			fmt = (struct v4l2_format *)arg;

			common = &(channel->common[VPFE_VIDEO_INDEX]);

			if (V4L2_BUF_TYPE_VIDEO_CAPTURE == fmt->type) {
				struct v4l2_pix_format *pixfmt = &fmt->fmt.pix;
				vpfe_try_format(channel, pixfmt, 0);
			} else {
				dev_err(vpfe_dev,
					"Buffer type is not supported\n");
				ret = -EINVAL;
			}
			break;
		}

		/* If the case is for querying controls */
	case VIDIOC_QUERYCTRL:
		{
			struct v4l2_queryctrl *qctrl =
			    (struct v4l2_queryctrl *)arg;

			dev_dbg(vpfe_dev,
				"VIDIOC_QUERYCTRL, 0x%x\n", qctrl->id);
			if (qctrl->id >= VPFE_CCDC_CID_START &&
			    qctrl->id <= VPFE_CCDC_CID_END) {
				/* CCDC CIDs. Call CCDC api */
				if (ISNULL(ccdc_hw_if->queryctrl)) {
					dev_err(vpfe_dev,
						"vpfe_doioctl:No queryctrl"
						" for ccdc\n");
					ret = -EINVAL;
				} else
					ret = ccdc_hw_if->queryctrl(qctrl);
			} else {
				/* decoder CIDs. Call decoder API */
				if (ISNULL(dec->ctrl_ops) ||
				    ISNULL(dec->ctrl_ops->queryctrl)) {
					dev_err(vpfe_dev,
						"vpfe_doioctl:No queryctrl"
						" for decoder\n");
					ret = -EINVAL;
				} else
					/* Call queryctrl function of
					 * decoder device
					 */
					ret =
					   dec->ctrl_ops->queryctrl(qctrl,
								    dec);
			}
			break;
		}

		/* If the case is for getting controls value */
	case VIDIOC_G_CTRL:
		{
			struct common_obj *common =
			    &(channel->common[VPFE_VIDEO_INDEX]);
			struct v4l2_control *ctrl = (struct v4l2_control *)arg;
			dev_dbg(vpfe_dev, "VIDIOC_G_CTRL\n");
			if (ctrl->id >= VPFE_CCDC_CID_START &&
			    ctrl->id <= VPFE_CCDC_CID_END) {
				/* CCDC CIDs. Call CCDC api */
				if (ISNULL(ccdc_hw_if->getcontrol)) {
					dev_err(vpfe_dev,
						"vpfe_doioctl:No getcontrol"
						" for ccdc\n");
					ret = -EINVAL;
				} else {
					ret = mutex_lock_interruptible(
							&common->lock);
					if (ret)
						break;
					ret = ccdc_hw_if->getcontrol(ctrl);
					mutex_unlock(&common->lock);
				}
			} else {
				if (ISNULL(dec->ctrl_ops) ||
				    ISNULL(dec->ctrl_ops->getcontrol)) {
					dev_err(vpfe_dev,
						"vpfe_doioctl:No getcontrol"
						" for decoder\n");
					ret = -EINVAL;
				} else {
					/* Call getcontrol function of
					 * decoder device
					 */
					ret = mutex_lock_interruptible(
								&common->lock);
					if (ret)
						break;
					ret = dec->ctrl_ops->getcontrol(ctrl,
									dec);
					mutex_unlock(&common->lock);
				}
			}
			break;
		}

		/* If the case is for getting controls value */
	case VIDIOC_S_CTRL:
		{
			struct common_obj *common =
			    &(channel->common[VPFE_VIDEO_INDEX]);
			struct v4l2_control *ctrl = (struct v4l2_control *)arg;
			dev_dbg(vpfe_dev, "VIDIOC_S_CTRL\n");
			if (ctrl->id >= VPFE_CCDC_CID_START &&
			    ctrl->id <= VPFE_CCDC_CID_END) {
				/* CCDC CIDs. Call CCDC api */
				if (ISNULL(ccdc_hw_if->setcontrol)) {
					dev_err(vpfe_dev,
						"vpfe_doioctl:No setcontrol"
						" for ccdc\n");
					ret = -EINVAL;
				} else {
					ret = mutex_lock_interruptible(
								&common->lock);
					if (ret)
						break;
					ret = ccdc_hw_if->setcontrol(ctrl);
					mutex_unlock(&common->lock);
				}
			} else {
				if (ISNULL(dec->ctrl_ops) ||
				    ISNULL(dec->ctrl_ops->setcontrol)) {
					dev_err(vpfe_dev,
						"vpfe_doioctl:No setcontrol"
						" for decoder\n");
					ret = -EINVAL;
				} else {
					/* Call setcontrol function of
					 * decoder device
					 */
					ret = mutex_lock_interruptible
								(&common->lock);
					if (ret)
						break;
					ret = dec->ctrl_ops->setcontrol(ctrl,
									dec);
					mutex_unlock(&common->lock);
				}
			}
			break;
		}

		/* If the case is for getting decoder parameters */
	case VPFE_CMD_G_DECODER_PARAMS:
		{
			struct common_obj *common =
			    &(channel->common[VPFE_VIDEO_INDEX]);
			dev_dbg(vpfe_dev, "VPFE_CMD_G_DECODER_PARAMS\n");
			if (ISNULL(dec->params_ops)
			    || ISNULL(dec->params_ops->getparams)) {
				dev_err(vpfe_dev, "vpfe_doioctl:No"
					"getdeviceparams\n");
				ret = -EINVAL;
			} else {
				/* Call getparams function of decoder device */
				ret = mutex_lock_interruptible(&common->lock);
				if (!ret) {
					ret = dec->params_ops->
						getparams((void *)arg, dec);
					mutex_unlock(&common->lock);
				}
			}
			break;
		}

		/* If the case is for setting decoder parameters */
	case VPFE_CMD_S_DECODER_PARAMS:
		{
			struct common_obj *common =
			    &(channel->common[VPFE_VIDEO_INDEX]);
			dev_dbg(vpfe_dev, "VPFE_CMD_S_DECODER_PARAMS\n");
			/* If channel is already started, return
			 * error
			 */
			if (common->started) {
				dev_err(vpfe_dev, "streaming is started\n");
				ret = -EINVAL;
				break;
			}
			if (ISNULL(dec->params_ops)
			    || ISNULL(dec->params_ops->setparams)) {
				dev_err(vpfe_dev, "vpfe_doioctl:No"
					"setdeviceparams\n");
				ret = -EINVAL;
				break;
			}
			/* Call getparams function of decoder device */
			ret = mutex_lock_interruptible(&common->lock);
			if (!ret) {
				ret = dec->params_ops->setparams(
							(void *)arg, dec);
				mutex_unlock(&common->lock);
			}
			break;
		}

		/* If the case is for requesting buffer allocation */
	case VIDIOC_REQBUFS:
		{
			struct common_obj *common = NULL;
			struct v4l2_requestbuffers *reqbuf;
			enum v4l2_field field;
			int buf_type_index;
			reqbuf = (struct v4l2_requestbuffers *)arg;
			dev_dbg(vpfe_dev, "VIDIOC_REQBUFS\n");
			if (V4L2_BUF_TYPE_VIDEO_CAPTURE != reqbuf->type)
				return -EINVAL;
			buf_type_index = VPFE_VIDEO_INDEX;
			common = &(channel->common[buf_type_index]);

			/* If io users of the channel is not zero,
			 * return error
			 */
			if (0 != common->io_usrs) {
				ret = -EBUSY;
				break;
			}

			ret = mutex_lock_interruptible(&common->lock);
			if (ret)
				break;
			if (common->fmt.fmt.pix.field != V4L2_FIELD_ANY)
				field = common->fmt.fmt.pix.field;
			else if (dec->if_type & INTERFACE_TYPE_RAW)
				field = V4L2_FIELD_NONE;
			else
				field = V4L2_FIELD_INTERLACED;

			/* Initialize videobuf queue as per the
			 * buffer type
			 */
			videobuf_queue_init(&common->buffer_queue,
					&video_qops,
					NULL,
					&common->irqlock,
					reqbuf->type,
					field,
					sizeof(struct videobuf_buffer),
					fh);
			/* Set buffer to Linear buffer */
			videobuf_set_buftype(&common->buffer_queue,
					     VIDEOBUF_BUF_LINEAR);
			/* Set io allowed member of file handle to
			 * TRUE
			 */
			fh->io_allowed[buf_type_index] = 1;

			/* Lock the chained imp resource */
			if (common->imp_chained)
				imp_hw_if->lock_chain();
			/* Increment io usrs member of channel object
			 * to 1
			 */
			common->io_usrs = 1;
			/* Store type of memory requested in channel
			 * object
			 */
			common->memory = reqbuf->memory;
			/* Initialize buffer queue */
			INIT_LIST_HEAD(&common->dma_queue);
			/* Allocate buffers */
			ret = videobuf_reqbufs(&common->buffer_queue, reqbuf);
			mutex_unlock(&common->lock);
			break;
		}

		/* If the case is for en-queing buffer in the buffer
		 * queue
		 */
	case VIDIOC_QBUF:
		{
			struct common_obj *common = NULL;
			struct v4l2_buffer tbuf = *(struct v4l2_buffer *)arg;
			struct videobuf_buffer *buf1;
			int buf_type_index;
			int expected_buf_size = 0;
			dev_dbg(vpfe_dev, "VIDIOC_QBUF\n");
			buf_type_index = VPFE_VIDEO_INDEX;
			if (V4L2_BUF_TYPE_VIDEO_CAPTURE !=
			    ((struct v4l2_buffer *)arg)->type) {
				dev_err(vpfe_dev,
					"VIDIOC_QBUF:Invalid buf type\n");
				ret = -EINVAL;
				break;
			}
			common = &(channel->common[buf_type_index]);

			/* If this file handle is not allowed to do IO,
			 * return error
			 */
			if (!fh->io_allowed[buf_type_index]) {
				dev_err(vpfe_dev, "fh->io_allowed\n");
				ret = -EACCES;
				break;
			}

			if (!(list_empty(&common->dma_queue)) ||
			    (common->curFrm != common->nextFrm) ||
			    !(common->started) ||
			    (common->started && (0 == channel->field_id))) {
				ret =
				    videobuf_qbuf(&common->buffer_queue,
						  (struct v4l2_buffer *)
						  arg);
				break;
			}
			/* bufferqueue is empty store buffer address in VPFE
			 * registers
			 */
			mutex_lock(&common->buffer_queue.lock);
			tbuf = *(struct v4l2_buffer *)arg;
			buf1 = common->buffer_queue.bufs[tbuf.index];
			if (buf1->memory != tbuf.memory) {
				dev_err(vpfe_dev, "invalid buffer" " type\n");
				mutex_unlock(&common->buffer_queue.lock);
				ret = -EINVAL;
				break;
			}
			if ((buf1->state == STATE_QUEUED) ||
			    (buf1->state == STATE_ACTIVE)) {
				dev_err(vpfe_dev, "invalid state\n");
				mutex_unlock(&common->buffer_queue.lock);
				ret = -EINVAL;
				break;
			}

			switch (buf1->memory) {
			case V4L2_MEMORY_MMAP:
				if (buf1->baddr == 0) {
					mutex_unlock(&common->
						     buffer_queue.lock);
					return -EINVAL;
				}
				break;
			case V4L2_MEMORY_USERPTR:
				expected_buf_size =
				    common->fmt.fmt.pix.sizeimage;
				if (common->second_out_img_sz)
					expected_buf_size +=
					    common->second_out_img_sz;
				if (tbuf.length <
				    (expected_buf_size +
				     common->second_out_img_sz)) {
					mutex_unlock(&common->buffer_queue.
						     lock);
					dev_err(vpfe_dev,
						"capture buffer size is less "
						"than required size\n");
					return -EINVAL;
				}

				if ((STATE_NEEDS_INIT != buf1->state)
				    && (buf1->baddr != tbuf.m.userptr))
					vpfe_buffer_release(&common->
							    buffer_queue, buf1);
				buf1->baddr = tbuf.m.userptr;
				break;
			default:
				mutex_unlock(&common->buffer_queue.lock);
				return -EINVAL;
			}
			local_irq_save(flags);
			ret =
			    vpfe_buffer_prepare(&common->buffer_queue,
						buf1,
						common->buffer_queue.field);
			if (ret < 0) {
				local_irq_restore(flags);
				mutex_unlock(&common->buffer_queue.lock);
				ret = -EINVAL;
				break;
			}
			buf1->state = STATE_ACTIVE;
			addr = buf1->boff;
			common->nextFrm = buf1;
			if (common->imp_chained) {
				if (imp_hw_if->
				    update_outbuf1_address(NULL, addr) < 0) {
					dev_err(vpfe_dev,
						"Error setting up address "
						"in IMP output1\n");
					local_irq_restore(flags);
					mutex_unlock(&common->
						     buffer_queue.lock);
					ret = -EINVAL;
					break;
				}
				if (common->second_output) {
					if (imp_hw_if->
					    update_outbuf2_address(NULL,
							   (addr +
							   common->
								second_off))
								< 0) {
						dev_err(vpfe_dev,
							"Error setting up"
							" address in IMP"
							" output2\n");
						local_irq_restore(flags);
						mutex_unlock(&common->
							     buffer_queue.lock);
						ret = -EINVAL;
						break;
					}
				}
			} else
				ccdc_hw_if->setfbaddr(addr);
			local_irq_restore(flags);

			list_add_tail(&buf1->stream,
				      &(common->buffer_queue.stream));
			mutex_unlock(&common->buffer_queue.lock);
			break;
		}

		/* If the case is for de-queing buffer from the
		 * buffer queue
		 */
	case VIDIOC_DQBUF:
		{
			struct common_obj *common = NULL;
			int buf_type_index = 0;
			buf_type_index = VPFE_VIDEO_INDEX;
			common = &(channel->common[buf_type_index]);

			if (V4L2_BUF_TYPE_VIDEO_CAPTURE !=
			    ((struct v4l2_buffer *)arg)->type) {
				dev_err(vpfe_dev,
					"VIDIOC_DQBUF:Invalid buf type\n");
				ret = -EINVAL;
				break;
			}
			if (file->f_flags & O_NONBLOCK)
				/* Call videobuf_dqbuf for non
				 * blocking mode
				 */
				ret =
				    videobuf_dqbuf(&common->
						   buffer_queue,
						   (struct v4l2_buffer *)
						   arg, 1);
			else
				/* Call videobuf_dqbuf for
				 * blocking mode
				 */
				ret =
				    videobuf_dqbuf(&common->
						   buffer_queue,
						   (struct v4l2_buffer *)
						   arg, 0);
			break;
		}

		/* If the case is for querying information about
		 * buffer for memory mapping io
		 */
	case VIDIOC_QUERYBUF:
		{
			struct common_obj *common = NULL;
			u8 buf_type_index = 0;
			dev_dbg(vpfe_dev, "VIDIOC_QUERYBUF\n");
			buf_type_index = VPFE_VIDEO_INDEX;
			if (V4L2_BUF_TYPE_VIDEO_CAPTURE !=
			    ((struct v4l2_buffer *)arg)->type) {
				dev_err(vpfe_dev,
					"VIDIOC_QUERYBUF:Invalid buf type\n");
				ret = -EINVAL;
				break;
			}
			common = &(channel->common[buf_type_index]);
			if (((struct v4l2_buffer *)arg)->memory !=
			    V4L2_MEMORY_MMAP)
				ret = -EINVAL;
			else
				/* Call videobuf_querybuf to get information */
				ret = videobuf_querybuf(&common->buffer_queue,
						(struct v4l2_buffer *)
						arg);
			break;
		}

		/* If the case is starting streaming */
	case VIDIOC_STREAMON:
		{
			struct common_obj *common = NULL;
			struct video_obj *vid_ch = NULL;
			enum v4l2_buf_type buftype
			    = *(enum v4l2_buf_type *)(arg);
			int buf_type_index = VPFE_VIDEO_INDEX;
			dev_dbg(vpfe_dev, "VIDIOC_STREAMON\n");
			if (V4L2_BUF_TYPE_VIDEO_CAPTURE != buftype) {
				dev_err(vpfe_dev,
					"VIDIOC_STREAMON:Invalid buf type\n");
				ret = -EINVAL;
				break;
			}
			common = &(channel->common[buf_type_index]);
			vid_ch = &(channel->video);
			/* If file handle is not allowed IO,
			 * return error
			 */
			if (!fh->io_allowed[buf_type_index]) {
				dev_err(vpfe_dev, "fh->io_allowed\n");
				ret = -EACCES;
				break;
			}
			/* If Streaming is already started,
			 * return error
			 */
			if (common->started) {
				dev_err(vpfe_dev, "channel->started\n");
				ret = -EBUSY;
				break;
			}

			/* Call videobuf_streamon to start streaming
			 * in videobuf
			 */
			ret = videobuf_streamon(&common->buffer_queue);
			if (ret) {
				dev_err(vpfe_dev, "videobuf_streamon\n");
				break;
			}
			ret = mutex_lock_interruptible(&common->lock);
			if (ret)
				break;
			/* If buffer queue is empty, return error */
			if (list_empty(&common->dma_queue)) {
				dev_err(vpfe_dev, "buffer queue is empty\n");
				ret = -EIO;
				mutex_unlock(&common->lock);
				break;
			}
			/* Get the next frame from the buffer queue */
			common->curFrm =
			    list_entry(common->dma_queue.next,
				       struct videobuf_buffer, queue);
			common->nextFrm = common->curFrm;
			/* Remove buffer from the buffer queue */
			list_del(&common->curFrm->queue);
			/* Mark state of the current frame to active */
			common->curFrm->state = STATE_ACTIVE;
			/* Initialize field_id and started member */
			channel->field_id = 0;
			channel->first_frame = 1;
			channel->fld_one_recvd = 0;
			common->started = 1;

			addr = common->curFrm->boff;

			/* Calculate field offset */
			vpfe_calculate_offsets(channel);

			if ((vid_ch->std_info.frame_format &&
			     ((common->fmt.fmt.pix.field !=
			       V4L2_FIELD_NONE) &&
			      (common->fmt.fmt.pix.field !=
			       V4L2_FIELD_ANY))) ||
			    (!vid_ch->std_info.frame_format &&
			     (common->fmt.fmt.pix.field == V4L2_FIELD_NONE))) {
				dev_err(vpfe_dev,
					"conflict in field format"
					" and std format\n");
				mutex_unlock(&common->lock);
				ret = -EINVAL;
				break;
			}

			if (vpfe_attach_irq(channel, dec->if_type) < 0) {
				dev_err(vpfe_dev,
					"Error in attaching "
					"interrupt handle\n");
				mutex_unlock(&common->lock);
				ret = -EFAULT;
				break;
			}

			if (dec->if_type & INTERFACE_TYPE_RAW)
				ccdc_hw_if->configure(common->imp_chained);
			else
				ccdc_hw_if->configure(common->imp_chained);

			if (!common->imp_chained) {
				ccdc_hw_if->setfbaddr((unsigned long)(addr));
				ccdc_hw_if->enable(1);
				if (ccdc_hw_if->enable_out_to_sdram)
					ccdc_hw_if->enable_out_to_sdram(1);
			} else {
				if (!cpu_is_davinci_dm365() &&
				   (dec->if_type != INTERFACE_TYPE_RAW)) {
					/* Continuous mode or on the fly capture
					 * and image processing is not available
					 * for yuv capture
					 */
					dev_err(vpfe_dev,
						"Continuous mode capture & "
						"image processing not"
						" supported");
					mutex_unlock(&common->lock);
					ret = -EINVAL;
					break;
				}
				if (imp_hw_if->hw_setup(vpfe_dev, NULL) < 0) {
					dev_err(vpfe_dev,
						"Error setting up IMP\n");
					mutex_unlock(&common->lock);
					ret = -EINVAL;
					break;
				}
				if (imp_hw_if->
				    update_outbuf1_address(NULL, addr) < 0) {
					dev_err(vpfe_dev,
						"Error setting up address "
						"in IMP output1\n");
					mutex_unlock(&common->lock);
					ret = -EINVAL;
					break;
				}

				if (common->second_output) {
					if (imp_hw_if->
					    update_outbuf2_address(NULL,
							   (addr +
							    common->
							    second_off))
								< 0) {
						dev_err(vpfe_dev,
							"Error setting up"
							"address "
							"in IMP output2\n");
						mutex_unlock(&common->lock);
						ret = -EINVAL;
						break;
					}
				}
				if (ccdc_hw_if->enable_out_to_sdram)
					ccdc_hw_if->enable_out_to_sdram(0);
				imp_hw_if->enable(1, NULL);
				ccdc_hw_if->enable(1);
			}
			mutex_unlock(&common->lock);
			break;
		}

		/* If the case is for stopping streaming */
	case VIDIOC_STREAMOFF:
		{
			struct common_obj *common = NULL;
			int buf_type_index = VPFE_VIDEO_INDEX;
			enum v4l2_buf_type buftype
			    = *(enum v4l2_buf_type *)(arg);
			if (V4L2_BUF_TYPE_VIDEO_CAPTURE != buftype) {
				dev_err(vpfe_dev,
					"VIDIOC_STREAMOFF:Invalid buf type\n");
				ret = -EINVAL;
				break;
			}
			common = &(channel->common[buf_type_index]);
			dev_dbg(vpfe_dev, "VIDIOC_STREAMOFF\n");
			/* If io is allowed for this file handle,
			 * return error
			 */
			if (!fh->io_allowed[buf_type_index]) {
				dev_err(vpfe_dev, "fh->io_allowed\n");
				ret = -EACCES;
				break;
			}
			/* If streaming is not started, return error */
			if (!common->started) {
				dev_err(vpfe_dev, "channel->started\n");
				ret = -EINVAL;
				break;
			}
			ret = mutex_lock_interruptible(&common->lock);
			if (ret)
				break;
			common->started = 0;
			ccdc_hw_if->enable(0);
			if (ccdc_hw_if->enable_out_to_sdram)
				ccdc_hw_if->enable_out_to_sdram(0);
			if (common->imp_chained)
				imp_hw_if->enable(0, NULL);
			if (vpfe_detach_irq(channel) < 0) {
				dev_err(vpfe_dev,
					"Error in detaching"
					" interrupt handler\n");
				mutex_unlock(&common->lock);
				ret = -EFAULT;
			} else {
				mutex_unlock(&common->lock);
				ret = videobuf_streamoff(&common->buffer_queue);
			}
			break;
		}

	case VPFE_CMD_CONFIG_CCDC_YCBCR:
		/* this can be used directly and bypass the V4L2 APIs */
		{
			struct common_obj *common = NULL;
			struct v4l2_rect image_win;
			enum ccdc_buftype buf_type;
			common = &(channel->common[VPFE_VIDEO_INDEX]);

			if (common->started) {
				/* only allowed if streaming is not started */
				dev_err(vpfe_dev, "channel already started\n");
				ret = -EBUSY;
				break;
			}
			ret = mutex_lock_interruptible(&common->lock);
			if (ret)
				break;
			if (ccdc_hw_if->
			    setparams((void *)arg) < 0) {
				/* only allowed if streaming is not started */
				dev_err(vpfe_dev,
					"Error in setting YCbCr"
					" parameters in CCDC \n");
				ret = -EINVAL;
				mutex_unlock(&common->lock);
				break;
			}
			ccdc_hw_if->get_image_window(&image_win);
			common->fmt.fmt.pix.width = image_win.width;
			common->fmt.fmt.pix.height = image_win.height;
			ccdc_hw_if->get_line_length(&common->fmt.
						    fmt.pix.bytesperline);
			common->fmt.fmt.pix.sizeimage =
			    common->fmt.fmt.pix.bytesperline *
			    common->fmt.fmt.pix.height;
			ccdc_hw_if->get_buftype(&buf_type);
			if (buf_type == CCDC_BUFTYPE_FLD_INTERLEAVED)
				common->fmt.fmt.pix.field =
				    V4L2_FIELD_INTERLACED;
			else if (buf_type == CCDC_BUFTYPE_FLD_SEPARATED)
				common->fmt.fmt.pix.field = V4L2_FIELD_SEQ_TB;
			ccdc_hw_if->get_pixelformat(&common->fmt.fmt.
						    pix.pixelformat);
			mutex_unlock(&common->lock);
			break;
		}
	case VPFE_CMD_S_CCDC_PARAMS:
		{
			/* This replaces VPFE_CMD_CONFIG_CCDC_RAW &
			 * VPFE_CMD_CONFIG_CCDC_YCBCR ioctls that are obsoleted
			 */
			if (!cpu_is_davinci_dm365()) {
				dev_err(vpfe_dev,
					"VPFE_CMD_S_CCDC_PARAMS"
					" not supported\n");
				ret = -EINVAL;
				break;
			}
			/* fall through */
		}
	case VPFE_CMD_CONFIG_CCDC_RAW:
		/* This command is used to configure driver for CCDC Raw
		 * mode parameters
		 */
		{
			struct common_obj *common = NULL;
			struct v4l2_rect image_win;
			enum ccdc_buftype buf_type;
			common = &(channel->common[VPFE_VIDEO_INDEX]);

			if (common->started) {
				/* only allowed if streaming is not started */
				dev_err(vpfe_dev, "channel already started\n");
				ret = -EBUSY;
				break;
			}

			ret = mutex_lock_interruptible(&common->lock);
			if (ret)
				break;
			if (ccdc_hw_if->
			    setparams((void *)arg) < 0) {
				/* only allowed if streaming is not started */
				dev_err(vpfe_dev,
					"Error in setting RAW mode "
					"parameters in CCDC \n");
				mutex_unlock(&common->lock);
				ret = -EINVAL;
				break;
			}
			ccdc_hw_if->get_image_window(&image_win);
			common->fmt.fmt.pix.width = image_win.width;
			common->fmt.fmt.pix.height = image_win.height;
			ccdc_hw_if->get_line_length(&common->fmt.
						    fmt.pix.bytesperline);
			common->fmt.fmt.pix.sizeimage =
			    common->fmt.fmt.pix.bytesperline *
			    common->fmt.fmt.pix.height;
			ccdc_hw_if->get_buftype(&buf_type);
			common->fmt.fmt.pix.field = V4L2_FIELD_NONE;
			ccdc_hw_if->get_pixelformat(&common->fmt.
						    fmt.pix.pixelformat);
			mutex_unlock(&common->lock);
			break;
		}

		/* If the case is for getting VPFE Parameters */
	case VPFE_CMD_G_CCDC_PARAMS:
		{
			struct common_obj *common = NULL;
			common = &(channel->common[VPFE_VIDEO_INDEX]);

			if (!cpu_is_davinci_dm365()) {
				dev_err(vpfe_dev,
					"VPFE_CMD_G_CCDC_PARAMS"
					" not supported\n");
				ret = -EINVAL;
				break;
			}
			ret = mutex_lock_interruptible(&common->lock);
			if (ret)
				break;
			ret = ccdc_hw_if->getparams((void *)arg);
			if (ret < 0) {
				dev_err(vpfe_dev,
					"Error in getting RAW mode parameters"
					" in CCDC \n");
			}
			mutex_unlock(&common->lock);
			break;
		}

		/* If the case is for getting TVP5146 parameters */
	case VPFE_CMD_CONFIG_TVP5146:
		{
			dev_err(vpfe_dev, "Device is not supported\n");
			ret = -ENODEV;
			break;
		}

	case VIDIOC_S_CROP:
		{
			struct v4l2_crop *crop = (struct v4l2_crop *)arg;
			struct video_obj *vid_ch = NULL;
			struct common_obj *common = NULL;
			struct imp_window imp_crop_win;

			common = &(channel->common[VPFE_VIDEO_INDEX]);
			vid_ch = &(channel->video);
			dev_dbg(vpfe_dev, "\nStarting VIDIOC_S_CROP ioctl");
			if (common->started) {
				/* make sure streaming is not started */
				ret = -EBUSY;
				break;
			}

			if (crop->c.top < 0 || crop->c.left < 0) {
				dev_err(vpfe_dev, "Doesn't support\n"
					" Negative values for top and left\n");
				ret = -EINVAL;
				break;
			}

			/* adjust the width to 16 pixel boundry */
			crop->c.width = ((crop->c.width + 15) / 16) * 16;

			/* make sure parameters are valid */
			if (crop->type == V4L2_BUF_TYPE_VIDEO_CAPTURE
			    && (crop->c.left + crop->c.width
				<= vid_ch->std_info.activepixels)
			    && (crop->c.top + crop->c.height
				<= vid_ch->std_info.activelines)) {
				ret = mutex_lock_interruptible(&common->lock);
				if (ret)
					break;
				common->crop = crop->c;
				ccdc_hw_if->set_image_window(&crop->c);
				if (!common->imp_chained) {
					/* We are using CCDC only. So image
					 * window and crop window are
					 * the same
					 */
					common->fmt.fmt.pix.width =
					    crop->c.width;
					common->fmt.fmt.pix.height =
					    crop->c.height;
					ccdc_hw_if->
					    get_line_length(&common->
							    fmt.fmt.pix.
							    bytesperline);
					common->fmt.fmt.pix.sizeimage =
					    common->fmt.fmt.pix.
					    bytesperline *
					    common->fmt.fmt.pix.height;
				} else {
					/* Chained. Set the crop values
					 * in the IMP
					 */
					imp_crop_win.width = crop->c.width;
					imp_crop_win.height = crop->c.height;
					imp_crop_win.hst = crop->c.left;
					/* vst starts from 1 */
					imp_crop_win.vst = crop->c.top + 1;
					if (imp_hw_if->
					    set_input_win(&imp_crop_win)
					    < 0) {
						dev_err(vpfe_dev,
							"Error in setting crop "
							"window in IMP\n");
						ret = -EINVAL;
					}
				}
				mutex_unlock(&common->lock);
			} else {
				ret = -EINVAL;
				dev_err(vpfe_dev, "Error in S_CROP params\n");
			}
			break;
		}

	case VIDIOC_G_CROP:
		{
			struct v4l2_crop *crop = (struct v4l2_crop *)arg;
			struct common_obj *common = NULL;
			common = &(channel->common[VPFE_VIDEO_INDEX]);
			dev_dbg(vpfe_dev, "\nStarting VIDIOC_G_CROP ioctl");
			if (crop->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
				dev_err(vpfe_dev,
					"VIDIOC_G_CROP:Invalid buffer type\n");
				ret = -EINVAL;
			} else {
				/* We may have IMP chained with CCDC */
				if (common->imp_chained) {
					/* Chained */
					crop->c = common->crop;
				} else {
					/* Not chained */
					if (ccdc_hw_if->
					    get_image_window(&crop->c) < 0) {
						ret = -EINVAL;
						dev_err(vpfe_dev,
							"Error in getting crop"
							" window in CCDC\n");
					}
				}
			}
			dev_dbg(vpfe_dev, "\nEnd of VIDIOC_G_CROP ioctl");
			break;
		}
		/* If the case is for setting mt9t001 parameters */
	case VPFE_CMD_S_MT9T001_PARAMS:
		{
			struct common_obj *common =
			    &(channel->common[VPFE_VIDEO_INDEX]);
			if (0 != strcmp(dec->name, "MT9T001"))
				ret = -ENODEV;
			else {
				if (ISNULL(dec->params_ops->setparams)) {
					dev_err(vpfe_dev,
						"vpfe_doioctl:No setparams\n");
					ret = -EINVAL;
				} else {
					ret = mutex_lock_interruptible(
							&common->lock);
					if (!ret) {
						ret = dec->params_ops->
							setparams((void *)arg,
								dec);
						mutex_unlock(&common->lock);
					}
				}
			}
			break;
		}

		/* If the case is for setting mt9t001 parameters */
	case VPFE_CMD_G_MT9T001_PARAMS:
		{
			struct common_obj *common;
			common = &(channel->common[VPFE_VIDEO_INDEX]);
			if (0 != strcmp(dec->name, "MT9T001"))
				return -ENODEV;
			if (ISNULL(dec->params_ops->getparams)) {
				dev_err(vpfe_dev,
					"vpfe_doioctl:No getparams\n");
				ret = -EINVAL;
			} else {
				ret = mutex_lock_interruptible(&common->lock);
				if (!ret) {
					ret = dec->params_ops->
						getparams((void *)arg, dec);
					mutex_unlock(&common->lock);
				}
			}
			break;
		}
	case VIDIOC_S_PRIORITY:
		{
			enum v4l2_priority *p = (enum v4l2_priority *)arg;
			ret = v4l2_prio_change(&channel->prio, &fh->prio, *p);
			break;
		}
	case VIDIOC_G_PRIORITY:
		{
			enum v4l2_priority *p = (enum v4l2_priority *)arg;
			*p = v4l2_prio_max(&channel->prio);
			break;
		}
		/* If the case if for getting cropping parameters */
	case VIDIOC_CROPCAP:
		{
			struct v4l2_cropcap *crop = (struct v4l2_cropcap *)arg;
			if (V4L2_BUF_TYPE_VIDEO_CAPTURE != crop->type)
				ret = -EINVAL;
			else
				vpfe_get_crop_cap(crop, channel);
			break;
		}
	case VIDIOC_S_PARM:
		{
			struct v4l2_streamparm *sparam =
				(struct v4l2_streamparm *)arg;
			struct common_obj *common;
			struct v4l2_standard standard;
			common = &(channel->common[VPFE_VIDEO_INDEX]);
			if (cpu_is_davinci_dm644x()) {
				dev_err(vpfe_dev,
					"Ioctl not supported on"
					" this platform\n");
				ret = -EINVAL;
				break;
			}
			if (sparam->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
				ret = -EINVAL;
			else {
				if (common->started) {
					dev_err(vpfe_dev,
						"Steaming ON. Cannot"
						" change capture"
						" streaming params.");
					ret = -EINVAL;
				} else {
					struct v4l2_captureparm *capparam
						= &sparam->parm.capture;

					if (common->fmt.fmt.pix.field
						!= V4L2_FIELD_NONE) {
						dev_err(vpfe_dev,
							"Supported only with"
							"progressive scan");
						ret = -EINVAL;
						break;
					}

					if (vpfe_get_decoder_std_info(
							channel,
							&standard) < 0) {
						dev_err(vpfe_dev,
							"Unable to get"
							"standard from"
							" decoder\n");
						ret = -EFAULT;
						break;
					}

					if (!capparam->
						timeperframe.numerator ||
					    !capparam->
						timeperframe.denominator) {
						dev_err(vpfe_dev,
							"invalid timeperframe");
						ret = -EFAULT;
						break;
					}

					if (capparam->
						timeperframe.numerator !=
					    standard.frameperiod.numerator ||
					    capparam->
						timeperframe.denominator >
					    standard.frameperiod.denominator) {
						dev_err(vpfe_dev,
							"Unable to get"
							"standard from"
							" decoder\n");
					}
					channel->timeperframe =
						capparam->timeperframe;

					channel->skip_frame_count =
					   standard.frameperiod.denominator/
					   capparam->timeperframe.denominator;
					channel->skip_frame_count_init =
						channel->skip_frame_count;
				}
			}
			break;
		}
	case VIDIOC_G_PARM:
		{
			struct v4l2_streamparm *sparam =
				(struct v4l2_streamparm *)arg;
			if (sparam->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
				ret = -EINVAL;
			else {
				struct v4l2_captureparm *capparam
						= &sparam->parm.capture;
				memset(capparam,
					0,
					sizeof(struct v4l2_captureparm));
				capparam->capability = V4L2_CAP_TIMEPERFRAME;
				capparam->timeperframe = channel->timeperframe;
			}
			break;
		}
	default:
		dev_err(vpfe_dev, "Invalid command\n");
		ret = -EINVAL;
	}

	dev_dbg(vpfe_dev, "End of vpfe_doioctl\n");
	return ret;
}

/* vpfe_ioctl : Calls vpfe_doioctl function
 */
static int vpfe_ioctl(struct inode *inode, struct file *file,
		      unsigned int cmd, unsigned long arg)
{
	int ret;
	char sbuf[128];
	void *mbuf = NULL;
	void *parg = NULL;

	dev_dbg(vpfe_dev, "Start of vpfe ioctl\n");
	if (ISEXTERNALCMD(cmd)) {
		ret = vpfe_doioctl(inode, file, cmd, arg);
		if (ret == -ENOIOCTLCMD)
			ret = -EINVAL;
		goto out;
	}

	/*  Copy arguments into temp kernel buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_NONE:
		parg = NULL;
		break;
	case _IOC_READ:
	case _IOC_WRITE:
	case (_IOC_WRITE | _IOC_READ):
		if (_IOC_SIZE(cmd) <= sizeof(sbuf)) {
			parg = sbuf;
		} else {
			/* too big to allocate from stack */
			mbuf = kmalloc(_IOC_SIZE(cmd), GFP_KERNEL);
			if (NULL == mbuf)
				return -ENOMEM;
			parg = mbuf;
		}

		ret = -EFAULT;
		if (_IOC_DIR(cmd) & _IOC_WRITE)
			if (copy_from_user(parg, (void __user *)arg,
					   _IOC_SIZE(cmd)))
				goto out;
		break;
	}

	/* call driver */
	ret = vpfe_doioctl(inode, file, cmd, (unsigned long)parg);
	if (ret == -ENOIOCTLCMD)
		ret = -EINVAL;

	/*  Copy results into user buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_READ:
	case (_IOC_WRITE | _IOC_READ):
		if (copy_to_user((void __user *)arg, parg, _IOC_SIZE(cmd)))
			ret = -EFAULT;
		break;
	}
out:
	kfree(mbuf);

	dev_dbg(vpfe_dev, "End of vpfe ioctl\n");
	return ret;
}

/*  vpfe_mmap : It is used to map kernel space buffers
 *  into user spaces
 */
static int vpfe_mmap(struct file *filep, struct vm_area_struct *vma)
{
	/* Get the channel object and file handle object */
	struct vpfe_fh *fh = filep->private_data;
	struct channel_obj *channel = fh->channel;
	struct common_obj *common;
	int err = 0;
	dev_dbg(vpfe_dev, "Start of vpfe mmap\n");
	common = &(channel->common[VPFE_VIDEO_INDEX]);

	err = videobuf_mmap_mapper(&common->buffer_queue, vma);
	dev_dbg(vpfe_dev, "End of vpfe mmap\n");
	return err;
}

/* vpfe_poll: It is used for select/poll system call
 */
static unsigned int vpfe_poll(struct file *filep, poll_table *wait)
{
	int err = 0;
	struct vpfe_fh *fh = filep->private_data;
	struct channel_obj *channel = fh->channel;
	struct common_obj *common = &(channel->common[VPFE_VIDEO_INDEX]);

	dev_dbg(vpfe_dev, "<vpfe_poll>");

	if (common->started)
		err = videobuf_poll_stream(filep, &common->buffer_queue, wait);

	dev_dbg(vpfe_dev, "</vpfe_poll>");
	return err;
}

static int vpfe_initialize_channel(struct channel_obj *channel,
		struct decoder_device *dec)
{
	struct common_obj *common = NULL;
	struct video_obj *vid_ch = NULL;
	int err = 0;
	common = &(channel->common[VPFE_VIDEO_INDEX]);
	vid_ch = &(channel->video);
	common->imp_chained = 0;
	common->second_output = 0;
	common->second_out_img_sz = 0;
	common->rsz_present = 0;
	common->out_from = VPFE_CCDC_OUT;
	channel->skip_frame_count = 1;
	channel->skip_frame_count_init = 1;
	if (!(ISNULL(imp_hw_if)) &&
		(imp_hw_if->get_preview_oper_mode() == IMP_MODE_CONTINUOUS)) {
		if (imp_hw_if->get_previewer_config_state()
			== STATE_CONFIGURED) {
			dev_info(vpfe_dev, "IMP chained \n");
			common->imp_chained = 1;
			common->out_from = VPFE_IMP_PREV_OUT;
			if (imp_hw_if->get_resizer_config_state()
				== STATE_CONFIGURED) {
				dev_info(vpfe_dev, "Resizer present\n");
				common->rsz_present = 1;
				common->out_from = VPFE_IMP_RSZ_OUT;
				if (imp_hw_if->get_output_state(1)) {
					dev_info(vpfe_dev, "second output"
							"present\n");
					common->second_output = 1;
					common->second_out_img_sz =
						imp_hw_if->
						get_line_length(1) *
						imp_hw_if->
						get_image_height(1);
				}
			}
		}
	}
	/* Allocate memory for six channel objects */
	vpfe_update_pix_table_info(dec->if_type, common);
	/* Initialize decoder by calling initialize function */
	err = dec->initialize(dec, DECODER_FULL_INIT_FLAG);
	/* Get the default standard and info about standard */
	err |= dec->std_ops->getstd(&vid_ch->std, dec);
	err |= vpfe_get_std_info(channel);

	/* Configure the default format information */
	err |= vpfe_config_format(channel);
	if (!err)
		channel->initialized = 1;
	return err;
}
/* vpfe_open : It creates object of file handle structure and
 * stores it in private_data  member of filepointer
 */
static int vpfe_open(struct inode *inode, struct file *filep)
{
	int minor = iminor(inode);
	int found = -1;
	int i = 0;
	struct channel_obj *channel = NULL;
	struct decoder_device *dec = NULL;
	struct video_obj *vid_ch = NULL;
	struct common_obj *common = NULL;
	struct vpfe_fh *fh = NULL;
	u8 name[20];

	dev_dbg(vpfe_dev, "<vpfe open>\n");

	/* Check for valid minor number */
	for (i = 0; i < VPFE_CAPTURE_MAX_DEVICES; i++) {
		/* Get the pointer to the channel object */
		channel = vpfe_obj.dev[i];
		vid_ch = &(channel->video);
		common = &(channel->common[VPFE_VIDEO_INDEX]);
		if (minor == channel->video_dev->minor) {
			found = i;
			break;
		}
	}
	/* If not found, return error no device */
	if (0 > found) {
		dev_err(vpfe_dev, "device not found\n");
		return -ENODEV;
	}

	/* Allocate memory for the file handle object */
	fh = kmalloc(sizeof(struct vpfe_fh), GFP_KERNEL);
	if (ISNULL(fh)) {
		dev_err(vpfe_dev,
			"unable to allocate memory for file handle object\n");
		return -ENOMEM;
	}
	/* store pointer to fh in private_data member of filep */
	filep->private_data = fh;
	fh->channel = channel;
	fh->initialized = 0;
	/* If decoder is not initialized. initialize it */
	if (!channel->initialized) {
		channel->current_decoder = 0;
		/* Select the decoder as per the device_type argument */
		if (config_params.device_type >= 0) {
			switch (config_params.device_type) {
			case MT9T001:
			case MT9T031:
				strncpy(name, "MT9T001", sizeof(name));
				break;
			case MT9P031:
				strncpy(name, "MT9P031", sizeof(name));
				break;
			case TVP5146:
				strncpy(name, "TVP514X", sizeof(name));
				break;
			case TVP7002:
				strncpy(name, "TVP7002", sizeof(name));
				break;
			default:
				strncpy(name, "TVP514X", sizeof(name));
				break;
			}
			for (i = 0; i < channel->numdecoders; i++) {
				if (0 ==
					strncmp(channel->decoder[i]->name,
						name,
						sizeof(name))) {
					channel->current_decoder = i;
					break;
				}
			}
		}
		channel->default_decoder = channel->current_decoder;
		dec = channel->decoder[channel->current_decoder];
		vid_ch->input_idx = 0;

		if (ISNULL(dec)) {
			dev_err(vpfe_dev, "No decoder registered\n");
			return -ENXIO;
		}
		if (ISNULL(dec->std_ops) || ISNULL(dec->std_ops->getstd)) {
			dev_err(vpfe_dev, "No standard functions in decoder\n");
			return -EINVAL;
		}
		if (vpfe_initialize_channel(channel, dec)) {
			dev_err(vpfe_dev, "Error in initializing channel\n");
			return -EINVAL;
		}
		fh->initialized = 1;
	}
	/* Increment channel usrs counter */
	channel->usrs++;
	/* Set io_allowed member to false */
	fh->io_allowed[VPFE_VIDEO_INDEX] = 0;
	/* Initialize priority of this instance to default priority */
	fh->prio = V4L2_PRIORITY_UNSET;
	v4l2_prio_open(&channel->prio, &fh->prio);

	dev_dbg(vpfe_dev, "</vpfe_open>\n");
	return 0;
}

static void vpfe_free_allbuffers(struct channel_obj *channel)
{
	int i;
	struct common_obj *common = &(channel->common[VPFE_VIDEO_INDEX]);
	u32 start = config_params.numbuffers[channel->channel_id];
	u32 end = common->numbuffers;
	u32 bufsize = config_params.channel_bufsize[channel->channel_id];
	for (i = start; i < end; i++) {
		if (common->fbuffers[i]) {
			vpfe_free_buffer((unsigned long)common->
					 fbuffers[i], bufsize);
		}
		common->fbuffers[i] = 0;
	}
}

/* vpfe_release : This function deletes buffer queue, frees the
 * buffers and the vpfe file  handle
 */
static int vpfe_release(struct inode *inode, struct file *filep)
{
	struct common_obj *common = NULL;
	/* Get the channel object and file handle object */
	struct vpfe_fh *fh = filep->private_data;
	struct channel_obj *channel = fh->channel;
	struct decoder_device *dec = channel->decoder[channel->current_decoder];

	dev_dbg(vpfe_dev, "<vpfe_release>\n");
	common = &(channel->common[VPFE_VIDEO_INDEX]);
	/* If this is doing IO and other channels are not closed */
	if ((channel->usrs != 1) && fh->io_allowed[VPFE_VIDEO_INDEX]) {
		dev_err(vpfe_dev, "Close other instances\n");
		return -EAGAIN;
	}
	/* Get the lock on channel object */
	mutex_lock(&common->lock);
	/* if this instance is doing IO */
	if (fh->io_allowed[VPFE_VIDEO_INDEX]) {
		/* Reset io_usrs member of channel object */
		if (common->started) {
			ccdc_hw_if->enable(0);
			if (ccdc_hw_if->enable_out_to_sdram)
				ccdc_hw_if->enable_out_to_sdram(0);
			if (common->imp_chained)
				imp_hw_if->enable(0, NULL);
			if (vpfe_detach_irq(channel) < 0) {
				dev_err(vpfe_dev, "Error in detaching IRQ\n");
				mutex_unlock(&common->lock);
				return -EFAULT;
			}
		}

		if (common->imp_chained)
			imp_hw_if->unlock_chain();

		common->io_usrs = 0;
		/* Disable channel/vbi as per its device type and channel id */
		common->started = 0;
		/* Free buffers allocated */
		videobuf_queue_cancel(&common->buffer_queue);
		vpfe_free_allbuffers(channel);
		common->numbuffers =
		    config_params.numbuffers[channel->channel_id];
		common->imp_chained = 0;
		common->second_output = 0;
		common->rsz_present = 0;
	}
	/* unlock semaphore on channel object */
	mutex_unlock(&common->lock);

	/* Decrement channel usrs counter */
	channel->usrs--;
	/* Close the priority */
	v4l2_prio_close(&channel->prio, &fh->prio);
	/* If this file handle has initialize decoder device, reset it */
	if (fh->initialized) {
		dec->deinitialize(dec);
		channel->initialized = 0;
		if (ccdc_hw_if->deinitialize)
			ccdc_hw_if->deinitialize();
	}
	filep->private_data = NULL;
	/* Free memory allocated to file handle object */
	kfree(fh);

	dev_dbg(vpfe_dev, "</vpfe_release>\n");
	return 0;
}

static void vpfe_platform_release(struct device
				  *device)
{
	/* This is called when the reference count goes to zero. */
}

static struct file_operations vpfe_fops = {
	.owner = THIS_MODULE,
	.open = vpfe_open,
	.release = vpfe_release,
	.ioctl = vpfe_ioctl,
	.mmap = vpfe_mmap,
	.poll = vpfe_poll
};

static struct video_device vpfe_video_template = {
	.name = "vpfe",
	.type = VID_TYPE_CAPTURE,
	.hardware = 0,
	.fops = &vpfe_fops,
	.minor = -1,
};

/* vpfe_probe : This function creates device entries by register
 * itself to the V4L2 driver and initializes fields of each
 * channel objects
 */
static __init int vpfe_probe(struct device *device)
{
	struct common_obj *common = NULL;
	int i, j = 0, k, err = 0, index = 0;
	struct video_device *vfd = NULL;
	struct channel_obj *channel = NULL;
	struct video_obj *vid_ch = NULL;
	vpfe_dev = device;

	printk(KERN_NOTICE "vpfe_probe\n");
	for (i = 0; i < VPFE_CAPTURE_MAX_DEVICES; i++) {
		/* Get the pointer to the channel object */
		channel = vpfe_obj.dev[i];
		/* Allocate memory for video device */
		vfd = video_device_alloc();
		if (ISNULL(vfd)) {
			for (j = 0; j < i; j++) {
				video_device_release(vpfe_obj.dev[j]->
						     video_dev);
			}
			return -ENOMEM;
		}

		/* Initialize field of video device */
		*vfd = vpfe_video_template;
		vfd->release = video_device_release;
		snprintf(vfd->name, sizeof(vfd->name),
			 "DaVinci_CCDC_Capture_DRIVER_V%d.%d.%d",
			 (VPFE_CAPTURE_VERSION_CODE >> 16) & 0xff,
			 (VPFE_CAPTURE_VERSION_CODE >> 8) & 0xff,
			 (VPFE_CAPTURE_VERSION_CODE) & 0xff);
		/* Set video_dev to the video device */
		channel->video_dev = vfd;
	}

	for (j = 0; j < VPFE_CAPTURE_MAX_DEVICES; j++) {
		channel = vpfe_obj.dev[j];
		channel->usrs = 0;
		common = &(channel->common[VPFE_VIDEO_INDEX]);
		common->io_usrs = 0;
		common->started = 0;
		spin_lock_init(&common->irqlock);
		common->numbuffers = 0;
		common->field_off = common->second_off = 0;
		common->curFrm = common->nextFrm = NULL;
		memset(&common->fmt, 0, sizeof(struct v4l2_format));
		channel->video.std = 0;
		channel->initialized = 0;
		channel->channel_id = j;
		vid_ch = &(channel->video);
		/* Initialize field of the channel objects */
		channel->usrs = common->io_usrs = vid_ch->std = 0;
		common->started = channel->initialized = 0;
		channel->channel_id = j;
		common->numbuffers =
		    config_params.numbuffers[channel->channel_id];
		common->irq_type = VPFE_NO_IRQ;
		channel->numdecoders = 0;
		channel->current_decoder = 0;
		for (index = 0; index < VPFE_CAPTURE_NUM_DECODERS; index++)
			channel->decoder[index] = NULL;

		/* Initialize prio member of channel object */
		v4l2_prio_init(&channel->prio);
		mutex_init(&common->lock);

		/* register video device */
		dev_dbg(vpfe_dev, "trying to register vpfe device.\n");
		dev_dbg(vpfe_dev, "channel=%x,channel->video_dev=%x\n",
			(int)channel, (int)&channel->video_dev);
		channel->common[VPFE_VIDEO_INDEX].fmt.type =
		    V4L2_BUF_TYPE_VIDEO_OUTPUT;
		err = video_register_device(channel->video_dev,
					    VFL_TYPE_GRABBER, vpfe_nr[0]);
		if (err)
			goto probe_out;
	}
	return 0;

probe_out:

	for (k = 0; k < j; k++) {
		/* Get the pointer to the channel object */
		channel = vpfe_obj.dev[k];
		/* Unregister video device */
		video_unregister_device(channel->video_dev);
	}

	dev_dbg(vpfe_dev, "</vpfe_probe>\n");
	return err;
}

/*  vpfe_remove : It un-register channels from V4L2 driver
 */
static int vpfe_remove(struct device *device)
{
	int i;
	struct channel_obj *channel;
	dev_dbg(vpfe_dev, "<vpfe_remove>\n");
	/* un-register device */
	for (i = 0; i < VPFE_CAPTURE_MAX_DEVICES; i++) {
		/* Get the pointer to the channel object */
		channel = vpfe_obj.dev[i];
		/* Unregister video device */
		video_unregister_device(channel->video_dev);
		channel->video_dev = NULL;
	}
	dev_dbg(vpfe_dev, "</vpfe_remove>\n");
	return 0;
}

static struct device_driver vpfe_driver = {
	.name = "vpfe ccdc capture",
	.bus = &platform_bus_type,
	.probe = vpfe_probe,
	.remove = vpfe_remove,
};

static struct platform_device _vpfe_device = {
	.name = "vpfe ccdc capture",
	.id = 1,
	.dev = {
		.release = vpfe_platform_release,
	}
};

/* vpfe_release_fbuffers: free frame buffers from channel 0 to
 *  maximum specified
 */
static void vpfe_release_fbuffers(int max_channels)
{
	u8 *addr;
	int i, j;
	for (i = 0; i < max_channels; i++) {
		for (j = 0; j < config_params.numbuffers[i]; j++) {
			addr =
			    vpfe_obj.dev[i]->common[VPFE_VIDEO_INDEX].
			    fbuffers[j];
			if (addr) {
				vpfe_free_buffer((unsigned long)addr,
						 config_params.
						 channel_bufsize[i]
				    );
				vpfe_obj.dev[i]->
				    common[VPFE_VIDEO_INDEX].fbuffers[j] = NULL;
			}
		}
	}
}
/* vpfe_init : This function registers device and driver to
 * the kernel, requests irq handler and allocates memory
 * for channel objects
 */
static __init int vpfe_init(void)
{
	int err = 0, i, j;
	int free_channel_objects_index;
	int free_buffer_channel_index;
	int free_buffer_index;
	u8 *addr;
	u32 size;

	printk(KERN_NOTICE "vpfe_init\n");
	/* Default number of buffers should be 3 */
	if ((channel0_numbuffers > 0) &&
	    (channel0_numbuffers < config_params.min_numbuffers))
		channel0_numbuffers = config_params.min_numbuffers;

	/* Set buffer size to min buffers size if invalid buffer size is
	 * given
	 */
	if (channel0_bufsize < config_params.min_bufsize[VPFE_CHANNEL0_VIDEO])
		channel0_bufsize =
		    config_params.min_bufsize[VPFE_CHANNEL0_VIDEO];

	config_params.numbuffers[VPFE_CHANNEL0_VIDEO] = channel0_numbuffers;

	if (channel0_numbuffers)
		config_params.channel_bufsize[VPFE_CHANNEL0_VIDEO]
		    = channel0_bufsize;

	/* Check the correct value of device_type */
	config_params.device_type = device_type;

	/* Initialise the ipipe hw module if exists */
	if (!cpu_is_davinci_dm644x()) {
		imp_hw_if = imp_get_hw_if();
		if (ISNULL(imp_hw_if))
			return -1;
	}

	ccdc_hw_if = ccdc_get_hw_interface();
	if (ISNULL(ccdc_hw_if))
		return -1;

	/* Make sure module implements mandatory functions. Check for
	 * for NULL and return error if not implemented
	 */
	if (ISNULL(ccdc_hw_if->enable) ||
		ISNULL(ccdc_hw_if->set_hw_if_type) ||
		ISNULL(ccdc_hw_if->configure) ||
		ISNULL(ccdc_hw_if->set_buftype) ||
		ISNULL(ccdc_hw_if->get_buftype) ||
		ISNULL(ccdc_hw_if->set_frame_format) ||
		ISNULL(ccdc_hw_if->get_frame_format) ||
		ISNULL(ccdc_hw_if->get_pixelformat) ||
		ISNULL(ccdc_hw_if->set_pixelformat) ||
		ISNULL(ccdc_hw_if->setparams) ||
		ISNULL(ccdc_hw_if->set_image_window) ||
		ISNULL(ccdc_hw_if->get_image_window) ||
		ISNULL(ccdc_hw_if->setstd) ||
		ISNULL(ccdc_hw_if->getstd_info) ||
		ISNULL(ccdc_hw_if->setfbaddr) ||
		ISNULL(ccdc_hw_if->getfid)) {
		return -1;
	}

	if (device_type == TVP5146)
		ccdc_hw_if->set_hw_if_type(CCDC_BT656);
	else if (device_type == TVP7002)
		ccdc_hw_if->set_hw_if_type(CCDC_BT1120);
	else if (device_type == MT9T001 || device_type == MT9T031 ||
		device_type == MT9P031)
		ccdc_hw_if->set_hw_if_type(CCDC_RAW_BAYER);
	else
		return -1;

	if (ccdc_hw_if->initialize)
		ccdc_hw_if->initialize();

	/* Allocate memory for six channel objects */
	for (i = 0; i < VPFE_CAPTURE_MAX_DEVICES; i++) {
		vpfe_obj.dev[i] =
		    kmalloc(sizeof(struct channel_obj), GFP_KERNEL);
		/* If memory allocation fails, return error */
		if (!vpfe_obj.dev[i]) {
			free_channel_objects_index = i;
			err = -ENOMEM;
			goto vpfe_init_free_channel_objects;
		}
	}
	free_channel_objects_index = VPFE_CAPTURE_MAX_DEVICES;

	/* Allocate memory for buffers */
	for (i = 0; i < CCDC_CAPTURE_NUM_CHANNELS; i++) {
		size = config_params.channel_bufsize[i];
		for (j = 0; j < config_params.numbuffers[i]; j++) {
			addr = (u8 *) vpfe_alloc_buffer(size);
			if (!addr) {
				free_buffer_channel_index = i;
				free_buffer_index = j;
				err = -ENOMEM;
				goto vpfe_init_free_buffers;
			}
			vpfe_obj.dev[i]->common[VPFE_VIDEO_INDEX].
			    fbuffers[j] = addr;
		}
	}
	free_buffer_channel_index = CCDC_CAPTURE_NUM_CHANNELS;
	free_buffer_index = config_params.numbuffers[i - 1];

	/* Register driver to the kernel */
	err = driver_register(&vpfe_driver);
	if (0 != err)
		goto vpfe_init_free_buffers;

	/* register device as a platform device to the kernel */
	err = platform_device_register(&_vpfe_device);
	if (0 != err)
		goto vpfe_init_unregister_vpfe_driver;
	return 0;

vpfe_init_unregister_vpfe_driver:
	driver_unregister(&vpfe_driver);

vpfe_init_free_buffers:
	vpfe_release_fbuffers(free_buffer_channel_index);
	if (free_buffer_channel_index != CCDC_CAPTURE_NUM_CHANNELS) {
		/* buffer allocation failed */
		for (j = 0; j < free_buffer_index; j++) {
			addr = vpfe_obj.dev[free_buffer_channel_index]->
					common[VPFE_VIDEO_INDEX].fbuffers[j];
			if (addr) {
				vpfe_free_buffer((unsigned long)addr,
					config_params.channel_bufsize[i]);
				vpfe_obj.dev[free_buffer_channel_index]->
				  common[VPFE_VIDEO_INDEX].fbuffers[j] = NULL;
			}
		}
	}

vpfe_init_free_channel_objects:
	for (j = 0; j < free_channel_objects_index; j++) {
		kfree(vpfe_obj.dev[j]);
		vpfe_obj.dev[i] = NULL;
	}

	return err;
}

/* vpfe_cleanup : This function un-registers device and driver
 * to the kernel, frees requested irq handler and de-allocates memory
 * allocated for channel objects.
 */
static void vpfe_cleanup(void)
{
	int i = 0;

	platform_device_unregister(&_vpfe_device);
	driver_unregister(&vpfe_driver);
	vpfe_release_fbuffers(CCDC_CAPTURE_NUM_CHANNELS);
	for (i = 0; i < VPFE_CAPTURE_MAX_DEVICES; i++) {
		kfree(vpfe_obj.dev[i]);
		vpfe_obj.dev[i] = NULL;
	}
	if (ccdc_hw_if->deinitialize)
		ccdc_hw_if->deinitialize();
}

/* vpfe_enum_std : Function to enumerate all the standards in
 * all the registered decoders
 */
static int vpfe_enum_std(struct channel_obj *channel)
{
	int i, j, index;
	struct video_obj *vid_ch = NULL;
	struct v4l2_standard standard;
	struct decoder_device *dec;
	v4l2_std_id all_std = 0;
	int ret = 0;
	vid_ch = &(channel->video);
	vid_ch->count_std = 0;

	/* For all the registered encoders */
	for (i = 0; i < channel->numdecoders; i++) {
		dec = channel->decoder[i];
		/* Do the enumstd */
		for (j = 0; j < dec->std_ops->count; j++) {
			standard.index = j;
			ret = dec->std_ops->enumstd(&standard, dec);
			if (ret)
				return ret;

			/* If the standard is already added,
			 * do not add it to the table */
			if (all_std & standard.id)
				continue;
			/* Store the standard information in the table */
			index = vid_ch->count_std;
			vid_ch->std_tbl[index].dec_idx = i;
			vid_ch->std_tbl[index].std_idx = j;
			vid_ch->std_tbl[index].std = standard.id;
			vid_ch->count_std++;
			all_std |= standard.id;
		}
	}
	return 0;
}

/* vpif_register_decoder : This function will be called by the decoder
 * driver to register its functionalities to vpfe driver
 */
int vpif_register_decoder(struct decoder_device *decoder)
{
	struct channel_obj *channel = vpfe_obj.dev[decoder->channel_id];
	int err = -EINVAL;

	dev_notice(vpfe_dev, "vpif_register_decoder: decoder = %s\n",
	       decoder->name);
	if (ISNULL(channel))
		return err;
	if (channel->numdecoders < VPFE_CAPTURE_NUM_DECODERS) {
		channel->decoder[channel->numdecoders++] = decoder;
		err = vpfe_enum_std(channel);
	}
	dev_dbg(vpfe_dev, "</vpfe_register_decoder>\n");
	return err;
}
EXPORT_SYMBOL(vpif_register_decoder);

/* vpif_unregister_decoder : This function will be called by the decoder
 * driver to un-register its functionalities to vpfe driver
 */
int vpif_unregister_decoder(struct decoder_device *decoder)
{
	int i, j = 0, err = 0;
	struct channel_obj *channel = vpfe_obj.dev[decoder->channel_id];
	dev_dbg(vpfe_dev, "<vpif_unregister_decoder>\n");

	for (i = 0; i < channel->numdecoders; i++) {
		if (decoder == channel->decoder[i]) {
			if (channel->
			    decoder[channel->current_decoder] == decoder
			    && channel->initialized)
				return -EBUSY;
			channel->decoder[i] = NULL;
			for (j = i; j < channel->numdecoders - 1; j++)
				channel->decoder[j] = channel->decoder[j + 1];
			channel->numdecoders--;
			err = vpfe_enum_std(channel);
			break;
		}
	}
	dev_dbg(vpfe_dev, "</vpfe_unregister_decoder>\n");
	return err;
}
EXPORT_SYMBOL(vpif_unregister_decoder);

MODULE_LICENSE("GPL");
/* Function for module initialization and cleanup */
module_init(vpfe_init);
module_exit(vpfe_cleanup);
