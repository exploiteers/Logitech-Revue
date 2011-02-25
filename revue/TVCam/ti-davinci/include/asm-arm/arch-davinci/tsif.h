/*
 * Copyright (C) 2006 Texas Instruments Inc
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

#ifndef __ASM_ARCH_TSIF_H
#define __ASM_ARCH_TSIF_H

#include <linux/types.h>
#include <linux/wait.h>
#include <linux/io.h>
#include <asm/arch/hardware.h>

/* TSIF Base Address */
#define TSIF_BASE       0x01C13000
#define TSIF_VIRT_BASE  0xE1013000
/* Number of PID Filters */
#define TSIF_NUM_PID_FILTER	7
/* Channel number of other PID packets */
#define TSIF_OTHER_CH         	7
#define TSIF_MAJORS		2
#define TSIF_MINORS		11

extern unsigned int tsif_control_minor_count;
extern unsigned int tsif_data_minor_count;

struct tsif_irq_data {
#ifdef __KERNEL__
	struct completion pat_complete;
	struct completion pmt_complete;
	struct completion spcpkt_complete;
#endif				/* __KERNEL__ */
};

extern unsigned int tsif_device_count;

#define ENDIAN 1		/* 0:Big Endian,1:Little Endian */

#if ENDIAN == 0			/* Big Endian */
#define SWAP_BIG_ENDIAN32(a) (a)
#define SWAP_BIG_ENDIAN16(a) (a)
#else				/* Little Endian */
#define SWAP_BIG_ENDIAN32(a)			\
		((((a)>>24)&0x000000ff)	|	\
		(((a)>>8)&0x0000ff00)	|	\
		(((a)<<8)&0x00ff0000)	|	\
		(((a)<<24)&0xff000000))
#define SWAP_BIG_ENDIAN16(a) ((((a)>>8)&0x00ff)|(((a)<<8)&0xff00))
#endif

/*******************************************************************************
*       Constant/Macro Define                                                  *
*******************************************************************************/
#define TSIF_IO(device, offset)						\
	(IO_ADDRESS((DAVINCI_DM646X_TSIF0_BASE + device * 0x400 + offset * 4)))

/****   TSIF Registers  **/
#define TSIF1_IO_PID0_FLT_CFG           (TSIF_IO(0, 0x11))
#define TSIF1_IO_PID1_FLT_CFG           (TSIF_IO(0, 0x12))
#define TSIF1_IO_PID2_FLT_CFG           (TSIF_IO(0, 0x13))
#define TSIF1_IO_PID3_FLT_CFG           (TSIF_IO(0, 0x14))
#define TSIF1_IO_PID4_FLT_CFG           (TSIF_IO(0, 0x15))
#define TSIF1_IO_PID5_FLT_CFG           (TSIF_IO(0, 0x16))
#define TSIF1_IO_PID6_FLT_CFG           (TSIF_IO(0, 0x17))
#define TSIF1_IO_WRB0_STRT_ADD          (TSIF_IO(0, 0x31))
#define TSIF1_IO_WRB0_END_ADD           (TSIF_IO(0, 0x32))
#define TSIF1_IO_WRB0_RP_ADD            (TSIF_IO(0, 0x33))
#define TSIF1_IO_WRB0_SUB_ADD           (TSIF_IO(0, 0x34))
#define TSIF1_IO_WRB0_WP_ADD            (TSIF_IO(0, 0x35))
#define TSIF1_IO_WRB1_STRT_ADD          (TSIF_IO(0, 0x38))
#define TSIF1_IO_WRB1_END_ADD           (TSIF_IO(0, 0x39))
#define TSIF1_IO_WRB1_RP_ADD            (TSIF_IO(0, 0x3a))
#define TSIF1_IO_WRB1_SUB_ADD           (TSIF_IO(0, 0x3b))
#define TSIF1_IO_WRB1_WP_ADD            (TSIF_IO(0, 0x3c))
#define TSIF1_IO_WRB2_STRT_ADD          (TSIF_IO(0, 0x40))
#define TSIF1_IO_WRB2_END_ADD           (TSIF_IO(0, 0x41))
#define TSIF1_IO_WRB2_RP_ADD            (TSIF_IO(0, 0x42))
#define TSIF1_IO_WRB2_SUB_ADD           (TSIF_IO(0, 0x43))
#define TSIF1_IO_WRB2_WP_ADD            (TSIF_IO(0, 0x44))
#define TSIF1_IO_WRB3_STRT_ADD          (TSIF_IO(0, 0x48))
#define TSIF1_IO_WRB3_END_ADD           (TSIF_IO(0, 0x49))
#define TSIF1_IO_WRB3_RP_ADD            (TSIF_IO(0, 0x4a))
#define TSIF1_IO_WRB3_SUB_ADD           (TSIF_IO(0, 0x4b))
#define TSIF1_IO_WRB3_WP_ADD            (TSIF_IO(0, 0x4c))
#define TSIF1_IO_WRB4_STRT_ADD          (TSIF_IO(0, 0x50))
#define TSIF1_IO_WRB4_END_ADD           (TSIF_IO(0, 0x51))
#define TSIF1_IO_WRB4_RP_ADD            (TSIF_IO(0, 0x52))
#define TSIF1_IO_WRB4_SUB_ADD           (TSIF_IO(0, 0x53))
#define TSIF1_IO_WRB4_WP_ADD            (TSIF_IO(0, 0x54))
#define TSIF1_IO_WRB5_STRT_ADD          (TSIF_IO(0, 0x58))
#define TSIF1_IO_WRB5_END_ADD           (TSIF_IO(0, 0x59))
#define TSIF1_IO_WRB5_RP_ADD            (TSIF_IO(0, 0x5a))
#define TSIF1_IO_WRB5_SUB_ADD           (TSIF_IO(0, 0x5b))
#define TSIF1_IO_WRB5_WP_ADD            (TSIF_IO(0, 0x5c))
#define TSIF1_IO_WRB6_STRT_ADD          (TSIF_IO(0, 0x60))
#define TSIF1_IO_WRB6_END_ADD           (TSIF_IO(0, 0x61))
#define TSIF1_IO_WRB6_RP_ADD            (TSIF_IO(0, 0x62))
#define TSIF1_IO_WRB6_SUB_ADD           (TSIF_IO(0, 0x63))
#define TSIF1_IO_WRB6_WP_ADD            (TSIF_IO(0, 0x64))
#define TSIF1_IO_WRB7_STRT_ADD          (TSIF_IO(0, 0x68))
#define TSIF1_IO_WRB7_END_ADD           (TSIF_IO(0, 0x69))
#define TSIF1_IO_WRB7_RP_ADD            (TSIF_IO(0, 0x6a))
#define TSIF1_IO_WRB7_SUB_ADD           (TSIF_IO(0, 0x6b))
#define TSIF1_IO_WRB7_WP_ADD            (TSIF_IO(0, 0x6c))

