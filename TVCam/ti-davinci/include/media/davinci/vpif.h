/*
 *
 * Copyright (C) 2007 Texas Instruments Inc
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
/* vpif_capture.h */

#ifndef VPIF_H
#define VPIF_H

#ifdef __KERNEL__

/* Kernel Header files */
#include <asm/arch/hardware.h>
#include <asm/io.h>

/* Registers Base Address */
#define VPIF_IOBASE_ADDR                	IO_ADDRESS(0x01C12000)

/* Maximum channel allowed */
#define VPIF_NUM_CHANNELS               	4
#define VPIF_CAPTURE_NUM_CHANNELS		2
#define VPIF_DISPLAY_NUM_CHANNELS		2

/* Macros to read/write registers */
#define regr(reg)               inl((reg) + VPIF_IOBASE_ADDR)
#define regw(value, reg)        outl(value, (reg) + VPIF_IOBASE_ADDR)


/* Register Addresss */
#define VPIF_PID                                (0x0000)
#define VPIF_CH0_CTRL                           (0x0004)
#define VPIF_CH1_CTRL                           (0x0008)
#define VPIF_CH2_CTRL                           (0x000C)
#define VPIF_CH3_CTRL                           (0x0010)

#define VPIF_INTEN                              (0x0020)
#define VPIF_INTEN_SET                          (0x0024)
#define VPIF_INTEN_CLR                          (0x0028)
#define VPIF_STATUS                             (0x002C)
#define VPIF_STATUS_CLR                         (0x0030)
#define VPIF_EMULATION_CTRL                     (0x0034)
#define VPIF_REQ_SIZE                           (0x0038)

#define VPIF_CH0_TOP_STRT_ADD_LUMA              (0x0040)
#define VPIF_CH0_BTM_STRT_ADD_LUMA              (0x0044)
#define VPIF_CH0_TOP_STRT_ADD_CHROMA            (0x0048)
#define VPIF_CH0_BTM_STRT_ADD_CHROMA            (0x004c)
#define VPIF_CH0_TOP_STRT_ADD_HANC              (0x0050)
#define VPIF_CH0_BTM_STRT_ADD_HANC              (0x0054)
#define VPIF_CH0_TOP_STRT_ADD_VANC              (0x0058)
#define VPIF_CH0_BTM_STRT_ADD_VANC              (0x005c)
#define VPIF_CH0_SP_CFG                         (0x0060)
#define VPIF_CH0_IMG_ADD_OFST                   (0x0064)
#define VPIF_CH0_HANC_ADD_OFST                  (0x0068)
#define VPIF_CH0_H_CFG                          (0x006c)
#define VPIF_CH0_V_CFG_00                       (0x0070)
#define VPIF_CH0_V_CFG_01                       (0x0074)
#define VPIF_CH0_V_CFG_02                       (0x0078)
#define VPIF_CH0_V_CFG_03                       (0x007c)

#define VPIF_CH1_TOP_STRT_ADD_LUMA              (0x0080)
#define VPIF_CH1_BTM_STRT_ADD_LUMA              (0x0084)
#define VPIF_CH1_TOP_STRT_ADD_CHROMA            (0x0088)
#define VPIF_CH1_BTM_STRT_ADD_CHROMA            (0x008c)
#define VPIF_CH1_TOP_STRT_ADD_HANC              (0x0090)
#define VPIF_CH1_BTM_STRT_ADD_HANC              (0x0094)
#define VPIF_CH1_TOP_STRT_ADD_VANC              (0x0098)
#define VPIF_CH1_BTM_STRT_ADD_VANC              (0x009c)
#define VPIF_CH1_SP_CFG                         (0x00a0)
#define VPIF_CH1_IMG_ADD_OFST                   (0x00a4)
#define VPIF_CH1_HANC_ADD_OFST                  (0x00a8)
#define VPIF_CH1_H_CFG                          (0x00ac)
#define VPIF_CH1_V_CFG_00                       (0x00b0)
#define VPIF_CH1_V_CFG_01                       (0x00b4)
#define VPIF_CH1_V_CFG_02                       (0x00b8)
#define VPIF_CH1_V_CFG_03                       (0x00bc)

