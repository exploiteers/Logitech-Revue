/*******************************************************************************
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ******************************************************************************/
#include <linux/io.h>
#include <linux/vlynq/vlynq.h>
#include <linux/vlynq/vlynq_ioctl.h>

#define IOCTL_WR         0x01

struct pal_vlynq_ioctl_info {
	s16 id;
	u16 offset;		/* Register offset or bit offset. */
	const char *name;
	u32 mask;
	u32 start_revision;
	u32 end_revision;
	/* 0x01 - write, otherwise read, can be enhanced later. */
	u8 access_flags;

};

static struct pal_vlynq_ioctl_info control_reg_ioctl[] = {
	{
		.id 			= VLYNQ_IOCTL_CNT_RESET_CMD,
		.offset			= 0,
		.name			= "reset",
		.mask			= 0x00000001,
		.start_revision		= 0x00010200,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_ILOOP_CMD,
		.offset			= 1,
		.name			= "iloop",
		.mask			= 0x00000002,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_AOPT_CMD,
		.offset			= 2,
		.name			= "aopt disable",
		.mask			= 0x00000004,
		.start_revision		= 0x00010200,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= IOCTL_WR,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_INT2CFG_CMD,
		.offset			= 7,
		.name			= "int2cfg",
		.mask			= 0x00000080,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_INTVEC_CMD,
		.offset			= 8,
		.name			= "intvec",
		.mask			= 0x00001f00,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_INT_EN_CMD,
		.offset			= 13,
		.name			= "intenable",
		.mask			= 0x00002000,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_INT_LOC_CMD,
		.offset			= 14,
		.name			= "intlocal",
		.mask			= 0x00004000,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_CLK_DIR_CMD,
		.offset			= 15,
		.name			= "clkdir",
		.mask			= 0x00008000,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_CLK_DIV_CMD,
		.offset			= 16,
		.name			= "clkdiv",
		.mask			= 0x00070000,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= IOCTL_WR,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_CLK_MOD_CMD,
		.offset			= 21,
		.name			= "mode",
		.mask			= 0x00e00000,
		.start_revision		= 0x00010100,
		.end_revision		= 0x000101ff,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_TX_FAST_CMD,
		.offset			= 21,
		.name			= "txfastpath",
		.mask			= 0x00000002,
		.start_revision		= 0x00010205,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_RTM_SELECT_CMD,
		.offset			= 22,
		.name			= "rtmenable",
		.mask			= 0x00400000,
		.start_revision		= 0x00010205,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_RTM_VALIDWR_CMD,
		.offset			= 23,
		.name			= "wr-rtmval",
		.mask			= 0x00800000,
		.start_revision		= 0x00010205,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_RTM_SAMPLE_CMD,
		.offset			= 24,
		.name			= "rtmsample",
		.mask			= 0x07000000,
		.start_revision		= 0x00010205,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_CLK_SLKPU_CMD,
		.offset			= 30,
		.name			= "slkpudis",
		.mask			= 0x40000000,
		.start_revision		= 0x00010204,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= IOCTL_WR,
	},
	{
		.id 			= VLYNQ_IOCTL_CNT_PMEM_CMD,
		.offset			= 31,
		.name			= "pmem",
		.mask			= 0x80000000,
		.start_revision		= 0x00010200,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= IOCTL_WR,
	},
	{
		.id 			= -1,
		.offset			= 32,
		.name			= "dead",
		.mask			= 0xdeadfeed,
		.start_revision		= 0x00000000,
		.end_revision		= 0xdeadfeed,
		.access_flags		= 0,
	},
};