#define TSIF2_IO_PID0_FLT_CFG           (TSIF_IO(1, 0x11))
#define TSIF2_IO_PID1_FLT_CFG           (TSIF_IO(1, 0x12))
#define TSIF2_IO_PID2_FLT_CFG           (TSIF_IO(1, 0x13))
#define TSIF2_IO_PID3_FLT_CFG           (TSIF_IO(1, 0x14))
#define TSIF2_IO_PID4_FLT_CFG           (TSIF_IO(1, 0x15))
#define TSIF2_IO_PID5_FLT_CFG           (TSIF_IO(1, 0x16))
#define TSIF2_IO_PID6_FLT_CFG           (TSIF_IO(1, 0x17))
#define TSIF2_IO_WRB0_STRT_ADD          (TSIF_IO(1, 0x31))
#define TSIF2_IO_WRB0_END_ADD           (TSIF_IO(1, 0x32))
#define TSIF2_IO_WRB0_RP_ADD            (TSIF_IO(1, 0x33))
#define TSIF2_IO_WRB0_SUB_ADD           (TSIF_IO(1, 0x34))
#define TSIF2_IO_WRB0_WP_ADD            (TSIF_IO(1, 0x35))
#define TSIF2_IO_WRB1_STRT_ADD          (TSIF_IO(1, 0x38))
#define TSIF2_IO_WRB1_END_ADD           (TSIF_IO(1, 0x39))
#define TSIF2_IO_WRB1_RP_ADD            (TSIF_IO(1, 0x3a))
#define TSIF2_IO_WRB1_SUB_ADD           (TSIF_IO(1, 0x3b))
#define TSIF2_IO_WRB1_WP_ADD            (TSIF_IO(1, 0x3c))
#define TSIF2_IO_WRB2_STRT_ADD          (TSIF_IO(1, 0x40))
#define TSIF2_IO_WRB2_END_ADD           (TSIF_IO(1, 0x41))
#define TSIF2_IO_WRB2_RP_ADD            (TSIF_IO(1, 0x42))
#define TSIF2_IO_WRB2_SUB_ADD           (TSIF_IO(1, 0x43))
#define TSIF2_IO_WRB2_WP_ADD            (TSIF_IO(1, 0x44))
#define TSIF2_IO_WRB3_STRT_ADD          (TSIF_IO(1, 0x48))
#define TSIF2_IO_WRB3_END_ADD           (TSIF_IO(1, 0x49))
#define TSIF2_IO_WRB3_RP_ADD            (TSIF_IO(1, 0x4a))
#define TSIF2_IO_WRB3_SUB_ADD           (TSIF_IO(1, 0x4b))
#define TSIF2_IO_WRB3_WP_ADD            (TSIF_IO(1, 0x4c))
#define TSIF2_IO_WRB4_STRT_ADD          (TSIF_IO(1, 0x50))
#define TSIF2_IO_WRB4_END_ADD           (TSIF_IO(1, 0x51))
#define TSIF2_IO_WRB4_RP_ADD            (TSIF_IO(1, 0x52))
#define TSIF2_IO_WRB4_SUB_ADD           (TSIF_IO(1, 0x53))
#define TSIF2_IO_WRB4_WP_ADD            (TSIF_IO(1, 0x54))
#define TSIF2_IO_WRB5_STRT_ADD          (TSIF_IO(1, 0x58))
#define TSIF2_IO_WRB5_END_ADD           (TSIF_IO(1, 0x59))
#define TSIF2_IO_WRB5_RP_ADD            (TSIF_IO(1, 0x5a))
#define TSIF2_IO_WRB5_SUB_ADD           (TSIF_IO(1, 0x5b))
#define TSIF2_IO_WRB5_WP_ADD            (TSIF_IO(1, 0x5c))
#define TSIF2_IO_WRB6_STRT_ADD          (TSIF_IO(1, 0x60))
#define TSIF2_IO_WRB6_END_ADD           (TSIF_IO(1, 0x61))
#define TSIF2_IO_WRB6_RP_ADD            (TSIF_IO(1, 0x62))
#define TSIF2_IO_WRB6_SUB_ADD           (TSIF_IO(1, 0x63))
#define TSIF2_IO_WRB6_WP_ADD            (TSIF_IO(1, 0x64))
#define TSIF2_IO_WRB7_STRT_ADD          (TSIF_IO(1, 0x68))
#define TSIF2_IO_WRB7_END_ADD           (TSIF_IO(1, 0x69))
#define TSIF2_IO_WRB7_RP_ADD            (TSIF_IO(1, 0x6a))
#define TSIF2_IO_WRB7_SUB_ADD           (TSIF_IO(1, 0x6b))
#define TSIF2_IO_WRB7_WP_ADD            (TSIF_IO(1, 0x6c))

/* PID Filter Config Register Table */
static unsigned int const tsif_pid_flt_cfg[2][7] = {
	{
		TSIF1_IO_PID0_FLT_CFG,
		TSIF1_IO_PID1_FLT_CFG,
		TSIF1_IO_PID2_FLT_CFG,
		TSIF1_IO_PID3_FLT_CFG,
		TSIF1_IO_PID4_FLT_CFG,
		TSIF1_IO_PID5_FLT_CFG,
		TSIF1_IO_PID6_FLT_CFG
	},
	{
		TSIF2_IO_PID0_FLT_CFG,
		TSIF2_IO_PID1_FLT_CFG,
		TSIF2_IO_PID2_FLT_CFG,
		TSIF2_IO_PID3_FLT_CFG,
		TSIF2_IO_PID4_FLT_CFG,
		TSIF2_IO_PID5_FLT_CFG,
		TSIF2_IO_PID6_FLT_CFG
	}
};

/* Write Ring Buffer Start Address Register Table */
static unsigned int const tsif_wrb_start_addr[2][8] = {
	{
		TSIF1_IO_WRB0_STRT_ADD,
		TSIF1_IO_WRB1_STRT_ADD,
		TSIF1_IO_WRB2_STRT_ADD,
		TSIF1_IO_WRB3_STRT_ADD,
		TSIF1_IO_WRB4_STRT_ADD,
		TSIF1_IO_WRB5_STRT_ADD,
		TSIF1_IO_WRB6_STRT_ADD,
		TSIF1_IO_WRB7_STRT_ADD
	},
	{
		TSIF2_IO_WRB0_STRT_ADD,
		TSIF2_IO_WRB1_STRT_ADD,
		TSIF2_IO_WRB2_STRT_ADD,
		TSIF2_IO_WRB3_STRT_ADD,
		TSIF2_IO_WRB4_STRT_ADD,
		TSIF2_IO_WRB5_STRT_ADD,
		TSIF2_IO_WRB6_STRT_ADD,
		TSIF2_IO_WRB7_STRT_ADD
	}
};