#define VPIF_CH2_TOP_STRT_ADD_LUMA              (0x00c0)
#define VPIF_CH2_BTM_STRT_ADD_LUMA              (0x00c4)
#define VPIF_CH2_TOP_STRT_ADD_CHROMA            (0x00c8)
#define VPIF_CH2_BTM_STRT_ADD_CHROMA            (0x00cc)
#define VPIF_CH2_TOP_STRT_ADD_HANC		(0x00d0)
#define VPIF_CH2_BTM_STRT_ADD_HANC              (0x00d4)
#define VPIF_CH2_TOP_STRT_ADD_VANC              (0x00d8)
#define VPIF_CH2_BTM_STRT_ADD_VANC              (0x00dc)
#define VPIF_CH2_SP_CFG                         (0x00e0)
#define VPIF_CH2_IMG_ADD_OFST                   (0x00e4)
#define VPIF_CH2_HANC_ADD_OFST                  (0x00e8)
#define VPIF_CH2_H_CFG                          (0x00ec)
#define VPIF_CH2_V_CFG_00                       (0x00f0)
#define VPIF_CH2_V_CFG_01                       (0x00f4)
#define VPIF_CH2_V_CFG_02                       (0x00f8)
#define VPIF_CH2_V_CFG_03                       (0x00fc)
#define VPIF_CH2_HANC0_STRT                     (0x0100)
#define VPIF_CH2_HANC0_SIZE                     (0x0104)
#define VPIF_CH2_HANC1_STRT                     (0x0108)
#define VPIF_CH2_HANC1_SIZE                     (0x010c)
#define VPIF_CH2_VANC0_STRT                     (0x0110)
#define VPIF_CH2_VANC0_SIZE                     (0x0114)
#define VPIF_CH2_VANC1_STRT                     (0x0118)
#define VPIF_CH2_VANC1_SIZE                     (0x011c)

#define VPIF_CH3_TOP_STRT_ADD_LUMA              (0x0140)
#define VPIF_CH3_BTM_STRT_ADD_LUMA              (0x0144)
#define VPIF_CH3_TOP_STRT_ADD_CHROMA            (0x0148)
#define VPIF_CH3_BTM_STRT_ADD_CHROMA            (0x014c)
#define VPIF_CH3_TOP_STRT_ADD_HANC              (0x0150)
#define VPIF_CH3_BTM_STRT_ADD_HANC              (0x0154)
#define VPIF_CH3_TOP_STRT_ADD_VANC              (0x0158)
#define VPIF_CH3_BTM_STRT_ADD_VANC              (0x015c)
#define VPIF_CH3_SP_CFG                         (0x0160)
#define VPIF_CH3_IMG_ADD_OFST                   (0x0164)
#define VPIF_CH3_HANC_ADD_OFST                  (0x0168)
#define VPIF_CH3_H_CFG                          (0x016c)
#define VPIF_CH3_V_CFG_00                       (0x0170)
#define VPIF_CH3_V_CFG_01                       (0x0174)
#define VPIF_CH3_V_CFG_02                       (0x0178)
#define VPIF_CH3_V_CFG_03                       (0x017c)
#define VPIF_CH3_HANC0_STRT                     (0x0180)
#define VPIF_CH3_HANC0_SIZE                     (0x0184)
#define VPIF_CH3_HANC1_STRT                     (0x0188)
#define VPIF_CH3_HANC1_SIZE                     (0x018c)
#define VPIF_CH3_VANC0_STRT                     (0x0190)
#define VPIF_CH3_VANC0_SIZE                     (0x0194)
#define VPIF_CH3_VANC1_STRT                     (0x0198)
#define VPIF_CH3_VANC1_SIZE                     (0x019c)

#define VPIF_IODFT_CTRL                         (0x01c0)

/* Macros for BIT Manipulation */
#define SETBIT(reg, bit)                ((reg) |= ((0x00000001)<<bit))
#define RESETBIT(reg, bit)              ((reg) &= (~((0x00000001)<<bit)))

/* Macros */
/* Macro for Generating mask */
#ifdef GENERATE_MASK
#undef GENERATE_MASK
#endif

#define GENERATE_MASK(bits, pos)        ((((0xFFFFFFFF) << (32-bits)) >> \
		(32-bits)) << pos)

