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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <media/v4l2-dev.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/page.h>
#include <asm/gpio.h>
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>
#include <asm/arch/hardware.h>
#include <video/davinci_vpbe.h>
#include <media/davinci/davinci_enc.h>
#include <media/davinci/davinci_display.h>

#define DAVINCI_DISPLAY_DRIVER "DavinciDisplay"
#define DM355_EVM_CARD  "DM355 EVM"
#define DM644X_EVM_CARD "DM644X EVM"
#define DM365_EVM_CARD  "DM365 EVM"
#define WOLVERINE_CARD  "WOLVERINE"

// uncomment this if want to work with FULL speed USB
//#define FULL_SPEED_USB 1

// HOLDMODE_TRANSFER_MAX must be a multiple of 32 to keep the buffer 
// cache aligned for the display engine
#if FULL_SPEED_USB

//#define HOLDMODE_TRANSFER_MAX  (0x0200)  // 512
#define HOLDMODE_TRANSFER_MAX  (0x0100)  // 256
#define HOLDMODE_LINES_MAX     (1)
#define HOLDMODE_PERLINE_TIME  (42) /* ~42 uS per line (of 1024, at 27 MHz PCLK (which is same as VPBE clock) */
//#define HOLDMODE_PERLINE_TIME  (50) /* ~42 uS per line */

#else

#define HOLDMODE_TRANSFER_MAX  (0x0400)  //1024
#define HOLDMODE_LINES_MAX     (2)
#define HOLDMODE_PERLINE_TIME  (42) /* ~42 uS per line (of 1024, at 27 MHz PCLK (which is same as VPBE clock) */

#endif

/* used for setting from full speed from USB UVC controls via ioctl */
#define USB_FS_HOLDMODE_TRANSFER_MAX  (0x0100)  // 256
#define USB_FS_HOLDMODE_LINES_MAX     (1)

#define MULTI_STREAM_HOLDMODE_TRANSFER_MAX  (0x0800)  // 256
#define MULTI_STREAM_HOLDMODE_LINES_MAX     (4)


/* below holdModeWriteBuffer is always allocated for High speed mode, even if used in Full-speed mode */
static unsigned char davinci_holdModeWriteBuffer[HOLDMODE_TRANSFER_MAX * HOLDMODE_LINES_MAX] __attribute__ ((aligned (32)));
static unsigned int  davinci_holdModeWaitVertical = 0;

/* for USB full-speed support */
static unsigned int  set_usb_full_sp = 0;
static unsigned int uHOLDMODE_TRANSFER_MAX = HOLDMODE_TRANSFER_MAX;
static unsigned int uHOLDMODE_LINES_MAX = HOLDMODE_LINES_MAX;

static unsigned int  set_multi_stream_vpbe = 0;

DECLARE_WAIT_QUEUE_HEAD(davinvi_holdModeWaitQueue);

static u32 video2_numbuffers = 3;
static u32 video3_numbuffers = 3;

#define DAVINCI_DISPLAY_HD_BUF_SIZE (1920*1088*2)
#define DAVINCI_DISPLAY_SD_BUF_SIZE (720*576*2)

#ifdef CONFIG_DAVINCI_THS8200_ENCODER
static u32 video2_bufsize = DAVINCI_DISPLAY_HD_BUF_SIZE;
#else
static u32 video2_bufsize = DAVINCI_DISPLAY_SD_BUF_SIZE;
#endif
static u32 video3_bufsize = DAVINCI_DISPLAY_SD_BUF_SIZE;

module_param(video2_numbuffers, uint, S_IRUGO);
module_param(video3_numbuffers, uint, S_IRUGO);

module_param(video2_bufsize, uint, S_IRUGO);
module_param(video3_bufsize, uint, S_IRUGO);

#define DAVINCI_DEFAULT_NUM_BUFS 3
static struct buf_config_params display_buf_config_params = {
	.min_numbuffers = DAVINCI_DEFAULT_NUM_BUFS,
	.numbuffers[0] = DAVINCI_DEFAULT_NUM_BUFS,
	.numbuffers[1] = DAVINCI_DEFAULT_NUM_BUFS,
	.min_bufsize[0] = DAVINCI_DISPLAY_SD_BUF_SIZE,
	.min_bufsize[1] = DAVINCI_DISPLAY_SD_BUF_SIZE,
#ifdef CONFIG_DAVINCI_THS8200_ENCODER
	.layer_bufsize[0] = DAVINCI_DISPLAY_HD_BUF_SIZE,
	.layer_bufsize[1] = DAVINCI_DISPLAY_SD_BUF_SIZE,
#else
	.layer_bufsize[0] = DAVINCI_DISPLAY_SD_BUF_SIZE,
	.layer_bufsize[1] = DAVINCI_DISPLAY_SD_BUF_SIZE,
#endif
};

unsigned long Addr= 0;

static int davinci_display_nr[] = { 2, 3 };

/* global variables */
static struct davinci_display davinci_dm;

struct device *davinci_display_dev = NULL;

static struct v4l2_capability davinci_display_videocap = {
	.driver = DAVINCI_DISPLAY_DRIVER,
	.bus_info = "Platform",
	.version = DAVINCI_DISPLAY_VERSION_CODE,
	.capabilities = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING
};

static struct v4l2_fract ntsc_aspect = DAVINCI_DISPLAY_PIXELASPECT_NTSC;
static struct v4l2_fract pal_aspect = DAVINCI_DISPLAY_PIXELASPECT_PAL;
static struct v4l2_fract sp_aspect = DAVINCI_DISPLAY_PIXELASPECT_SP;

static struct v4l2_rect ntsc_bounds = DAVINCI_DISPLAY_WIN_NTSC;
static struct v4l2_rect pal_bounds = DAVINCI_DISPLAY_WIN_PAL;
static struct v4l2_rect vga_bounds = DAVINCI_DISPLAY_WIN_640_480;
static struct v4l2_rect hd_720p_bounds = DAVINCI_DISPLAY_WIN_720P;
static struct v4l2_rect hd_1080i_bounds = DAVINCI_DISPLAY_WIN_1080I;

/*
 * davinci_alloc_buffer()
 * Function to allocate memory for buffers
 */
static inline unsigned long davinci_alloc_buffer(unsigned int buf_size)
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

/*
 * davinci_free_buffer()
 * function to Free memory for buffers
 */
static inline void davinci_free_buffer(unsigned long addr,
				       unsigned int buf_size)
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

/*
 * davinci_uservirt_to_phys()
 * This inline function is used to convert user space virtual address
 * to physical address.
 */
static inline u32 davinci_uservirt_to_phys(u32 virt)
{
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;

	struct mm_struct *mm = current->mm;
	pgd = pgd_offset(mm, virt);
	if (!(pgd_none(*pgd) || pgd_bad(*pgd))) {
		pmd = pmd_offset(pgd, virt);

		if (!(pmd_none(*pmd) || pmd_bad(*pmd))) {
			pte = pte_offset_kernel(pmd, virt);

			if (pte_present(*pte)) {
				return __pa(page_address(pte_page(*pte))
					    + (virt & ~PAGE_MASK));
			}
		}
	}

	return 0;
}

/*
 * davinci_buffer_prepare()
 * This is the callback function called from videobuf_qbuf() function
 * the buffer is prepared and user space virtual address is converted into
 * physical address
 */
static int davinci_buffer_prepare(struct videobuf_queue *q,
				  struct videobuf_buffer *vb,
				  enum v4l2_field field)
{
	/* Get the file handle object and layer object */
	struct davinci_fh *fh = q->priv_data;
	struct display_obj *layer = fh->layer;
	dev_dbg(davinci_display_dev, "<davinci_buffer_prepare>\n");

	/* If buffer is not initialized, initialize it */
	if (STATE_NEEDS_INIT == vb->state) {
		vb->width = davinci_dm.mode_info.xres;
		vb->height = davinci_dm.mode_info.yres;
		vb->size = vb->width * vb->height;
		vb->field = field;
	}
	vb->state = STATE_PREPARED;
	/* if user pointer memory mechanism is used, get the physical
	 * address of the buffer
	 */
	if (V4L2_MEMORY_USERPTR == layer->memory) {
		vb->boff = davinci_uservirt_to_phys(vb->baddr);
		if (!ISALIGNED(vb->boff)) {
			dev_err(davinci_display_dev, "buffer_prepare:offset is \
				not aligned to 8 bytes\n");
			return -EINVAL;
		}
	}
	dev_dbg(davinci_display_dev, "</davinci_buffer_prepare>\n");
	return 0;
}

/*
 * davinci_buffer_config()
 * This function is responsible to responsible for buffer's
 * physical address
 */
static void davinci_buffer_config(struct videobuf_queue *q, unsigned int count)
{
	/* Get the file handle object and layer object */
	struct davinci_fh *fh = q->priv_data;
	struct display_obj *layer = fh->layer;
	int i;
	dev_dbg(davinci_display_dev, "<davinci_buffer_config>\n");

	/* If memory type is not mmap, return */
	if (V4L2_MEMORY_MMAP != layer->memory)
		return;
	/* Convert kernel space virtual address to physical address */
	for (i = 0; i < count; i++) {
		q->bufs[i]->boff = virt_to_phys((void *)layer->fbuffers[i]);
		dev_dbg(davinci_display_dev, "buffer address: %x\n",
			q->bufs[i]->boff);
	}
	dev_dbg(davinci_display_dev, "</davinci_buffer_config>\n");
}

/*
 * davinci_buffer_setup()
 * This function allocates memory for the buffers
 */
static int davinci_buffer_setup(struct videobuf_queue *q, unsigned int *count,
				unsigned int *size)
{
	/* Get the file handle object and layer object */
	struct davinci_fh *fh = q->priv_data;
	struct display_obj *layer = fh->layer;
	int i;
	dev_dbg(davinci_display_dev, "<davinci_buffer_setup>\n");

	/* If memory type is not mmap, return */
	if (V4L2_MEMORY_MMAP != layer->memory)
		return 0;

	/* Calculate the size of the buffer */
	*size = display_buf_config_params.layer_bufsize[layer->device_id];

	for (i = display_buf_config_params.numbuffers[layer->device_id];
	     i < *count; i++) {
		/* Allocate memory for the buffers */
		layer->fbuffers[i] = davinci_alloc_buffer(*size);
		if (!layer->fbuffers[i])
			break;
	}
	/* Store number of buffers allocated in numbuffer member */
	*count = layer->numbuffers = i;
	dev_dbg(davinci_display_dev, "</davinci_buffer_setup>\n");
	return 0;
}

/*
 * davinci_buffer_queue()
 * This function adds the buffer to DMA queue
 */
static void davinci_buffer_queue(struct videobuf_queue *q,
				 struct videobuf_buffer *vb)
{
	/* Get the file handle object and layer object */
	struct davinci_fh *fh = q->priv_data;
	struct display_obj *layer = fh->layer;
	dev_dbg(davinci_display_dev, "<davinci_buffer_queue>\n");