/* Write End Address Register Table */
static unsigned int const tsif_wrb_end_addr[2][8] = {
	{
		TSIF1_IO_WRB0_END_ADD,
		TSIF1_IO_WRB1_END_ADD,
		TSIF1_IO_WRB2_END_ADD,
		TSIF1_IO_WRB3_END_ADD,
		TSIF1_IO_WRB4_END_ADD,
		TSIF1_IO_WRB5_END_ADD,
		TSIF1_IO_WRB6_END_ADD,
		TSIF1_IO_WRB7_END_ADD
	},
	{
		TSIF2_IO_WRB0_END_ADD,
		TSIF2_IO_WRB1_END_ADD,
		TSIF2_IO_WRB2_END_ADD,
		TSIF2_IO_WRB3_END_ADD,
		TSIF2_IO_WRB4_END_ADD,
		TSIF2_IO_WRB5_END_ADD,
		TSIF2_IO_WRB6_END_ADD,
		TSIF2_IO_WRB7_END_ADD
	}
};

/* Write Ring Buffer Read Address Register Table */
static unsigned int const tsif_wrb_read_addr[2][8] = {
	{
		TSIF1_IO_WRB0_RP_ADD,
		TSIF1_IO_WRB1_RP_ADD,
		TSIF1_IO_WRB2_RP_ADD,
		TSIF1_IO_WRB3_RP_ADD,
		TSIF1_IO_WRB4_RP_ADD,
		TSIF1_IO_WRB5_RP_ADD,
		TSIF1_IO_WRB6_RP_ADD,
		TSIF1_IO_WRB7_RP_ADD
	},
	{
		TSIF2_IO_WRB0_RP_ADD,
		TSIF2_IO_WRB1_RP_ADD,
		TSIF2_IO_WRB2_RP_ADD,
		TSIF2_IO_WRB3_RP_ADD,
		TSIF2_IO_WRB4_RP_ADD,
		TSIF2_IO_WRB5_RP_ADD,
		TSIF2_IO_WRB6_RP_ADD,
		TSIF2_IO_WRB7_RP_ADD
	}
};

/* Write Ring Buffer Subtraction Register Table */
static unsigned int const tsif_wrb_subtraction[2][8] = {
	{
		TSIF1_IO_WRB0_SUB_ADD,
		TSIF1_IO_WRB1_SUB_ADD,
		TSIF1_IO_WRB2_SUB_ADD,
		TSIF1_IO_WRB3_SUB_ADD,
		TSIF1_IO_WRB4_SUB_ADD,
		TSIF1_IO_WRB5_SUB_ADD,
		TSIF1_IO_WRB6_SUB_ADD,
		TSIF1_IO_WRB7_SUB_ADD
	},
	{
		TSIF2_IO_WRB0_SUB_ADD,
		TSIF2_IO_WRB1_SUB_ADD,
		TSIF2_IO_WRB2_SUB_ADD,
		TSIF2_IO_WRB3_SUB_ADD,
		TSIF2_IO_WRB4_SUB_ADD,
		TSIF2_IO_WRB5_SUB_ADD,
		TSIF2_IO_WRB6_SUB_ADD,
		TSIF2_IO_WRB7_SUB_ADD
	}
};

/* Write Ring Buffer Present Address Register Table */
static unsigned int const tsif_wrb_present_addr[2][8] = {
	{
		TSIF1_IO_WRB0_WP_ADD,
		TSIF1_IO_WRB1_WP_ADD,
		TSIF1_IO_WRB2_WP_ADD,
		TSIF1_IO_WRB3_WP_ADD,
		TSIF1_IO_WRB4_WP_ADD,
		TSIF1_IO_WRB5_WP_ADD,
		TSIF1_IO_WRB6_WP_ADD,
		TSIF1_IO_WRB7_WP_ADD
	},
	{
		TSIF2_IO_WRB0_WP_ADD,
		TSIF2_IO_WRB1_WP_ADD,
		TSIF2_IO_WRB2_WP_ADD,
		TSIF2_IO_WRB3_WP_ADD,
		TSIF2_IO_WRB4_WP_ADD,
		TSIF2_IO_WRB5_WP_ADD,
		TSIF2_IO_WRB6_WP_ADD,
		TSIF2_IO_WRB7_WP_ADD
	}
};

