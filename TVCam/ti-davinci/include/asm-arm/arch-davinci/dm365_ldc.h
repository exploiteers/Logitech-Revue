/************************************************************************
 * Description 
 * -----------
 * LDC (Lens Distortion Correction) is to correct aberrations of image 
 * resulted from lens geometry that ususally appear at the edges of 
 * the picture. 
 *
 * device name : dm365_ldc
 ************************************************************************/

#ifndef _DAVINCI_LDC_H
#define _DAVINCI_LDC_H
#include <linux/ioctl.h>

/* LDC modes */
#define LDC_MODE_YC422 (0)
#define LDC_MODE_BAYER_CAC (1) 
#define LDC_MODE_YC420 (2)

/* Bayer formats for LDC_MODE_BAYER_CAC */
#define LDC_BMODE_U12 (0)
#define LDC_BMODE_P12 (1)
#define LDC_BMODE_P8 (2)
#define LDC_BMODE_A8 (3)

/* single or multipass */
#define LDC_SINGLE_PASS (0)
#define LDC_MULTI_PASS (1)

/* LDC frame parameters */
struct ldc_frame_param_s {
	u16 width;
	u16 height;
	/* each address has to be 32-B aligned */
	u32 readbase;
	u32 writebase;
	u32 readbase420c;
	u32 writebase420c;
	/* each offset is line length in bytes */
	u32 readofst;
	u32 writeofst;
};

/* LDC configurations */
struct ldc_config_s {
	u8 pass; /* single or multiple */

	u16 skipWidth; /* width and height dimensions of center  */
	u16 skipHeight; /* tile to be skipped in case of multi-pass */
	u8 mode;  /* mode */
	u8 bmode; /* Bayer mode */

	u8 kvl; /* vertical lower scaling factor */
	u8 kvu; /* vertical upper scaling factor */
	u8 khr; /* horizontal right scaling factor */
	u8 khl; /* horizontal left scaling factor */

	u8 tbits; /* round-down number of bits */
	u8 pixpad; /* pixel pad */
	u8 obh; /* output block height */
	u8 obw; /* output block width */

	u8 initc; /* Initial color for Bayer mode */
	u8 yint_type; /* Y component interpolation type */
	u16 lut[256]; /* LUT entries */

	u16 rth; /* threshold in back-mapping */
	u16 lens_cntr_x; /* lens center X */
	u16 lens_cntr_y; /* lens center Y */
};

/* affine transform parameters */
struct ldc_affine_param_s {
	u16 affine_a;
	u16 affine_b;
	u16 affine_c;
	u16 affine_d;
	u16 affine_e;
	u16 affine_f;
};

/* ioctls definition for LDC operations */
#define LDC_IOC_BASE   	'L'
#define LDC_SET_FRMDATA _IOWR(LDC_IOC_BASE, 1, struct ldc_frame_param_s)
#define LDC_GET_FRMDATA	_IOR(LDC_IOC_BASE, 2, struct ldc_frame_param_s)
#define LDC_SET_CONFIG _IOWR(LDC_IOC_BASE, 3, struct ldc_config_s)
#define LDC_GET_CONFIG _IOR(LDC_IOC_BASE, 4, struct ldc_config_s)
#define LDC_SET_AFFINE _IOWR(LDC_IOC_BASE, 5, struct ldc_affine_param_s)
#define LDC_GET_AFFINE _IOR(LDC_IOC_BASE, 6, struct ldc_affine_param_s)
#define LDC_START	_IO(LDC_IOC_BASE, 7)
#define LDC_IOC_MAXNR 7

#ifdef __KERNEL__

#include <linux/kernel.h>       /* printk() */
#include <asm/io.h>             /* For IO_ADDRESS */

#define DM365_LDC_BASE (0x01C71600)
#define DAVINCI_LDC_BASE DM365_LDC_BASE

static __inline__ u32 ldc_regr(u32 offset)
{
	return davinci_readl(DAVINCI_LDC_BASE + offset);
}

static __inline__ u32 ldc_regw(u32 val, u32 offset)
{
	davinci_writel(val, DAVINCI_LDC_BASE + offset);
	return val;
}

static __inline__ u32 ldc_regm(u32 val, u32 mask, u32 offset)
{
	u32 addr, new_val;
  addr = DAVINCI_LDC_BASE + offset;
  new_val = (davinci_readl(addr) & ~mask) | (val & mask);
  davinci_writel(new_val, addr);
  return (new_val);
}

#define DAVINCI_LDC_PID (0x0)
#define DAVINCI_LDC_PCR (0x4)
#define DAVINCI_LDC_RD_BASE (0x8)
#define DAVINCI_LDC_RD_OFST (0xC)
#define DAVINCI_LDC_FRAME_SZ (0x10)
#define DAVINCI_LDC_INITXY (0x14)
#define DAVINCI_LDC_WR_BASE (0x18)
#define DAVINCI_LDC_WR_OFST (0x1C)
#define DAVINCI_LDC_420C_RD_BASE (0x20)
#define DAVINCI_LDC_420C_WR_BASE (0x24)
#define DAVINCI_LDC_CONFIG (0x28)
#define DAVINCI_LDC_CENTER (0x2C)
#define DAVINCI_LDC_KHV (0x30)
#define DAVINCI_LDC_BLOCK (0x34)
#define DAVINCI_LDC_LUT_ADDR (0x38)
#define DAVINCI_LDC_LUT_WDATA (0x3C)
#define DAVINCI_LDC_LUT_RDATA (0x40)
#define DAVINCI_LDC_AB (0x44)
#define DAVINCI_LDC_CD (0x48)
#define DAVINCI_LDC_EF (0x4C)

#define LDC_AFFINE_SHIFT (16)
#define LDC_MAX_TILE_NUM (5)

#define ISNULL(val) ((val == NULL) ? 1:0)

/* LDC tile params */
struct ldc_tile_param_s {
	u16 initX;
	u16 initY;
	struct ldc_frame_param_s frm_params;
};

struct ldc_device_s {

	struct semaphore ldc_sem;
	spinlock_t lock;

	/* if single-pass mode, only first tile is used; 
	   if multi-pass mode, tile 4 is not processed but copied  */
	struct ldc_tile_param_s ldc_tiles[LDC_MAX_TILE_NUM];
	
	struct ldc_config_s ldc_config;
	struct ldc_affine_param_s ldc_affine_param;
	int users;
};

struct ldc_fh {
	char primary_user;
	/* This is primary used configured
	   the previewer channel
	 */
	struct imp_logical_channel *chan;
	/* channel associated with this file handle
	 */
};

#endif
#endif
