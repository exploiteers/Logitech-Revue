/*
 * Copyright (C) 2008 MontaVista Software Inc.
 * Copyright (C) 2008 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <asm/arch/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/da8xx_lcdc.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include "da8xx_fb.h"
#include "da8xx_lcdc.h"
#ifdef CONFIG_GLCD_SHARP_COLOR
#include "sharp_color.h"
#endif
#define DRIVER_NAME "da8xx_lcdc"

#undef FB_DEBUG

#define da8xx_fb_read(addr)		__raw_readl(da8xx_fb_reg_base + (addr))
#define da8xx_fb_write(val, addr)	__raw_writel(val, \
						da8xx_fb_reg_base + (addr))

#define WSI_TIMEOUT 50

static resource_size_t da8xx_fb_reg_base;
static wait_queue_head_t da8xx_wq;

struct da8xxfb_par {
	resource_size_t p_regs_base;
	resource_size_t p_screen_base;
	resource_size_t p_palette_base;
	unsigned char *v_screen_base;
	unsigned char *v_palette_base;
	unsigned long screen_size;
	unsigned int palette_size;
	unsigned int bpp;
	struct clk *lcdc_clk;
	unsigned int irq;
	u16 pseudo_palette[16];
};

/* Variable Screen Information */
static struct fb_var_screeninfo da8xxfb_var __devinitdata = {
	.xoffset = 0,
	.yoffset = 0,
	.transp = {0, 0, 0},
	.nonstd = 0,
	.activate = 0,
	.height = -1,
	.width = -1,
	.pixclock = 46666,	/* 46us - AUO Display */
	.accel_flags = 0,
	.left_margin = LEFT_MARGIN,
	.right_margin = RIGHT_MARGIN,
	.upper_margin = UPPER_MARGIN,
	.lower_margin = LOWER_MARGIN,
	.sync = 0,
	.vmode = FB_VMODE_NONINTERLACED
};

static struct fb_fix_screeninfo da8xxfb_fix __devinitdata = {
	.id = "DA8XX FB Driver",
	.type = FB_TYPE_PACKED_PIXELS,
	.type_aux = 0,
	.visual = FB_VISUAL_PSEUDOCOLOR,
	.xpanstep = 1,
	.ypanstep = 1,
	.ywrapstep = 1,
	.accel = FB_ACCEL_NONE
};

static u32 g_databuf_sz;
static u32 g_palette_sz;
static struct fb_ops da8xx_fb_ops;
static int lcd_wait_status_int(int int_status_mask);

#ifdef FB_DEBUG
static void lcd_show_raster_status(void)
{
	u32 value = da8xx_fb_read(LCD_STAT_REG);
	if (value & LCD_FIFO_UNDERFLOW)
		printk(KERN_ALERT "DA8XX LCD: FIFO Underflow!\n");
	if (value & LCD_AC_BIAS_COUNT_STATUS)
		printk(KERN_INFO "DA8XX LCD: AC Bias Count Reached\n");
	if (value & LCD_PALETTE_LOADED)
		printk(KERN_INFO "DA8XX LCD: Palette is Loaded\n");
	else
		printk(KERN_ALERT "DA8XX LCD: Palette is not Loaded!\n");
	if (value & LCD_SYNC_LOST) {
		printk(KERN_ALERT "DA8XX LCD: Frame Synchronization Error!\n");
		da8xx_fb_write(da8xx_fb_read(LCD_STAT_REG) | LCD_SYNC_LOST,
			       LCD_STAT_REG);
	}
}