#define PID			(0x00)
#define CTRL0			(0x04)
#define CTRL1			(0x08)
#define INTEN			(0x0C)
#define INTEN_SET		(0x10)
#define INTEN_CLR		(0x14)
#define INT_STATUS		(0x18)
#define INT_STATUS_CLR		(0x1C)
#define EMULATION_CTRL		(0x20)
#define ASYNC_TX_WAIT		(0x24)
#define PAT_SENSE_CFG		(0x28)
#define PAT_STORE_ADD		(0x2C)
#define PMT_SENSE_CFG		(0x30)
#define PMT_STORE_ADD		(0x34)
#define BSP_PID			(0x38)
#define BSP_STORE_ADD		(0x3C)
#define PCR_SENSE_CFG		(0x40)
#define PID0_FLT_CFG		(0x44)
#define PID1_FLT_CFG		(0x48)
#define PID2_FLT_CFG		(0x4C)
#define PID3_FLT_CFG		(0x50)
#define PID4_FLT_CFG		(0x54)
#define PID5_FLT_CFG		(0x58)
#define PID6_FLT_CFG		(0x5C)
#define BP_MODE_CFG		(0x60)
#define TX_ATS_INIT		(0x64)
#define TX_ATS_MONITOR		(0x68)
#define VBUS_WR_STATUS		(0x6C)
#define RCV_PKT_STAT		(0x70)
#define STC_CTRL		(0x80)
#define STC_INIT		(0x84)
#define STC_INTVAL_0		(0x88)
#define STC_INTVAL_1		(0x8C)
#define STC_INTVAL_2		(0x90)
#define STC_INTVAL_3		(0x94)
#define STC_INTVAL_4		(0x98)
#define STC_INTVAL_5		(0x9C)
#define STC_INTVAL_6		(0xA0)
#define STC_INTVAL_7		(0xA4)
#define WRB_CH_CTRL		(0xC0)
#define WRB0_STRT_ADD		(0xC4)
#define WRB0_END_ADD		(0xC8)
#define WRB0_RP_ADD		(0xCC)
#define WRB0_SUB_ADD		(0xD0)
#define WRB0_WP_ADD		(0xD4)
#define WRB1_STRT_ADD		(0xE0)
#define WRB1_END_ADD		(0xE4)
#define WRB1_RP_ADD		(0xE8)
#define WRB1_SUB_ADD		(0xEC)
#define WRB1_WP_ADD		(0xF0)
#define WRB2_STRT_ADD		(0x100)
#define WRB2_END_ADD		(0x104)
#define WRB2_RP_ADD		(0x108)
#define WRB2_SUB_ADD		(0x10C)
#define WRB2_WP_ADD		(0x110)
#define WRB3_STRT_ADD		(0x120)
#define WRB3_END_ADD		(0x124)
#define WRB3_RP_ADD		(0x128)
#define WRB3_SUB_ADD		(0x12C)
#define WRB3_WP_ADD		(0x130)
#define WRB4_STRT_ADD		(0x140)
#define WRB4_END_ADD		(0x144)
#define WRB4_RP_ADD		(0x148)
#define WRB4_SUB_ADD		(0x14C)
#define WRB4_WP_ADD		(0x150)
#define WRB5_STRT_ADD		(0x160)
#define WRB5_END_ADD		(0x164)
#define WRB5_RP_ADD		(0x168)
#define WRB5_SUB_ADD		(0x16C)
#define WRB5_WP_ADD		(0x170)
#define WRB6_STRT_ADD		(0x180)
#define WRB6_END_ADD		(0x184)
#define WRB6_RP_ADD		(0x188)
#define WRB6_SUB_ADD		(0x18C)
#define WRB6_WP_ADD		(0x190)
#define WRB7_STRT_ADD		(0x1A0)
#define WRB7_END_ADD		(0x1A4)
#define WRB7_RP_ADD		(0x1A8)
#define WRB7_SUB_ADD		(0x1AC)
#define WRB7_WP_ADD		(0x1B0)
#define RRB_CH_CTRL		(0x1C0)
#define RRB0_STRT_ADD		(0x1C4)
#define RRB0_END_ADD		(0x1C8)
#define RRB0_WP_ADD		(0x1CC)
#define RRB0_SUB_ADD		(0x1D0)
#define RRB0_RP_ADD		(0x1D4)
#define PKT_CNT_VAL		(0x1D8)
#define TST_BUF_CTRL		(0x1E8)
#define TST_RXBUF_U		(0x1EC)
#define TST_RXBUF_L		(0x1F0)
#define TST_TXBUF_U		(0x1F4)
#define TST_TXBUF_L		(0x1F8)
#define TST_CTRL		(0x1FC)

/**************************************************************************\
* Field Definition Macros
\**************************************************************************/