/* Bit positions in the channel control registers */
#define VPIF_CH_DATA_MODE_BIT                   (2)
#define VPIF_CH_YC_MUX_BIT                      (3)
#define VPIF_CH_SDR_FMT_BIT                     (4)
#define VPIF_CH_HANC_EN_BIT                     (8)
#define VPIF_CH_VANC_EN_BIT                     (9)

#define VPIF_CAPTURE_CH_NIP			(10)
#define VPIF_DISPLAY_CH_NIP			(11)

#define VPIF_DISPLAY_PIX_EN_BIT			(10)

#define VPIF_CH_INPUT_FIELD_FRAME_BIT           (12)

#define VPIF_CH_FID_POLARITY_BIT		(15)
#define VPIF_CH_V_VALID_POLARITY_BIT		(14)
#define VPIF_CH_H_VALID_POLARITY_BIT		(13)
#define VPIF_CH_DATA_WIDTH_BIT			(28)

#define VPIF_CH_CLK_EDGE_CTRL_BIT               (31)

/* Mask various length */
#define VPIF_CH_EAVSAV_MASK             GENERATE_MASK(13, 0)
#define VPIF_CH_LEN_MASK                GENERATE_MASK(12, 0)
#define VPIF_CH_WIDTH_MASK              GENERATE_MASK(13, 0)
#define VPIF_CH_LEN_SHIFT               (16)

/* VPIF masks for registers */
#define VPIF_REQ_SIZE_MASK              (0x1ff)

/* enable/disables interrupt in vpif_ch_intr register */
#define VPIF_INTEN_FRAME_CH0		(0x00000001)
/* enable/disables interrupt in vpif_ch_intr register */
#define VPIF_INTEN_FRAME_CH1		(0x00000002)
/* bit position of clock enable in vpif_ch0_ctrl register */
#define VPIF_CH0_CLK_EN		        (0x00000002)
/* bit position to enable channel2 in vpif_ch0_ctrl register */
#define VPIF_CH0_EN		        (0x00000001)
/* bit position to enable channel2 in vpif_ch0_ctrl register */
#define VPIF_CH1_EN		        (0x00000001)
/* bit position of clock enable in vpif_ch0_ctrl register */
#define VPIF_CH1_CLK_EN			(0x00000002)

#define VPIF_CH_CLK_EN			(0x00000002)
#define VPIF_CH_EN		        (0x00000001)

/* enable/disables interrupt in vpif_ch_intr register */
#define VPIF_INTEN_FRAME_CH2		(0x00000004)
/* enable/disables interrupt in vpif_ch_intr register */
#define VPIF_INTEN_FRAME_CH3		(0x00000008)
/* bit position of clock enable in vpif_ch0_ctrl register */
#define VPIF_CH2_CLK_EN		        (0x00000002)
/* bit position to enable channel2 in vpif_ch0_ctrl register */
#define VPIF_CH2_EN		        (0x00000001)
/* bit position to enable channel2 in vpif_ch0_ctrl register */
#define VPIF_CH3_EN		        (0x00000001)
/* bit position of clock enable in vpif_ch0_ctrl register */
#define VPIF_CH3_CLK_EN			(0x00000002)


#define VPIF_INT_TOP			(0x00)
#define VPIF_INT_BOTTOM			(0x01)
#define VPIF_INT_BOTH			(0x02)

#define VPIF_CH0_INT_CTRL_SHIFT		6
#define VPIF_CH1_INT_CTRL_SHIFT		6
#define VPIF_CH2_INT_CTRL_SHIFT		6
#define VPIF_CH3_INT_CTRL_SHIFT		6
#define VPIF_CH_INT_CTRL_SHIFT		6
/* enabled interrupt on both the fields on vpid_ch0_ctrl register */
#define channel0_intr_assert() 		(regw((regr(VPIF_CH0_CTRL)|\
					(VPIF_INT_BOTH << \
					VPIF_CH0_INT_CTRL_SHIFT)), \
					VPIF_CH0_CTRL))