static void lcd_show_raster_interface(void)
{
	printk(KERN_INFO "\n");
	printk(KERN_INFO "LCD Raster Interface Configuration\n");
	printk(KERN_INFO "----------------------------------\n");
	printk(KERN_INFO "LCD Control Register:    0x%x\n",
	       da8xx_fb_read(LCD_CTRL_REG));
	printk(KERN_INFO "Raster Control:          0x%x\n",
	       da8xx_fb_read(LCD_RASTER_CTRL_REG));
	printk(KERN_INFO "Timing Register 0:       0x%x\n",
	       da8xx_fb_read(LCD_RASTER_TIMING_0_REG));
	printk(KERN_INFO "Timing Register 1:       0x%x\n",
	       da8xx_fb_read(LCD_RASTER_TIMING_1_REG));
	printk(KERN_INFO "Timing Register 2:       0x%x\n",
	       da8xx_fb_read(LCD_RASTER_TIMING_2_REG));
	printk(KERN_INFO "Subpanel Register:       0x%x\n",
	       da8xx_fb_read(LCD_RASTER_SUBPANEL_DISP_REG));
	printk(KERN_INFO "DMA Control Register:    0x%x\n",
	       da8xx_fb_read(LCD_DMA_CTRL_REG));
	printk(KERN_INFO "DMA 0 Start Address:     0x%x\n",
	       da8xx_fb_read(LCD_DMA_FRM_BUF_BASE_ADDR_0_REG));
	printk(KERN_INFO "DMA 0 End Address:       0x%x\n",
	       da8xx_fb_read(LCD_DMA_FRM_BUF_CEILING_ADDR_0_REG));
	printk(KERN_INFO "DMA 1 Start Address:     0x%x\n",
	       da8xx_fb_read(LCD_DMA_FRM_BUF_BASE_ADDR_1_REG));
	printk(KERN_INFO "DMA 1 End Address:       0x%x\n",
	       da8xx_fb_read(LCD_DMA_FRM_BUF_CEILING_ADDR_1_REG));
	printk(KERN_INFO "\n");
	lcd_show_raster_status();
	printk(KERN_INFO "\n");
}
#endif

/* Disable the Raster Engine of the LCD Controller */
static int lcd_disable_raster(void)
{
	u32 reg;
	int ret = 0;

	reg = da8xx_fb_read(LCD_RASTER_CTRL_REG);
	if (reg & LCD_RASTER_ENABLE) {
		da8xx_fb_write(reg & ~LCD_RASTER_ENABLE, LCD_RASTER_CTRL_REG);
		ret = lcd_wait_status_int(LCD_END_OF_FRAME0);
	}
	return ret;
}

static int lcd_blit(int load_mode, u32 p_buf)
{
	u32 reg;
	int ret = 0;
	u32 tmp = p_buf + g_databuf_sz - 2;

	/* Update the databuf in the hw. */
	da8xx_fb_write(p_buf, LCD_DMA_FRM_BUF_BASE_ADDR_0_REG);
	da8xx_fb_write(tmp, LCD_DMA_FRM_BUF_CEILING_ADDR_0_REG);

	/* Start the DMA. */
	reg = da8xx_fb_read(LCD_RASTER_CTRL_REG);
	reg &= ~(3 << 20);
	if (load_mode == LOAD_DATA) {
		reg |= LCD_PALETTE_LOAD_MODE(PALETTE_AND_DATA);
		da8xx_fb_write(reg, LCD_RASTER_CTRL_REG);
	} else if (LOAD_PALETTE == load_mode) {
		reg |= LCD_PALETTE_LOAD_MODE(PALETTE_ONLY);
		da8xx_fb_write(reg, LCD_RASTER_CTRL_REG);
	}
	return ret;
}

/* Configure the Burst Size of DMA */
static int lcd_cfg_dma(int burst_size)
{
	u32 reg;
	reg = da8xx_fb_read(LCD_DMA_CTRL_REG) & 0x00000001;
	switch (burst_size) {
	case 1:
		reg |= LCD_DMA_BURST_SIZE(LCD_DMA_BURST_1);
		break;
	case 2:
		reg |= LCD_DMA_BURST_SIZE(LCD_DMA_BURST_2);
		break;
	case 4:
		reg |= LCD_DMA_BURST_SIZE(LCD_DMA_BURST_4);
		break;
	case 8:
		reg |= LCD_DMA_BURST_SIZE(LCD_DMA_BURST_8);
		break;
	case 16:
		reg |= LCD_DMA_BURST_SIZE(LCD_DMA_BURST_16);
		break;
	default:
		return -EINVAL;
#ifdef FB_DEBUG
		printk(KERN_INFO
		       "DA8XX LCD: Configured LCD DMA Burst Size...\n");
#endif
	}
	da8xx_fb_write(reg | LCD_END_OF_FRAME_INT_ENA, LCD_DMA_CTRL_REG);

	return 0;
}