#define TSIF_RING_BUF_WR_CH_CTL_EN_ACTIVATE        0x1
#define TSIF_CTRL0_TX_CLK_INV_MASK                 (0x80000000u)
#define TSIF_CTRL0_TX_PKT_SIZE                     (0x07000000u)
#define TSIF_CTRL0_TX_ATS_MODE                     (0x00C00000u)
#define TSIF_CTRL0_TX_STREAM_MODE                  (0x00200000u)
#define TSIF_CTRL0_TX_PKTSTRT_WIDTH                (0x00100000u)
#define TSIF_CTRL0_TX_IF_MODE                      (0x000C0000u)
#define TSIF_CTRL0_TX_DMA_CTL                      (0x00020000u)
#define TSIF_CTRL0_TX_CTL                          (0x00010000u)
#define TSIF_CTRL0_RCV_PKT_SIZE                    (0x00000700u)
#define TSIF_CTRL0_RCV_ATS_MODE                    (0x000000C0u)
#define TSIF_CTRL0_RCV_STREAM_MODE                 (0x00000020u)
#define TSIF_CTRL0_RCV_INPUT_PORT                  (0x00000010u)
#define TSIF_CTRL0_RCV_IF_MODE                     (0x0000000Cu)
#define TSIF_CTRL0_RCV_DMA_CTL                     (0x00000002u)
#define TSIF_CTRL0_RCV_CTL                         (0x00000001u)
#define TSIF_CTRL0_RESETVAL                        (0x00000000u)
#define TSIF_CTRL1_STREAM_BNDRY_CTL                (0x00000010u)
#define TSIF_CTRL1_PID_FILTER_CTL                  (0x00000007u)
#define TSIF_CTRL1_RESETVAL                        (0x00000000u)
#define TSIF_INTEN_RBW7_FULL_INTEN                 (0x00008000u)
#define TSIF_INTEN_RBW6_FULL_INTEN                 (0x00004000u)
#define TSIF_INTEN_RBW5_FULL_INTEN                 (0x00002000u)
#define TSIF_INTEN_RBW4_FULL_INTEN                 (0x00001000u)
#define TSIF_INTEN_RBW3_FULL_INTEN                 (0x00000800u)
#define TSIF_INTEN_RBW2_FULL_INTEN                 (0x00000400u)
#define TSIF_INTEN_RBW1_FULL_INTEN                 (0x00000200u)
#define TSIF_INTEN_RBW0_FULL_INTEN                 (0x00000100u)
#define TSIF_INTEN_RBR0_FULL_INTEN                 (0x00000010u)
#define TSIF_INTEN_PMT_DETECT_INTEN                (0x00000008u)
#define TSIF_INTEN_PAT_DETECT_INTEN                (0x00000004u)
#define TSIF_INTEN_GOP_START_INTEN                 (0x00000002u)
#define TSIF_INTEN_RESETVAL                        (0x00000000u)
#define TSIF_INTEN_SET_STC_07_INTEN_SET            (0x00800000u)
#define TSIF_INTEN_SET_STC_06_INTEN_SET            (0x00400000u)
#define TSIF_INTEN_SET_STC_05_INTEN_SET            (0x00200000u)
#define TSIF_INTEN_SET_STC_04_INTEN_SET            (0x00100000u)
#define TSIF_INTEN_SET_STC_03_INTEN_SET            (0x00080000u)
#define TSIF_INTEN_SET_STC_02_INTEN_SET            (0x00040000u)
#define TSIF_INTEN_SET_STC_01_INTEN_SET            (0x00020000u)
#define TSIF_INTEN_SET_STC_00_INTEN_SET            (0x00010000u)
#define TSIF_INTEN_SET_RBW7_FULL_INTEN_SET         (0x00008000u)
#define TSIF_INTEN_SET_RBW6_FULL_INTEN_SET         (0x00004000u)
#define TSIF_INTEN_SET_RBW5_FULL_INTEN_SET         (0x00002000u)
#define TSIF_INTEN_SET_RBW4_FULL_INTEN_SET         (0x00001000u)
#define TSIF_INTEN_SET_RBW3_FULL_INTEN_SET         (0x00000800u)
#define TSIF_INTEN_SET_RBW2_FULL_INTEN_SET         (0x00000400u)
#define TSIF_INTEN_SET_RBW1_FULL_INTEN_SET         (0x00000200u)
#define TSIF_INTEN_SET_RBW0_FULL_INTEN_SET         (0x00000100u)
#define TSIF_INTEN_SET_RCV_PKTERR_INTEN_SET        (0x00000040u)
#define TSIF_INTEN_SET_VBUS_ERR_INTEN_SET          (0x00000020u)
#define TSIF_INTEN_SET_RBR0_FULL_INTEN_SET         (0x00000010u)
#define TSIF_INTEN_SET_PMT_DETECT_INTEN_SET        (0x00000008u)
#define TSIF_INTEN_SET_PAT_DETECT_INTEN_SET        (0x00000004u)
#define TSIF_INTEN_SET_GOP_START_INTEN_SET         (0x00000002u)
#define TSIF_INTEN_SET_BOUNDARY_SPECIFIC_INTEN_SET (0x00000001u)
#define TSIF_INTEN_CLR_STC_07_INTEN_CLR            (0x00800000u)
#define TSIF_INTEN_CLR_STC_06_INTEN_CLR            (0x00400000u)
#define TSIF_INTEN_CLR_STC_05_INTEN_CLR            (0x00200000u)
#define TSIF_INTEN_CLR_STC_04_INTEN_CLR            (0x00100000u)
#define TSIF_INTEN_CLR_STC_03_INTEN_CLR            (0x00080000u)
#define TSIF_INTEN_CLR_STC_02_INTEN_CLR            (0x00040000u)
#define TSIF_INTEN_CLR_STC_01_INTEN_CLR            (0x00020000u)
#define TSIF_INTEN_CLR_STC_00_INTEN_CLR            (0x00010000u)
#define TSIF_INTEN_CLR_RBW7_FULL_INTEN_CLR         (0x00008000u)
#define TSIF_INTEN_CLR_RBW6_FULL_INTEN_CLR         (0x00004000u)
#define TSIF_INTEN_CLR_RBW5_FULL_INTEN_CLR         (0x00002000u)
#define TSIF_INTEN_CLR_RBW4_FULL_INTEN_CLR         (0x00001000u)
#define TSIF_INTEN_CLR_RBW3_FULL_INTEN_CLR         (0x00000800u)
#define TSIF_INTEN_CLR_RBW2_FULL_INTEN_CLR         (0x00000400u)
#define TSIF_INTEN_CLR_RBW1_FULL_INTEN_CLR         (0x00000200u)
#define TSIF_INTEN_CLR_RBW0_FULL_INTEN_CLR         (0x00000100u)
#define TSIF_INTEN_CLR_RCV_PKTERR_INTEN_CLR        (0x00000040u)
#define TSIF_INTEN_CLR_VBUS_ERR_INTEN_CLR          (0x00000020u)
#define TSIF_INTEN_CLR_RBR0_FULL_INTEN_CLR         (0x00000010u)
#define TSIF_INTEN_CLR_PMT_DETECT_INTEN_CLR        (0x00000008u)
#define TSIF_INTEN_CLR_PAT_DETECT_INTEN_CLR        (0x00000004u)
#define TSIF_INTEN_CLR_GOP_START_INTEN_CLR         (0x00000002u)
#define TSIF_INTEN_CLR_BOUNDARY_SPECIFIC_INTEN_CLR (0x00000001u)
#define TSIF_STATUS_PMT_DETECT_STATUS              (0x00000008u)
#define TSIF_STATUS_PAT_DETECT_STATUS              (0x00000004u)
#define TSIF_EMU_CTRL_TSIF_EMULSUSP_FREE           (0x00000001u)
#define TSIF_ASYNC_TX_WAIT_RESETVAL                (0x00000000u)
#define TSIF_PAT_SENSE_CFG_PAT_SENSE_EN            (0x00010000u)
#define TSIF_PAT_SENSE_CFG_RESETVAL                (0x00000000u)
#define TSIF_PAT_STORE_ADDRESS_RESETVAL            (0x00000000u)
#define TSIF_PMT_SENSE_CFG_PMT_SENSE_EN            (0x00010000u)
#define TSIF_PMT_SENSE_CFG_RESETVAL                (0x00000000u)
#define TSIF_PMT_STORE_ADDRESS_RESETVAL            (0x00000000u)
#define TSIF_PCR_SENSE_CFG_PCR_SENSE_EN            (0x00010000u)
#define TSIF_PCR_SENSE_CFG_RESETVAL                (0x00000000u)
#define TSIF_BSP_STORE_ADDR_RESETVAL               (0x00000000u)
#define TSIF_TX_ATS_INIT_TX_ATS_INIT_EN            (0x40000000u)
#define TSIF_RING_BUFFER_WRITE_CHANNEL_CONTROL_RESETVAL (0x00000000u)
#define TSIF_RING_BUF_WR_CH7_START_ADDR_RESETVAL        (0x00000000u)
#define TSIF_RING_BUF_WR_CH7_END_ADDR_RESETVAL          (0x00000000u)
#define TSIF_RING_BUF_WR_CH7_RD_POINT_ADDR_RESETVAL     (0x00000000u)
#define TSIF_RING_BUF_WR_CH7_SUB_RESETVAL               (0x00000000u)
#define TSIF_RING_BUF_RD_CH_CTL_RESETVAL                (0x00000000u)
#define TSIF_RING_BUF_RD_CH0_START_ADDR_RESETVAL        (0x00000000u)
#define TSIF_RING_BUF_RD_CH0_END_ADDR_RESETVAL          (0x00000000u)
#define TSIF_RING_BUF_RD_CH0_WR_POINT_ADDR_RESETVAL     (0x00000000u)
#define TSIF_RING_BUF_RD_CH0_SUB_RESETVAL               (0x00000000u)