	/* add the buffer to the DMA queue */
	list_add_tail(&vb->queue, &layer->dma_queue);
	/* Change state of the buffer */
	vb->state = STATE_QUEUED;
	dev_dbg(davinci_display_dev, "</davinci_buffer_queue>\n");
}

/*
 * davinci_buffer_release()
 * This function is called from the videobuf layer to free memory allocated to
 * the buffers
 */
static void davinci_buffer_release(struct videobuf_queue *q,
				   struct videobuf_buffer *vb)
{
	/* Get the file handle object and layer object */
	struct davinci_fh *fh = q->priv_data;
	struct display_obj *layer = fh->layer;
	unsigned int buf_size = 0;
	dev_dbg(davinci_display_dev, "<davinci_buffer_release>\n");

	/* If memory type is not mmap, return */
	if (V4L2_MEMORY_MMAP != layer->memory)
		return;
	/* Calculate the size of the buffer */
	buf_size = display_buf_config_params.layer_bufsize[layer->device_id];

	if (((vb->i < layer->numbuffers)
	     && (vb->i >=
		 display_buf_config_params.numbuffers[layer->device_id]))
	    && layer->fbuffers[vb->i]) {
		davinci_free_buffer(layer->fbuffers[vb->i], buf_size);
		layer->fbuffers[vb->i] = 0;
	}
	vb->state = STATE_NEEDS_INIT;
	dev_dbg(davinci_display_dev, "</davinci_buffer_release>\n");
}

static struct videobuf_queue_ops video_qops = {
	.buf_setup = davinci_buffer_setup,
	.buf_prepare = davinci_buffer_prepare,
	.buf_queue = davinci_buffer_queue,
	.buf_release = davinci_buffer_release,
	.buf_config = davinci_buffer_config,
};

static u8 layer_first_int = 1;

/* davinci_display_isr()
 * ISR function. It changes status of the displayed buffer, takes next buffer
 * from the queue and sets its address in VPBE registers
 */
static void davinci_display_isr(unsigned int event, void *dispObj)
{
	unsigned long jiffies_time = get_jiffies_64();
	struct timeval timevalue;
	int i, fid;
	unsigned long addr = 0;
	struct display_obj *layer = NULL;
	struct davinci_display *dispDevice = (struct davinci_display *)dispObj;

	/* Convert time represention from jiffies to timeval */
	jiffies_to_timeval(jiffies_time, &timevalue);

    /* Toggle an I/O so we can see where the interrupt occurs */
#if 0
    {
        static int iToggle = 0;
        if (iToggle) {
            iToggle = 0;
            davinci_writel(0x80000, DAVINCI_GPIO_BASE + 0x6C); // CLR_DATA45 <= GPIO83 aka VSYNC
        } else {
            iToggle = 1;
            davinci_writel(0x80000, DAVINCI_GPIO_BASE + 0x68); // SET_DATA45 <= GPIO83 aka VSYNC
        }
    }
#endif 

    /* Wake up any hold mode tasks */
    wake_up_interruptible(&davinvi_holdModeWaitQueue);
    davinci_holdModeWaitVertical = 0;

	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		layer = dispDevice->dev[i];
		/* If streaming is started in this layer */
		if (!layer->started)
			continue;
		/* Check the field format */
		if ((V4L2_FIELD_NONE == layer->pix_fmt.field) &&
		    (!list_empty(&layer->dma_queue)) &&
		    (event & DAVINCI_DISP_END_OF_FRAME)) {
			/* Progressive mode */
			if (layer_first_int) {
				layer_first_int = 0;
				continue;
			} else {
				/* Mark status of the curFrm to
				 * done and unlock semaphore on it
				 */
				layer->curFrm->ts = timevalue;
				layer->curFrm->state = STATE_DONE;
				wake_up_interruptible(&layer->curFrm->done);
				/* Make curFrm pointing to nextFrm */
				layer->curFrm = layer->nextFrm;
			}
			/* Get the next buffer from buffer queue */
			layer->nextFrm =
			    list_entry(layer->dma_queue.next,
				       struct videobuf_buffer, queue);
			/* Remove that buffer from the buffer queue */
			list_del(&layer->nextFrm->queue);
			/* Mark status of the buffer as active */
			layer->nextFrm->state = STATE_ACTIVE;
//			addr = layer->curFrm->boff;
            //printk("Display from dispBuf = %p\n",addr);
//			addr = Addr;
//			davinci_disp_start_layer(layer->layer_info.id,
//						 addr,
//						 davinci_dm.cbcr_ofst);
		} else {
			/* Interlaced mode
			 * If it is first interrupt, ignore it
			 */
			if (layer_first_int) {
				layer_first_int = 0;
				dev_dbg(davinci_display_dev,
					"irq_first time\n");
				return;
			}

			layer->field_id ^= 1;
			if (event & DAVINCI_DISP_FIRST_FIELD)
				fid = 0;
			else if (event & DAVINCI_DISP_SECOND_FIELD)
				fid = 1;
			else
				return;

			/* If field id does not match with stored
			 * field id
			 */
			if (fid != layer->field_id) {
				/* Make them in sync */
				if (0 == fid) {
					layer->field_id = fid;
					dev_dbg(davinci_display_dev,
						"field synced\n");
				}
				return;
			}
			/* device field id and local field id are
			 * in sync. If this is even field
			 */
			if (0 == fid) {
				if (layer->curFrm == layer->nextFrm)
					continue;
				/* one frame is displayed If next frame is
				 * available, release curFrm and move on
				 * Copy frame display time
				 */
				layer->curFrm->ts = timevalue;
				/* Change status of the curFrm */
				dev_dbg(davinci_display_dev,
					"Done with this video buffer\n");
				layer->curFrm->state = STATE_DONE;
				/* unlock semaphore on curFrm */
				wake_up_interruptible(&layer->curFrm->done);
				/* Make curFrm pointing to
				 * nextFrm
				 */
				layer->curFrm = layer->nextFrm;
			} else if (1 == fid) {	/* odd field */
				if (list_empty(&layer->dma_queue)
				    || (layer->curFrm != layer->nextFrm))
					continue;

				/* one field is displayed configure
				 * the next frame if it is available
				 * otherwise hold on current frame
				 * Get next from the buffer queue
				 */
				layer->nextFrm = list_entry(layer->
							    dma_queue.
							    next, struct
							    videobuf_buffer,
							    queue);

				/* Remove that from the
				 * buffer queue
				 */
				list_del(&layer->nextFrm->queue);

				/* Mark state of the frame
				 * to active
				 */
				layer->nextFrm->state = STATE_ACTIVE;
				addr = layer->nextFrm->boff;
				davinci_disp_start_layer(layer->layer_info.id,
							addr,
							davinci_dm.cbcr_ofst);
			}
		}
	}
}

static struct display_obj*
_davinci_disp_get_other_win(struct display_obj *layer)
{
	enum davinci_display_device_id thiswin, otherwin;
	thiswin = layer->device_id;

	otherwin = (thiswin == DAVINCI_DISPLAY_DEVICE_0) ?
		DAVINCI_DISPLAY_DEVICE_1 : DAVINCI_DISPLAY_DEVICE_0;
	return davinci_dm.dev[otherwin];
}

static int davinci_config_layer(enum davinci_display_device_id id);

static int davinci_set_video_display_params(struct display_obj *layer)
{
	unsigned long addr;
	addr = layer->curFrm->boff;

	/* Set address in the display registers */
	davinci_disp_start_layer(layer->layer_info.id,
				 addr,
				 davinci_dm.cbcr_ofst);
	davinci_disp_enable_layer(layer->layer_info.id, 0);
	/* Enable the window */
	layer->layer_info.enable = 1;
	if (layer->layer_info.config.pixfmt == PIXFMT_NV12) {
		struct display_obj *otherlayer =
			_davinci_disp_get_other_win(layer);
		davinci_disp_enable_layer(otherlayer->layer_info.id, 1);
		otherlayer->layer_info.enable = 1;
	}
	return 0;
}

static void davinci_disp_calculate_scale_factor(struct display_obj *layer,
						int expected_xsize,
						int expected_ysize)
{
	struct display_layer_info *layer_info = &layer->layer_info;
	struct v4l2_pix_format *pixfmt = &layer->pix_fmt;
	int h_scale = 0, v_scale = 0, h_exp = 0, v_exp = 0, temp;
	/* Application initially set the image format. Current display
	 * size is obtained from the encoder manager. expected_xsize
	 * and expected_ysize are set through S_CROP ioctl. Based on this,
	 * driver will calculate the scale factors for vertical and
	 * horizontal direction so that the image is displayed scaled
	 * and expanded. Application uses expansion to display the image
	 * in a square pixel. Otherwise it is displayed using displays
	 * pixel aspect ratio.It is expected that application chooses
	 * the crop coordinates for cropped or scaled display. if crop
	 * size is less than the image size, it is displayed cropped or
	 * it is displayed scaled and/or expanded.
	 *
	 * to begin with, set the crop window same as expected. Later we
	 * will override with scaled window size
	 */
	layer->layer_info.config.xsize = pixfmt->width;
	layer->layer_info.config.ysize = pixfmt->height;
	layer_info->h_zoom = ZOOM_X1;	/* no horizontal zoom */
	layer_info->v_zoom = ZOOM_X1;	/* no horizontal zoom */
	layer_info->h_exp = H_EXP_OFF;	/* no horizontal zoom */
	layer_info->v_exp = V_EXP_OFF;	/* no horizontal zoom */

	if (pixfmt->width < expected_xsize) {
		h_scale = davinci_dm.mode_info.xres / pixfmt->width;
		if (h_scale < 2)
			h_scale = 1;
		else if (h_scale >= 4)
			h_scale = 4;
		else
			h_scale = 2;
		layer->layer_info.config.xsize *= h_scale;
		if (layer->layer_info.config.xsize < expected_xsize) {
			if (!strcmp(davinci_dm.mode_info.name, VID_ENC_STD_NTSC)
			    || !strcmp(davinci_dm.mode_info.name,
				       VID_ENC_STD_PAL)) {
				temp =
				    (layer->layer_info.config.xsize *
				     DAVINCI_DISPLAY_H_EXP_RATIO_N)
				    / DAVINCI_DISPLAY_H_EXP_RATIO_D;
				if (temp <= expected_xsize) {
					h_exp = 1;
					layer->layer_info.config.xsize = temp;
				}
			}
		}
		if (h_scale == 2)
			layer_info->h_zoom = ZOOM_X2;
		else if (h_scale == 4)
			layer_info->h_zoom = ZOOM_X4;
		if (h_exp)
			layer_info->h_exp = H_EXP_9_OVER_8;
	} else {
		/* no scaling, only cropping. Set display area to crop area */
		layer->layer_info.config.xsize = expected_xsize;
	}

	if (pixfmt->height < expected_ysize) {
		v_scale = expected_ysize / pixfmt->height;
		if (v_scale < 2)
			v_scale = 1;
		else if (v_scale >= 4)
			v_scale = 4;
		else
			v_scale = 2;
		layer->layer_info.config.ysize *= v_scale;
		if (layer->layer_info.config.ysize < expected_ysize) {
			if (!strcmp(davinci_dm.mode_info.name, "PAL")) {
				temp =
				    (layer->layer_info.config.ysize *
				     DAVINCI_DISPLAY_V_EXP_RATIO_N)
				    / DAVINCI_DISPLAY_V_EXP_RATIO_D;
				if (temp <= expected_ysize) {
					v_exp = 1;
					layer->layer_info.config.ysize = temp;
				}
			}
		}
		if (v_scale == 2)
			layer_info->v_zoom = ZOOM_X2;
		else if (v_scale == 4)
			layer_info->v_zoom = ZOOM_X4;
		if (v_exp)
			layer_info->h_exp = V_EXP_6_OVER_5;
	} else {
		/* no scaling, only cropping. Set display area to crop area */
		layer->layer_info.config.ysize = expected_ysize;
	}
	dev_dbg(davinci_display_dev,
		"crop display xsize = %d, ysize = %d\n",
		layer->layer_info.config.xsize, layer->layer_info.config.ysize);
}

