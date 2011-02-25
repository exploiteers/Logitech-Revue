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

#ifndef __VLYNQ_IOCTL_H__
#define __VLYNQ_IOCTL_H__

/*
 * The 32 bit IOCTL Command.
 * ------------------------------------------------------------------------
 * |           |          |          |       |          |      |          |
 * | 31:Bit Op | 30: R/W* | 29: Peer | 28-24 | 23-16    | 15-8 | 7-0      |
 * |           |          |          | Resv  | Major id | Resv | Minor id |
 * |           |          |          |       |          |      |          |
 * ------------------------------------------------------------------------
 *
 * The IOCTL operations have been broadly divided into two categories of o-
 * perations; Bit Op(eration) and non-Bit Op(eration).
 *
 * Bit Op(eration) means manipulation of certain bits in certain register of
 * VLYNQ control modules. The register and its contents (bits) are specified
 * by the hardware reference manual. For Bit Op=1, the manjor id are the re-
 * gister id(s) and the minor id(s) are the bits identified within that reg.
 *
 * Non Bit Op(eration), when Bit Op=0, means raw 32 bits accesses of certain
 * VLYNQ register or any other operation as specified in the implementation.
 * For raw 32 bit accesses VLYNQ_IOCTL_REG_CMD is the major command (id)
 * and reg id(s) refer(s) to minor id(s).
 *
 * R/W*: 0 - Write, 1 - Read.
 * Peer: When, set means that operation has to be carried out on remote or
 *       peer vlynq.
 */

#define VLYNQ_IOCTL_BIT_CMD            (1 << 31)
#define VLYNQ_IOCTL_READ_CMD           (1 << 30)
#define VLYNQ_IOCTL_REMOTE_CMD         (1 << 29)

#define VLYNQ_IOCTL_MAJOR_VAL(val)     ((val & 0xff) << 16)
#define VLYNQ_IOCTL_MAJOR_DE_VAL(cmd)  ((cmd >> 16) & 0xff)

#define VLYNQ_IOCTL_MINOR_VAL(val)     (val & 0xff)
#define VLYNQ_IOCTL_MINOR_DE_VAL(cmd)  (cmd & 0xff)

/* Major commads; if bit option is not selected. */
/* Vlynq register access. */
#define VLYNQ_IOCTL_REG_CMD            (0x00)
/* Prepare to teardown the link. */
#define VLYNQ_IOCTL_PREP_LINK_DOWN     (0x01)
/* Setup vlynq, now the link is up. */
#define VLYNQ_IOCTL_PREP_LINK_UP       (0x02)
/* Clear internal interrupt errors. */
#define VLYNQ_IOCTL_CLEAR_INTERN_ERR   (0x03)

/* Control Register bits. The minor id(s) for control reg. */
#define VLYNQ_IOCTL_CNT_RESET_CMD      (0x00)
#define VLYNQ_IOCTL_CNT_ILOOP_CMD      (0x01)
#define VLYNQ_IOCTL_CNT_AOPT_CMD       (0x02)   /* Write */
#define VLYNQ_IOCTL_CNT_INT2CFG_CMD    (0x07)
#define VLYNQ_IOCTL_CNT_INTVEC_CMD     (0x08)
#define VLYNQ_IOCTL_CNT_INT_EN_CMD     (0x0d)
#define VLYNQ_IOCTL_CNT_INT_LOC_CMD    (0x0e)
#define VLYNQ_IOCTL_CNT_CLK_DIR_CMD    (0x0f)
#define VLYNQ_IOCTL_CNT_CLK_DIV_CMD    (0x10)   /* Write */
#define VLYNQ_IOCTL_CNT_CLK_MOD_CMD    (0x15)
#define VLYNQ_IOCTL_CNT_TX_FAST_CMD    (0x15)
#define VLYNQ_IOCTL_CNT_RTM_SELECT_CMD (0x16)
#define VLYNQ_IOCTL_CNT_RTM_VALIDWR_CMD (0x17u)
#define VLYNQ_IOCTL_CNT_RTM_SAMPLE_CMD (0x18)
#define VLYNQ_IOCTL_CNT_CLK_SLKPU_CMD  (0x1e)   /* Write */
#define VLYNQ_IOCTL_CNT_PMEM_CMD       (0x1f)   /* Write */