static struct pal_vlynq_ioctl_info status_reg_ioctl[] = {
	{
		.id 			= VLYNQ_IOCTL_STS_LINK,
		.offset			= 0,
		.name			= "link",
		.mask			= 0x00000001,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_MPEND,
		.offset			= 1,
		.name			= "mpend",
		.mask			= 0x00000002,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_SPEND,
		.offset			= 2,
		.name			= "spend",
		.mask			= 0x00000004,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_NFEMP0,
		.offset			= 3,
		.name			= "nfempty0",
		.mask			= 0x00000008,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_NFEMP1,
		.offset			= 4,
		.name			= "nfempty1",
		.mask			= 0x00000010,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_NFEMP2,
		.offset			= 5,
		.name			= "nfempty2",
		.mask			= 0x00000020,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_NFEMP3,
		.offset			= 6,
		.name			= "nfempty3",
		.mask			= 0x00000040,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_LERR,
		.offset			= 7,
		.name			= "lerror",
		.mask			= 0x00000080,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_RERR,
		.offset			= 8,
		.name			= "rerror",
		.mask			= 0x00000100,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_OFLOW,
		.offset			= 9,
		.name			= "oflow",
		.mask			= 0x00000200,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_IFLOW,
		.offset			= 10,
		.name			= "iflow",
		.mask			= 0x00000400,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_RTM,
		.offset			= 11,
		.name			= "rtm",
		.mask			= 0x00000800,
		.start_revision		= 0x00010205,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_RTM_VAL,
		.offset			= 12,
		.name			= "rtmcurrval",
		.mask			= 0x00007000,
		.start_revision		= 0x00010205,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_SWIDOUT,
		.offset			= 20,
		.name			= "swidthout",
		.mask			= 0x00f00000,
		.start_revision		= 0x00010200,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_MODESUP,
		.offset			= 21,
		.name			= "modesup",
		.mask			= 0x00700000,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_SWIDTH,
		.offset			= 24,
		.name			= "swidth",
		.mask			= 0x03000000,
		.start_revision		= 0x00010100,
		.end_revision		= 0x00010200,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_SWIDIN,
		.offset			= 24,
		.name			= "swidthin",
		.mask			= 0x0f000000,
		.start_revision		= 0x00010200,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STS_DEBUG,
		.offset			= 29,
		.name			= "debug",
		.mask			= 0x70000000,
		.start_revision		= 0x00010100,
		.end_revision		= 0x000101ff,
		.access_flags		= 0,
	},
	{
		.id 			= -1,
		.offset			= 32,
		.name			= "dead",
		.mask			= 0xdeadfeed,
		.start_revision		= 0x00000000,
		.end_revision		= 0xdeadfeed,
		.access_flags		= 0,
	},
	{-1, 32, "dead end", 0xdeadfeed, 0x00000000, 0xdeadfeed, 0}

};

static struct pal_vlynq_ioctl_info vlynq_reg32_ioctl[] = {
	{
		.id 			= VLYNQ_IOCTL_REVSION_REG,
		.offset			= 0x00,
		.name			= "revision",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CONTROL_REG,
		.offset			= 0x04,
		.name			= "control",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_STATUS_REG,
		.offset			= 0x08,
		.name			= "status",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_INT_PRIR_REG,
		.offset			= 0x0c,
		.name			= "intPriority",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010200,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_INT_STS_REG,
		.offset			= 0x10,
		.name			= "intStatus",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_INT_PEND_REG,
		.offset			= 0x14,
		.name			= "intPending",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_INT_PTR_REG,
		.offset			= 0x18,
		.name			= "intPtr",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_TX_MAP_REG,
		.offset			= 0x1c,
		.name			= "txMap",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_RX1_SZ_REG,
		.offset			= 0x20,
		.name			= "rxSize1",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_RX1_OFF_REG,
		.offset			= 0x24,
		.name			= "rxoffset1",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_RX2_SZ_REG,
		.offset			= 0x28,
		.name			= "rxSize2",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_RX2_OFF_REG,
		.offset			= 0x2c,
		.name			= "rxoffset2",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_RX3_SZ_REG,
		.offset			= 0x30,
		.name			= "rxSize3",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_RX3_OFF_REG,
		.offset			= 0x34,
		.name			= "rxoffset3",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_RX4_SZ_REG,
		.offset			= 0x38,
		.name			= "rxSize4",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_RX4_OFF_REG,
		.offset			= 0x3c,
		.name			= "rxoffset4",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_CVR_REG,
		.offset			= 0x40,
		.name			= "chipVersion",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_AUTO_NEG_REG,
		.offset			= 0x44,
		.name			= "autoNegn",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010200,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_MAN_NEG_REG,
		.offset			= 0x48,
		.name			= "manualNegn",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010200,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_NEG_STS_REG,
		.offset			= 0x4c,
		.name			= "negnStatus",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010200,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_ENDIAN_REG,
		.offset			= 0x5c,
		.name			= "endian",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010200,
		.end_revision		= 0x00010204,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_IVR30_REG,
		.offset			= 0x60,
		.name			= "ivr30",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= VLYNQ_IOCTL_IVR74_REG,
		.offset			= 0x64,
		.name			= "ivr74",
		.mask			= 0xffffffff,
		.start_revision		= 0x00010100,
		.end_revision		= LATEST_VLYNQ_REV,
		.access_flags		= 0,
	},
	{
		.id 			= -1,
		.offset			= 32,
		.name			= "dead",
		.mask			= 0xdeadfeed,
		.start_revision		= 0x00000000,
		.end_revision		= 0xdeadfeed,
		.access_flags		= 0,
	},
	{-1, 32, "dead end", 0xdeadfeed, 0x00000000, 0xdeadfeed}
};