static void davinci_disp_adj_position(struct display_obj *layer, int top,
				      int left)
{
	layer->layer_info.config.xpos = 0;
	layer->layer_info.config.ypos = 0;
	if (left + layer->layer_info.config.xsize <= davinci_dm.mode_info.xres)
		layer->layer_info.config.xpos = left;
	if (top + layer->layer_info.config.ysize <= davinci_dm.mode_info.yres)
		layer->layer_info.config.ypos = top;
	dev_dbg(davinci_display_dev,
		"new xpos = %d, ypos = %d\n",
		layer->layer_info.config.xpos, layer->layer_info.config.ypos);
}

static int davinci_disp_check_window_params(struct v4l2_rect *c)
{
	if ((c->width == 0)
	    || ((c->width + c->left) > davinci_dm.mode_info.xres)
	    || (c->height == 0)
	    || ((c->height + c->top) > davinci_dm.mode_info.yres)) {
		dev_err(davinci_display_dev, "Invalid crop values\n");
		return -1;
	}
	if ((c->height & 0x1) && (davinci_dm.mode_info.interlaced)) {
		dev_err(davinci_display_dev,
			"window height must be even for interlaced display\n");
		return -1;
	}
	return 0;
}

/* vpbe_try_format()
 * If user application provides width and height, and have bytesperline set
 * to zero, driver calculates bytesperline and sizeimage based on hardware
 * limits. If application likes to add pads at the end of each line and
 * end of the buffer , it can set bytesperline to line size and sizeimage to
 * bytesperline * height of the buffer. If driver fills zero for active
 * video width and height, and has requested user bytesperline and sizeimage,
 * width and height is adjusted to maximum display limit or buffer width
 * height which ever is lower
 */
static int vpbe_try_format(struct v4l2_pix_format *pixfmt, int check)
{
	struct vid_enc_mode_info *mode_info;
	int min_sizeimage, bpp, min_height = 1, min_width = 32,
		max_width, max_height, user_info = 0;

	mode_info = &davinci_dm.mode_info;
	davinci_enc_get_mode(0, mode_info);

	if ((pixfmt->pixelformat != V4L2_PIX_FMT_UYVY) &&
	    (pixfmt->pixelformat != V4L2_PIX_FMT_NV12))
		/* choose default as V4L2_PIX_FMT_UYVY */
		pixfmt->pixelformat = V4L2_PIX_FMT_UYVY;

	if (pixfmt->field == V4L2_FIELD_ANY) {
		if (mode_info->interlaced)
			pixfmt->field = V4L2_FIELD_INTERLACED;
		else
			pixfmt->field = V4L2_FIELD_NONE;
	}

	if (pixfmt->field == V4L2_FIELD_INTERLACED)
		min_height = 2;

	if (pixfmt->pixelformat == V4L2_PIX_FMT_NV12)
		bpp = 1;
	else
		bpp = 2;

	max_width = mode_info->xres;
	max_height = mode_info->yres;

	min_width /= bpp;

	if (!pixfmt->width && !pixfmt->bytesperline) {
		dev_err(davinci_display_dev, "bytesperline and width"
			" cannot be zero\n");
		return -EINVAL;
	}

	/* if user provided bytesperline, it must provide sizeimage as well */
	if (pixfmt->bytesperline && !pixfmt->sizeimage) {
		dev_err(davinci_display_dev,
			"sizeimage must be non zero, when user"
			" provides bytesperline\n");
		return -EINVAL;
	}

	/* adjust bytesperline as per hardware - multiple of 32 */
	if (!pixfmt->width)
		pixfmt->width = pixfmt->bytesperline / bpp;

	if (!pixfmt->bytesperline)
		pixfmt->bytesperline = pixfmt->width * bpp;
	else
		user_info = 1;
	pixfmt->bytesperline = ((pixfmt->bytesperline + 31) & ~31);

	if (pixfmt->width < min_width) {
		if (check) {
			dev_err(davinci_display_dev,
				"height is less than minimum,"
				"input width = %d, min_width = %d \n",
				pixfmt->width, min_width);
			return -EINVAL;
		}
		pixfmt->width = min_width;
	}

	if (pixfmt->width > max_width) {
		if (check) {
			dev_err(davinci_display_dev,
				"width is more than maximum,"
				"input width = %d, max_width = %d\n",
				pixfmt->width, max_width);
			return -EINVAL;
		}
		pixfmt->width = max_width;
	}

	/* If height is zero, then atleast we need to have sizeimage
	 * to calculate height
	 */
	if (!pixfmt->height) {
		if (user_info) {
			if (pixfmt->pixelformat == V4L2_PIX_FMT_NV12) {
				/* for NV12 format, sizeimage is y-plane size
				 * + CbCr plane which is half of y-plane
				 */
				pixfmt->height = pixfmt->sizeimage /
						(pixfmt->bytesperline +
						(pixfmt->bytesperline >> 1));
			} else
				pixfmt->height = pixfmt->sizeimage/
						pixfmt->bytesperline;
		}
	}

	if (pixfmt->height > max_height) {
		if (check && !user_info) {
			dev_err(davinci_display_dev,
				"height is more than maximum,"
				"input height = %d, max_height = %d\n",
				pixfmt->height, max_height);
			return -EINVAL;
		}
		pixfmt->height = max_height;
	}

	if (pixfmt->height < min_height) {
		if (check && !user_info) {
			dev_err(davinci_display_dev,
				"width is less than minimum,"
				"input height = %d, min_height = %d\n",
				pixfmt->height, min_height);
			return -EINVAL;
		}
		pixfmt->height = min_width;
	}

	/* if user has not provided bytesperline calculate it based on width */
	if (!user_info)
		pixfmt->bytesperline = (((pixfmt->width * bpp) + 31) & ~31);

	if (pixfmt->pixelformat == V4L2_PIX_FMT_NV12)
		min_sizeimage = pixfmt->bytesperline * pixfmt->height +
				(pixfmt->bytesperline * pixfmt->height >> 1);
	else
		min_sizeimage = pixfmt->bytesperline * pixfmt->height;

	if (pixfmt->sizeimage < min_sizeimage) {
		if (check && user_info) {
			dev_err(davinci_display_dev, "sizeimage is less, %d\n",
				min_sizeimage);
			return -EINVAL;
		}
		pixfmt->sizeimage = min_sizeimage;
	}
	return 0;
}

/*
 * davinci_doioctl()
 * This function will provide different V4L2 commands.This function can be
 * used to configure driver or get status of driver as per command passed
 * by application
 */