/* enabled interrupt on both the fields on vpid_ch1_ctrl register */
#define channel1_intr_assert() 		(regw((regr(VPIF_CH1_CTRL)|\
					(VPIF_INT_BOTH << \
					VPIF_CH1_INT_CTRL_SHIFT)), \
					VPIF_CH1_CTRL))

/* enabled interrupt on both the fields on vpid_ch0_ctrl register */
#define channel2_intr_assert() 		(regw((regr(VPIF_CH2_CTRL)|\
					(VPIF_INT_BOTH << \
					VPIF_CH2_INT_CTRL_SHIFT)), \
					VPIF_CH2_CTRL))

/* enabled interrupt on both the fields on vpid_ch1_ctrl register */
#define channel3_intr_assert() 		(regw((regr(VPIF_CH3_CTRL)|\
					(VPIF_INT_BOTH << \
					VPIF_CH3_INT_CTRL_SHIFT)), \
					VPIF_CH3_CTRL))


#define VPIF_CH_FID_MASK		0x20
#define VPIF_CH_FID_SHIFT		5

#define VPIF_NTSC_VBI_START_FIELD0	1
#define VPIF_NTSC_VBI_START_FIELD1	263
#define VPIF_PAL_VBI_START_FIELD0	624
#define VPIF_PAL_VBI_START_FIELD1	311

#define VPIF_NTSC_HBI_START_FIELD0	1
#define VPIF_NTSC_HBI_START_FIELD1	263
#define VPIF_PAL_HBI_START_FIELD0	624
#define VPIF_PAL_HBI_START_FIELD1	311

#define VPIF_NTSC_VBI_COUNT_FIELD0	20
#define VPIF_NTSC_VBI_COUNT_FIELD1	19
#define VPIF_PAL_VBI_COUNT_FIELD0	24
#define VPIF_PAL_VBI_COUNT_FIELD1	25

#define VPIF_NTSC_HBI_COUNT_FIELD0	263
#define VPIF_NTSC_HBI_COUNT_FIELD1	262
#define VPIF_PAL_HBI_COUNT_FIELD0	312
#define VPIF_PAL_HBI_COUNT_FIELD1	313

#define VPIF_NTSC_VBI_SAMPLES_PER_LINE	720
#define VPIF_PAL_VBI_SAMPLES_PER_LINE	720
#define VPIF_NTSC_HBI_SAMPLES_PER_LINE	268
#define VPIF_PAL_HBI_SAMPLES_PER_LINE	280

#define VPIF_CH_VANC_EN			0x20
#define VPIF_DMA_REQ_SIZE		0x080
#define VPIF_EMULATION_DISABLE		0x01

extern u8 irq_vpif_capture_channel[VPIF_NUM_CHANNELS];

/* inline function to enable/disable channel2 */
static inline void enable_channel0(int enable)
{
	if (enable) {
		/* Enable clock and channel2 */
		regw((regr(VPIF_CH0_CTRL) | (VPIF_CH0_EN)), VPIF_CH0_CTRL);
	} else {
		/* Disable clock and channel2 */
		regw((regr(VPIF_CH0_CTRL) & (~VPIF_CH0_EN)),
		     VPIF_CH0_CTRL);
	}
}

/* inline function to enable/disable channel2 */
static inline void enable_channel1(int enable)
{
	if (enable) {
		/* Enable clock and channel3 */
		regw((regr(VPIF_CH1_CTRL) | (VPIF_CH1_EN)), VPIF_CH1_CTRL);
	} else {
		/* Disable clock and channel3 */
		regw((regr(VPIF_CH1_CTRL) & (~VPIF_CH1_EN)),
		     VPIF_CH1_CTRL);
	}
}

/* inline function to enable interrupt for channel 2 */
static inline void channel0_intr_enable(int enable)
{
	if (enable) {

		regw((regr(VPIF_INTEN) | 0x10), VPIF_INTEN);
		regw((regr(VPIF_INTEN_SET) | 0x10), VPIF_INTEN_SET);

		regw((regr(VPIF_INTEN) | VPIF_INTEN_FRAME_CH0),
		     VPIF_INTEN);
		regw((regr(VPIF_INTEN_SET) | VPIF_INTEN_FRAME_CH0),
		     VPIF_INTEN_SET);
	} else {
		regw((regr(VPIF_INTEN) & (~VPIF_INTEN_FRAME_CH0)),
		     VPIF_INTEN);
		regw((regr(VPIF_INTEN_SET) | VPIF_INTEN_FRAME_CH0),
		     VPIF_INTEN_SET);
	}
}