u32 vlynq_endian_swap(u32);

/* Definitely, not re-entrant. */
static u32 vlynq_check_and_swap(u32 val)
{
	if (vlynq_swap_hack == 1u)
		val = vlynq_endian_swap(val);

	return (val);
}

static int vlynq_dump_cmd(struct pal_vlynq_ioctl_info *vlynq_ioctl,
			  u32 p_start_reg, char *buf, int limit,
			  u32 revision, u8 bit_op)
{
	int len = 0;
	struct pal_vlynq_ioctl_info *p_ioctl;

	for (p_ioctl = vlynq_ioctl; p_ioctl->id > -1; p_ioctl++) {
		u32 val = 0;

		if (revision < p_ioctl->start_revision ||
		    revision > p_ioctl->end_revision)
			continue;

		if (len < limit)
			len += snprintf(buf + len, limit - len,
					"%20s : ", p_ioctl->name);
		else
			break;

		if (bit_op) {
			val = vlynq_check_and_swap(__raw_readl(p_start_reg));
			val = (val & p_ioctl->mask) >> p_ioctl->offset;
		} else {
			val = __raw_readl(p_start_reg + p_ioctl->offset / 0x04);
			val = vlynq_check_and_swap(val) & p_ioctl->mask;
		}

		if (len < limit)
			len += snprintf(buf + len, limit - len,
					"0x%08x.\n", val);
		else
			break;
	}

	return (len);
}

static int vlynq_write_cmd(struct pal_vlynq_ioctl_info *vlynq_ioctl,
			   u32 p_reg, u32 cmd, u32 val, u32 revision,
			   u8 bit_op)
{
	int ret_val = VLYNQ_INVALID_PARAM;
	u32 tmp;
	struct pal_vlynq_ioctl_info *p_ioctl;

	for (p_ioctl = vlynq_ioctl; p_ioctl->id > -1; p_ioctl++) {
		if (revision < p_ioctl->start_revision ||
		    revision > p_ioctl->end_revision)
			continue;

		if (cmd != p_ioctl->id)
			continue;

		/* Can we write ? */
		if (!(p_ioctl->access_flags & IOCTL_WR))
			break;

		/* Hack. One of the case. We need to enhance the data
		 * structure for generic implementation.
		 */
		if (cmd == VLYNQ_IOCTL_CNT_CLK_DIV_CMD) {
			if (val != 0) {
				/* Adjusting the clock divisor. */
				val -= 1;
			} else
				break;
		}

		if (bit_op) {
			tmp = __raw_readl(p_reg) &
			      vlynq_check_and_swap(~p_ioctl->mask);
			__raw_writel(tmp, p_reg);
			tmp |= vlynq_check_and_swap((val << p_ioctl->offset) &
						    p_ioctl->mask);
			__raw_writel(tmp, p_reg);
		} else {
			__raw_writel(vlynq_check_and_swap(val & p_ioctl->mask),
			     p_reg + p_ioctl->offset / 0x04);
		}

		ret_val = VLYNQ_OK;
		break;
	}

	return (ret_val);
}