static int davinci_doioctl(struct inode *inode, struct file *file,
			   unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct davinci_fh *fh = file->private_data;
	struct display_obj *layer = fh->layer;
	unsigned int index = 0;
	unsigned long addr, flags;
	dev_dbg(davinci_display_dev, "<davinci_doioctl>\n");

	/* Check for the priority */
	switch (cmd) {
	case VIDIOC_S_FMT:
		ret = v4l2_prio_check(&layer->prio, &fh->prio);
		if (0 != ret)
			return ret;
		break;
	}

	/* Check for null value of parameter */
	if (ISNULL((void *)arg)) {
		dev_err(davinci_display_dev, "Null pointer\n");
		return -EINVAL;
	}
	/* Switch on the command value */
	switch (cmd) {
		/* If the case is for querying capabilities */
	case VIDIOC_QUERYCAP:
		{
			struct v4l2_capability *cap =
			    (struct v4l2_capability *)arg;
			dev_dbg(davinci_display_dev,
				"VIDIOC_QUERYCAP, layer id = %d\n",
				layer->device_id);
			memset(cap, 0, sizeof(*cap));
			*cap = davinci_display_videocap;
			break;
		}

	case VIDIOC_S_USB_FULL_SP:
		{
			struct v4l2_usb_full_sp *pArg =
			    (struct v4l2_usb_full_sp *)arg;
			//printk(KERN_INFO "davinci_display.c: ioctl called: before %d\n", set_usb_full_sp);
			set_usb_full_sp = pArg->set_usb_full_sp;
			set_multi_stream_vpbe = pArg->set_multi_stream_vpbe;
			if (set_usb_full_sp) {
				uHOLDMODE_TRANSFER_MAX = USB_FS_HOLDMODE_TRANSFER_MAX;
				uHOLDMODE_LINES_MAX = USB_FS_HOLDMODE_LINES_MAX;
			} else if (set_multi_stream_vpbe) {
				uHOLDMODE_TRANSFER_MAX = MULTI_STREAM_HOLDMODE_TRANSFER_MAX;
				uHOLDMODE_LINES_MAX = MULTI_STREAM_HOLDMODE_LINES_MAX;
			} else {
				uHOLDMODE_TRANSFER_MAX = HOLDMODE_TRANSFER_MAX;
				uHOLDMODE_LINES_MAX = HOLDMODE_LINES_MAX;
			}
			//printk(KERN_INFO "davinci_display.c: ioctl called: after %d\n", set_usb_full_sp);
			break;
		}

	case VIDIOC_CROPCAP:
		{
			struct v4l2_cropcap *cropcap =
			    (struct v4l2_cropcap *)arg;
			dev_dbg(davinci_display_dev,
				"\nStart of VIDIOC_CROPCAP ioctl");
			if (davinci_enc_get_mode(0, &davinci_dm.mode_info)) {
				dev_err(davinci_display_dev,
					"Error in getting current display mode"
					" from enc mngr\n");
				ret = -EINVAL;
				break;
			}
			ret = mutex_lock_interruptible(&davinci_dm.lock);
			if (ret)
				break;
			cropcap->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			if (!strcmp
			    (davinci_dm.mode_info.name, VID_ENC_STD_NTSC)) {
				cropcap->bounds = cropcap->defrect =
				    ntsc_bounds;
				cropcap->pixelaspect = ntsc_aspect;
			} else
			    if (!strcmp
				(davinci_dm.mode_info.name, VID_ENC_STD_PAL)) {
				cropcap->bounds = cropcap->defrect = pal_bounds;
				cropcap->pixelaspect = pal_aspect;
			} else
			    if (!strcmp
				(davinci_dm.mode_info.name,
				 VID_ENC_STD_640x480)) {
				cropcap->bounds = cropcap->defrect = vga_bounds;
				cropcap->pixelaspect = sp_aspect;
			} else
			    if (!strcmp
				(davinci_dm.mode_info.name,
				 VID_ENC_STD_640x400)) {
				cropcap->bounds = cropcap->defrect = vga_bounds;
				cropcap->bounds.height =
				    cropcap->defrect.height = 400;
				cropcap->pixelaspect = sp_aspect;
			} else
			    if (!strcmp
				(davinci_dm.mode_info.name,
				 VID_ENC_STD_640x350)) {
				cropcap->bounds = cropcap->defrect = vga_bounds;
				cropcap->bounds.height =
				    cropcap->defrect.height = 350;
				cropcap->pixelaspect = sp_aspect;
			} else
			    if (!strcmp
				(davinci_dm.mode_info.name,
				 VID_ENC_STD_720P_60)) {
				cropcap->bounds = cropcap->defrect =
				    hd_720p_bounds;
				cropcap->pixelaspect = sp_aspect;
			} else
			    if (!strcmp
				(davinci_dm.mode_info.name,
				 VID_ENC_STD_1080I_30)) {
				cropcap->bounds = cropcap->defrect =
				    hd_1080i_bounds;
				cropcap->pixelaspect = sp_aspect;
			} else {
				dev_err(davinci_display_dev,
					"Unknown encoder display mode\n");
				ret = -EINVAL;
			}
			mutex_unlock(&davinci_dm.lock);
			dev_dbg(davinci_display_dev,
				"\nEnd of VIDIOC_CROPCAP ioctl");
			break;
		}

	case VIDIOC_G_CROP:
		{
			/* TBD to get the x,y and height/width params */
			struct v4l2_crop *crop = (struct v4l2_crop *)arg;
			dev_dbg(davinci_display_dev,
				"VIDIOC_G_CROP, layer id = %d\n",
				layer->device_id);

			if (crop->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
				struct v4l2_rect *rect = &crop->c;
				ret = mutex_lock_interruptible(
							&davinci_dm.lock);
				if (ret)
					break;
				davinci_disp_get_layer_config(layer->layer_info.
							      id,
							      &layer->
							      layer_info.
							      config);
				rect->top = layer->layer_info.config.ypos;
				rect->left = layer->layer_info.config.xpos;
				rect->width = layer->layer_info.config.xsize;
				rect->height = layer->layer_info.config.ysize;
				mutex_unlock(&davinci_dm.lock);
			} else {
				dev_err(davinci_display_dev,
					"Invalid buf type \n");
				ret = -EINVAL;
			}
			break;
		}
	case VIDIOC_S_CROP:
		{
			/* TBD to get the x,y and height/width params */
			struct v4l2_crop *crop = (struct v4l2_crop *)arg;
			dev_dbg(davinci_display_dev,
				"VIDIOC_S_CROP, layer id = %d\n",
				layer->device_id);

			if (crop->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
				struct v4l2_rect *rect = &crop->c;

				if (rect->top < 0 || rect->left < 0) {
					dev_err(davinci_display_dev,
						"Error in S_CROP params"
						" Negative values for"
						" top/left" );
					ret = -EINVAL;
					break;
				}

				if (davinci_disp_check_window_params(rect)) {
					dev_err(davinci_display_dev,
						"Error in S_CROP params\n");
					ret = -EINVAL;
					break;
				}
				ret = mutex_lock_interruptible(
						&davinci_dm.lock);
				if (ret)
					break;
				davinci_disp_get_layer_config(layer->layer_info.
							      id,
							      &layer->
							      layer_info.
							      config);

				davinci_disp_calculate_scale_factor(layer,
								    rect->width,
								    rect->
								    height);

				davinci_disp_adj_position(layer, rect->top,
							  rect->left);

				if (davinci_disp_set_layer_config
				    (layer->layer_info.id,
				     &layer->layer_info.config)) {
					dev_err(davinci_display_dev,
						"Error in S_CROP params\n");
					mutex_unlock(&davinci_dm.lock);
					ret = -EINVAL;
					break;
				}
				/* apply zooming and h or v expansion */
				davinci_disp_set_zoom
				    (layer->layer_info.id,
				     layer->layer_info.h_zoom,
				     layer->layer_info.v_zoom);

				davinci_disp_set_vid_expansion
				    (layer->layer_info.h_exp,
				     layer->layer_info.v_exp);

				if ((layer->layer_info.h_zoom != ZOOM_X1) ||
				    (layer->layer_info.v_zoom != ZOOM_X1) ||
				    (layer->layer_info.h_exp != H_EXP_OFF) ||
				    (layer->layer_info.v_exp != V_EXP_OFF))
					/* Enable expansion filter */
					davinci_disp_set_interpolation_filter
					    (1);
				else
					davinci_disp_set_interpolation_filter
					    (0);
				mutex_unlock(&davinci_dm.lock);
			} else {
				dev_err(davinci_display_dev,
					"Invalid buf type \n");
				ret = -EINVAL;
			}
			break;
		}
		/* If the case is for enumerating formats */
	case VIDIOC_ENUM_FMT:
		{
			struct v4l2_fmtdesc *fmt = (struct v4l2_fmtdesc *)arg;
			dev_dbg(davinci_display_dev,
				"VIDIOC_ENUM_FMT, layer id = %d\n",
				layer->device_id);
			if (fmt->index > 0) {
				dev_err(davinci_display_dev,
					"Invalid format index\n");
				ret = -EINVAL;
				break;
			}
			/* Fill in the information about format */

			index = fmt->index;
			memset(fmt, 0, sizeof(*fmt));
			fmt->index = index;
			fmt->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
			if (index == 0) {
				strcpy(fmt->description, "YUV 4:2:2 - UYVY");
				fmt->pixelformat = V4L2_PIX_FMT_UYVY;
			} else if (index == 1) {
				strcpy(fmt->description, "Y/CbCr 4:2:0");
				fmt->pixelformat = V4L2_PIX_FMT_NV12;
			}
			break;
		}

		/* If the case is for getting formats */
	case VIDIOC_G_FMT:
		{
			struct v4l2_format *fmt = (struct v4l2_format *)arg;
			dev_dbg(davinci_display_dev,
				"VIDIOC_G_FMT, layer id = %d\n",
				layer->device_id);

			/* If buffer type is video output */
			if (V4L2_BUF_TYPE_VIDEO_OUTPUT == fmt->type) {
				struct v4l2_pix_format *pixfmt = &fmt->fmt.pix;
				/* Fill in the information about
				 * format
				 */
				ret = mutex_lock_interruptible(
							&davinci_dm.lock);
				if (!ret) {
					*pixfmt = layer->pix_fmt;
					mutex_unlock(&davinci_dm.lock);
				}
			} else {
				dev_err(davinci_display_dev, "invalid type\n");
				ret = -EINVAL;
			}
			break;
		}

		/* If the case is for setting formats */
	case VIDIOC_S_FMT:
		{
			struct v4l2_format *fmt = (struct v4l2_format *)arg;
			dev_dbg(davinci_display_dev,
				"VIDIOC_S_FMT, layer id = %d\n",
				layer->device_id);

			/* If streaming is started, return error */
			if (layer->started) {
				dev_err(davinci_display_dev,
					"Streaming is started\n");
				ret = -EBUSY;
				break;
			}
			if (V4L2_BUF_TYPE_VIDEO_OUTPUT == fmt->type) {
				struct v4l2_pix_format *pixfmt = &fmt->fmt.pix;
				/* Check for valid field format */
				ret = vpbe_try_format(pixfmt, 1);
				if (ret)
					return ret;

				/* YUV420 is requested, check availability of
				 * the other video window
				 */
				ret = mutex_lock_interruptible(
						&davinci_dm.lock);
				if (ret)
					break;

				layer->pix_fmt = *pixfmt;
				if (pixfmt->pixelformat == V4L2_PIX_FMT_NV12 &&
				    cpu_is_davinci_dm365()) {
					struct display_obj *otherlayer =
					_davinci_disp_get_other_win(layer);

					/* if other layer is available, only
					 * claim it, do not configure it
					 */
					if (davinci_disp_request_layer(
								otherlayer->
								layer_info.
								id)) {
						/* Couldn't get layer */
						dev_err(davinci_display_dev,
							"Display Manager"
							" failed to allocate"
							" the other layer:"
							"vidwin %d\n",
							otherlayer->
							layer_info.id);
						mutex_unlock(&davinci_dm.lock);
						ret = -EBUSY;
						break;
					}
				}
				/* store the pixel format in the layer
				 * object
				 */
				davinci_disp_get_layer_config(layer->layer_info.
							      id,
							      &layer->
							      layer_info.
							      config);

				layer->layer_info.config.xsize =
					pixfmt->width;
				layer->layer_info.config.ysize =
					pixfmt->height;
				layer->layer_info.config.line_length =
					pixfmt->bytesperline;
				layer->layer_info.config.ypos = 0;
				layer->layer_info.config.xpos = 0;
				layer->layer_info.config.interlaced =
					davinci_dm.mode_info.interlaced;

				/* change of the default pixel format for
				 * both vid windows
				 */
				if (V4L2_PIX_FMT_NV12 == pixfmt->pixelformat) {
					struct display_obj *otherlayer;
					layer->layer_info.config.pixfmt =
						PIXFMT_NV12;
					otherlayer =
					_davinci_disp_get_other_win(layer);
					otherlayer->layer_info.config.pixfmt =
						PIXFMT_NV12;
				}

				if (davinci_disp_set_layer_config(layer->
							layer_info.id,
							&layer->
							layer_info.config)) {
					dev_err(davinci_display_dev,
					    "Error in S_FMT params:\n");
					mutex_unlock(&davinci_dm.lock);
					ret = -EINVAL;
					break;
				}

				/* readback and fill the local copy of current
				 * pix format
				 */
				davinci_disp_get_layer_config(layer->layer_info.
							id,
							&layer->
							layer_info.
							config);


				/* verify if readback values are as expected */
				if (layer->pix_fmt.width !=
					layer->layer_info.config.xsize ||
				    layer->pix_fmt.height !=
					layer->layer_info.config.ysize ||
				    layer->pix_fmt.bytesperline !=
					layer->layer_info.config.line_length ||
				    (layer->layer_info.config.interlaced &&
				     layer->pix_fmt.field !=
					 V4L2_FIELD_INTERLACED) ||
				    (!layer->layer_info.config.interlaced &&
				     layer->pix_fmt.field != V4L2_FIELD_NONE)) {
					dev_err(davinci_display_dev,
					    "mismatch with layer config"
					    " params:\n");
					dev_err(davinci_display_dev,
					    "layer->layer_info.config.xsize ="
					    "%d layer->pix_fmt.width = %d\n",
					    layer->layer_info.config.xsize,
					    layer->pix_fmt.width);
					dev_err(davinci_display_dev,
					    "layer->layer_info.config.ysize ="
					    "%d layer->pix_fmt.height = %d\n",
					    layer->layer_info.config.ysize,
					    layer->pix_fmt.height);
					dev_err(davinci_display_dev,
					    "layer->layer_info.config."
					    "line_length= %d layer->pix_fmt"
					    ".bytesperline = %d\n",
					    layer->layer_info.config.
						line_length,
					    layer->pix_fmt.bytesperline);
					dev_err(davinci_display_dev,
					    "layer->layer_info.config."
					    "interlaced =%d layer->pix_fmt."
					    "field = %d\n",
					    layer->layer_info.config.interlaced,
					    layer->pix_fmt.field);
					mutex_unlock(&davinci_dm.lock);
					ret = -EFAULT;
					break;
				}

				dev_notice(davinci_display_dev,
					"Before finishing with S_FMT:\n"
					"layer.pix_fmt.bytesperline = %d,\n"
					" layer.pix_fmt.width = %d, \n"
					" layer.pix_fmt.height = %d, \n"
					" layer.pix_fmt.sizeimage =%d\n",
					layer->pix_fmt.bytesperline,
					layer->pix_fmt.width,
					layer->pix_fmt.height,
					layer->pix_fmt.sizeimage);

				dev_notice(davinci_display_dev,
					"pixfmt->width = %d,\n"
					" layer->layer_info.config.line_length"
					"= %d\n",
					pixfmt->width,
					layer->layer_info.config.line_length);
				mutex_unlock(&davinci_dm.lock);
			} else {
				dev_err(davinci_display_dev, "invalid type\n");
				ret = -EINVAL;
			}
			break;
		}
		/* If the case is for trying formats */
	case VIDIOC_TRY_FMT:
		{
			struct v4l2_format *fmt;
			dev_dbg(davinci_display_dev, "VIDIOC_TRY_FMT\n");
			fmt = (struct v4l2_format *)arg;

			if (V4L2_BUF_TYPE_VIDEO_OUTPUT == fmt->type) {
				struct v4l2_pix_format *pixfmt = &fmt->fmt.pix;
				/* Check for valid field format */
				ret = vpbe_try_format(pixfmt, 0);
			} else {
				dev_err(davinci_display_dev, "invalid type\n");
				ret = -EINVAL;
			}
			break;
		}

		/* If the case is for requesting buffer allocation */
	case VIDIOC_REQBUFS:
		{
			struct v4l2_requestbuffers *reqbuf;
			reqbuf = (struct v4l2_requestbuffers *)arg;
			dev_dbg(davinci_display_dev,
				"VIDIOC_REQBUFS, count= %d, type = %d,"
				"memory = %d\n",
				reqbuf->count, reqbuf->type, reqbuf->memory);

			/* If io users of the layer is not zero,
			 * return error
			 */
			if (0 != layer->io_usrs) {
				dev_err(davinci_display_dev, "not IO user\n");
				ret = -EBUSY;
				break;
			}
			ret = mutex_lock_interruptible(&davinci_dm.lock);
			if (ret)
				break;
			/* Initialize videobuf queue as per the
			 * buffer type
			 */
			videobuf_queue_init(&layer->buffer_queue,
					    &video_qops, NULL,
					    &layer->irqlock,
					    V4L2_BUF_TYPE_VIDEO_OUTPUT,
					    layer->pix_fmt.field,
					    sizeof(struct videobuf_buffer), fh);
			/* Set buffer to Linear buffer */
			videobuf_set_buftype(&layer->buffer_queue,
					     VIDEOBUF_BUF_LINEAR);
			/* Set io allowed member of file handle to
			 * TRUE
			 */
			fh->io_allowed = 1;
			/* Increment io usrs member of layer object
			 * to 1
			 */
			layer->io_usrs = 1;
			/* Store type of memory requested in layer
			 * object
			 */
			layer->memory = reqbuf->memory;
			/* Initialize buffer queue */
			INIT_LIST_HEAD(&layer->dma_queue);
			/* Allocate buffers */
			ret = videobuf_reqbufs(&layer->buffer_queue, reqbuf);
			mutex_unlock(&davinci_dm.lock);
			break;
		}
		/* If the case is for en-queing buffer in the buffer
		 * queue
		 */
	case VIDIOC_QBUF:
		{
			struct v4l2_buffer tbuf;
			struct videobuf_buffer *buf1;
			dev_dbg(davinci_display_dev,
				"VIDIOC_QBUF, layer id = %d\n",
				layer->device_id);

			/* If this file handle is not allowed to do IO,
			 * return error
			 */
			if (!fh->io_allowed) {
				dev_err(davinci_display_dev, "No io_allowed\n");
				ret = -EACCES;
				break;
			}
			if (!(list_empty(&layer->dma_queue)) ||
			    (layer->curFrm != layer->nextFrm) ||
			    !(layer->started) ||
			    (layer->started && (0 == layer->field_id))) {

				ret = videobuf_qbuf(&layer->buffer_queue,
						    (struct v4l2_buffer *)arg);
				break;
			}
			/* bufferqueue is empty store buffer address
			 * in VPBE registers
			 */
			mutex_lock(&layer->buffer_queue.lock);
			tbuf = *(struct v4l2_buffer *)arg;
			buf1 = layer->buffer_queue.bufs[tbuf.index];
			if (buf1->memory != tbuf.memory) {
				mutex_unlock(&layer->buffer_queue.lock);
				dev_err(davinci_display_dev,
					"invalid buffer type\n");
				ret = -EINVAL;
				break;
			}
			if ((buf1->state == STATE_QUEUED) ||
			    (buf1->state == STATE_ACTIVE)) {
				mutex_unlock(&layer->buffer_queue.lock);
				dev_err(davinci_display_dev, "invalid state\n");
				ret = -EINVAL;
				break;
			}

			switch (buf1->memory) {
			case V4L2_MEMORY_MMAP:
				if (buf1->baddr == 0) {
					mutex_unlock(&layer->buffer_queue.lock);
					dev_err(davinci_display_dev,
						"No Buffer address\n");
					return -EINVAL;
				}
				break;
			case V4L2_MEMORY_USERPTR:
				/*
				 * if user requested size is less than the S_FMT
				 * calculated img size, return error
				 */
				if (tbuf.length < layer->pix_fmt.sizeimage) {
					mutex_unlock(&layer->buffer_queue.lock);
					dev_err(davinci_display_dev,
					"Insufficient USEPTR buffer size\n");
					return -EINVAL;
				}
				if ((STATE_NEEDS_INIT != buf1->state)
				    && (buf1->baddr != tbuf.m.userptr))
					davinci_buffer_release(&layer->
							       buffer_queue,
							       buf1);
				buf1->baddr = tbuf.m.userptr;
				break;
			default:
				mutex_unlock(&layer->buffer_queue.lock);
				dev_err(davinci_display_dev,
					"Unknow Buffer type \n");
				return -EINVAL;
			}
			local_irq_save(flags);
			ret =
			    davinci_buffer_prepare(&layer->buffer_queue,
						   buf1,
						   layer->buffer_queue.field);
			buf1->state = STATE_ACTIVE;
			addr = buf1->boff;
			layer->nextFrm = buf1;

			davinci_disp_start_layer(layer->layer_info.id,
						 addr,
						 davinci_dm.cbcr_ofst);
			local_irq_restore(flags);
			list_add_tail(&buf1->stream,
				      &(layer->buffer_queue.stream));
			mutex_unlock(&layer->buffer_queue.lock);
			break;
		}

		/* If the case is for de-queing buffer from the
		 * buffer queue
		 */
	case VIDIOC_DQBUF:
		{
			dev_dbg(davinci_display_dev,
				"VIDIOC_DQBUF, layer id = %d\n",
				layer->device_id);

			/* If this file handle is not allowed to do IO,
			 * return error
			 */
			if (!fh->io_allowed) {
				dev_err(davinci_display_dev, "No io_allowed\n");
				ret = -EACCES;
				break;
			}
			if (file->f_flags & O_NONBLOCK)
				/* Call videobuf_dqbuf for non
				 * blocking mode
				 */
				ret =
				    videobuf_dqbuf(&layer->buffer_queue,
						   (struct v4l2_buffer *)
						   arg, 1);
			else
				/* Call videobuf_dqbuf for
				 * blocking mode
				 */
				ret =
				    videobuf_dqbuf(&layer->buffer_queue,
						   (struct v4l2_buffer *)
						   arg, 0);
			break;
		}

		/* If the case is for querying information about
		 * buffer for memory mapping io
		 */
	case VIDIOC_QUERYBUF:
		{
			dev_dbg(davinci_display_dev,
				"VIDIOC_QUERYBUF, layer id = %d\n",
				layer->device_id);
			/* Call videobuf_querybuf to get information */
			ret = videobuf_querybuf(&layer->buffer_queue,
						(struct v4l2_buffer *)
						arg);
			break;
		}

		/* If the case is starting streaming */
	case VIDIOC_STREAMON:
		{
			dev_dbg(davinci_display_dev,
				"VIDIOC_STREAMON, layer id = %d\n",
				layer->device_id);
			/* If file handle is not allowed IO,
			 * return error
			 */
			if (!fh->io_allowed) {
				dev_err(davinci_display_dev, "No io_allowed\n");
				ret = -EACCES;
				break;
			}
			/* If Streaming is already started,
			 * return error
			 */
			if (layer->started) {
				dev_err(davinci_display_dev,
					"layer is already streaming\n");
				ret = -EBUSY;
				break;
			}

			/*
			 * Call videobuf_streamon to start streaming
			 * in videobuf
			 */
			ret = videobuf_streamon(&layer->buffer_queue);
			if (ret) {
				dev_err(davinci_display_dev,
					"error in videobuf_streamon\n");
				break;
			}
			ret = mutex_lock_interruptible(&davinci_dm.lock);
			if (ret)
				break;
			/* If buffer queue is empty, return error */
			if (list_empty(&layer->dma_queue)) {
				dev_err(davinci_display_dev,
					"buffer queue is empty\n");
				ret = -EIO;
				mutex_unlock(&davinci_dm.lock);
				break;
			}
			/* Get the next frame from the buffer queue */
			layer->nextFrm = layer->curFrm =
			    list_entry(layer->dma_queue.next,
				       struct videobuf_buffer, queue);
			/* Remove buffer from the buffer queue */
			list_del(&layer->curFrm->queue);
			/* Mark state of the current frame to active */
			layer->curFrm->state = STATE_ACTIVE;
			/* Initialize field_id and started member */

			layer->field_id = 0;

			/* Set parameters in OSD and VENC */
			ret = davinci_set_video_display_params(layer);
			if (ret < 0) {
				mutex_unlock(&davinci_dm.lock);
				break;
			}
			/* if request format is yuv420 semiplanar, need to
			 * enable both video windows
			 */

			layer->started = 1;
			dev_dbg(davinci_display_dev,
				"Started streaming on layer id = %d,"
				" ret = %d\n",
				layer->device_id, ret);
			layer_first_int = 1;
			mutex_unlock(&davinci_dm.lock);
			break;
		}
		/* If the case is for stopping streaming */
	case VIDIOC_STREAMOFF:
		{
			dev_dbg(davinci_display_dev,
				"VIDIOC_STREAMOFF,layer id = %d\n",
				layer->device_id);
			/* If io is allowed for this file handle,
			 * return error
			 */
			if (!fh->io_allowed) {
				dev_err(davinci_display_dev, "No io_allowed\n");
				ret = -EACCES;
				break;
			}
			/* If streaming is not started, return error */
			if (!layer->started) {
				dev_err(davinci_display_dev,
					"streaming not started in layer"
					" id = %d\n",
					layer->device_id);
				ret = -EINVAL;
				break;
			}
			ret = mutex_lock_interruptible(&davinci_dm.lock);
			if (ret)
				break;
			davinci_disp_disable_layer(layer->layer_info.id);
			layer->started = 0;
			mutex_unlock(&davinci_dm.lock);
			ret = videobuf_streamoff(&layer->buffer_queue);
			break;
		}
	case VIDIOC_S_PRIORITY:
		{
			enum v4l2_priority *p = (enum v4l2_priority *)arg;
			ret = v4l2_prio_change(&layer->prio, &fh->prio, *p);
			break;
		}
	case VIDIOC_G_PRIORITY:
		{
			enum v4l2_priority *p = (enum v4l2_priority *)arg;
			*p = v4l2_prio_max(&layer->prio);
			break;
		}
	case VIDIOC_S_COFST:
		{
			ret = mutex_lock_interruptible(&davinci_dm.lock);
			if (!ret) {
				davinci_dm.cbcr_ofst = *((unsigned long *) arg);
				mutex_unlock(&davinci_dm.lock);
			}
			break;
		}
	case VIDIOC_R_ADDR:
		{
			*(unsigned long *)arg = Addr;
			break;
		}
	case VIDIOC_W_ADDR:
		{
			 Addr = *(unsigned long *)arg;
			 davinci_disp_start_layer(layer->layer_info.id,
						 Addr,
						 davinci_dm.cbcr_ofst);
			 break;
		}	
	default:
		ret = -EINVAL;
	}

	dev_dbg(davinci_display_dev, "<davinci_doioctl>\n");
	return ret;
}