/* Macros for internal use */
#define TSIF_PID_FILTER_ENABLE_MASK		(0x01000000u)
#define TSIF_CTRL0_RCV_IF_MODE_SER_SYN 		(0x00000000u)
#define TSIF_CTRL0_RCV_IF_MODE_SER_ASYN 	(0x00000004u)
#define TSIF_CTRL0_RCV_IF_MODE_PAR_SYN 		(0x00000008u)
#define TSIF_CTRL0_RCV_IF_MODE_PAR_ASYN 	(0x0000000Cu)
#define TSIF_CTRL0_RCV_IF_MODE_DMA 		(0x00000010u)
#define TSIF_CTRL0_RCV_STREAM_MODE_TS_INACTIVE 	(0x00000000u)
#define TSIF_CTRL0_RCV_STREAM_MODE_TS_ACTIVE 	(0x00000020u)
#define TSIF_CTRL0_RCV_ATS_MODE_IN_192 		(0x00000000u)
#define TSIF_CTRL0_RCV_ATS_MODE_IN_188 		(0x00000040u)
#define TSIF_CTRL0_RCV_ATS_MODE_CHANGE_192 	(0x00000080u)
#define TSIF_CTRL0_RCV_ATS_MODE_ADD_188 	(0x000000C0u)
#define TSIF_CTRL1_PID_FILTER_EN_INACTIVATE 	(0x00000000u)
#define TSIF_CTRL1_PID_FILTER_EN_ACTIVATE 	(0x00000008u)
#define TSIF_CTRL1_PID_FILTER_CTL_FULL_MAN 	(0x00000000u)
#define TSIF_CTRL1_PID_FILTER_CTL_SEMI_A 	(0x00000001u)
#define TSIF_CTRL1_PID_FILTER_CTL_SEMI_B 	(0x00000002u)
#define TSIF_CTRL1_PID_FILTER_CTL_FULL_AUT 	(0x00000003u)
#define TSIF_CTRL0_RCV_PKT_SIZE_BYTE_200 	(0x00000000u)
#define TSIF_CTRL0_RCV_PKT_SIZE_BYTE_208 	(0x00000100u)
#define TSIF_CTRL0_RCV_PKT_SIZE_BYTE_216 	(0x00000200u)
#define TSIF_CTRL0_RCV_PKT_SIZE_BYTE_224 	(0x00000300u)
#define TSIF_CTRL0_RCV_PKT_SIZE_BYTE_232 	(0x00000400u)
#define TSIF_CTRL0_RCV_PKT_SIZE_BYTE_240 	(0x00000500u)
#define TSIF_CTRL0_RCV_PKT_SIZE_BYTE_248 	(0x00000600u)
#define TSIF_CTRL0_RCV_PKT_SIZE_BYTE_256 	(0x00000700u)
#define TSIF_CTRL0_TX_IF_MODE_SER_SYN 		(0x00000000u)
#define TSIF_CTRL0_TX_IF_MODE_SER_ASYN 		(0x00040000u)
#define TSIF_CTRL0_TX_IF_MODE_PAR_SYN 		(0x00080000u)
#define TSIF_CTRL0_TX_IF_MODE_PAR_ASYN 		(0x000C0000u)
#define TSIF_CTRL0_TX_STREAM_MODE_TS_INACTIVE 	(0x00000000u)
#define TSIF_CTRL0_TX_STREAM_MODE_TS_ACTIVE 	(0x00200000u)
#define TSIF_CTRL0_TX_ATS_MODE_OUT_188 		(0x00000000u)
#define TSIF_CTRL0_TX_ATS_MODE_OUT_192 		(0x00400000u)
#define TSIF_CTRL0_TX_PKT_SIZE_BYTE_200 	(0x00000000u)
#define TSIF_CTRL0_TX_PKT_SIZE_BYTE_208 	(0x01000000u)
#define TSIF_CTRL0_TX_PKT_SIZE_BYTE_216 	(0x02000000u)
#define TSIF_CTRL0_TX_PKT_SIZE_BYTE_224 	(0x03000000u)
#define TSIF_CTRL0_TX_PKT_SIZE_BYTE_232 	(0x04000000u)
#define TSIF_CTRL0_TX_PKT_SIZE_BYTE_240 	(0x05000000u)
#define TSIF_CTRL0_TX_PKT_SIZE_BYTE_248 	(0x06000000u)
#define TSIF_CTRL0_TX_PKT_SIZE_BYTE_256 	(0x07000000u)

#define TSIF_CTRL0_RCV_IF_MODE_MASK		(0x0000000Cu)
#define TSIF_CTRL0_RCV_ATS_MODE_MASK 		(0x000000C0u)
#define TSIF_CTRL1_PID_FILTER_CTL_MASK 		(0x00000007u)
#define TSIF_CTRL0_RCV_PKT_SIZE_MASK 		(0x00000700u)
#define TSIF_CTRL0_TX_PKT_SIZE_MASK 		(0x07000000u)

#define RX_CONFIG		(TSIF_CTRL0_RCV_PKT_SIZE | \
				TSIF_CTRL0_RCV_ATS_MODE | \
				TSIF_CTRL0_RCV_STREAM_MODE | \
				TSIF_CTRL0_RCV_INPUT_PORT | \
				TSIF_CTRL0_RCV_IF_MODE | \
				TSIF_CTRL0_RCV_DMA_CTL | \
				TSIF_CTRL0_RCV_CTL)

#define TX_CONFIG		(TSIF_CTRL0_TX_PKT_SIZE | \
				TSIF_CTRL0_TX_ATS_MODE | \
				TSIF_CTRL0_TX_STREAM_MODE | \
				TSIF_CTRL0_TX_PKTSTRT_WIDTH | \
				TSIF_CTRL0_TX_IF_MODE | \
				TSIF_CTRL0_TX_DMA_CTL | \
				TSIF_CTRL0_TX_CTL)

/*
 * Enumerations
 */

enum tsif_if_mode {
	/* Synchronous serial interface */
	TSIF_IF_SERIAL_SYNC,
	/* Asynchronous serial interface */
	TSIF_IF_SERIAL_ASYNC,
	/* Synchronous parallel interface */
	TSIF_IF_PARALLEL_SYNC,
	/* Asynchronous parallel interface */
	TSIF_IF_PARALLEL_ASYNC,
	/* Consequential Interface */
	TSIF_IF_DMA
};

enum tsif_stream_mode {
	TSIF_STREAM_TS,		/* TS */
	TSIF_STREAM_NON_TS	/* Non-TS */
};

enum tsif_tx_ats_mode {
	/* Output 188 byte length TS data after deleting ATS value */
	TSIF_TX_ATS_REMOVE,
	/* Output 192 byte length TS data without any changing ATS value */
	TSIF_TX_ATS_THROUGH,
};

enum tsif_rx_ats_mode {
	/* Receiving 192 byte length TS packet without changing ATS data */
	TSIF_RX_ATS_THROUGH,
	/* to be discussed */
	TSIF_RX_ATS_NOADD,
	/* Receiving 192 byte length TS packet with changing ATS data */
	TSIF_RX_ATS_CHANGE,
	/* Receiving 188 byte length TS packet with adding ATS data */
	TSIF_RX_ATS_ADD
};

enum tsif_pid_filter_mode {
	/* Bypass Mode(PID Filter Inactive) */
	TSIF_PID_FILTER_BYPASS,
	/* Full Manual Mode */
	TSIF_PID_FILTER_FULL_MANUAL,
	/* Semi-automatic Mode-A */
	TSIF_PID_FILTER_SEMI_AUTO_A,
	/* Semi-automatic Mode-B */
	TSIF_PID_FILTER_SEMI_AUTO_B,
	/* Full Automatic Mode */
	TSIF_PID_FILTER_FULL_AUTO
};