static void lcd_cfg_ac_bias(int period, int transitions_per_int)
{
	u32 reg;
	/* Set the AC Bias Period and Number of Transisitons per Interrupt */
	reg = da8xx_fb_read(LCD_RASTER_TIMING_2_REG) & 0xFFF00000;
	reg |= LCD_AC_BIAS_FREQUENCY(period) |
	    LCD_AC_BIAS_TRANSITIONS_PER_INT(transitions_per_int);
	da8xx_fb_write(reg, LCD_RASTER_TIMING_2_REG);
#ifdef FB_DEBUG
	printk(KERN_INFO "DA8XX LCD: Configured AC Bias...\n");
#endif
}

static void lcd_cfg_horizontal_sync(int back_porch, int pulse_width,
				    int front_porch)
{
	u32 reg;
	reg = da8xx_fb_read(LCD_RASTER_TIMING_0_REG) & 0xf;
	reg |= ((back_porch & 0xff) << 24)
	    | ((front_porch & 0xff) << 16)
	    | ((pulse_width & 0x3f) << 10);
	da8xx_fb_write(reg, LCD_RASTER_TIMING_0_REG);
#ifdef FB_DEBUG
	printk(KERN_INFO
	       "DA8XX LCD: Configured Horizontal Sync Properties...\n");
#endif
}

static void lcd_cfg_vertical_sync(int back_porch, int pulse_width,
				  int front_porch)
{
	u32 reg;
	reg = da8xx_fb_read(LCD_RASTER_TIMING_1_REG) & 0x3ff;
	reg |= ((back_porch & 0xff) << 24)
	    | ((front_porch & 0xff) << 16)
	    | ((pulse_width & 0x3f) << 10);
	da8xx_fb_write(reg, LCD_RASTER_TIMING_1_REG);
#ifdef FB_DEBUG
	printk(KERN_INFO "DA8XX LCD: Configured Vertical Sync Properties...\n");
#endif
}

static void lcd_cfg_display(const struct lcd_ctrl_config *cfg)
{
	u32 reg;

	reg =
	    da8xx_fb_read(LCD_RASTER_CTRL_REG) & ~(LCD_TFT_MODE |
						   LCD_MONO_8BIT_MODE |
						   LCD_MONOCHROME_MODE);

	switch (cfg->p_disp_panel->panel_shade) {
	case MONOCROME:
		reg |= LCD_MONOCHROME_MODE;
		if (cfg->mono_8bit_mode)
			reg |= LCD_MONO_8BIT_MODE;
		break;
	case COLOR_ACTIVE:
		reg |= LCD_TFT_MODE;
		if (cfg->tft_alt_mode)
			reg |= LCD_TFT_ALT_ENABLE;
		break;

	case COLOR_PASSIVE:
		if (cfg->stn_565_mode)
			reg |= LCD_STN_565_ENABLE;
		break;

	default:
#ifdef FB_DEBUG
		printk(KERN_ERR "Undefined LCD type\n");
#endif
		break;
	}
	reg |= LCD_UNDERFLOW_INT_ENA;

	da8xx_fb_write(reg, LCD_RASTER_CTRL_REG);

	reg = da8xx_fb_read(LCD_RASTER_TIMING_2_REG);

	if (cfg->sync_ctrl)
		reg |= LCD_SYNC_CTRL;
	else
		reg &= ~LCD_SYNC_CTRL;

	if (cfg->sync_edge)
		reg |= LCD_SYNC_EDGE;
	else
		reg &= ~LCD_SYNC_EDGE;

	if (cfg->invert_pxl_clock)
		reg |= LCD_INVERT_PIXEL_CLOCK;
	else
		reg &= ~LCD_INVERT_PIXEL_CLOCK;

	if (cfg->invert_line_clock)
		reg |= LCD_INVERT_LINE_CLOCK;
	else
		reg &= ~LCD_INVERT_LINE_CLOCK;

	if (cfg->invert_frm_clock)
		reg |= LCD_INVERT_FRAME_CLOCK;
	else
		reg &= ~LCD_INVERT_FRAME_CLOCK;

	da8xx_fb_write(reg, LCD_RASTER_TIMING_2_REG);

#ifdef FB_DEBUG
	printk(KERN_INFO "GLCD: Configured to Active Color...\n");
#endif

}