/* inline function to enable interrupt for channel 3 */
static inline void channel1_intr_enable(int enable)
{
	if (enable) {
		regw((regr(VPIF_INTEN) | 0x10), VPIF_INTEN);
		regw((regr(VPIF_INTEN_SET) | 0x10), VPIF_INTEN_SET);

		regw((regr(VPIF_INTEN) | VPIF_INTEN_FRAME_CH1),
		     VPIF_INTEN);
		regw((regr(VPIF_INTEN_SET) | VPIF_INTEN_FRAME_CH1),
		     VPIF_INTEN_SET);
	} else {
		regw((regr(VPIF_INTEN) & (~VPIF_INTEN_FRAME_CH1)),
		     VPIF_INTEN);
		regw((regr(VPIF_INTEN_SET) | VPIF_INTEN_FRAME_CH1),
		     VPIF_INTEN_SET);
	}
}

/* static inline function to set buffer addresses in in case of Y/C
   non mux mode */
static inline void ch0_set_videobuf_addr_yc_nmux(unsigned long
						 top_strt_luma,
						 unsigned long
						 btm_strt_luma,
						 unsigned long
						 top_strt_chroma,
						 unsigned long
						 btm_strt_chroma)
{
	regw(top_strt_luma, VPIF_CH0_TOP_STRT_ADD_LUMA);
	regw(btm_strt_luma, VPIF_CH0_BTM_STRT_ADD_LUMA);
	regw(top_strt_chroma, VPIF_CH1_TOP_STRT_ADD_CHROMA);
	regw(btm_strt_chroma, VPIF_CH1_BTM_STRT_ADD_CHROMA);
}

/* static inline function to set buffer addresses in VPIF registers for
   video data */
static inline void ch0_set_videobuf_addr(unsigned long top_strt_luma,
					 unsigned long btm_strt_luma,
					 unsigned long top_strt_chroma,
					 unsigned long btm_strt_chroma)
{
	regw(top_strt_luma, VPIF_CH0_TOP_STRT_ADD_LUMA);
	regw(btm_strt_luma, VPIF_CH0_BTM_STRT_ADD_LUMA);
	regw(top_strt_chroma, VPIF_CH0_TOP_STRT_ADD_CHROMA);
	regw(btm_strt_chroma, VPIF_CH0_BTM_STRT_ADD_CHROMA);
}

static inline void ch1_set_videobuf_addr(unsigned long top_strt_luma,
					 unsigned long btm_strt_luma,
					 unsigned long top_strt_chroma,
					 unsigned long btm_strt_chroma)
{

	regw(top_strt_luma, VPIF_CH1_TOP_STRT_ADD_LUMA);
	regw(btm_strt_luma, VPIF_CH1_BTM_STRT_ADD_LUMA);
	regw(top_strt_chroma, VPIF_CH1_TOP_STRT_ADD_CHROMA);
	regw(btm_strt_chroma, VPIF_CH1_BTM_STRT_ADD_CHROMA);
}

static inline void ch0_set_vbi_addr(unsigned long top_vbi,
	unsigned long btm_vbi, unsigned long a, unsigned long b)
{
	regw(top_vbi, VPIF_CH0_TOP_STRT_ADD_VANC);
	regw(btm_vbi, VPIF_CH0_BTM_STRT_ADD_VANC);
}
static inline void ch0_set_hbi_addr(unsigned long top_vbi,
	unsigned long btm_vbi, unsigned long a, unsigned long b)
{
	regw(top_vbi, VPIF_CH0_TOP_STRT_ADD_HANC);
	regw(btm_vbi, VPIF_CH0_BTM_STRT_ADD_HANC);
}