enum tsif_channel_id {
	/* Receive Data Channel 0 */
	TSIF_RX_FILTER0,
	/* Receive Data Channel 1 */
	TSIF_RX_FILTER1,
	/* Receive Data Channel 2 */
	TSIF_RX_FILTER2,
	/* Receive Data Channel 3 */
	TSIF_RX_FILTER3,
	/* Receive Data Channel 4 */
	TSIF_RX_FILTER4,
	/* Receive Data Channel 5 */
	TSIF_RX_FILTER5,
	/* Receive Data Channel 6 */
	TSIF_RX_FILTER6,
	/* Receive Data Channel 7 */
	TSIF_RX_FILTER7,
	/* Receive Control Channel */
	TSIF_RX_CONTROL_CHANNEL,
	/* Transmit Control Channel */
	TSIF_TX_CONTROL_CHANNEL,
	/* ATS */
	TSIF_TX_ATS
};

enum tsif_pkt_size {
	/* 200 bytes per packet */
	TSIF_200_BYTES_PER_PKT,
	/* 208 bytes per packet */
	TSIF_208_BYTES_PER_PKT,
	/* 216 bytes per packet */
	TSIF_216_BYTES_PER_PKT,
	/* 224 bytes per packet */
	TSIF_224_BYTES_PER_PKT,
	/* 232 bytes per packet */
	TSIF_232_BYTES_PER_PKT,
	/* 240 bytes per packet */
	TSIF_240_BYTES_PER_PKT,
	/* 248 bytes per packet */
	TSIF_248_BYTES_PER_PKT,
	/* 256 bytes per packet */
	TSIF_256_BYTES_PER_PKT
};

enum tsif_clk_speed {
	/* 13.5 MHz for serial/parallel mode */
	TSIF_13_5_MHZ_SERIAL_PARALLEL,
	/* 16.875 MHz for serial/parallel mode */
	TSIF_16_875_MHZ_SERIAL_PARALLEL,
	/* 27.0 MHz for serial mode */
	TSIF_27_0_MHZ_SERIAL,
	/* 54.0 MHz for serial mode */
	TSIF_54_0_MHZ_SERIAL,
	/* 81.0 MHz for serial mode */
	TSIF_81_0_MHZ_SERIAL
};

/*
 * Structures
 */
struct tsif_tx_config {
	/* Mode of transmitter interface */
	enum tsif_if_mode if_mode;
	/* Stream mode of transmitter */
	enum tsif_stream_mode stream_mode;
	/* ATS mode  */
	enum tsif_tx_ats_mode ats_mode;
	/* Interval between transmitter packets */
	unsigned char interval_wait;
	/* Packet size for transmitter */
	unsigned int pkt_size;
	/* Clock speed for transmitter */
	enum tsif_clk_speed clk_speed;
};

struct tsif_rx_config {
	/* Mode of receiver interface */
	enum tsif_if_mode if_mode;
	/* Stream mode of receiver */
	enum tsif_stream_mode stream_mode;
	/* ATS mode  */
	enum tsif_rx_ats_mode ats_mode;
	/* Mode of PID filter */
	enum tsif_pid_filter_mode filter_mode;
	/* Packet size for receiver */
	unsigned int pkt_size;
};

struct tsif_tx_ring_buf_config {
	/* Receive buffer start address */
	char *pstart;
	/* Receive buffer write pointer address */
	char *pwrite;
	/* Receive buffer size */
	unsigned long buf_size;
	/* Receive buffer subtraction address */
	unsigned int psub;
};

struct tsif_rx_ring_buf_config {
	/* Receive buffer start address */
	char *pstart;
	/* Receive buffer read pointer address */
	char *pread;
	/* Receive buffer size */
	unsigned long buf_size;
	/* Receive buffer subtraction address */
	unsigned int psub;
};

struct tsif_pid_filter_config {
	/* Target PID */
	unsigned short pid;
	/* Stream type for Semi-automatic Mode-B/Full Automatic Mode */
	unsigned char stream_type;
};

struct tsif_pat_config {
	/* Flag indicating whether PAT is enabled or not */
	unsigned char flag;
};

struct tsif_pmt_config {
	/* PMT Id */
	unsigned short pmt_pid;
	/* Flag to enable/disbale PMT */
	unsigned char flag;
};

struct tsif_pcr_config {
	/* PMT Id */
	unsigned short pcr_pid;
};

struct tsif_spcpkt_config {
	/* PID of Specific Packet */
	unsigned short pid;
};

struct tsif_buf_status {
	/* Pointer for start address */
	char *pstart;
	/* Pointer for end address */
	char *pend;
	/* Destination of write pointer for transmitter/read
	 * pointer for receiver */
	char *pread_write;
	/* Current write pointer for transmitter / read pointer for receiver */
	char *pcurrent;
	/* Subtraction address */
	unsigned int psub;
};

struct tsif_read_avail {
	/* Offset from where read has to start */
	unsigned int offset;
	/* Length that can be read from user */
	unsigned int len;
	/* Variable in which the number of bytes read successfully in the
	 * last transaction is populated by the user */
	unsigned int bytes_last_read;
};

struct tsif_write_avail {
	/* Offset from where write has to start */
	unsigned int offset;
	/* Length that can be written from user */
	unsigned int len;
	/* Variable in which the number of bytes wrote successfully in the
	 * last transaction is populated by the user */
	unsigned int bytes_last_wrote;
};

/*
 * IOCTLs
 */
#define TSIF_IOC_BASE			'S'
/* Set configuration for PID filter */
#define TSIF_SET_PID_FILTER_CONFIG	\
			_IOW(TSIF_IOC_BASE, 1, struct tsif_pid_filter_config *)

/* Get configuration of PID filter */
#define TSIF_GET_PID_FILTER_CONFIG	\
			_IOR(TSIF_IOC_BASE, 2, struct tsif_pid_filter_config *)

/* Set configuration for receiver */
#define TSIF_SET_RX_CONFIG		\
			_IOW(TSIF_IOC_BASE, 3, struct tsif_rx_config *)

/* Get configuration of receiver */
#define TSIF_GET_RX_CONFIG		\
			_IOR(TSIF_IOC_BASE, 4, struct tsif_rx_config *)

/* Set configuration for transmitter */
#define TSIF_SET_TX_CONFIG		\
				_IOW(TSIF_IOC_BASE, 5, struct tsif_tx_config *)

/* Get configuration of transmitter */
#define TSIF_GET_TX_CONFIG		\
			_IOR(TSIF_IOC_BASE, 6, struct tsif_tx_config *)

/* Start receiver */
#define TSIF_START_RX                   _IO(TSIF_IOC_BASE, 7)