static int lcd_cfg_frame_buffer(u32 width, u32 height, u32 bpp,
				u32 raster_order)
{
	u32 reg;
	u32 g_bpl;
	u32 g_bpp = bpp;

	/* Disable Dual Frame Buffer. */
	reg = da8xx_fb_read(LCD_DMA_CTRL_REG);
	da8xx_fb_write(reg & ~LCD_DUAL_FRAME_BUFFER_ENABLE, LCD_DMA_CTRL_REG);
	/* Set the Panel Width */
	/* Pixels per line = (PPL + 1)*16 */
	/*0x3F in bits 4..9 gives max horisontal resolution = 1024 pixels*/
	width &= 0x3f0;
	reg = da8xx_fb_read(LCD_RASTER_TIMING_0_REG);
	reg = (((width >> 4) - 1) << 4) | (reg & 0xfffffc00);
	da8xx_fb_write(reg, LCD_RASTER_TIMING_0_REG);

	/* Set the Panel Height */
	reg = da8xx_fb_read(LCD_RASTER_TIMING_1_REG);
	reg = ((height - 1) & 0x3ff) | (reg & 0xfffffc00);
	da8xx_fb_write(reg, LCD_RASTER_TIMING_1_REG);

	/* Set the Raster Order of the Frame Buffer */
	reg = da8xx_fb_read(LCD_RASTER_CTRL_REG) & ~(1 << 8);
	if (raster_order)
		reg |= LCD_RASTER_ORDER;

	switch (g_bpp) {
	case 1:
	case 2:
	case 4:
	case 16:
		g_palette_sz = 16 * 2;
		g_bpl = width * g_bpp / 8;
		g_databuf_sz = height * g_bpl + g_palette_sz;
		break;

	case 8:
		g_palette_sz = 256 * 2;
		g_bpl = width * g_bpp / 8;
		g_databuf_sz = height * g_bpl + g_palette_sz;
		break;

	default:
#ifdef FB_DEBUG
		printk(KERN_ALERT "DA8XX LCD: Unsupported BPP!\n");
#endif
		break;
	}
#ifdef FB_DEBUG
	printk(KERN_INFO "DA8XX LCD: Configured Frame Buffer...\n");
#endif

	return 0;
}

/* Wait for Interrupt Status to appear. */
static int lcd_wait_status_int(int int_status_mask)
{
	int ret;

	if (int_status_mask == LCD_SYNC_LOST)
		ret = wait_event_interruptible_timeout(da8xx_wq,
						       da8xx_fb_read
						       (LCD_STAT_REG) &
						       int_status_mask,
						       WSI_TIMEOUT);
	else
		ret = wait_event_interruptible_timeout(da8xx_wq,
						       !da8xx_fb_read
						       (LCD_STAT_REG) &
						       int_status_mask,
						       WSI_TIMEOUT);
#ifdef FB_DEBUG
	if (ret <= 0) {
		printk(KERN_ALERT
		       "DA8XX LCD: status wait returned %d, mask %d\n",
		       ret, int_status_mask);
		lcd_show_raster_status();
	}
#endif
	if (ret < 0)
		return ret;

	if (ret == 0)
		return -ETIMEDOUT;

	return 0;
}

/* Palette Initialization */
static void da8xxfb_init_palette(struct fb_info *info)
{
	unsigned short i, size;
	struct da8xxfb_par *par = info->par;
	unsigned short *palette = (unsigned short *)par->p_palette_base;

	/* Palette Size */
	size = (par->palette_size / sizeof(*palette));

	/* Clear the Palette */
	memset(palette, 0, par->palette_size);

	/* Initialization of Palette for Default values */
	for (i = 0; i < size; i++)
		*(unsigned short *)(palette + i) = i;

	/* Setup the BPP */
	switch (par->bpp) {
	case 1:
		palette[0] |= (1 << 11);
		break;
	case 2:
		palette[0] |= (1 << 12);
		break;
	case 4:
		palette[0] |= (2 << 12);
		break;
	case 8:
		palette[0] |= (3 << 12);
		break;
	case 16:
		palette[0] |= (4 << 12);
		break;
	default:
		printk(KERN_ALERT "DA8XX LCD: Unsupported Video BPP %d!\n",
		       par->bpp);
		break;
	}

	for (i = 0; i < size; i++)
		par->pseudo_palette[i] = i;

#ifdef FB_DEBUG
	printk(KERN_INFO "GLCD: Palette initialization done successfully\n");
#endif
}