static inline void ch1_set_vbi_addr(unsigned long top_vbi,
	unsigned long btm_vbi, unsigned long a, unsigned long b)
{
	regw(top_vbi, VPIF_CH1_TOP_STRT_ADD_VANC);
	regw(btm_vbi, VPIF_CH1_BTM_STRT_ADD_VANC);
}
static inline void ch1_set_hbi_addr(unsigned long top_vbi,
	unsigned long btm_vbi, unsigned long a, unsigned long b)
{
	regw(top_vbi, VPIF_CH1_TOP_STRT_ADD_HANC);
	regw(btm_vbi, VPIF_CH1_BTM_STRT_ADD_HANC);
}
/* Inline function to enable raw vbi in the given channel */
static inline void disable_raw_feature(u8 channel_id, u8 index)
{
	u32 ctrl_reg;
	u32 val;
	if(0 == channel_id)
		ctrl_reg = VPIF_CH0_CTRL;
	else
		ctrl_reg = VPIF_CH1_CTRL;
	val = regr(ctrl_reg);
	if(1 == index)
		RESETBIT(val, VPIF_CH_VANC_EN_BIT);
	else
		RESETBIT(val, VPIF_CH_HANC_EN_BIT);
	regw(val, ctrl_reg);

}

static inline void enable_raw_feature(u8 channel_id, u8 index)
{
	u32 ctrl_reg;
	u32 val;
	if(0 == channel_id)
		ctrl_reg = VPIF_CH0_CTRL;
	else
		ctrl_reg = VPIF_CH1_CTRL;

	val = regr(ctrl_reg);
	if(1 == index)
		SETBIT(val, VPIF_CH_VANC_EN_BIT);
	else
		SETBIT(val, VPIF_CH_HANC_EN_BIT);
	regw(val, ctrl_reg);

}

/* inline function to enable/disable channel2 */
static inline void enable_channel2(int enable)
{
	if (enable) {
		/* Enable clock and channel2 */
		regw((regr(VPIF_CH2_CTRL) | (VPIF_CH2_CLK_EN)),
		     VPIF_CH2_CTRL);
		regw((regr(VPIF_CH2_CTRL) | (VPIF_CH2_EN)), VPIF_CH2_CTRL);
	} else {
		/* Disable clock and channel2 */
		regw((regr(VPIF_CH2_CTRL) & (~VPIF_CH2_CLK_EN)),
		     VPIF_CH2_CTRL);
		regw((regr(VPIF_CH2_CTRL) & (~VPIF_CH2_EN)),
		     VPIF_CH2_CTRL);
	}
}

/* inline function to enable/disable channel2 */
static inline void enable_channel3(int enable)
{
	if (enable) {
		/* Enable clock and channel3 */
		regw((regr(VPIF_CH3_CTRL) | (VPIF_CH3_CLK_EN)),
		     VPIF_CH3_CTRL);
		regw((regr(VPIF_CH3_CTRL) | (VPIF_CH3_EN)), VPIF_CH3_CTRL);
	} else {
		/* Disable clock and channel3 */
		regw((regr(VPIF_CH3_CTRL) & (~VPIF_CH3_CLK_EN)),
		     VPIF_CH3_CTRL);
		regw((regr(VPIF_CH3_CTRL) & (~VPIF_CH3_EN)),
		     VPIF_CH3_CTRL);
	}
}

/* inline function to enable interrupt for channel 2 */
static inline void channel2_intr_enable(int enable)
{
	if (enable) {
		regw((regr(VPIF_INTEN) | 0x10), VPIF_INTEN);
		regw((regr(VPIF_INTEN_SET) | 0x10), VPIF_INTEN_SET);

		regw((regr(VPIF_INTEN) | VPIF_INTEN_FRAME_CH2),
		     VPIF_INTEN);
		regw((regr(VPIF_INTEN_SET) | VPIF_INTEN_FRAME_CH2),
		     VPIF_INTEN_SET);
	} else {
		regw((regr(VPIF_INTEN) & (~VPIF_INTEN_FRAME_CH2)),
		     VPIF_INTEN);
		regw((regr(VPIF_INTEN_SET) | VPIF_INTEN_FRAME_CH2),
		     VPIF_INTEN_SET);
	}
}