/* Status register bits. The minor id(s) for status reg. */
#define VLYNQ_IOCTL_STS_LINK           (0x00)
#define VLYNQ_IOCTL_STS_MPEND          (0x01)
#define VLYNQ_IOCTL_STS_SPEND          (0x02)
#define VLYNQ_IOCTL_STS_NFEMP0         (0x03)
#define VLYNQ_IOCTL_STS_NFEMP1         (0x04)
#define VLYNQ_IOCTL_STS_NFEMP2         (0x05)
#define VLYNQ_IOCTL_STS_NFEMP3         (0x06)
#define VLYNQ_IOCTL_STS_LERR           (0x07)
#define VLYNQ_IOCTL_STS_RERR           (0x08)
#define VLYNQ_IOCTL_STS_OFLOW          (0x09)
#define VLYNQ_IOCTL_STS_IFLOW          (0x0A)
#define VLYNQ_IOCTL_STS_RTM            (0x0B)
#define VLYNQ_IOCTL_STS_RTM_VAL        (0x0C)
#define VLYNQ_IOCTL_STS_SWIDOUT        (0x14)
#define VLYNQ_IOCTL_STS_MODESUP        (0x15)
#define VLYNQ_IOCTL_STS_SWIDTH         (0x18)
#define VLYNQ_IOCTL_STS_SWIDIN         (0x18)
#define VLYNQ_IOCTL_STS_DEBUG          (0x1d)

/* The VLYNQ registers; the reg id(s).
 *
 * For non Bit operations, they are minor id(s) for the raw 32 bit access
 * done using the major cmd VLYNQ_IOCTL_REG_CMD.
 *
 * For bit operations, they are the major id(s) and identify the register
 * whose bits have to be manipulated.
 *
 */
#define VLYNQ_IOCTL_REVSION_REG        (0x00)
#define VLYNQ_IOCTL_CONTROL_REG        (0x04)
#define VLYNQ_IOCTL_STATUS_REG         (0x08)
#define VLYNQ_IOCTL_INT_PRIR_REG       (0x0c)
#define VLYNQ_IOCTL_INT_STS_REG        (0x10)
#define VLYNQ_IOCTL_INT_PEND_REG       (0x14)
#define VLYNQ_IOCTL_INT_PTR_REG        (0x18)
#define VLYNQ_IOCTL_TX_MAP_REG         (0x1c)
#define VLYNQ_IOCTL_RX1_SZ_REG         (0x20)
#define VLYNQ_IOCTL_RX1_OFF_REG        (0x24)
#define VLYNQ_IOCTL_RX2_SZ_REG         (0x28)
#define VLYNQ_IOCTL_RX2_OFF_REG        (0x2c)
#define VLYNQ_IOCTL_RX3_SZ_REG         (0x30)
#define VLYNQ_IOCTL_RX3_OFF_REG        (0x34)
#define VLYNQ_IOCTL_RX4_SZ_REG         (0x38)
#define VLYNQ_IOCTL_RX4_OFF_REG        (0x3c)
#define VLYNQ_IOCTL_CVR_REG            (0x40)
#define VLYNQ_IOCTL_AUTO_NEG_REG       (0x44)
#define VLYNQ_IOCTL_MAN_NEG_REG        (0x48)
#define VLYNQ_IOCTL_NEG_STS_REG        (0x4c)
#define VLYNQ_IOCTL_ENDIAN_REG         (0x5c)
#define VLYNQ_IOCTL_IVR30_REG          (0x60)
#define VLYNQ_IOCTL_IVR74_REG          (0x64)

/* Dumping options, they are not part of ioctl options.
 * But are for the Dump API. */
#define VLYNQ_DUMP_ALL_ROOT         (0x10000)
#define VLYNQ_DUMP_RAW_DATA         (0x20000)
#define VLYNQ_DUMP_ALL_REGS         (0x30000)
#define VLYNQ_DUMP_STS_REG          (0x00008)
#define VLYNQ_DUMP_CNTL_REG         (0x00004)

#endif              /* #ifndef __VLYNQ_IOCTL_H__ */