/*
 * davinci_ioctl()
 * Calls davinci_doioctl function
 */
static int davinci_ioctl(struct inode *inode, struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	int ret;
	char sbuf[128];
	void *mbuf = NULL;
	void *parg = NULL;

	dev_dbg(davinci_display_dev, "Start of davinci ioctl\n");

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
	ret = davinci_doioctl(inode, file, cmd, (unsigned long)parg);
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
	dev_dbg(davinci_display_dev, "</davinci_ioctl>\n");
	return ret;
}

/*
 * davinci_mmap()
 * It is used to map kernel space buffers into user spaces
 */
static int davinci_mmap(struct file *filep, struct vm_area_struct *vma)
{
	/* Get the layer object and file handle object */
	struct davinci_fh *fh = filep->private_data;
	struct display_obj *layer = fh->layer;
	int err = 0;
	dev_dbg(davinci_display_dev, "<davinci_mmap>\n");

	err = videobuf_mmap_mapper(&layer->buffer_queue, vma);
	dev_dbg(davinci_display_dev, "</davinci_mmap>\n");
	return err;
}

/* davinci_poll(): It is used for select/poll system call
 */
static unsigned int davinci_poll(struct file *filep, poll_table *wait)
{
	int err = 0;
	struct davinci_fh *fh = filep->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "<davinci_poll>");

	if (layer->started)
		err = videobuf_poll_stream(filep, &layer->buffer_queue, wait);

	if (err & POLLIN) {
		err &= (~POLLIN);
		err |= POLLOUT;
	}
	if (err & POLLRDNORM) {
		err &= (~POLLRDNORM);
		err |= POLLWRNORM;
	}

	dev_dbg(davinci_display_dev, "</davinci_poll>");
	return err;
}