/* inline function to enable interrupt for channel 3 */
static inline void channel3_intr_enable(int enable)
{
	if (enable) {
		regw((regr(VPIF_INTEN) | 0x10), VPIF_INTEN);
		regw((regr(VPIF_INTEN_SET) | 0x10), VPIF_INTEN_SET);

		regw((regr(VPIF_INTEN) | VPIF_INTEN_FRAME_CH3),
		     VPIF_INTEN);
		regw((regr(VPIF_INTEN_SET) | VPIF_INTEN_FRAME_CH3),
		     VPIF_INTEN_SET);
	} else {
		regw((regr(VPIF_INTEN) & (~VPIF_INTEN_FRAME_CH3)),
		     VPIF_INTEN);
		regw((regr(VPIF_INTEN_SET) | VPIF_INTEN_FRAME_CH3),
		     VPIF_INTEN_SET);
	}
}

/* inline function to enable raw vbi data for channel 2 */
static inline void channel2_raw_enable(int enable, u8 index)
{
	u32 mask, val;
	if(1 == index)
		mask = VPIF_CH_VANC_EN_BIT;
	else
		mask = VPIF_CH_HANC_EN_BIT;
	val = regr(VPIF_CH2_CTRL);
	if (enable) {
		/*regw((regr(VPIF_CH3_CTRL) | mask), VPIF_CH3_CTRL);*/
		SETBIT(val, mask);
	} else {
		RESETBIT(val, mask);
	}
	regw(val, VPIF_CH2_CTRL);
}

/* inline function to enable raw vbi data for channel 3*/
static inline void channel3_raw_enable(int enable, u8 index)
{
	u32 mask, val;
	if(1 == index)
		mask = VPIF_CH_VANC_EN_BIT;
	else
		mask = VPIF_CH_HANC_EN_BIT;
	val = regr(VPIF_CH3_CTRL);
	if (enable) {
		/*regw((regr(VPIF_CH3_CTRL) | mask), VPIF_CH3_CTRL);*/
		SETBIT(val, mask);
	} else {
		RESETBIT(val, mask);
	}
	regw(val, VPIF_CH3_CTRL);
}

/* static inline function to set buffer addresses in in case of Y/C
   non mux mode */
static inline void ch2_set_videobuf_addr_yc_nmux(unsigned long
						 top_strt_luma,
						 unsigned long
						 btm_strt_luma,
						 unsigned long
						 top_strt_chroma,
						 unsigned long
						 btm_strt_chroma)
{
	regw(top_strt_luma, VPIF_CH2_TOP_STRT_ADD_LUMA);
	regw(btm_strt_luma, VPIF_CH2_BTM_STRT_ADD_LUMA);
	regw(top_strt_chroma, VPIF_CH3_TOP_STRT_ADD_CHROMA);
	regw(btm_strt_chroma, VPIF_CH3_BTM_STRT_ADD_CHROMA);
}

/* static inline function to set buffer addresses in VPIF registers for
   video data */
static inline void ch2_set_videobuf_addr(unsigned long top_strt_luma,
					 unsigned long btm_strt_luma,
					 unsigned long top_strt_chroma,
					 unsigned long btm_strt_chroma)
{
	regw(top_strt_luma, VPIF_CH2_TOP_STRT_ADD_LUMA);
	regw(btm_strt_luma, VPIF_CH2_BTM_STRT_ADD_LUMA);
	regw(top_strt_chroma, VPIF_CH2_TOP_STRT_ADD_CHROMA);
	regw(btm_strt_chroma, VPIF_CH2_BTM_STRT_ADD_CHROMA);

}

static inline void ch3_set_videobuf_addr(unsigned long top_strt_luma,
					 unsigned long btm_strt_luma,
					 unsigned long top_strt_chroma,
					 unsigned long btm_strt_chroma)
{
	regw(top_strt_luma, VPIF_CH3_TOP_STRT_ADD_LUMA);
	regw(btm_strt_luma, VPIF_CH3_BTM_STRT_ADD_LUMA);
	regw(top_strt_chroma, VPIF_CH3_TOP_STRT_ADD_CHROMA);
	regw(btm_strt_chroma, VPIF_CH3_BTM_STRT_ADD_CHROMA);

}

/* static inline function to set buffer addresses in VPIF registers for
   vbi data */