static int da8xx_fb_setcolreg(unsigned regno, unsigned red, unsigned green,
			      unsigned blue, unsigned transp,
			      struct fb_info *info)
{
	u_short pal;
	struct da8xxfb_par *par = info->par;
	unsigned short *palette = (unsigned short *)par->v_palette_base;

	if (regno > 255)
		return 1;

	if (info->fix.visual == FB_VISUAL_DIRECTCOLOR ||
	    info->fix.visual == FB_VISUAL_TRUECOLOR)
		return 1;

	switch (par->bpp) {
	case 8:
		red >>= 8;
		green >>= 8;
		blue >>= 8;
		break;
	}

	pal = (red & 0x0f00);
	pal |= (green & 0x00f0);
	pal |= (blue & 0x000f);

	palette[regno] = pal;

	return 0;
}

static void lcd_reset(void)
{
	/* Disable the Raster if previously Enabled */
	if (da8xx_fb_read(LCD_RASTER_CTRL_REG) & LCD_RASTER_ENABLE) {
#ifdef FB_DEBUG
		printk(KERN_INFO
		       "DA8XX LCD: Waiting for Raster Frame Done...\n");
#endif
		lcd_disable_raster();
	}
	/* DMA has to be disabled */
	da8xx_fb_write(0, LCD_DMA_CTRL_REG);
#ifdef FB_DEBUG
	printk(KERN_INFO "DA8XX LCD: Raster is going for Reset now...\n");
	lcd_show_raster_interface();
#endif
	da8xx_fb_write(0, LCD_RASTER_CTRL_REG);
	/* TODO: Place the LCD block in reset */
	/* TODO: Release it from Reset */
#ifdef FB_DEBUG
	printk(KERN_INFO "GLCD: LCD Controller Reset...\n");
	lcd_show_raster_interface();
#endif
}

static int lcd_init(const struct lcd_ctrl_config *cfg)
{
	u32 bpp;
	if (da8xx_fb_read(LCD_BLK_REV_REG) != DA8XX_LCDC_REVISION)
		return -ENOENT;
	lcd_reset();
	/* Configure the LCD clock divisor. */
	da8xx_fb_write(LCD_CLK_DIVISOR(cfg->pxl_clk) | (LCD_RASTER_MODE & 0x1),
		       LCD_CTRL_REG);
	/* Configure the DMA burst size. */
	lcd_cfg_dma(cfg->dma_burst_sz);
	/* Configure the AC bias properties. */
	lcd_cfg_ac_bias(cfg->ac_bias, cfg->ac_bias_intrpt);
	/* Configure the vertical and horizontal sync properties. */
	lcd_cfg_vertical_sync(cfg->vbp, cfg->vsw, cfg->vfp);
	lcd_cfg_horizontal_sync(cfg->hbp, cfg->hsw, cfg->hfp);
	/* Configure for disply */
	lcd_cfg_display(cfg);
	if (QVGA != cfg->p_disp_panel->panel_type) {
		printk(KERN_ALERT
		       "\nError: Only QVGA panel is currently supported !");
		return -EINVAL;
	}
	if (cfg->bpp <= cfg->p_disp_panel->max_bpp &&
	    cfg->bpp >= cfg->p_disp_panel->min_bpp)
		bpp = cfg->bpp;
	else
		bpp = cfg->p_disp_panel->max_bpp;
	if (bpp == 12)
		bpp = 16;
	lcd_cfg_frame_buffer((unsigned int)cfg->p_disp_panel->width,
			     (unsigned int)cfg->p_disp_panel->height, bpp,
			     cfg->raster_order);
	/* Configure FDD */
	da8xx_fb_write((da8xx_fb_read(LCD_RASTER_CTRL_REG) & 0xfff00fff) |
		       (cfg->fdd << 12), LCD_RASTER_CTRL_REG);

	return 0;
}

static irqreturn_t da8xx_lcdc_irq_handler(int irq, void *arg,
					  struct pt_regs *regs)
{

	u32 stat = da8xx_fb_read(LCD_STAT_REG);
	u32 reg;