static int davinci_config_layer(enum davinci_display_device_id id)
{
	int err = 0;
	struct davinci_layer_config *layer_config;
	struct vid_enc_mode_info *mode_info;
	struct display_obj *layer = davinci_dm.dev[id];

	/* First claim the layer for this device */
	if (davinci_disp_request_layer(layer->layer_info.id)) {
		/* Couldn't get layer */
		dev_err(davinci_display_dev,
			"Display Manager failed to allocate layer\n");
		return -EBUSY;
	}

	/* get the current video display mode from encoder manager */
	mode_info = &davinci_dm.mode_info;
	if (davinci_enc_get_mode(0, mode_info)) {
		dev_err(davinci_display_dev,
			"Error in getting current display mode from enc mngr\n");
		return -1;
	}

	layer_config = &layer->layer_info.config;
	/* Set the default image and crop values */
	layer_config->pixfmt = PIXFMT_YCbCrI;
	layer->pix_fmt.pixelformat = V4L2_PIX_FMT_UYVY;
	layer->pix_fmt.bytesperline = layer_config->line_length =
	    mode_info->xres * 2;

	layer->pix_fmt.width = layer_config->xsize = mode_info->xres;
	layer->pix_fmt.height = layer_config->ysize = mode_info->yres;
	layer->pix_fmt.sizeimage =
	    layer->pix_fmt.bytesperline * layer->pix_fmt.height;
	layer_config->xpos = 0;
	layer_config->ypos = 0;
	layer_config->interlaced = mode_info->interlaced;

	/* turn off ping-pong buffer and field inversion to fix
	 * the image shaking problem in 1080I mode
	 */
	if (id == DAVINCI_DISPLAY_DEVICE_0 &&
	    strcmp(mode_info->name, VID_ENC_STD_1080I_30) == 0 &&
	    (cpu_is_davinci_dm644x_pg1x() || cpu_is_davinci_dm357()))
		davinci_disp_set_field_inversion(0);

	if (layer->layer_info.config.interlaced)
		layer->pix_fmt.field = V4L2_FIELD_INTERLACED;
	else
		layer->pix_fmt.field = V4L2_FIELD_NONE;
	davinci_disp_set_layer_config(layer->layer_info.id, layer_config);
	return err;
}

/*
 * davinci_open()
 * It creates object of file handle structure and stores it in private_data
 * member of filepointer
 */
static int davinci_open(struct inode *inode, struct file *filep)
{
	int minor = iminor(inode);
	int found = -1;
	int i = 0;
	struct display_obj *layer;
	struct davinci_fh *fh = NULL;

	/* Check for valid minor number */
	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		/* Get the pointer to the layer object */
		layer = davinci_dm.dev[i];
		if (minor == layer->video_dev->minor) {
			found = i;
			break;
		}
	}

	/* If not found, return error no device */
	if (0 > found) {
		dev_err(davinci_display_dev, "device not found\n");
		return -ENODEV;
	}

	/* Allocate memory for the file handle object */
	fh = kmalloc(sizeof(struct davinci_fh), GFP_KERNEL);
	if (ISNULL(fh)) {
		dev_err(davinci_display_dev,
			"unable to allocate memory for file handle object\n");
		return -ENOMEM;
	}
	dev_dbg(davinci_display_dev, "<davinci open> plane = %d\n",
		layer->device_id);
	/* store pointer to fh in private_data member of filep */
	filep->private_data = fh;
	fh->layer = layer;

	if (!layer->usrs) {
		/* Configure the default values for the layer */
		if (davinci_config_layer(layer->device_id)) {
			dev_err(davinci_display_dev,
				"Unable to configure video layer for id = %d\n",
				layer->device_id);
			return -EINVAL;
		}
	}

	/* Increment layer usrs counter */
	layer->usrs++;
	/* Set io_allowed member to false */
	fh->io_allowed = 0;
	/* Initialize priority of this instance to default priority */
	fh->prio = V4L2_PRIORITY_UNSET;
	v4l2_prio_open(&layer->prio, &fh->prio);
	dev_dbg(davinci_display_dev, "</davinci_open>\n");
	return 0;
}

/*
 * davinci_release()
 * This function deletes buffer queue, frees the buffers and the davinci
 * display file * handle
 */
static int davinci_release(struct inode *inode, struct file *filep)
{
	/* Get the layer object and file handle object */
	struct davinci_fh *fh = filep->private_data;
	struct display_obj *layer = fh->layer;

	dev_dbg(davinci_display_dev, "<davinci_release>\n");
	/* If this is doing IO and other layer are not closed */
	if ((layer->usrs != 1) && fh->io_allowed) {
		dev_err(davinci_display_dev, "Close other instances\n");
		return -EAGAIN;
	}
	/* Get the lock on layer object */
	mutex_lock_interruptible(&davinci_dm.lock);
	/* if this instance is doing IO */
	if (fh->io_allowed) {
		/* Reset io_usrs member of layer object */
		layer->io_usrs = 0;
		davinci_disp_disable_layer(layer->layer_info.id);
		layer->started = 0;
		/* Free buffers allocated */
		videobuf_queue_cancel(&layer->buffer_queue);
		videobuf_mmap_free(&layer->buffer_queue);
	}

	/* Decrement layer usrs counter */
	layer->usrs--;
	/* If this file handle has initialize encoder device, reset it */
	if (!layer->usrs) {
		if (layer->layer_info.config.pixfmt == PIXFMT_NV12) {
			struct display_obj *otherlayer;
			otherlayer = _davinci_disp_get_other_win(layer);
			davinci_disp_disable_layer(otherlayer->layer_info.id);
			davinci_disp_release_layer(otherlayer->layer_info.id);
		}
		davinci_disp_disable_layer(layer->layer_info.id);
		davinci_disp_release_layer(layer->layer_info.id);
	}

	/* Close the priority */
	v4l2_prio_close(&layer->prio, &fh->prio);
	filep->private_data = NULL;

	/* Free memory allocated to file handle object */
	if (!ISNULL(fh))
		kfree(fh);
	/* unlock mutex on layer object */
	mutex_unlock(&davinci_dm.lock);

	davinci_dm.cbcr_ofst = 0;

	dev_dbg(davinci_display_dev, "</davinci_release>\n");
	return 0;
}

static void davinci_platform_release(struct device
				     *device)
{
	/* This is called when the reference count goes to zero */
}

/*
 * davinci_write 
 * This function implements the hold mode feature of the DM365 and uses it to send the
 * contents of userBuffer (size bytes).
 */