static int vlynq_read_cmd(struct pal_vlynq_ioctl_info *vlynq_ioctl,
			  u32 p_reg, u32 cmd, u32 *val,
			  u32 revision, u8 bit_op)
{
	int ret_val = VLYNQ_INVALID_PARAM;
	struct pal_vlynq_ioctl_info *p_ioctl;
	u32 tmp;

	for (p_ioctl = vlynq_ioctl; p_ioctl->id > -1; p_ioctl++) {
		if (revision < p_ioctl->start_revision ||
		    revision > p_ioctl->end_revision)
			continue;

		if (cmd != p_ioctl->id)
			continue;

		if (bit_op) {
			tmp = vlynq_check_and_swap(__raw_readl(p_reg));
			*val = (tmp & p_ioctl->mask) >> p_ioctl->offset;
		} else {
			tmp = __raw_readl(p_reg + p_ioctl->offset / 0x4);
			*val = vlynq_check_and_swap(tmp) & p_ioctl->mask;
		}

		ret_val = VLYNQ_OK;
		break;
	}

	return (ret_val);
}

int vlynq_read_write_ioctl(u32 p_start_reg, u32 cmd,
			   u32 val, u32 vlynq_rev)
{
	u8 read_flag = 0;
	u32 sub_cmd = VLYNQ_IOCTL_MINOR_DE_VAL(cmd);
	int ret_val = VLYNQ_INVALID_PARAM;
	int valid = 1;
	u8 bit_op = 1;

	struct pal_vlynq_ioctl_info *p_ioctl = NULL;

	if (cmd & VLYNQ_IOCTL_READ_CMD)
		read_flag = 1;

	/*
	 * Since we are supporting just one non bit operation, we are hacking
	 * the implementation. Ideally, we need to have a separate function.
	 */

	if (!(cmd & VLYNQ_IOCTL_BIT_CMD)) {
		bit_op = 0;

		if (!VLYNQ_IOCTL_MAJOR_DE_VAL(cmd))
			p_ioctl = vlynq_reg32_ioctl;
		else
			valid = 0;
	} else {
		/* We are doing bit accesses. */
		switch (VLYNQ_IOCTL_MAJOR_DE_VAL(cmd)) {
		case VLYNQ_IOCTL_CONTROL_REG:

			p_ioctl = control_reg_ioctl;
			break;

		case VLYNQ_IOCTL_STATUS_REG:

			p_ioctl = status_reg_ioctl;
			break;

		default:

			valid = 0;
			break;
		}
	}

	if (valid) {
		if (read_flag)
			ret_val = vlynq_read_cmd(p_ioctl, p_start_reg, sub_cmd,
						 (u32 *) val, vlynq_rev,
						 bit_op);
		else
			ret_val = vlynq_write_cmd(p_ioctl, p_start_reg, sub_cmd,
						  val, vlynq_rev, bit_op);
	}

	return (ret_val);
}

int vlynq_dump_ioctl(u32 start_reg, u32 dump_type, char *buf,
		     int limit, u32 vlynq_rev)
{
	int len = 0;
	u32 revision = vlynq_rev;

	switch (dump_type) {
	case VLYNQ_DUMP_STS_REG:
		{
			if (len < limit)
				len +=
				    vlynq_dump_cmd(status_reg_ioctl, start_reg,
						   buf + len, limit - len,
						   revision, 1);
			break;
		}

	case VLYNQ_DUMP_CNTL_REG:
		{
			if (len < limit)
				len +=
				    vlynq_dump_cmd(control_reg_ioctl, start_reg,
						   buf + len, limit - len,
						   revision, 1);
			break;
		}

	case VLYNQ_DUMP_ALL_REGS:
		{
			if (len < limit)
				len +=
				    vlynq_dump_cmd(vlynq_reg32_ioctl, start_reg,
						   buf + len, limit - len,
						   revision, 0);
			break;
		}

	default:
		break;
	}

	return (len);
}