	if ((stat & LCD_SYNC_LOST) && (stat & LCD_FIFO_UNDERFLOW)) {
		reg = da8xx_fb_read(LCD_RASTER_CTRL_REG);
		da8xx_fb_write(reg & ~LCD_RASTER_ENABLE, LCD_RASTER_CTRL_REG);
		da8xx_fb_write(stat, LCD_STAT_REG);
		da8xx_fb_write(reg | LCD_RASTER_ENABLE, LCD_RASTER_CTRL_REG);
	} else
		da8xx_fb_write(stat, LCD_STAT_REG);

	/*TODO: m.b. use lcd_clear_status_int((u32)int_status_mask) here*/
	wake_up_interruptible(&da8xx_wq);
	return IRQ_HANDLED;
}

static int da8xx_fb_check_var(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
	int err = 0;
	switch (var->bits_per_pixel) {
	case 1:
	case 8:
		var->red.offset = 0;
		var->red.length = 8;
		var->green.offset = 0;
		var->green.length = 8;
		var->blue.offset = 0;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 4:
		var->red.offset = 0;
		var->red.length = 4;
		var->green.offset = 0;
		var->green.length = 4;
		var->blue.offset = 0;
		var->blue.length = 4;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 16:		/* RGB 565 */
		var->red.offset = 0;
		var->red.length = 5;
		var->green.offset = 5;
		var->green.length = 6;
		var->blue.offset = 11;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	default:
		err = -EINVAL;
	}

	var->red.msb_right = 0;
	var->green.msb_right = 0;
	var->blue.msb_right = 0;
	var->transp.msb_right = 0;
	return err;
}

static int da8xx_fb_set_par(struct fb_info *info)
{
	struct fb_fix_screeninfo *fix = &info->fix;
	struct fb_var_screeninfo *var = &info->var;

	switch (var->bits_per_pixel) {
	case 1:
		fix->visual = FB_VISUAL_MONO01;
		break;

	case 2:
	case 4:
	case 8:
		fix->visual = FB_VISUAL_PSEUDOCOLOR;
		break;

	case 16:
		fix->visual = FB_VISUAL_TRUECOLOR;
		break;
	}

	fix->line_length = (var->xres_virtual * var->bits_per_pixel) / 8;
	return 0;
}