static ssize_t davinci_write (struct file * file, const char __user * userBuffer, size_t size, loff_t * lloffset)
{
    struct davinci_fh *fh = file->private_data;
    struct display_obj *layer = fh->layer;
    unsigned int reg;
    int iBytesPerLine = 0;
    int iWholeLines = 0;
    int iSent = 0;
    int ret;                    // The return value of a function
    u32 physBuffer;             // The physical address of the buffer
    unsigned long flags=0;      // Used to store the irq state
    bool fUserBuffer = 1;       // Use the user space buffer unless it is not 32 byte aligned

    /* Get the lock on the device mutex */
    ret = mutex_lock_interruptible(&davinci_dm.lock);
    if (ret) return ret;

    /* Get the physical address of the buffer */
    physBuffer = davinci_uservirt_to_phys((u32) userBuffer);
    /* Check if the user buffer is on a 32 byte boundary */
    if ((physBuffer & 0x1f) != 0) {
        /* The buffer is not on a 32 byte boundary so we have to use the kernel buffer */
        physBuffer = virt_to_phys(davinci_holdModeWriteBuffer);
        fUserBuffer = 0;
#if 0
        printk(KERN_INFO "%s: using kernel buffer\n", __FUNCTION__);
    } else {
        printk(KERN_INFO "%s: using user buffer\n", __FUNCTION__);
#endif
    }
    /* The OSD engine wants the offset from SDRAM start. */
    physBuffer -= DAVINCI_DDR_BASE; 

    #if 0
        printk(KERN_INFO "%s: filep=%p userBuffer=%p size=%d loff_t=%p\n",
               __FUNCTION__,
               filep,
               userBuffer,
               size,
               lloffset);
    #endif
    #if 0
        printk(KERN_INFO "%s: &davinci_holdModeWriteBuffer=%p physBuffer=0x%08X\n",
               __FUNCTION__,
               davinci_holdModeWriteBuffer,
               physBuffer);
    #endif
    #if 0
    {
        int i;
        printk(KERN_INFO "%s: userBuffer = ",
               __FUNCTION__);
        for (i = 0 ; i < size; i++) {
            printk("%02X ",
                   userBuffer[i]);
        }
        printk("\n");
    }
    #endif

    /* Setup the base position */
    reg = 20; // Same as HSTART / 2
    davinci_writel(reg, DM365_OSD_REG_BASE + OSD_BASEPX);
    reg = 1; // Same as VSTART
    davinci_writel(reg, DM365_OSD_REG_BASE + OSD_BASEPY);

    /* Setup the position of the video window 0 */
    reg = 0;
    davinci_writel(reg, DM365_OSD_REG_BASE + OSD_VIDWIN0XP);
    reg = 0;
    davinci_writel(reg, DM365_OSD_REG_BASE + OSD_VIDWIN0YP);
    reg = uHOLDMODE_TRANSFER_MAX >> 1;
    davinci_writel(reg, DM365_OSD_REG_BASE + OSD_VIDWIN0XL);
    reg = 4;
    davinci_writel(reg, DM365_OSD_REG_BASE + OSD_VIDWIN0YL);

    /* Enable the Video Window 0 */
    reg = 1; // Enable video window 0 in frame mode
    davinci_writel(reg, DM365_OSD_REG_BASE + OSD_VIDWINMD);

    /* Now setup for 1 line */
    // This was already taken care of by the set mode
    //davinci_writel(1, DM365_VENC_REG_BASE + VENC_VVALID); // Set the VVALID
    //davinci_writel(1, DM365_VENC_REG_BASE + VENC_VSPLS); // Set the VSPLS
    //davinci_writel(1, DM365_VENC_REG_BASE + VENC_VSTART); // Set the VSTART
    //davinci_writel(5, DM365_VENC_REG_BASE + VENC_VINT); // Set the VINT

    /* Now setup the basic line structure */
    // This was already taken care of by the set mode
    //davinci_writel(10, DM365_VENC_REG_BASE + VENC_HSPLS); // Set the HSPLS
    //davinci_writel(40, DM365_VENC_REG_BASE + VENC_HSTART); // Set the HSTART
    //davinci_writel(HOLDMODE_TRANSFER_MAX + 41, DM365_VENC_REG_BASE + VENC_HINT); // Set the HINT  The | 1 makes sure that HINT + 1 is even.

    /* Set the VSYNC output */
#if 1
    //gpio_direction_output(83,1); // GPIO83 aka VSYNC
    davinci_writel(0x80000, DAVINCI_GPIO_BASE + 0x68); // SET_DATA45 <= GPIO83 aka VSYNC
#endif 

    /* 
     * Transfer multiple frames of data upto 
     * HOLDMODE_LINES_MAX of HOLDMODE_TRANSFER_MAX bytes 
     * followed by one frame of what is left 
     */
    /*printk(KERN_INFO "%s: size = %d\n", __FUNCTION__, size);*/
    while (size) {
        /* Setup the buffer for the video window 0 */
        reg = ((physBuffer & 0xF0000000) >> (28 - OSD_WINOFST_AH_SHIFT)) | 0x1000;
        reg |= (uHOLDMODE_TRANSFER_MAX + 31) / 32; // Set the number of 32 byte transfers in a line.
        davinci_writel(reg, DM365_OSD_REG_BASE + OSD_VIDWIN0OFST);

        reg = ((physBuffer & 0xFE00000) >> (21 - OSD_VIDWINADH_V0AH_SHIFT));
        davinci_writel(reg, DM365_OSD_REG_BASE + OSD_VIDWINADH);

        reg = ((physBuffer & 0x1FFFE0) >> 5);
        davinci_writel(reg, DM365_OSD_REG_BASE + OSD_VIDWIN0ADL);

        /* See how many whole lines we can send */
        iWholeLines = size / uHOLDMODE_TRANSFER_MAX;
        if (iWholeLines > uHOLDMODE_LINES_MAX) iWholeLines = uHOLDMODE_LINES_MAX;
        if (iWholeLines == 0) iWholeLines = 1; // We can always do at least one line of iBytesPerLine = size

        /* Calculate the line size */
        iBytesPerLine = uHOLDMODE_TRANSFER_MAX;
        if (size < iBytesPerLine) iBytesPerLine = size;

        /* If we are not using the kernel buffer we need to copy from user to kernel space */
        if (!fUserBuffer) {
            copy_from_user(davinci_holdModeWriteBuffer, userBuffer, iBytesPerLine * iWholeLines);
        }

        #if 0
        {
            int i;
            printk(KERN_INFO "%s: Before davinci_writeBuffer = ",
                   __FUNCTION__);
            for (i = 0 ; i < iBytesPerLine; i++) {
                printk("%02X ",
                       davinci_holdModeWriteBuffer[i]);
            }
            printk("\n");
        }
        #endif

        /* Now setup the line size */
        davinci_writel(iBytesPerLine, DM365_VENC_REG_BASE + VENC_HVALID); // Set the HVALID
    
        /* Setup how many lines */
        davinci_writel(iWholeLines, DM365_VENC_REG_BASE + VENC_VVALID); // Set the VVALID

        /* Note that will be waiting for the interrupt */
        davinci_holdModeWaitVertical = 1;

        /* Make sure we are exclusively accessing this so we get predictable timing */
        spin_lock_irqsave(&layer->irqlock,flags);

        /* This is the critical section protected by the spin lock */
        {
              /* Read the LINECTL so we can toggle the hold */
              reg = davinci_readl(DM365_VENC_REG_BASE + VENC_LINECTL);
              
              /* Clear both HLDF and HLDL */
              reg &= ~0x30; 
              davinci_writel(reg, DM365_VENC_REG_BASE + VENC_LINECTL);
              
              /* 
               * We need to wait for the VPBE state machine to start
               * 
               * Can't wait for vertical interrupt here due to 
               * the spinlock and the variability of the latency in 
               * linux.  So instead we do a local delay loop
               */
              udelay(HOLDMODE_PERLINE_TIME);
              
              /* Set HLDF */
              reg |= 0x20; 
              davinci_writel(reg, DM365_VENC_REG_BASE + VENC_LINECTL);
        }

        /* exclusivity is no longer an issue so release the lock */
        spin_unlock_irqrestore(&layer->irqlock,flags);

        /*printk(KERN_INFO "size = %d, iWholeLines = %d, iBytesPerLine = %d\n", size, iWholeLines, iBytesPerLine);*/
        /* Decrement the size and increment the data pointers */
        size -= iBytesPerLine * iWholeLines;
        userBuffer += iBytesPerLine * iWholeLines;
        iSent += iBytesPerLine * iWholeLines;
        if (fUserBuffer) {
            /* in the user buffer case we need to increment the physBuffer as well */
            physBuffer += iBytesPerLine * iWholeLines;
        }

        /* Now we need to delay a little while */
        /*
        udelay(HOLDMODE_PERLINE_TIME * (iWholeLines + 1));
        udelay(HOLDMODE_PERLINE_TIME * (iWholeLines + 1));
        */
        udelay(HOLDMODE_PERLINE_TIME * (4 + 1));
        udelay(HOLDMODE_PERLINE_TIME * (4 + 1));
        //printk(KERN_INFO "%s: Frame Sent\n", __FUNCTION__);
        
        #if 0
        {
            int i;
            printk(KERN_INFO "%s: After davinci_writeBuffer = ",
                   __FUNCTION__);
            for (i = 0 ; i < iBytesPerLine; i++) {
                printk("%02X ",
                       davinci_holdModeWriteBuffer[i]);
            }
            printk("\n");
        }
        #endif

    }

    /* Clear the VSYNC output */
#if 1
    //gpio_direction_output(83,0); // GPIO83 aka VSYNC
    davinci_writel(0x80000, DAVINCI_GPIO_BASE + 0x6C); // CLR_DATA45 <= GPIO83 aka VSYNC
#endif

    /* unlock the device mutex */
    mutex_unlock(&davinci_dm.lock);

    return iSent;
}

static struct file_operations davinci_fops = {
	.owner = THIS_MODULE,
	.open = davinci_open,
	.release = davinci_release,
	.ioctl = davinci_ioctl,
	.mmap = davinci_mmap,
	.poll = davinci_poll,
    .write = davinci_write
};

static struct video_device davinci_video_template = {
	.name = "davinci",
	.type = VID_TYPE_CAPTURE,
	.hardware = 0,
	.fops = &davinci_fops,
	.minor = -1
};

/*
 * davinci_probe()
 * This function creates device entries by register itself to the V4L2 driver
 * and initializes fields of each layer objects
 */