/* Stop receiver */
#define TSIF_STOP_RX                    _IO(TSIF_IOC_BASE, 8)

/* Start transmitter */
#define TSIF_START_TX                   _IO(TSIF_IOC_BASE, 9)

/* Stop transmitter */
#define TSIF_STOP_TX                    _IO(TSIF_IOC_BASE, 10)

/* Get status of receiver buffer */
#define TSIF_GET_RX_BUF_STATUS		\
			_IOR(TSIF_IOC_BASE, 11, struct tsif_buf_status *)

/* Get status of transmitter buffer */
#define TSIF_GET_TX_BUF_STATUS		\
			_IOR(TSIF_IOC_BASE, 12, struct tsif_buf_status *)

/* Enable/Disable PAT Detection */
#define TSIF_CONFIG_PAT			_IOW(TSIF_IOC_BASE, 13, unsigned char *)

/* Get PAT packet */
#define TSIF_GET_PAT_PKT                _IOR(TSIF_IOC_BASE, 14, void *)

/* Set configuration for detecting PMT */
#define TSIF_CONFIG_PMT			\
			_IOW(TSIF_IOC_BASE, 15, struct tsif_pmt_config *)

/* Get PMT packet */
#define TSIF_GET_PMT_PKT                _IOR(TSIF_IOC_BASE, 16, void *)

/* Enable PCR detection */
#define TSIF_ENABLE_PCR			\
			_IOW(TSIF_IOC_BASE, 17, struct tsif_pcr_config *)

/* Disable PCR detection */
#define TSIF_DISABLE_PCR		_IO(TSIF_IOC_BASE, 18)

/* Set configuration for detecting specific packet */
#define TSIF_SET_SPCPKT_CONFIG		\
			_IOW(TSIF_IOC_BASE, 19, struct tsif_spcpkt_config *)

/* Get configuration for detecting specific packet */
#define TSIF_GET_SPCPKT_CONFIG		\
			_IOR(TSIF_IOC_BASE, 20, struct tsif_spcpkt_config *)

/* Get read availability */
#define TSIF_GET_READ_AVAIL		\
			_IOR(TSIF_IOC_BASE, 21, struct tsif_read_avail *)

/* Get write availability */
#define TSIF_GET_WRITE_AVAIL		\
			_IOR(TSIF_IOC_BASE, 22, struct tsif_write_avail *)

/* Allocate a TX buffer */
#define TSIF_REQ_TX_BUF                	_IOWR(TSIF_IOC_BASE, 23, unsigned int *)

/* Query a TX buffer */
#define TSIF_QUERY_TX_BUF               _IOR(TSIF_IOC_BASE, 24, unsigned int *)

/* Allocate a RX buffer */
#define TSIF_REQ_RX_BUF                	_IOWR(TSIF_IOC_BASE, 25, unsigned int *)

/* Query a RX buffer */
#define TSIF_QUERY_RX_BUF               _IOR(TSIF_IOC_BASE, 26, unsigned int *)

/* Set read availability */
#define TSIF_SET_READ_AVAIL		\
			_IOW(TSIF_IOC_BASE, 27, struct tsif_read_avail *)

/* Set write availability */
#define TSIF_SET_WRITE_AVAIL		\
			_IOW(TSIF_IOC_BASE, 28, struct tsif_write_avail *)

/* Set ATS */
#define TSIF_SET_ATS            	_IOW(TSIF_IOC_BASE, 29, unsigned int *)

/* Wait for RX to complete */
#define TSIF_WAIT_FOR_RX_COMPLETE	_IO(TSIF_IOC_BASE, 30)

/* Configure the receiver ring buffer */
#define TSIF_RX_RING_BUF_CONFIG		\
		_IOW(TSIF_IOC_BASE, 31, struct tsif_rx_ring_buf_config *)

/* Configure the transmit ring buffer */
#define TSIF_TX_RING_BUF_CONFIG		\
		_IOW(TSIF_IOC_BASE, 32, struct tsif_tx_ring_buf_config *)

/* Enable by-pass mode */
#define TSIF_BYPASS_ENABLE		_IO(TSIF_IOC_BASE, 33)

/* Enable consequential mode */
#define TSIF_SET_CONSEQUENTIAL_MODE	_IO(TSIF_IOC_BASE, 34)

#define TSIF_GET_PAT_CONFIG		\
			_IOR(TSIF_IOC_BASE, 35, struct tsif_pat_config *)

#define TSIF_GET_PMT_CONFIG		\
			_IOR(TSIF_IOC_BASE, 36, struct tsif_pmt_config *)

/* Get ATS */
#define TSIF_GET_ATS            	_IOR(TSIF_IOC_BASE, 37, unsigned int *)

/* Enable/Disable GOP Detection */
#define TSIF_ENABLE_GOP_DETECT		_IOW(TSIF_IOC_BASE, 38, unsigned int *)

/* Get the Specific Packet */
#define TSIF_GET_SPCPKT         	_IOR(TSIF_IOC_BASE, 39, void *)

/* Wait for TX to complete */
#define TSIF_WAIT_FOR_TX_COMPLETE	_IO(TSIF_IOC_BASE, 40)

/* Set/Reset the Endianness control bit */
#define TSIF_SET_ENDIAN_CTL		_IOWR(TSIF_IOC_BASE, 41, unsigned int *)

struct inode;
struct file;
struct vm_area_struct;
struct device;
struct pt_regs;

struct tsif_dev *tsif_dev_get_by_major(unsigned int);
int tsif_set_pid_filter_config(struct inode *, unsigned long);
int tsif_set_rx_config(struct inode *, unsigned long);
int tsif_set_tx_config(struct inode *, unsigned long);
int tsif_stop_rx(struct inode *);
int tsif_set_pat_config(struct inode *, unsigned long);
int tsif_set_pmt_config(struct inode *, unsigned long);
int tsif_set_spec_pkt_config(struct inode *, unsigned long);
int tsif_pid_open(struct inode *, struct file *);
int tsif_control_open(struct inode *, struct file *);
int tsif_pid_release(struct inode *, struct file *);
int tsif_control_release(struct inode *, struct file *);
int tsif_control_mmap(struct file *, struct vm_area_struct *);
int tsif_pid_mmap(struct file *, struct vm_area_struct *);
int tsif_pid_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
int tsif_control_ioctl(struct inode *, struct file *, unsigned int,
		       unsigned long);
void tsif_pat_complete_cmd(void);
void tsif_pmt_complete_cmd(void);
void tsif_spcpkt_complete_cmd(void);
int set_tsif_clk(enum tsif_clk_speed);
unsigned int tsif_user_virt_to_phys(unsigned int);
static int tsif_remove(struct device *);
static void tsif_platform_release(struct device *);

#endif				/* __ASM_ARCH_TSIF_H */