static int __devexit da8xx_fb_remove(struct platform_device *dev)
{
	struct fb_info *info = dev_get_drvdata(&dev->dev);
	if (info) {
		struct da8xxfb_par *par = info->par;
		/*TODO: do we need this along with line below? */
		if (da8xx_fb_read(LCD_RASTER_CTRL_REG) & LCD_RASTER_ENABLE)
			lcd_disable_raster();
		da8xx_fb_write(0, LCD_RASTER_CTRL_REG);
		/* disable DMA  */
		da8xx_fb_write(0, LCD_DMA_CTRL_REG);

		unregister_framebuffer(info);

		fb_dealloc_cmap(&info->cmap);

		dma_free_coherent(NULL, g_databuf_sz, par->v_palette_base,
				  par->p_palette_base);

		free_irq(par->irq, NULL);

		clk_disable(par->lcdc_clk);

		clk_put(par->lcdc_clk);

		framebuffer_release(info);

	}
	return 0;
}
static int __init da8xx_fb_probe(struct platform_device *device)
{
	struct fb_info *da8xxfb_info;
	struct da8xxfb_par *par;
	int ret;
	struct resource *lcdc_regs;
	struct da8xx_lcdc_platform_data *fb_pdata = device->dev.platform_data;
	struct clk *fb_clk = NULL;

	if (fb_pdata == NULL) {
		dev_err(&device->dev, "Can not get platform data\n");
		return -ENOENT;
	}

	lcdc_regs = platform_get_resource(device, IORESOURCE_MEM, 0);
	if (!lcdc_regs) {
		dev_err(&device->dev,
			"Can not get memory resource for LCD controller\n");
		return -ENOENT;
	}
	da8xx_fb_reg_base = IO_ADDRESS(lcdc_regs->start);

	fb_clk = clk_get(&device->dev, fb_pdata->lcdc_clk_name);
	if (IS_ERR(fb_clk)) {
		dev_err(&device->dev, "Can not get device clock\n");
		return -ENODEV;
	}
	ret = clk_enable(fb_clk);
	if (ret)
		goto err_clk_put;

	if (lcd_init(&lcd_cfg)) {
		dev_err(&device->dev, "lcd_init failed\n");
		ret = -EFAULT;
		goto err_clk_disable;
	}

	da8xxfb_info = framebuffer_alloc(sizeof(struct fb_info), &device->dev);
	if (!da8xxfb_info) {
		dev_dbg(device, "Memory allocation failed for fb_info\n");
		ret = -ENOMEM;
		goto err_clk_disable;
	}

	par = da8xxfb_info->par;
	/* allocate frame buffer */
	par->v_palette_base = dma_alloc_coherent(NULL, g_databuf_sz,
						 &par->p_palette_base,
						 GFP_KERNEL | GFP_DMA);

	if (!par->v_palette_base) {
		dev_err(&device->dev,
			"GLCD: kmalloc for frame buffer failed\n");
		ret = -EINVAL;
		goto err_release_fb;
	}

	/* First g_palette_sz byte of the frame buffer is the palett */
	par->palette_size = g_palette_sz;

	/* the rest of the frame buffer is pixel data */
	par->v_screen_base = par->v_palette_base + g_palette_sz;
	par->p_screen_base = par->p_palette_base + g_palette_sz;
	par->screen_size = g_databuf_sz - g_palette_sz;

	par->lcdc_clk = fb_clk;

	da8xxfb_fix.smem_start = (unsigned long)par->p_screen_base;
	da8xxfb_fix.smem_len = par->screen_size;

#ifdef FB_DEBUG
	printk(KERN_INFO "GLCD: Frame Buffer Memory allocation successful\n");
	printk(KERN_INFO "GLCD: Palette Memory allocation successful\n");
#endif

	init_waitqueue_head(&da8xx_wq);

	par->irq = platform_get_irq(device, 0);
	if (par->irq < 0) {
		ret = -ENOENT;
		goto err_release_fb_mem;
	}

	ret = request_irq(par->irq, da8xx_lcdc_irq_handler, 0,
			  DRIVER_NAME, NULL);
	if (ret)
		goto err_free_irq;

	/* Initialize par */
	par->bpp = lcd_cfg.bpp;

	da8xxfb_var.xres = lcd_cfg.p_disp_panel->width;
	da8xxfb_var.xres_virtual = lcd_cfg.p_disp_panel->width;

	da8xxfb_var.yres = lcd_cfg.p_disp_panel->height;
	da8xxfb_var.yres_virtual = lcd_cfg.p_disp_panel->height;

	da8xxfb_var.grayscale =
	    lcd_cfg.p_disp_panel->panel_shade == MONOCROME ? 1 : 0;
	da8xxfb_var.bits_per_pixel = lcd_cfg.bpp;

	da8xxfb_var.hsync_len = lcd_cfg.hsw;
	da8xxfb_var.vsync_len = lcd_cfg.vsw;

	/* Initialize fbinfo */
	da8xxfb_info->flags = FBINFO_FLAG_DEFAULT;
	da8xxfb_info->screen_base = par->v_screen_base;
	da8xxfb_info->device = &device->dev;
	da8xxfb_info->fix = da8xxfb_fix;
	da8xxfb_info->var = da8xxfb_var;
	da8xxfb_info->fbops = &da8xx_fb_ops;
	da8xxfb_info->pseudo_palette = par->pseudo_palette;

	/* Initialize the Palette */
#ifdef FB_DEBUG
	printk(KERN_INFO "GLCD: Initializing the Palette...\n");
#endif
	da8xxfb_init_palette(da8xxfb_info);

	ret = fb_alloc_cmap(&da8xxfb_info->cmap, PALETTE_SIZE, 0);
	if (ret)
		goto err_free_irq;

	/* Map Video Memory */
#ifdef FB_DEBUG
	printk(KERN_INFO "GLCD: Mapping the Video Memory...\n");
#endif

	/* Flush the buffer to the screen. */
	lcd_blit(LOAD_DATA, (u32) par->p_palette_base);

	/* initialize var_screeninfo */
	da8xxfb_var.activate = FB_ACTIVATE_FORCE;
	fb_set_var(da8xxfb_info, &da8xxfb_var);

	dev_set_drvdata(&device->dev, da8xxfb_info);
	/* Register the Frame Buffer  */
	if (register_framebuffer(da8xxfb_info) < 0) {
		printk(KERN_ALERT "GLCD: Frame Buffer Registration Failed!\n");
		ret = -EINVAL;
		goto err_dealloc_cmap;
	}

	/* enable raster engine */
	da8xx_fb_write(da8xx_fb_read(LCD_RASTER_CTRL_REG) | LCD_RASTER_ENABLE,
		       LCD_RASTER_CTRL_REG);

	return 0;

err_dealloc_cmap:
	fb_dealloc_cmap(&da8xxfb_info->cmap);

err_free_irq:
	free_irq(par->irq, NULL);

err_release_fb_mem:
	dma_free_coherent(NULL, g_databuf_sz, par->v_palette_base,
			  par->p_palette_base);

err_release_fb:
	framebuffer_release(da8xxfb_info);

err_clk_disable:
	clk_disable(fb_clk);

err_clk_put:
	clk_put(fb_clk);

	return ret;
}