static inline void ch2_set_vbi_addr(unsigned long top_strt_luma,
					 unsigned long btm_strt_luma,
					 unsigned long top_strt_chroma,
					 unsigned long btm_strt_chroma)
{
	regw(top_strt_luma, VPIF_CH2_TOP_STRT_ADD_VANC);
	regw(btm_strt_luma, VPIF_CH2_BTM_STRT_ADD_VANC);
}

static inline void ch3_set_vbi_addr(unsigned long top_strt_luma,
					 unsigned long btm_strt_luma,
					 unsigned long top_strt_chroma,
					 unsigned long btm_strt_chroma)
{
	regw(top_strt_luma, VPIF_CH3_TOP_STRT_ADD_VANC);
	regw(btm_strt_luma, VPIF_CH3_BTM_STRT_ADD_VANC);
}

#define VPIF_MAX_NAME   30

/* This structure will store size parameters as per the mode selected by
   the user */
struct vpif_channel_config_params {
	char name[VPIF_MAX_NAME];	/* Name of the */
	u16 width;		/* Indicates width of the image for
				   this mode */
	u16 height;		/* Indicates height of the image for
				   this mode */
	u8 fps;
	u8 frm_fmt;		/* Indicates whether this is interlaced
				   or progressive format */
	u8 ycmux_mode;		/* Indicates whether this mode requires
				   single or two channels */
	u16 eav2sav;		/* length of sav 2 eav */
	u16 sav2eav;		/* length of sav 2 eav */
	u16 l1, l3, l5, l7, l9, l11;	/* Other parameter configurations */
	u16 vsize;		/* Vertical size of the image */
	u8 capture_format;	/* Indicates whether capture format
				   is in BT or in CCD/CMOS */
	u8  vbi_supported;	  /* Indicates whether this mode
				     supports capturing vbi or not */
	u8 hd_sd;
};

struct vpif_stdinfo {
	u8 channel_id;
	u32 activepixels;
	u32 activelines;
	u16 fps;
	u8 frame_format;
	char name[VPIF_MAX_NAME];
	u8 ycmux_mode;
	u8 vbi_supported;	  /* Indicates whether this mode
				     supports capturing vbi or not */
	u8 hd_sd;
};

struct vpif_interface;
struct vpif_params;
struct vpif_vbi_params;

int vpif_get_mode_info(struct vpif_stdinfo *std_info);

int vpif_set_video_params(struct vpif_params *, u8 channel_id);

int vpif_set_vbi_display_params(struct vpif_vbi_params *vbiparams,
	       	u8 channel_id);

int vpif_channel_getfid(u8 channel_id);

int vpif_get_irq_number(int);
#endif				/* End of #ifdef __KERNEL__ */

/* Enumerated data types */
typedef enum {
	VPIF_CAPTURE_PINPOL_SAME = 0,
	VPIF_CAPTURE_PINPOL_INVERT = 1
} vpif_capture_pinpol;

typedef enum {
	_8BITS = 0,
	_10BITS,
	_12BITS,
} data_size;

typedef struct {
	data_size data_sz;
	vpif_capture_pinpol fid_pol;
	vpif_capture_pinpol vd_pol;
	vpif_capture_pinpol hd_pol;
} vpif_capture_params_raw;

/* structure for vpif parameters */
struct vpif_interface {
	char name[25];
	__u8 storage_mode;	/* Indicates whether it is field or field
				   based storage mode */
	unsigned long hpitch;
};

/* Structure for vpif parameters for raw vbi data */
struct vpif_vbi_params {
	__u32 hstart0;  /* Horizontal start of raw vbi data for first field */
	__u32 vstart0;  /* Vertical start of raw vbi data for first field */
	__u32 hsize0;   /* Horizontal size of raw vbi data for first field */
	__u32 vsize0;   /* Vertical size of raw vbi data for first field */
	__u32 hstart1;  /* Horizontal start of raw vbi data for second field */
	__u32 vstart1;  /* Vertical start of raw vbi data for second field */
	__u32 hsize1;   /* Horizontal size of raw vbi data for second field */
	__u32 vsize1;   /* Vertical size of raw vbi data for second field */
};

struct vpif_params {
	struct vpif_interface video_params;
	union param{
		struct vpif_vbi_params      	vbi_params;
		vpif_capture_params_raw 	raw_params;
	}params;
};

#endif				/* End of #ifndef VPIF_H */