static __init int davinci_probe(struct device *device)
{
	int i, j = 0, k, err = 0;
	struct video_device *vbd = NULL;
	struct display_obj *layer = NULL;
	struct platform_device *pdev;

	davinci_display_dev = device;

	dev_dbg(davinci_display_dev, "<davinci_probe>\n");

	/* First request memory region for io */
	pdev = to_platform_device(device);
	if (pdev->num_resources != 0) {
		dev_err(davinci_display_dev, "probed for an unknown device\n");
		return -ENODEV;
	}
	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		/* Get the pointer to the layer object */
		layer = davinci_dm.dev[i];
		/* Allocate memory for video device */
		vbd = video_device_alloc();
		if (ISNULL(vbd)) {
			for (j = 0; j < i; j++) {
				video_device_release
				    (davinci_dm.dev[j]->video_dev);
			}
			dev_err(davinci_display_dev, "ran out of memory\n");
			return -ENOMEM;
		}

		/* Initialize field of video device */
		*vbd = davinci_video_template;
		vbd->dev = device;
		vbd->release = video_device_release;
		snprintf(vbd->name, sizeof(vbd->name),
			 "DaVinci_VPBEDisplay_DRIVER_V%d.%d.%d",
			 (DAVINCI_DISPLAY_VERSION_CODE >> 16)
			 & 0xff,
			 (DAVINCI_DISPLAY_VERSION_CODE >> 8) &
			 0xff, (DAVINCI_DISPLAY_VERSION_CODE) & 0xff);
		/* Set video_dev to the video device */
		layer->video_dev = vbd;
		layer->device_id = i;
		layer->layer_info.id =
		    ((i == DAVINCI_DISPLAY_DEVICE_0) ? WIN_VID0 : WIN_VID1);
		if (display_buf_config_params.numbuffers[i] == 0)
			layer->memory = V4L2_MEMORY_USERPTR;
		else
			layer->memory = V4L2_MEMORY_MMAP;
		/* Initialize field of the layer objects */
		layer->usrs = layer->io_usrs = 0;
		layer->started = 0;

		/* Initialize prio member of layer object */
		v4l2_prio_init(&layer->prio);

		/* register video device */
		printk(KERN_NOTICE
		       "Trying to register davinci display video device.\n");
		printk(KERN_NOTICE "layer=%x,layer->video_dev=%x\n", (int)layer,
		       (int)&layer->video_dev);

		err = video_register_device(layer->
					    video_dev,
					    VFL_TYPE_GRABBER,
					    davinci_display_nr[i]);
		if (err)
			goto probe_out;
	}
	/* Initialize mutex */
	mutex_init(&davinci_dm.lock);
	return 0;

probe_out:
	for (k = 0; k < j; k++) {
		/* Get the pointer to the layer object */
		layer = davinci_dm.dev[k];
		/* Unregister video device */
		video_unregister_device(layer->video_dev);
		/* Release video device */
		video_device_release(layer->video_dev);
		layer->video_dev = NULL;
	}
	return err;
}

/*
 * davinci_remove()
 * It un-register hardware planes from V4L2 driver
 */
static int davinci_remove(struct device *device)
{
	int i;
	struct display_obj *plane;
	dev_dbg(davinci_display_dev, "<davinci_remove>\n");
	/* un-register device */
	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		/* Get the pointer to the layer object */
		plane = davinci_dm.dev[i];
		/* Unregister video device */
		video_unregister_device(plane->video_dev);

		plane->video_dev = NULL;
	}

	dev_dbg(davinci_display_dev, "</davinci_remove>\n");
	return 0;
}

static struct device_driver davinci_driver = {
	.name = DAVINCI_DISPLAY_DRIVER,
	.bus = &platform_bus_type,
	.probe = davinci_probe,
	.remove = davinci_remove,
};
static struct platform_device _davinci_display_device = {
	.name = DAVINCI_DISPLAY_DRIVER,
	.id = 1,
	.dev = {
		.release = davinci_platform_release,
		}
};

/*
 * davinci_display_init()
 * This function registers device and driver to the kernel, requests irq
 * handler and allocates memory for layer objects
 */
static __init int davinci_display_init(void)
{
	int err = 0, i, j;
	int free_layer_objects_index;
	int free_buffer_layer_index;
	int free_buffer_index;
	u32 addr;
	int size;

	printk(KERN_DEBUG "<davinci_display_init>\n");

	/* Default number of buffers should be 3 */
	if ((video2_numbuffers > 0) &&
	    (video2_numbuffers < display_buf_config_params.min_numbuffers))
		video2_numbuffers = display_buf_config_params.min_numbuffers;
	if ((video3_numbuffers > 0) &&
	    (video3_numbuffers < display_buf_config_params.min_numbuffers))
		video3_numbuffers = display_buf_config_params.min_numbuffers;

	/* Set buffer size to min buffers size if invalid buffer size is
	 * given */
	if (video2_bufsize <
	    display_buf_config_params.min_bufsize[DAVINCI_DISPLAY_DEVICE_0])
		video2_bufsize =
		    display_buf_config_params.
		    min_bufsize[DAVINCI_DISPLAY_DEVICE_0];

	if (video3_bufsize <
	    display_buf_config_params.min_bufsize[DAVINCI_DISPLAY_DEVICE_1])
		video3_bufsize =
		    display_buf_config_params.
		    min_bufsize[DAVINCI_DISPLAY_DEVICE_1];

	/* set number of buffers, they could come from boot/args */
	display_buf_config_params.numbuffers[DAVINCI_DISPLAY_DEVICE_0] =
		video2_numbuffers;
	display_buf_config_params.numbuffers[DAVINCI_DISPLAY_DEVICE_1] =
		video3_numbuffers;

	if (cpu_is_davinci_dm355()) {
		strcpy(davinci_display_videocap.card, DM355_EVM_CARD);
	} else if (cpu_is_davinci_dm365())
        #ifdef CONFIG_MACH_DAVINCI_WOLVERINE
            strcpy(davinci_display_videocap.card, WOLVERINE_CARD);
        #else // ! CONFIG_MACH_DAVINCI_WOLVERINE
		    strcpy(davinci_display_videocap.card, DM365_EVM_CARD);
        #endif // CONFIG_MACH_DAVINCI_WOLVERINE
	else
		strcpy(davinci_display_videocap.card, DM644X_EVM_CARD);


	/* Allocate memory for four plane display objects */
	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		davinci_dm.dev[i] =
		    kmalloc(sizeof(struct display_obj), GFP_KERNEL);
		/* If memory allocation fails, return error */
		if (!davinci_dm.dev[i]) {
			free_layer_objects_index = i;
			printk(KERN_ERR "ran out of memory\n");
			err = -ENOMEM;
			goto davinci_init_free_layer_objects;
		}
		spin_lock_init(&davinci_dm.dev[i]->irqlock);
	}
	free_layer_objects_index = DAVINCI_DISPLAY_MAX_DEVICES;

	/* Allocate memory for buffers */
	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		size = display_buf_config_params.layer_bufsize[i];
		for (j = 0; j < display_buf_config_params.numbuffers[i]; j++) {
			addr = davinci_alloc_buffer(size);
			if (!addr) {
				free_buffer_layer_index = i;
				free_buffer_index = j;
				printk(KERN_ERR "ran out of memory\n");
				err = -ENOMEM;
				goto davinci_init_free_buffers;
			}
			davinci_dm.dev[i]->fbuffers[j] = addr;
		}
	}
	if (display_buf_config_params.numbuffers[0] == 0)
		printk(KERN_ERR "no vid2 buffer allocated\n");
	if (display_buf_config_params.numbuffers[1] == 0)
		printk(KERN_ERR "no vid3 buffer allocated\n");
	free_buffer_layer_index = DAVINCI_DISPLAY_MAX_DEVICES;
	free_buffer_index = display_buf_config_params.numbuffers[i - 1];
	/* Register driver to the kernel */
	err = driver_register(&davinci_driver);
	if (0 != err) {
		goto davinci_init_free_buffers;
	}
	/* register device as a platform device to the kernel */
	err = platform_device_register(&_davinci_display_device);
	if (0 != err) {
		goto davinci_init_unregister_driver;
	}

	davinci_dm.event_callback.mask = (DAVINCI_DISP_END_OF_FRAME |
					  DAVINCI_DISP_FIRST_FIELD |
					  DAVINCI_DISP_SECOND_FIELD);

	davinci_dm.event_callback.arg = &davinci_dm;
	davinci_dm.event_callback.handler = davinci_display_isr;

	err = davinci_disp_register_callback(&davinci_dm.event_callback);

	if (0 != err) {
		goto davinci_init_unregister_driver;
	}
	printk(KERN_NOTICE
	       "davinci_init:DaVinci V4L2 Display Driver V1.0 loaded\n");
	printk(KERN_DEBUG "</davinci_init>\n");
	return 0;

davinci_init_unregister_driver:
	driver_unregister(&davinci_driver);

davinci_init_free_buffers:
	for (i = 0; i < free_buffer_layer_index; i++) {
		for (j = 0; j < display_buf_config_params.numbuffers[i]; j++) {
			addr = davinci_dm.dev[i]->fbuffers[j];
			if (addr) {
				davinci_free_buffer(addr,
						    display_buf_config_params.
						    layer_bufsize[i]
				    );
				davinci_dm.dev[i]->fbuffers[j] = 0;
			}
		}
	}
	for (j = 0; j < display_buf_config_params.numbuffers[free_buffer_index];
	     j++) {
		addr = davinci_dm.dev[free_buffer_layer_index]->fbuffers[j];
		if (addr) {
			davinci_free_buffer(addr,
					    display_buf_config_params.
					    layer_bufsize[i]);
			davinci_dm.dev[free_buffer_layer_index]->fbuffers[j]
			    = 0;
		}

	}

davinci_init_free_layer_objects:
	for (j = 0; j < free_layer_objects_index; j++) {
		if (davinci_dm.dev[i]) {
			kfree(davinci_dm.dev[j]);
			davinci_dm.dev[i] = NULL;
		}
	}
	return err;
}

/*
 * davinci_cleanup()
 * This function un-registers device and driver to the kernel, frees requested
 * irq handler and de-allocates memory allocated for layer objects.
 */
static void davinci_cleanup(void)
{
	int i = 0, j = 0;
	u32 addr;
	printk(KERN_INFO "<davinci_cleanup>\n");

	davinci_disp_unregister_callback(&davinci_dm.event_callback);
	platform_device_unregister(&_davinci_display_device);
	driver_unregister(&davinci_driver);
	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		for (j = 0; j < display_buf_config_params.numbuffers[i]; j++) {
			addr = davinci_dm.dev[i]->fbuffers[j];
			if (addr) {
				davinci_free_buffer(addr,
						    display_buf_config_params.
						    layer_bufsize[i]);
				davinci_dm.dev[i]->fbuffers[j] = 0;
			}
		}
	}
	for (i = 0; i < DAVINCI_DISPLAY_MAX_DEVICES; i++) {
		if (davinci_dm.dev[i]) {
			kfree(davinci_dm.dev[i]);
			davinci_dm.dev[i] = NULL;
		}
	}
	printk(KERN_INFO "</davinci_cleanup>\n");
}

EXPORT_SYMBOL(davinci_display_dev);
MODULE_LICENSE("GPL");
/* Function for module initialization and cleanup */
module_init(davinci_display_init);
module_exit(davinci_cleanup);