static int da8xx_fb_ioctl(struct fb_info *info, unsigned int cmd,
			  unsigned long arg)
{
	int val;
	struct lcd_sync_arg sync_arg;
#ifdef CONFIG_GLCD_SHARP_COLOR
	struct lcd_contrast_arg contrast_arg;
	unsigned long hal_arg = arg;
#endif

	switch (cmd) {
	case FBIOPUT_CONTRAST:
		val = (int)arg;
#ifdef CONFIG_GLCD_SHARP_COLOR

		if (copy_from_user
		    (&contrast_arg, (char *)arg,
		     sizeof(struct lcd_contrast_arg)))
			return -EINVAL;
		hal_arg = contrast_arg.cnt;
		if (hal_arg < TI_GLCD_SHARP_COLOR_MIN_CONTRAST
		    || hal_arg > TI_GLCD_SHARP_COLOR_MAX_CONTRAST)
			return -EINVAL;
		hal_arg = contrast_arg.cnt | (contrast_arg.is_up << 8);
		/*TODO : implement contrast setting here */
		return -EINVAL;
#endif
		break;
	case FBIGET_BRIGHTNESS:
	case FBIPUT_BRIGHTNESS:
	case FBIGET_COLOR:
	case FBIPUT_COLOR:
		return -EINVAL;
	case FBIPUT_HSYNC:
		if (copy_from_user(&sync_arg, (char *)arg,
				sizeof(struct lcd_sync_arg)))
			return -EINVAL;
		lcd_cfg_horizontal_sync(sync_arg.back_porch,
					sync_arg.pulse_width,
					sync_arg.front_porch);
		break;
	case FBIPUT_VSYNC:
		if (copy_from_user(&sync_arg, (char *)arg,
				sizeof(struct lcd_sync_arg)))
			return -EINVAL;
		lcd_cfg_vertical_sync(sync_arg.back_porch,
					sync_arg.pulse_width,
					sync_arg.front_porch);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct fb_ops da8xx_fb_ops = {
	.owner = THIS_MODULE,
	.fb_check_var = da8xx_fb_check_var,
	.fb_set_par = da8xx_fb_set_par,
	.fb_setcolreg = da8xx_fb_setcolreg,
	.fb_ioctl = da8xx_fb_ioctl,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
};

#ifdef CONFIG_PM
static int da8xx_fb_suspend(struct platform_device *dev, pm_message_t state)
{
	 /*TODO*/ return -EBUSY;
}
static int da8xx_fb_resume(struct platform_device *dev)
{
	 /*TODO*/ return -EBUSY;
}
#else
#define da8xx_fb_suspend NULL
#define da8xx_fb_resume NULL
#endif

static struct platform_driver da8xx_fb_driver = {
	.probe = da8xx_fb_probe,
	.remove = da8xx_fb_remove,
	.suspend = da8xx_fb_suspend,
	.resume = da8xx_fb_resume,
	.driver = {
		   .name = DRIVER_NAME,
		   .owner = THIS_MODULE,
		   },
};

static int __init da8xxfb_init(void)
{
	return platform_driver_register(&da8xx_fb_driver);
}

static void __exit da8xxfb_cleanup(void)
{
	platform_driver_unregister(&da8xx_fb_driver);
}

module_init(da8xxfb_init);
module_exit(da8xxfb_cleanup);

MODULE_DESCRIPTION("Framebuffer driver for TI da8xx");
MODULE_AUTHOR("MontaVista Software");
MODULE_LICENSE("GPL");
