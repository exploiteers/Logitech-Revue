/*
 * linux/drivers/net/davinci_emac.c
 *
 * Copyright (C) 2006 Texas Instruments.
 *
 * ----------------------------------------------------------------------------
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
 * ----------------------------------------------------------------------------
 Modifications:
 ver. 0.0 Suraj Iyer - Original Linux drive
      0.1 Anant Gole - Recoded as per TI PSPF architecture (converted to DDA)
      2.0 Suraj Iyer, Sharath Kumar, Ajay Singh - Completed for TI BCG
      3.0 Anant Gole - Modified and ported for DaVinci
      4.0 Kevin Hilman, Anant Gole - Linuxification of the driver
      4.? Paul Bartholomew - Use PHY_DUPLEX_* constants instead
                             of hard-coded numbers.  Also, fixed
                             EMAC_IOCTL_READ_PHY_REG in emac_control() -
                             the phy_num and reg_addr args were swapped
                             in call to emac_mdio_read().
      5.0 Sriramakrishnan - Modified and added support for dm6467
 */

/*
    Driver Features:

    The following flags should be defined by the make file for support
    of the features:

    (1) EMAC_CACHE_WRITEBACK_MODE to support write back cache mode.

    (2) EMAC_MULTIPACKET_TX_COMPLETE_NOTIFY - to support of multiple
        Tx complete notifications. If this is defined the Tx complete
        DDA callback function contains multiple packet Tx complete
        events.  Note: BY DEFAULT THIS DRIVER HANDLES MULTIPLE TX
        COMPLETE VIA ITS CALLBACK IN THE SAME FUNCTION FOR SINGLE
        PACKET COMPLETE NOTIFY.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/in.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ethtool.h>
#include <linux/highmem.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <linux/version.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/davinci_emac.h>
#include <linux/phy.h>

#include <asm/irq.h>
#include <asm/bitops.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <asm/page.h>
#include <asm/arch/memory.h>
#include <asm/arch/hardware.h>
#include <asm/arch/cpu.h>

/* ---------------------------------------------------------------
 * linux module options
 * --------------------------------------------------------------- */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Davinci EMAC Maintainer: Anant Gole <anantgole@ti.com>");
MODULE_DESCRIPTION("DaVinci Ethernet driver - EMAC (EMAC)");

static int cfg_link_speed = 0;
module_param(cfg_link_speed, int, 0);
MODULE_PARM_DESC(cfg_link_speed, "Fixed speed of the Link: <1000/100/10>");

static char *cfg_link_mode = "auto";
module_param(cfg_link_mode, charp, 0);
MODULE_PARM_DESC(cfg_link_mode, "Fixed mode of the Link: <fd/hd>");

/* version info */
#define EMAC_MAJOR_VERSION	4
#define EMAC_MINOR_VERSION	0
#define EMAC_MODULE_VERSION	"4.0"
MODULE_VERSION(EMAC_MODULE_VERSION);
const char emac_version_string[] = "TI DaVinci EMAC Linux version updated 4.0";

/* Debug options */
#define EMAC_DEBUG
#define EMAC_CACHE_WRITEBACK_MODE
#define EMAC_CACHE_INVALIDATE_FIX

/* ---------------------------------------------------------------
 * types
 * --------------------------------------------------------------- */
#define TRUE		((bool) 1)
#define FALSE		((bool) 0)

typedef void *emac_net_data_token;

/* ---------------------------------------------------------------
 * defines
 * --------------------------------------------------------------- */
#define EMAC_MULTIPACKET_TX_COMPLETE_NOTIFY

/* NO PHY used in case of external ethernet switches */
#define CONFIG_EMAC_NOPHY	9999

/* feature macros here */

/* If multi packet Tx complete notifications is enabled (via
   EMAC_MULTIPACKET_TX_COMPLETE_NOTIFY), Max number of Tx packets that
   can be notified - the actual number will depend upon user
   configuration for parameter "maxPktsToProcess" */
#define EMAC_MAX_TX_COMPLETE_PKTS_TO_NOTIFY	8


/* config macros */
#define EMAC_MIN_ETHERNET_PKT_SIZE		60

/* max RX fragments calculation - 1500 byte packet and 64 byte
   buffer. fragments=1500/64=24 */
#define EMAC_MAX_RX_FRAGMENTS			24

/* theoratically TX max fragments are equal to 24 */
#define EMAC_MAX_TX_FRAGMENTS			8

/* EMAC hardware specific */
#define EMAC_RESET_CLOCKS_WAIT			64
#define EMAC_MAX_TXRX_CHANNELS			8 /* Max hardware channels */
#define EMAC_MIN_FREQUENCY_FOR_10MBPS		5500000
#define EMAC_MIN_FREQUENCY_FOR_100MBPS		55000000
#define EMAC_MIN_FREQUENCY_FOR_1000MBPS		125000000

/*
 * The following are EMAC registers which have been removed from the
 * CPGMAC register map. Thus we access them using macros to avoid
 * having more CSL register overlay structures for older EMAC register
 * map.
 */

/* statistics clear value */
#define EMAC_NUM_STAT_REGS			36
#define EMAC_STAT_CLEAR				0xFFFFFFFF

/* EMAC all multicast set register value */
#define EMAC_ALL_MULTI_REG_VALUE		0xFFFFFFFF

/* EMAC number of multicast bits that can be set/cleared - currently
   64 bits - hash1/2 regs */
#define EMAC_NUM_MULTICAST_BITS			64

/* EMAC teardown value */
#define EMAC_TEARDOWN_VALUE			0xFFFFFFFC

/* TX / RX control bits */
#define EMAC_TX_CONTROL_TX_ENABLE_VAL		0x1
#define EMAC_RX_CONTROL_RX_ENABLE_VAL		0x1

/* host interrupt bits */
#define EMAC_MAC_HOST_ERR_INTMASK_VAL		0x2
#define EMAC_MAC_STAT_INT_INTMASK_VAL		0x1

/* rx config masks */
#define EMAC_RX_UNICAST_CLEAR_ALL		0xFF

/* type 0 address filtering macros */
#define EMAC_TYPE_0_MACSRCADDR0_MASK		(0xFF)
#define EMAC_TYPE_0_MACSRCADDR0_SHIFT		0
#define EMAC_TYPE_0_MACSRCADDR1_MASK		(0xFF)
#define EMAC_TYPE_0_MACSRCADDR1_SHIFT		0

#define EMAC_TYPE_0_MACSRCADDR2_MASK		(0xFF<<24)
#define EMAC_TYPE_0_MACSRCADDR2_SHIFT		24
#define EMAC_TYPE_0_MACSRCADDR3_MASK		(0xFF<<16)
#define EMAC_TYPE_0_MACSRCADDR3_SHIFT		16
#define EMAC_TYPE_0_MACSRCADDR4_MASK		(0xFF<<8)
#define EMAC_TYPE_0_MACSRCADDR4_SHIFT		8
#define EMAC_TYPE_0_MACSRCADDR5_MASK		(0xFF)
#define EMAC_TYPE_0_MACSRCADDR5_SHIFT		0

/* type 1 address filtering macros */
#define EMAC_TYPE_1_MACSRCADDR0_MASK		(0xFF<<8)
#define EMAC_TYPE_1_MACSRCADDR0_SHIFT		8
#define EMAC_TYPE_1_MACSRCADDR1_MASK		(0xFF)
#define EMAC_TYPE_1_MACSRCADDR1_SHIFT		0

#define EMAC_TYPE_1_MACSRCADDR2_MASK		(0xFF<<24)
#define EMAC_TYPE_1_MACSRCADDR2_SHIFT		24
#define EMAC_TYPE_1_MACSRCADDR3_MASK		(0xFF<<16)
#define EMAC_TYPE_1_MACSRCADDR3_SHIFT		16
#define EMAC_TYPE_1_MACSRCADDR4_MASK		(0xFF<<8)
#define EMAC_TYPE_1_MACSRCADDR4_SHIFT		8
#define EMAC_TYPE_1_MACSRCADDR5_MASK		(0xFF)
#define EMAC_TYPE_1_MACSRCADDR5_SHIFT		0

/* CP(G)MAC address filtering bit macros */
#define CPGMAC_VALID_MASK			(0x1<<20)
#define CPGMAC_VALID_SHIFT			20
#define CPGMAC_MATCH_FILTER_MASK		(0x1<<19)
#define CPGMAC_MATCH_FILTER_SHIFT		19
#define CPGMAC_CHANNEL_MASK			(0x7<<16)
#define CPGMAC_CHANNEL_SHIFT			16
#define CPGMAC_TYPE_2_3_MACSRCADDR0_MASK	(0xFF<<8)
#define CPGMAC_TYPE_2_3_MACSRCADDR0_SHIFT	8
#define CPGMAC_TYPE_2_3_MACSRCADDR1_MASK	(0xFF)
#define CPGMAC_TYPE_2_3_MACSRCADDR1_SHIFT	0

#define CPGMAC_TYPE_2_3_MACSRCADDR2_MASK	(0xFF<<24)
#define CPGMAC_TYPE_2_3_MACSRCADDR2_SHIFT	24
#define CPGMAC_TYPE_2_3_MACSRCADDR3_MASK	(0xFF<<16)
#define CPGMAC_TYPE_2_3_MACSRCADDR3_SHIFT	16
#define CPGMAC_TYPE_2_3_MACSRCADDR4_MASK	(0xFF<<8)
#define CPGMAC_TYPE_2_3_MACSRCADDR4_SHIFT	8
#define CPGMAC_TYPE_2_3_MACSRCADDR5_MASK	(0xFF)
#define CPGMAC_TYPE_2_3_MACSRCADDR5_SHIFT	0

/* RX MBP register bit positions */
#define EMAC_RXMBP_PASSCRC_SHIFT		30
#define EMAC_RXMBP_PASSCRC_MASK			(0x1 << 30)
#define EMAC_RXMBP_QOSEN_SHIFT			29
#define EMAC_RXMBP_QOSEN_MASK			(0x1 << 29)
#define EMAC_RXMBP_NOCHAIN_SHIFT		28
#define EMAC_RXMBP_NOCHAIN_MASK			(0x1 << 28)
#define EMAC_RXMBP_CMFEN_SHIFT			24
#define EMAC_RXMBP_CMFEN_MASK			(0x1 << 24)
#define EMAC_RXMBP_CSFEN_SHIFT			23
#define EMAC_RXMBP_CSFEN_MASK			(0x1 << 23)
#define EMAC_RXMBP_CEFEN_SHIFT			22
#define EMAC_RXMBP_CEFEN_MASK			(0x1 << 22)
#define EMAC_RXMBP_CAFEN_SHIFT			21
#define EMAC_RXMBP_CAFEN_MASK			(0x1 << 21)
#define EMAC_RXMBP_PROMCH_SHIFT			16
#define EMAC_RXMBP_PROMCH_MASK			(0x7 << 16)
#define EMAC_RXMBP_BROADEN_SHIFT		13
#define EMAC_RXMBP_BROADEN_MASK			(0x1 << 13)
#define EMAC_RXMBP_BROADCH_SHIFT		8
#define EMAC_RXMBP_BROADCH_MASK			(0x7 << 8)
#define EMAC_RXMBP_MULTIEN_SHIFT		5
#define EMAC_RXMBP_MULTIEN_MASK			(0x1 << 5)
#define EMAC_RXMBP_MULTICH_SHIFT		0
#define EMAC_RXMBP_MULTICH_MASK			0x7

#define EMAC_RXMBP_CHMASK			0x7

/* mac control register bit fields */
#define EMAC_MACCONTROL_GIG_FORCE_SHIFT        17
#define EMAC_MACCONTROL_RMIISPEED_SHIFT        15
#define EMAC_MACCONTROL_RMIISPEED_MASK         (0x1 << 15)
#define EMAC_MACCONTROL_RXOFFLENBLOCK_SHIFT    14
#define EMAC_MACCONTROL_RXOFFLENBLOCK_MASK     (0x1 << 14)
#define EMAC_MACCONTROL_RXOWNERSHIP_SHIFT      13
#define EMAC_MACCONTROL_RXOWNERSHIP_MASK       (0x1 << 13)
#define EMAC_MACCONTROL_CMDIDLE_SHIFT          11
#define EMAC_MACCONTROL_CMDIDLE_MASK           (0x1 << 11)
#define EMAC_MACCONTROL_TXSHORTGAPEN_SHIFT     10
#define EMAC_MACCONTROL_TXSHORTGAPEN_MASK      (0x1 << 10)
#define EMAC_MACCONTROL_TXPTYPE_SHIFT          9
#define EMAC_MACCONTROL_TXPTYPE_MASK           (0x1 << 9)
#define EMAC_MACCONTROL_GIGABITEN_SHIFT        7
#define EMAC_MACCONTROL_GIGABITEN_MASK         (0x1 << 7)
#define EMAC_MACCONTROL_TXPACEEN_SHIFT         6
#define EMAC_MACCONTROL_TXPACEEN_MASK          (0x1 << 6)
#define EMAC_MACCONTROL_MIIEN_SHIFT            5
#define EMAC_MACCONTROL_MIIEN_MASK             (0x1 << 5)
#define EMAC_MACCONTROL_TXFLOWEN_SHIFT         4
#define EMAC_MACCONTROL_TXFLOWEN_MASK          (0x1 << 4)
#define EMAC_MACCONTROL_RXFLOWEN_SHIFT         3
#define EMAC_MACCONTROL_RXFLOWEN_MASK          (0x1 << 3)
#define EMAC_MACCONTROL_LOOPBKEN_SHIFT         1
#define EMAC_MACCONTROL_LOOPBKEN_MASK          (0x1 << 1)
#define EMAC_MACCONTROL_FULLDUPLEXEN_SHIFT     0
#define EMAC_MACCONTROL_FULLDUPLEXEN_MASK      (0x1)

/* mac_status register */
#define EMAC_MACSTATUS_TXERRCODE_MASK		0xF00000
#define EMAC_MACSTATUS_TXERRCODE_SHIFT		20
#define EMAC_MACSTATUS_TXERRCH_MASK		0x7
#define EMAC_MACSTATUS_TXERRCH_SHIFT		16
#define EMAC_MACSTATUS_RXERRCODE_MASK		0xF000
#define EMAC_MACSTATUS_RXERRCODE_SHIFT		12
#define EMAC_MACSTATUS_RXERRCH_MASK		0x7
#define EMAC_MACSTATUS_RXERRCH_SHIFT		8

/* EMAC RX max packet length mask */
#define EMAC_RX_MAX_LEN_SHIFT			0
#define EMAC_RX_MAX_LEN_MASK			0xFFFF

/* EMAC RX max packet length mask */
#define EMAC_RX_BUFFER_OFFSET_SHIFT		0
#define EMAC_RX_BUFFER_OFFSET_MASK		0xFFFF

/* MAC_IN_VECTOR (0x180) register bit fields */
#define EMAC_MAC_IN_VECTOR_HOST_INT		(0x20000)
#define EMAC_MAC_IN_VECTOR_STATPEND_INT		(0x10000)
#define EMAC_MAC_IN_VECTOR_RX_INT_VEC		(0xFF00)
#define EMAC_MAC_IN_VECTOR_TX_INT_VEC		(0xFF)
#define EMAC_MAC_IN_VECTOR_RX_INT_SHIFT		(8)

/** NOTE:: For davincihd the IN_VECTOR has changed*/
#define EMAC_DM646X_MAC_IN_VECTOR_HOST_INT	(0x04000000)
#define EMAC_DM646X_MAC_IN_VECTOR_STATPEND_INT	(0x08000000)
#define EMAC_DM646X_MAC_IN_VECTOR_RX_INT_VEC	(0xFF)
#define EMAC_DM646X_MAC_IN_VECTOR_TX_INT_VEC	(0xFF0000)
#define EMAC_DM646X_MAC_IN_VECTOR_TX_INT_SHIFT	(16)
#define EMAC_DM646X_MAC_IN_VECTOR_RX_INT_SHIFT	(0)

/* EMAC EOI codes for C0 */
#define EMAC_DM646X_MAC_EOI_C0_RXTHRESH		(0x00)
#define EMAC_DM646X_MAC_EOI_C0_RXEN		(0x01)
#define EMAC_DM646X_MAC_EOI_C0_TXEN		(0x02)
#define EMAC_DM646X_MAC_EOI_C0_MISCEN		(0x03)

/* CPPI bit positions */
#define EMAC_CPPI_SOP_BIT			(1 << 31)
#define EMAC_CPPI_EOP_BIT			(1 << 30)
#define EMAC_CPPI_OWNERSHIP_BIT			(1 << 29)
#define EMAC_CPPI_EOQ_BIT			(1 << 28)
#define EMAC_CPPI_TEARDOWN_COMPLETE_BIT		(1 << 27)
#define EMAC_CPPI_PASS_CRC_BIT			(1 << 26)

/* defining the macro EMAC_INSTANCE_CODE to 0 so that it can be usable in DDA */
#define EMAC_INSTANCE_CODE			0
#define EMAC_ERROR_CODE				(EMAC_INSTANCE_CODE << 16)
#define EMAC_ERROR_INFO				(EMAC_ERROR_CODE)
#define EMAC_ERROR_WARNING			(EMAC_ERROR_CODE | 0x10000000)
#define EMAC_ERROR_MINOR			(EMAC_ERROR_CODE | 0x20000000)
#define EMAC_ERROR_MAJOR			(EMAC_ERROR_CODE | 0x30000000)
#define EMAC_ERROR_CRITICAL			(EMAC_ERROR_CODE | 0x40000000)

/* EMAC success code */
#define EMAC_SUCCESS				0

/* EMAC error codes */
#define EMAC_ERR_DEV_ALREADY_INSTANTIATED(inst_id) \
		(0x30000000 + ((inst_id) << 16))
#define EMAC_ERR_DEV_NOT_INSTANTIATED		(EMAC_ERROR_MAJOR + 1)
#define EMAC_INVALID_PARAM			(EMAC_ERROR_MAJOR + 2)
#define EMAC_ERR_TX_CH_INVALID			(EMAC_ERROR_CRITICAL + 3)
#define EMAC_ERR_TX_CH_ALREADY_INIT		(EMAC_ERROR_MAJOR + 4)
#define EMAC_ERR_TX_CH_ALREADY_CLOSED		(EMAC_ERROR_MAJOR + 5)
#define EMAC_ERR_TX_CH_NOT_OPEN			(EMAC_ERROR_MAJOR + 6)
#define EMAC_ERR_TX_NO_LINK			(EMAC_ERROR_MAJOR + 7)
#define EMAC_ERR_TX_OUT_OF_BD			(EMAC_ERROR_MAJOR + 8)
#define EMAC_ERR_RX_CH_INVALID			(EMAC_ERROR_CRITICAL + 9)
#define EMAC_ERR_RX_CH_ALREADY_INIT		(EMAC_ERROR_MAJOR + 10)
#define EMAC_ERR_RX_CH_ALREADY_CLOSED		(EMAC_ERROR_MAJOR + 11)
#define EMAC_ERR_RX_CH_NOT_OPEN			(EMAC_ERROR_MAJOR + 12)
#define EMAC_ERR_DEV_ALREADY_CREATED		(EMAC_ERROR_MAJOR + 13)
#define EMAC_ERR_DEV_NOT_OPEN			(EMAC_ERROR_MAJOR + 14)
#define EMAC_ERR_DEV_ALREADY_CLOSED		(EMAC_ERROR_MAJOR + 15)
#define EMAC_ERR_DEV_ALREADY_OPEN		(EMAC_ERROR_MAJOR + 16)
#define EMAC_ERR_RX_BUFFER_ALLOC_FAIL		(EMAC_ERROR_CRITICAL + 17)

/*
 * ioctls
 */
#define EMAC_IOCTL_BASE				0

#define EMAC_IOCTL_GET_STATISTICS		(EMAC_IOCTL_BASE + 0)
#define EMAC_IOCTL_CLR_STATISTICS		(EMAC_IOCTL_BASE + 1)
#define EMAC_IOCTL_GET_SWVER			(EMAC_IOCTL_BASE + 2)
#define EMAC_IOCTL_GET_HWVER			(EMAC_IOCTL_BASE + 3)
#define EMAC_IOCTL_SET_RXCFG			(EMAC_IOCTL_BASE + 4)
#define EMAC_IOCTL_GET_STATUS			(EMAC_IOCTL_BASE + 6)
#define EMAC_IOCTL_READ_PHY_REG			(EMAC_IOCTL_BASE + 7)
#define EMAC_IOCTL_WRITE_PHY_REG		(EMAC_IOCTL_BASE + 8)
/* add/delete */
#define EMAC_IOCTL_MULTICAST_ADDR		(EMAC_IOCTL_BASE + 9)
/* set/clear */
#define EMAC_IOCTL_ALL_MULTI			(EMAC_IOCTL_BASE + 10)
#define EMAC_IOCTL_TYPE2_3_FILTERING		(EMAC_IOCTL_BASE + 11)
#define EMAC_IOCTL_SET_MAC_ADDRESS		(EMAC_IOCTL_BASE + 12)
#define EMAC_IOCTL_IF_COUNTERS			(EMAC_IOCTL_BASE + 13)
#define EMAC_IOCTL_ETHER_COUNTERS		(EMAC_IOCTL_BASE + 14)
#define EMAC_IOCTL_IF_PARAMS_UPDT		(EMAC_IOCTL_BASE + 15)

#define EMAC_PRIV_FILTERING			(EMAC_IOCTL_BASE + 21)
#define EMAC_PRIV_MII_READ			(EMAC_IOCTL_BASE + 22)
#define EMAC_PRIV_MII_WRITE			(EMAC_IOCTL_BASE + 23)
#define EMAC_PRIV_GET_STATS			(EMAC_IOCTL_BASE + 24)
#define EMAC_PRIV_CLR_STATS			(EMAC_IOCTL_BASE + 25)
#define EMAC_EXTERNAL_SWITCH			(EMAC_IOCTL_BASE + 26)

/*
 * MII module port settings
 *
 * DDA sets the Phy mode as a combination of the following in "phyMode"
 * parameter in the init configuration structure
 */
#define SNWAY_AUTOMDIX	(1<<16)	/* bit 16 and above not used by MII register */
#define SNWAY_FD1000	(1<<13)
#define SNWAY_HD1000	(1<<12)
#define SNWAY_NOPHY	(1<<10)
#define SNWAY_LPBK	(1<<9)
#define SNWAY_FD100	(1<<8)
#define SNWAY_HD100	(1<<7)
#define SNWAY_FD10	(1<<6)
#define SNWAY_HD10	(1<<5)
#define SNWAY_AUTO	(1<<0)
#define DM644X_SNWAY_AUTOALL	(SNWAY_AUTO|SNWAY_FD100|SNWAY_FD10|\
				SNWAY_HD100|SNWAY_HD10)
#define DM646X_SNWAY_AUTOALL	(SNWAY_AUTO|SNWAY_FD1000|SNWAY_HD1000|\
				SNWAY_FD100|SNWAY_FD10|SNWAY_HD100|SNWAY_HD10)
#define SNWAY_AUTOALL	((cpu_is_davinci_dm6467()) ? DM646X_SNWAY_AUTOALL : \
			DM644X_SNWAY_AUTOALL)

/* phy mode values */
#define NWAY_AUTOMDIX	(1<<16)
#define NWAY_FD1000	(1<<13)
#define NWAY_HD1000	(1<<12)
#define NWAY_NOPHY	(1<<10)
#define NWAY_LPBK	(1<<9)
#define NWAY_FD100	(1<<8)
#define NWAY_HD100	(1<<7)
#define NWAY_FD10	(1<<6)
#define NWAY_HD10	(1<<5)
#define NWAY_AUTO	(1<<0)

/* phy duplex values */
#define	PHY_DUPLEX_AUTO		0	/* Auto Negotiate */
#define	PHY_DUPLEX_UNKNOWN	1	/* Unknown */
#define	PHY_DUPLEX_HALF		2	/* Half Duplex */
#define	PHY_DUPLEX_FULL		3	/* Full Duplex */

/**
 * DDC Status Ioctl - Error status
 *
 * Note that each error code is a bit position so that multiple
 * errors can be clubbed together and passed in a integer value
 */
#define EMAC_NO_ERROR		0
#define EMAC_TX_HOST_ERROR	0x1	/* MSB 8 bits: err code, channel no */
#define EMAC_RX_HOST_ERROR	0x8	/* LSB 8 bits: err code, channel no */

#define EGRESS_TRAILOR_LEN	0

/* defaut configuration values required for passing on to DDC */
#define EMAC_DEFAULT_MLINK_MASK				0
#define EMAC_DEFAULT_PASS_CRC				FALSE
#define EMAC_DEFAULT_QOS_ENABLE				FALSE
#define EMAC_DEFAULT_NO_BUFFER_CHAINING			FALSE
#define EMAC_DEFAULT_COPY_MAC_CONTROL_FRAMES_ENABLE	FALSE
#define EMAC_DEFAULT_COPY_SHORT_FRAMES_ENABLE		FALSE
#define EMAC_DEFAULT_COPY_ERROR_FRAMES_ENABLE		FALSE
#define EMAC_DEFAULT_PROMISCOUS_CHANNEL			0
#define EMAC_DEFAULT_BROADCAST_CHANNEL			0
#define EMAC_DEFAULT_MULTICAST_CHANNEL			0
#define EMAC_DEFAULT_BUFFER_OFFSET			0
#define EMAC_DEFAULT_TX_SHORT_GAP_ENABLE		FALSE
#define EMAC_DEFAULT_TX_PACING_ENABLE			FALSE
#define EMAC_DEFAULT_MII_ENABLE				TRUE
#define EMAC_DEFAULT_TX_FLOW_ENABLE			FALSE
#define EMAC_DEFAULT_RX_FLOW_ENABLE			FALSE
#define EMAC_DEFAULT_LOOPBACK_ENABLE			FALSE
#define EMAC_DEFAULT_FULL_DUPLEX_ENABLE			TRUE
#define EMAC_DEFAULT_TX_INTERRUPT_DISABLE		FALSE
#define CONFIG_EMAC_MIB_TIMER_TIMEOUT			5000	/* 5 seconds */

#define EMAC_DEFAULT_PROMISCOUS_ENABLE			0
#define EMAC_DEFAULT_BROADCAST_ENABLE			1
#define EMAC_DEFAULT_MULTICAST_ENABLE			1

/* system value for ticks per seconds */
#define EMAC_TICKS_PER_SEC	HZ

/*
 * Extra bytes for Cache alignment of skbuf - should be equal to
 * processor cache line size - in case of ARM926 its 32 bytes
 */
#define EMAC_DEFAULT_EXTRA_RXBUF_SIZE	32

/*
 * Default max frame size = 1522 = 1500 byte data + 14 byte eth header
 * + 4 byte checksum + 4 byte vlan tag + 32 bytes for cache
 * alignment
 */
#define EMAC_DEFAULT_MAX_FRAME_SIZE	(1500 + 14 + 4 + 4 + \
		EGRESS_TRAILOR_LEN + EMAC_DEFAULT_EXTRA_RXBUF_SIZE)

/* default number of TX channels */
#define EMAC_DEFAULT_NUM_TX_CHANNELS	1

/* default TX channel number */
#define EMAC_DEFAULT_TX_CHANNEL		0

/* default TX number of BD's/buffers */
#define EMAC_DEFAULT_TX_NUM_BD		128

/* default TX max service BD's */
#define EMAC_DEFAULT_TX_MAX_SERVICE	8

/* default number of RX channels */
#define EMAC_DEFAULT_NUM_RX_CHANNELS	1

/* default RX channel number */
#define EMAC_DEFAULT_RX_CHANNEL		0

#define EMAC_DEFAULT_RX_NUM_BD		128

/* default RX max service BD's */
#define EMAC_DEFAULT_RX_MAX_SERVICE	32	/* should = netdev->weight */

#if ((EMAC_DEFAULT_TX_NUM_BD + EMAC_DEFAULT_RX_NUM_BD) > 256)
#error "Error. DaVinci has space for no more than 256 TX+RX BD's"
#endif

/*
 * Size of EMAC peripheral footprint in memory that needs to be
 * reserved in Linux Note that this value is actually a hardware
 * memory footprint value taken from the specs and ideally should have
 * been in the csl files. Keeping it for convinience since EMAC
 * peripheral footprint will not change unless the peripheral itself
 * changes drastically and it will be called with a different name and
 * will have a different driver anyway
 *
 * For Davinci size = control regs (4k) + wrapper regs (4k) + wrapper
 * RAM (8k) + mdio regs (2k)
 */
#define EMAC_DEFAULT_EMAC_SIZE			0x4800

/* Maximum multicast addresses list to be handled by the driver - If
 * this is not restricted then the driver will spend considerable time
 * in handling multicast lists
 */
#define EMAC_DEFAULT_MAX_MULTICAST_ADDRESSES	64

#define NETDEV_PRIV(net_dev)	netdev_priv(net_dev)
#define FREE_NETDEV(net_dev)	free_netdev(net_dev)

/* misc error codes */
#define EMAC_INTERNAL_FAILURE	-1

/* LED codes required for controlling LED's */
#define EMAC_LINK_OFF		0
#define EMAC_LINK_ON		1
#define EMAC_SPEED_100		2
#define EMAC_SPEED_10		3
#define EMAC_FULL_DPLX		4
#define EMAC_HALF_DPLX		5
#define EMAC_TX_ACTIVITY	6
#define EMAC_RX_ACTIVITY	7

/*
 * L3 Alignment mechanism: The below given macro returns the number of
 * bytes required to align the given size to a L3 frame 4 byte
 * boundry. This is typically required to add 2 bytes to the ethernet
 * frame start to make sure the IP header (L3) is aligned on a 4 byte
 * boundry
 */
static char emac_L3_align[] = { 0x02, 0x01, 0x00, 0x03 };

#define EMAC_L3_ALIGN(size)	emac_L3_align[(size) & 0x3]

/* 4 Byte alignment for skb's:
 *
 * Currently Skb's dont need to be on a 4 byte boundry, but other OS's
 * have such requirements Just to make sure we comply if there is any
 * such requirement on SKB's in future, we align it on a 4 byte
 * boundry.
 */
char emac_4byte_align[] = { 0x0, 0x03, 0x02, 0x1 };

#define EMAC_4BYTE_ALIGN(size) emac_4byte_align[(size) & 0x3]

#define EMAC_DEBUG_FUNCTION_ENTRY	(0x1 << 1)
#define EMAC_DEBUG_FUNCTION_EXIT	(0x1 << 2)
#define EMAC_DEBUG_BUSY_FUNCTION_ENTRY	(0x1 << 3)
#define EMAC_DEBUG_BUSY_FUNCTION_EXIT	(0x1 << 4)
#define EMAC_DEBUG_TX			(0x1 << 6)
#define EMAC_DEBUG_RX			(0x1 << 7)
#define EMAC_DEBUG_PORT_UPDATE		(0x1 << 8)
#define EMAC_DEBUG_MII			(0x1 << 9)
#define EMAC_DEBUG_TEARDOWN		(0x1 << 10)

/* Debug flags
 *
 * IMPORTANT NOTE: The debug flags need to be enabled carefully as it
 * could flood the console/sink point of the debug traces and also
 * affect the functionality of the overall system
 */
#ifdef EMAC_DEBUG
#define LOG(lvl, format, args...) \
		printk(lvl "%s:%d" format, __func__, __LINE__, ##args)
#define LOGERR(format, args...) LOG(KERN_ERR, format, ##args)
#define LOGMSG(flag, format, args...) \
		do { if (flag & emac_debug) \
			LOG(KERN_DEBUG, #flag format, ##args); \
		} while (0)
#define ERR(format, args...) \
		printk(KERN_ERR "ERROR: davinci_emac: " format, ##args)
#else
#define LOGERR(format, args...)
#define LOGMSG(flag, format, args... )
#endif

/* DDC internal macros */
#define EMAC_RX_BD_BUF_SIZE		0xFFFF;
#define EMAC_BD_LENGTH_FOR_CACHE	16	/* only CPPI bytes */
#define EMAC_RX_BD_PKT_LENGTH_MASK	0xFFFF

/*
 * MDIO registers
 */

#define MDIO_VERSION		0x00 /* MDIO Version Register */
#define MDIO_CONTROL		0x04 /* MDIO Control Register */
#define MDIO_ALIVE		0x08 /* PHY Alive Status register */
#define MDIO_LINK		0x0c /* PHY Link Status Register */
#define MDIO_LINKINTRAW		0x10 /* Status Change Interrupt (Unmasked) */
#define MDIO_LINKINTMASKED	0x14 /* Status Change Interrupt (Masked) */
#define MDIO_USERINTRAW		0x20 /* Command Complete Interrupt (Unmasked) */
#define MDIO_USERINTMASKED	0x24 /* Command Complete Interrupt (Masked) */
#define MDIO_USERINTMASKSET	0x28 /* Command Complete Interrupt Mask Set */
#define MDIO_USERINTMASKCLEAR	0x2c /* Command Complete Interrupt Mask Clear */
#define MDIO_USERACCESS0	0x80 /* Access Register 0 */
#define MDIO_USERPHYSEL0	0x84 /* PHY Select Register 0 */
#define MDIO_USERACCESS1	0x88 /* Access Register 1 */
#define MDIO_USERPHYSEL1	0x8c /* PHY Select Register 1 */

#define MDIO_CONTROL_CLKDIV	(0xffff)
#define MDIO_CONTROL_ENABLE	(1 << 30)

#define MDIO_USERACCESS_GO	(1 << 31)
#define MDIO_USERACCESS_WRITE	(1 << 30)
#define MDIO_USERACCESS_READ	(0 << 30)
#define MDIO_USERACCESS_WRITE	(1 << 30)
#define MDIO_USERACCESS_REGADR	(0x1F << 21)
#define MDIO_USERACCESS_PHYADR	(0x1F << 16)
#define MDIO_USERACCESS_DATA	(0xffff)

/* ---------------------------------------------------------------
 * structs, enums
 * --------------------------------------------------------------- */
typedef enum {
	DRV_CREATED,
	DRV_INITIALIZED,
	DRV_OPENED,
	DRV_CLOSED,
	DRV_DEINITIALIZED,
	DRV_POWERED_DOWN,
} emac_drv_state;

/**
 * Network Buffer Object
 *
 * Holds attributes of a buffer/fragment
 *
 * Send: Usually when the buffers are allocated by DDA, the
 * Start of Packet token will be the handle to the whole packet. This
 * token/handle should be good enough to free the packet or return to
 * its pool. When the buffers are allocated by DDC, typically token
 * for each buffer needs to be indicated (TxComplete) rather than
 * only the Start of Packet token.
 *
 * Receive: For each buffer the token will be a handle to the buffer
 * that can be used by the allocater (DDA or DDC) of the buffer to
 * free it or return to a pool.
 */
typedef struct {
	emac_net_data_token buf_token;
	char *data_ptr;
	int length;
} net_buf_obj;

/**
 * Network Packet Object
 *
 * Holds attributes of a network packet (NetBufObjs and packet size).
 */
typedef struct {
	emac_net_data_token pkt_token;	/* data token may hold tx/rx chan id */
	net_buf_obj *buf_list;	/* array of network buffer objects */
	int num_bufs;		/* number of network buffer objects */
	int pkt_length;		/* packet length (number of bytes) */
} net_pkt_obj;

/**
 * Net Channel State
 *
 * State of the channel (initialized, about to be closed, closed etc
 */
typedef enum {
	NET_CH_DIR_TX = 0,	/* transmit only */
	NET_CH_DIR_RX,		/* receive only */
	NET_CH_DIR_BIDIRECTIONAL, /* bidirectonaly - TX/RX */
	NET_CH_DIR_UNDEFINED	/* not defined */
} net_ch_dir;

/**
 * Net Channel State
 *
 * State of the channel (initialized, about to be closed, closed etc
 */
typedef enum {
	NET_CH_UNINITIALIZED = 0,
	NET_CH_INITIALIZED,
	NET_CH_OPENED,
	NET_CH_CLOSE_IN_PROGRESS,
	NET_CH_CLOSED
} net_ch_state;

/**
 * EMAC Peripheral Device Register Memory Layout structure
 *
 * The structure instance variable points to CP(G)MAC register space in
 * SOC memory map directly.
 * This is a template only, no memory is ever allocated for this!
 */
struct emac_regs {
	u32 tx_id_ver;
	u32 tx_control;
	u32 tx_teardown;
	u32 reserved1;
	u32 rx_id_ver;
	u32 rx_control;
	u32 rx_teardown;
	u32 pad2[25];
	u32 tx_int_stat_raw;
	u32 tx_int_stat_masked;
	u32 tx_int_mask_set;
	u32 tx_int_mask_clear;
	u32 mac_in_vector;
	u32 mac_EOI_vector;
	u32 pad3[2];
	u32 rx_int_stat_raw;
	u32 rx_int_stat_masked;
	u32 rx_int_mask_set;
	u32 rx_int_mask_clear;
	u32 mac_int_stat_raw;
	u32 mac_int_stat_masked;
	u32 mac_int_mask_set;
	u32 mac_int_mask_clear;
	u32 pad4[16];
	u32 rx_MBP_enable;
	u32 rx_unicast_set;
	u32 rx_unicast_clear;
	u32 rx_maxlen;
	u32 rx_buffer_offset;
	u32 rx_filter_low_thresh;
	u32 pad5[2];
	u32 rx_flow_thresh[8];
	u32 rx_free_buffer[8];
	u32 mac_control;
	u32 mac_status;
	u32 EMControl;
	u32 fifo_control;	/* CP(G)MAC added register */
	u32 mac_cfig;		/* CP(G)MAC added register */
	u32 soft_reset;		/* CP(G)MAC 2.6 added register */
	u32 pad6[22];
	u32 mac_src_addr_lo;	/* CP(G)MAC modified register */
	u32 mac_src_addr_hi;	/* CP(G)MAC modified register */
	u32 mac_hash1;
	u32 mac_hash2;
	u32 boff_test;
	u32 tpace_test;		/* CP(G)MAC modified register */
	u32 rx_pause;
	u32 tx_pause;
	u32 pad7[4];
	u32 rx_good_frames;
	u32 rx_broadcast_frames;
	u32 rx_multicast_frames;
	u32 rx_pause_frames;
	u32 rx_crcerrors;
	u32 rx_align_code_errors;
	u32 rx_oversized_frames;
	u32 rx_jabber_frames;
	u32 rx_undersized_frames;
	u32 rx_fragments;
	u32 rx_filtered_frames;
	u32 rx_qos_filtered_frames;
	u32 rx_octets;
	u32 tx_good_frames;
	u32 tx_broadcast_frames;
	u32 tx_multicast_frames;
	u32 tx_pause_frames;
	u32 tx_deferred_frames;
	u32 tx_collision_frames;
	u32 tx_single_coll_frames;
	u32 tx_mult_coll_frames;
	u32 tx_excessive_collisions;
	u32 tx_late_collisions;
	u32 tx_underrun;
	u32 tx_carrier_sense_errors;
	u32 tx_octets;
	u32 reg64octet_frames;
	u32 reg65t127octet_frames;
	u32 reg128t255octet_frames;
	u32 reg256t511octet_frames;
	u32 reg512t1023octet_frames;
	u32 reg1024t_upoctet_frames;
	u32 net_octets;
	u32 rx_sof_overruns;
	u32 rx_mof_overruns;
	u32 rx_dma_overruns;
	u32 pad8[156];		/* CP(G)MAC modified register */
	u32 mac_addr_lo;	/* CP(G)MAC added register */
	u32 mac_addr_hi;	/* CP(G)MAC added register */
	u32 mac_index;		/* CP(G)MAC added register */
	u32 pad9[61];		/* CP(G)MAC modified register */
	u32 tx_HDP[8];
	u32 rx_HDP[8];
	u32 tx_CP[8];
	u32 rx_CP[8];
};

/**
 * EMAC Addressing Type
 *
 * Addressing type based upon cfig register. For EMAC peripheral cfig
 * register reads a value of 0 i.e Type 0 addressing
 */
typedef enum {
	RX_ADDR_TYPE0 = 0,	/* old style used in (EMAC) */
	RX_ADDR_TYPE1 = 1,	/* new CPGMAC style */
	RX_ADDR_TYPE2 = 2,	/* new CPGMAC "filtering" style */
	RX_ADDR_TYPE3 = 3	/* new CPGMAC "filtering" style */
} emac_rx_addr_type;

/**
 * EMAC Single Multicast Ioctl - EMAC_IOCTL_MULTICAST_ADDR operations
 *
 * Add/Del operations for adding/deleting a single multicast address
 */
typedef enum {
	EMAC_MULTICAST_ADD = 0,
	EMAC_MULTICAST_DEL
} emac_single_multi_oper;

/**
 * EMAC All Multicast Ioctl - EMAC_IOCTL_ALL_MULTI operations
 *
 * Set/Clear all multicast operation
 */
typedef enum {
	EMAC_ALL_MULTI_SET = 0,
	EMAC_ALL_MULTI_CLR
} emac_all_multi_oper;

/**
 * MII Read/Write PHY register
 *
 * Parameters to read/write a PHY register via MII interface
 */
typedef struct {
	u32 phy_num;		/* phy number to be read/written */
	u32 reg_addr;		/* register to be read/written */
	u32 data;		/* data to be read/written */
} emac_phy_params;

/**
 * MAC Address params
 *
 * Parameters for Configuring Mac address
 */
typedef struct {
	u32 channel;
	char *mac_address;
} emac_address_params;

/**
 * Type 2/3 Addressing
 *
 * Parameters for programming CFIG 2/3 addressing mode
 *
 */
typedef struct {
	u32 channel;		/* channel to which this filtering params apply */
	char *mac_address;	/* mac address for filtering */
	int index;		/* index of filtering list to update */
	bool valid;		/* entry valid */
	int match;		/* entry matching */
} emac_type2_3_addr_filter_params;

/**
 * EMAC Hardware Statistics
 *
 * Statistics counters provided by EMAC Hardware. The names of the
 * counters in this structure are of "MIB style" and corrospond
 * directly to the hardware counters provided by EMAC
 */
typedef struct {
	u32 if_in_good_frames;
	u32 if_in_broadcasts;
	u32 if_in_multicasts;
	u32 if_in_pause_frames;
	u32 if_in_crcerrors;
	u32 if_in_align_code_errors;
	u32 if_in_oversized_frames;
	u32 if_in_jabber_frames;
	u32 if_in_undersized_frames;
	u32 if_in_fragments;
	u32 if_in_filtered_frames;
	u32 if_in_qos_filtered_frames;
	u32 if_in_octets;
	u32 if_out_good_frames;
	u32 if_out_broadcasts;
	u32 if_out_multicasts;
	u32 if_out_pause_frames;
	u32 if_deferred_transmissions;
	u32 if_collision_frames;
	u32 if_single_collision_frames;
	u32 if_multiple_collision_frames;
	u32 if_excessive_collision_frames;
	u32 if_late_collisions;
	u32 if_out_underrun;
	u32 if_carrier_sense_errors;
	u32 if_out_octets;
	u32 if64octet_frames;
	u32 if65to127octet_frames;
	u32 if128to255octet_frames;
	u32 if256to511octet_frames;
	u32 if512to1023octet_frames;
	u32 if1024to_upoctet_frames;
	u32 if_net_octets;
	u32 if_rx_sof_overruns;
	u32 if_rx_mof_overruns;
	u32 if_rx_dmaoverruns;
} emac_hw_statistics;

/**************************************************************************
 *                 MIB-2 Common MIB Constants                             *
 **************************************************************************/
#define MIB2_TRUTH_VALUE_TRUE	1
#define MIB2_TRUTH_VALUE_FALSE	2

/* MIB-2 interface admin/oper status values */
/* device is in operational status unless status is down. */
#define MIB2_STATUS_UP		1
#define MIB2_STATUS_DOWN	2
#define MIB2_STATUS_TEST	3
#define MIB2_STATUS_UNKNOWN	4
#define MIB2_STATUS_DORMANT	5

#define TI_SIOC_OFFSET		0

/* definitions for interface group MIB variables: GET */
#define TI_SIOCGINTFCOUNTERS	((TI_SIOC_OFFSET) + 0x01)
#define TI_SIOCGINTFPARAMS	((TI_SIOC_OFFSET) + 0x02)

/* SET command definitions */
#define TI_SIOCSINTFADMINSTATUS	((TI_SIOC_OFFSET) + 0x03)

/* definitions for ether-like group MIB variables: GET */
#define TI_SIOCGETHERCOUNTERS	((TI_SIOC_OFFSET) + 0x04)
#define TI_SIOCGETHERPARAMS	((TI_SIOC_OFFSET) + 0x05)

/* defines MIB II INTERFACE objects */
struct mib2_if_counters {
	unsigned long in_bytes_low;
	unsigned long in_bytes_high;
	unsigned long in_unicast_pkts_low;
	unsigned long in_unicast_pkts_high;
	unsigned long in_multicast_pkts_low;
	unsigned long in_multicast_pkts_high;
	unsigned long in_broadcast_pkts_low;
	unsigned long in_broadcast_pkts_high;
	unsigned long in_discard_pkts;
	unsigned long in_error_pkts;
	unsigned long in_unknown_prot_pkts;
	unsigned long out_bytes_low;
	unsigned long out_bytes_high;
	unsigned long out_unicast_pkts_low;
	unsigned long out_unicast_pkts_high;
	unsigned long out_multicast_pkts_low;
	unsigned long out_multicast_pkts_high;
	unsigned long out_broadcast_pkts_low;
	unsigned long out_broadcast_pkts_high;
	unsigned long out_discard_pkts;
	unsigned long out_error_pkts;
};

struct mib2_if_hccounters {
	struct mib2_if_counters mib2if_counter;
	unsigned long long in_bytes_hc;
	unsigned long long in_unicast_pkts_hc;
	unsigned long long in_multicast_pkts_hc;
	unsigned long long in_broadcast_pkts_hc;
	unsigned long long out_bytes_hc;
	unsigned long long out_unicast_pkts_hc;
	unsigned long long out_multicast_pkts_hc;
	unsigned long long out_broadcast_pkts_hc;
	unsigned long long in_bytes;
	unsigned long long in_unicast_pkts;
	unsigned long long in_multicast_pkts;
	unsigned long long in_broadcast_pkts;
	unsigned long long out_bytes;
	unsigned long long out_unicast_pkts;
	unsigned long long out_multicast_pkts;
	unsigned long long out_broadcast_pkts;
};

struct mib2_if_params {
	unsigned long if_speed;		/* speed in bits per second */
	unsigned long if_high_speed;	/* speed in mega-bits per second */
	long if_oper_status;
	long if_promiscuous_mode;
};

struct mib2_if_command {
	long if_admin_status;	/* desired interface state */
};

/* ether_like-MIB constants */
#define	MIB2_UNKNOWN_DUPLEX	1
#define	MIB2_HALF_DUPLEX 	2
#define	MIB2_FULL_DUPLEX	3

/* ioctl/cmd value to be used by snmpd like applications */
#define SIOTIMIB2	SIOCDEVPRIVATE + 1

/* defines MIB II ether_like-MIB objects */
struct mib2_phy_counters {
	unsigned long eth_alignment_errors;
	unsigned long eth_fcserrors;
	unsigned long eth_single_collisions;
	unsigned long eth_multiple_collisions;
	unsigned long eth_sqetest_errors;
	unsigned long eth_deferred_tx_frames;
	unsigned long eth_late_collisions;
	unsigned long eth_excessive_collisions;
	unsigned long eth_internal_mac_tx_errors;
	unsigned long eth_carrier_sense_errors;
	unsigned long eth_too_long_rx_frames;
	unsigned long eth_internal_mac_rx_errors;
	unsigned long eth_symbol_errors;
};

struct mib2_eth_params {
	long eth_duplex_status;	/* current emac duplex status */
};

typedef struct {
	unsigned long cmd;
	unsigned long port;
	void *data;
} TI_SNMP_CMD_T;

/**
 * DDC Status values
 *
 * Provides status of the device - error status, phy status etc
 */
typedef struct {
	u32 hw_status;
	u32 hw_err_info;
	u32 phy_linked;		/* link status: 1=linked, 0=no link */
	u32 phy_duplex;		/* duplex status: 3=full duplex, 2=half duplex*/

	u32 phy_speed;		/* link speed = 10, 100, 1000 */
	u32 phy_num;		/* phy number - useful if phy number is
				   discovered */
} emac_status;

/**
 * EMAC Channel Config Info
 *
 * Common to both TX/RX
 * Used to pass channel config info from DDA to DDC for EMAC channels
 */
typedef struct {
	int ch_num;		/* DDC_net_ch_info: channel number */
	net_ch_dir ch_dir;	/* DDC_net_ch_info: channel direction */
	net_ch_state ch_state;	/* DDC_net_ch_info: channel state */
	int num_bd;		/* number of BD (& buffers for RX) */
	int service_max;	/* maximum BD's processed in one go */
	int buf_size;		/* buffer size (applicable for RX only) */
} emac_ch_info;

typedef struct {
	u32 rx_pkts;		/* number of rx pkts to be processed */
	u32 tx_pkts;		/* number of tx pkts to be processed */
	u32 ret_rx_pkts;	/* number of rx pkts processed */
	u32 ret_tx_pkts;	/* number of tx pkts processed */
} rx_tx_params;

/*
 * EMAC Private Ioctl Structure
 *
 * Private Ioctl commands provided by the EMAC Linux Driver use this
 * structure
 */
typedef struct {
	unsigned int cmd;
	void *data;
} emac_drv_priv_ioctl;

/*
 * EMAC DDA maintained statistics
 *
 * Driver maintained statistics (apart from Hardware statistics)
 */
typedef struct {
	unsigned long tx_discards;
	unsigned long rx_discards;
	unsigned long start_tick;
} emac_drv_stats;

/**************************************************************************
 * Register Overlay Structure
 **************************************************************************/
struct ewrap_dm646x_regs {
	u32 IDVER;
	u32 SOFTRST;
	u32 EMCTRL;
	u32 INTCTRL;
	u32 C0_RXTHRESHEN;
	u32 C0_RXINTEN;
	u32 C0_TXINTEN;
	u32 C0_MISCEN;
	u32 C1_RXTHRESHEN;
	u32 C1_RXINTEN;
	u32 C1_TXINTEN;
	u32 C1_MISCEN;
	u32 C2_RXTHRESHEN;
	u32 C2_RXINTEN;
	u32 C2_TXINTEN;
	u32 C2_MISCEN;
	u32 C0_RXTHRESHSTAT;
	u32 C0_RXINTSTAT;
	u32 C0_TXINTSTAT;
	u32 C0_MISCSTAT;
	u32 C1_RXTHRESHSTAT;
	u32 C1_RXINTSTAT;
	u32 C1_TXINTSTAT;
	u32 C1_MISCSTAT;
	u32 C2_RXTHRESHSTAT;
	u32 C2_RXINTSTAT;
	u32 C2_TXINTSTAT;
	u32 C2_MISCSTAT;
	u32 C0_RXIMAX;
	u32 C0_TXIMAX;
	u32 C1RXIMAX;
	u32 C1_TXIMAX;
	u32 C2_RXIMAX;
	u32 C2_TXIMAX;
};

struct ewrap_dm644x_regs {
	u32 RSVD0;
	u32 EWCTL;
	u32 EWINTCNT;
};

union ewrap_regs {
	struct ewrap_dm646x_regs wrapper_646x_regs;
	struct ewrap_dm644x_regs wrapper_644x_regs;
};

/*
 * TX Buffer Descriptor
 *
 * CPPI 3.0 TX BD structure specific to EMAC.
 */
typedef struct {
	int h_next;		/* next buffer descriptor pointer */
	int buff_ptr;		/* data buffer pointer */
	int off_b_len;		/* (buffer_offset_16)(buffer_length_16) */
	int mode;		/* SOP, EOP, ownership, EOQ, teardown,
				 * Qstarv, length */
	void *next;		/* next TX buffer descriptor (linked list) */
	emac_net_data_token buf_token;
	void *eop_bd;		/* pointer to end of packet BD */
} emac_tx_bd;

/* forward declaration */
typedef struct _emac_rx_cppi_ch_t emac_rx_cppi_ch;

/**
 * RX Buffer Descriptor
 *
 * CPPI 3.0 RX BD structure specific to EMAC.
 */
typedef struct {
	int h_next;		/* next (hardware) buffer descriptor pointer */
	int buff_ptr;		/* data buffer pointer */
	int off_b_len;		/* (buffer_offset_16)(buffer_length_16) */
	int mode;		/* SOP, EOP, ownership, EOQ, teardown,
				 * Q starv, length */
	void *next;		/* pointer to the next RX buffer in BD queue */
	void *data_ptr;		/* virtual address of the buffer allocated */
	emac_net_data_token buf_token;
	emac_rx_cppi_ch *rx_cppi;
} emac_rx_bd;

/**
 * TX Channel Control Structure
 *
 * Used by EMAC DDC code to process TX Buffer Descriptors
 */
typedef struct {
	/** configuration info */
	emac_ch_info ch_info;	/* channel config/info */

	/* CPPI specific */
	u32 alloc_size;		/* BD pool allocated memory size */
	char *bd_mem;		/* buffer descriptor memory pointer */
	emac_tx_bd *bd_pool_head;	/* free BD pool head */
	emac_tx_bd *active_queue_head;	/* head of active packet queue */
	emac_tx_bd *active_queue_tail;	/* last hardware BD written */

	emac_tx_bd *last_hw_bdprocessed;/* last HW BD processed */
	bool queue_active;	/* queue active ? TRUE/FALSE */
#ifdef EMAC_MULTIPACKET_TX_COMPLETE_NOTIFY
	u32 *tx_complete;	/* tx complete notification queue */
#endif
	/** statistics */
	u32 proc_count;		/* TX: # of times emac_tx_bdproc is called */
	u32 mis_queued_packets;	/* misqueued packets */
	u32 queue_reinit;	/* queue reinit - head ptr reinit */
	u32 end_of_queue_add;	/* packet added to end of queue in send */
	u32 out_of_tx_bd;	/* out of tx bd errors */
	u32 no_active_pkts;	/* IRQ when there were no packets to process */
	u32 active_queue_count;	/* active tx bd count */
	u32 num_multi_frag_pkts;
} emac_tx_cppi_ch;

/**
 * RX Channel Control Structure
 *
 * Used by EMAC DDC code to process RX Buffer Descriptors
 */
typedef struct _emac_rx_cppi_ch_t {
	/* configuration info */
	emac_ch_info ch_info;	/* channel config/info */

	/* EMAC (ethernet) specific configuration info */
	char mac_addr[6];	/* ethernet MAC address */

	/** CPPI specific */
	u32 alloc_size;		/* BD pool allocated memory size */
	char *bd_mem;		/* buffer descriptor memory pointer */
	emac_rx_bd *bd_pool_head;
	emac_rx_bd *active_queue_head;
	emac_rx_bd *active_queue_tail;
	bool queue_active;

	/* packet and buffer objects required for passing up to DDA
	 * layer for the given instance */
	net_pkt_obj pkt_queue;
	net_buf_obj buf_queue[EMAC_MAX_RX_FRAGMENTS];
#ifdef EMAC_MULTIFRAGMENT
	u32 rx_buffer_ptr[EMAC_MAX_RX_FRAGMENTS];
	u32 rx_data_token[EMAC_MAX_RX_FRAGMENTS];
#endif
	/** statistics */
	u32 proc_count;		/* number of times emac_rx_bdproc is called */
	u32 processed_bd;	/* number of BD's processed */
	u32 recycled_bd;	/* number of recycled BD's */
	u32 out_of_rx_bd;	/* NO BD's available */
	u32 out_of_rx_buffers;	/* NO buffers available */
	u32 queue_reinit;	/* condition when recycling buffers */
	u32 end_of_queue_add;	/* when adding BD at end */
	u32 end_of_queue;	/* end of queue condition */
	u32 mis_queued_packets;	/* mis-queued packet condition */
	u32 num_multi_frag_pkts;
} _emac_rx_cppi_ch;

/* data structures and header files required for MII-MDIO module */
/* typedef struct _phy_device PHY_DEVICE; */

/**
 * EMAC Private data structure
 *
 * Each EMAC device maintains its own private data structure and has a
 * pointer to the net_device data structure representing the instance
 * with the kernel.  The private data structure contains a "owner"
 * member pointing to the net_device structure and the net_device data
 * structure's "priv" member points back to this data structure.
 */
struct emac_dev {
	struct net_device *owner;
	struct list_head emac_devices;
	unsigned int link_speed;
	unsigned int link_mode;
	unsigned long set_to_close;

	/* DDC related parameters */
	emac_status ddc_status;

	/* configuration parameters */
	unsigned char mac_addr[6];
	struct emac_init_config init_cfg;
	unsigned int rx_buf_size;
	unsigned int rx_buf_offset;

	/* channel configuration - though only 1 TX/RX channel is
	 * supported, provision is made for max */
	emac_ch_info tx_ch_info[EMAC_MAX_TXRX_CHANNELS];
	emac_ch_info rx_ch_info[EMAC_MAX_TXRX_CHANNELS];

	struct timer_list mib_timer;	/* for 64 bit MIB counter */
	u32 mib_ticks;		/* ticks for this timer */
	bool mib_timer_active;	/* periodic timer active ??? */

	/* statistics */
	emac_hw_statistics device_mib;	/* hardware statistics counters */
	emac_drv_stats device_stats;	/* device statstics */
	struct net_device_stats net_dev_stats;

	/* statistics counters for debugging */
	u32 isr_count;

	/* tx_rx_param struct added */
	rx_tx_params napi_rx_tx;

	/* TX/RX locks */
	spinlock_t tx_lock;
	spinlock_t rx_lock;

	emac_drv_state drv_state;

	/** EMAC specific parameters - DDC device specifics */
	emac_tx_cppi_ch * tx_cppi[EMAC_MAX_TXRX_CHANNELS];
	emac_rx_cppi_ch * rx_cppi[EMAC_MAX_TXRX_CHANNELS];
	bool tx_is_created[EMAC_MAX_TXRX_CHANNELS];
	bool rx_is_created[EMAC_MAX_TXRX_CHANNELS];
	bool tx_is_open[EMAC_MAX_TXRX_CHANNELS];
	bool rx_is_open[EMAC_MAX_TXRX_CHANNELS];
	bool tx_teardown_pending[EMAC_MAX_TXRX_CHANNELS];
	bool rx_teardown_pending[EMAC_MAX_TXRX_CHANNELS];
	int tx_int_threshold[EMAC_MAX_TXRX_CHANNELS];
	bool tx_interrupt_disable;
	bool giga_bit_enable;

	/* register mirror values - maintained to avoid costly
	 * register access for reads */
	u32 rx_unicast_set;
	u32 rx_unicast_clear;
	u32 rx_MBP_enable;
	u32 mac_hash1;
	u32 mac_hash2;
	u32 mac_control;
	emac_status status;

	/* number of multicast hash bits used in hardware */
	u32 multicast_hash_cnt[EMAC_NUM_MULTICAST_BITS];

	/* EMAC/CPGMAC addressing mechanism */
	u32 rx_addr_type;	/* 0 (EMAC), 1 or 2 (CPGMAC) */

	/* resources */
	volatile struct emac_regs *regs;
	volatile union ewrap_regs *ewrap_regs;
	char *ewrap_ram;
	unsigned long ewrap_ram_size;

	/* mii bus */
	struct mii_bus mdio_bus;
	unsigned long mdio_base_address;
	int phy_active;
	struct phy_device *phydev[PHY_MAX_ADDR];

	struct mib2_if_hccounters mib2if_hccounter;

	/*Interrupt enable Mask state variables */
	u32 rx_thresh_en;
	u32 rx_en;
	u32 tx_en;
	u32 misc_en;
};

/* ---------------------------------------------------------------
 * globals
 * --------------------------------------------------------------- */

/* global variables required during initialization */
static int g_link_speed = 0;	/* 0=auto negotiate, 100=100mbps, 10=10mbps */
static int g_link_mode = 0;	/* 0=Auto Negotiate, Full Duplex = 3;
				 * Half Duplex = 2 Unknown = 1 */
static LIST_HEAD(emac_devices);

/* clock frequency for EMAC */
static struct clk *emac_clk;

static const char emac_ddcversion_string[] = "EMAC DDC version 0.5";

static u32 emac_debug = 0x0;	/* no debug flags by default */

/* ---------------------------------------------------------------
 * prototypes
 * --------------------------------------------------------------- */
extern int davinci_get_macaddr(char *ptr);

static int emac_dev_tx(struct sk_buff *skb, struct net_device *netdev);

static irqreturn_t emac_hal_isr(int irq, void *dev_id, struct pt_regs *regs);

static void *emac_net_alloc_rx_buf(struct emac_dev *dev, int buf_size,
		emac_net_data_token *data_token, u32 channel,
		void *alloc_args);

static int emac_net_free_rx_buf(struct emac_dev *dev, void *buffer,
		emac_net_data_token data_token, u32 channel, void *free_args);

static int emac_net_tx_complete(struct emac_dev *dev,
		emac_net_data_token *net_data_tokens, int num_tokens,
		u32 channel);

static int emac_net_rx_cb(struct emac_dev *dev, net_pkt_obj *net_pkt_list,
		void *rx_args);


static int emac_poll(struct net_device *netdev, int *budget);
#ifdef CONFIG_NET_POLL_CONTROLLER
static void emac_poll_controller(struct net_device *dev);
#endif

/* net device related private function prototypes */
static int emac_dev_init(struct net_device *netdev);

static int emac_dev_open(struct net_device *dev);

static int emac_ioctl(struct net_device *netdev, struct ifreq *rq, int cmd);

static int emac_dev_close(struct net_device *netdev);

static void emac_dev_mcast_set(struct net_device *netdev);

static void emac_tx_timeout(struct net_device *netdev);

static struct net_device_stats *emac_dev_get_net_stats(struct net_device *dev);

/* internal function prototypes */
static int __init emac_p_detect_manual_cfg(int, char *);

static int emac_p_update_statistics(struct net_device *netdev, char *buf,
		int limit, int *p_len);

static int emac_p_reset_statistics(struct net_device *netdev);

static int emac_p_read_rfc2665_stats(char *buf, char **start, off_t offset,
		int count, int *eof, void *data);

static int emac_dev_set_mac_addr(struct net_device *netdev, void *addr);

static void emac_disable_interrupt(struct emac_dev *edev);

static void emac_enable_interrupt(struct emac_dev *edev, int ack_eoi);

/* function prototype for emac_p_tick_timer_expiry() function as per
 * linux timer API */
typedef void (*timer_tick_func) (unsigned long);

/* function prototypes */
static int emac_send(struct emac_dev *dev, net_pkt_obj *pkt,
		int channel, bool send_args);

static int emac_pkt_process(struct emac_dev *dev, int *pkts_pending,
		void *pkt_args);

static int emac_pkt_process_end(struct emac_dev *dev, void *proc_args);

static int emac_tx_bdproc(struct emac_dev *dev, u32 channel, u32 *more_pkts,
		bool *is_eoq);

static int emac_rx_bdproc(struct emac_dev *dev, u32 channel, int *more_pkts);

#ifdef EMAC_MULTIFRAGMENT
static void emac_add_bdto_rx_queue(struct emac_dev *dev,
		emac_rx_cppi_ch *rx_cppi, emac_rx_bd *sop_bd,
		emac_rx_bd *eop_bd, u32 *buffer,
		emac_net_data_token *buf_token, u32 num_bd);

#else
static void emac_add_bdto_rx_queue(struct emac_dev *dev,
		emac_rx_cppi_ch *rx_cppi, emac_rx_bd *curr_bd, char *buffer,
		emac_net_data_token buf_token);
#endif

static void emac_update_phy_status(struct net_device *netdev);

static int emac_open(struct emac_dev *dev, void *param);

static int emac_close(struct emac_dev *dev, void *param);

static int emac_control(struct emac_dev *dev, int cmd, void *cmd_arg,
		void *param);

static int emac_ch_open(struct emac_dev *dev, emac_ch_info *ch_info,
			void *ch_open_args);

static int emac_ch_close(struct emac_dev *dev, int channel,
			 int direction, void *ch_close_args);

static int emac_wait_for_teardown_complete(struct emac_dev *dev, u32 channel,
		net_ch_dir direction, bool blocking);

static int emac_enable_channel(struct emac_dev *dev, u32 channel,
		u32 direction);

static int emac_disable_channel(struct emac_dev *dev, u32 channel,
		net_ch_dir direction);

static int emac_init_tx_channel(struct emac_dev *dev, emac_ch_info *ch_info,
		void *ch_open_args);

static int emac_init_rx_channel(struct emac_dev *dev, emac_ch_info *ch_info,
		void *ch_open_args);

static int emac_un_init_tx_channel(struct emac_dev *dev, u32 channel,
		void *ch_close_args);

static int emac_un_init_rx_channel(struct emac_dev *dev, u32 channel,
		void *ch_close_args);

static void emac_setmac(struct emac_dev *dev, u32 channel, char *mac_addr);

static void emac_ddcifcnt_clear(struct emac_dev *dev);

static void emac_ddcifcnt_updt(struct emac_dev *dev);

static void emac_ddcphycnt(struct emac_dev *dev, u32 *cmd_arg);

/* ---------------------------------------------------------------
 * internal utility functions
 * --------------------------------------------------------------- */
static inline u32 emac_virt_to_phys(u32 addr)
{
	/* NOTE: must handle memory and IO addresses */
	if ((addr >= IO_VIRT) && (addr < (IO_VIRT + IO_SIZE)))
		addr = io_v2p(addr);
	else
		addr = virt_to_phys((void *)addr);

	return addr;
}

#define EMAC_VIRT_TO_PHYS(x) emac_virt_to_phys((u32)x)
#define EMAC_VIRT_NOCACHE(addr)(addr)

/* alloc and zero memoy */
static inline int emac_malloc(u32 n, void **buf)
{
	void *tmp = kcalloc(n, 1, GFP_KERNEL);

	if (!tmp) {
		printk(KERN_ERR "emac_malloc(): kmalloc() failed.\n");
		dump_stack();
		return -1;
	}

	*buf = tmp;
	return 0;
}

static inline void emac_free(void *ptr)
{
	kfree(ptr);
}

#define EMAC_CACHE_INVALIDATE(addr, size) dma_cache_maint((void *)addr, size, DMA_FROM_DEVICE)
#define EMAC_CACHE_WRITEBACK(addr, size) dma_cache_maint((void *)addr, size, DMA_TO_DEVICE)
#define EMAC_CACHE_WRITEBACK_INVALIDATE(addr, size) dma_cache_maint((void *)addr,size, DMA_BIDIRECTIONAL)

/* buffer-descriptors in IO space.  No cache invalidation needed */
#define BD_CACHE_INVALIDATE(addr, size)
#define BD_CACHE_WRITEBACK(addr, size)
#define BD_CACHE_WRITEBACK_INVALIDATE(addr, size)

/* string to hex conversion */
static unsigned char emac_str_to_hexnum(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

/* string to ethernet address conversion */
static void emac_str_to_ethaddr(unsigned char *ea, unsigned char *str)
{
	int i;
	unsigned char num;

	for (i = 0; i < 6; i++) {
		if ((*str == '.') || (*str == ':')) {
			str++;
		}
		num = emac_str_to_hexnum(*str) << 4;
		++str;
		num |= (emac_str_to_hexnum(*str));
		++str;
		ea[i] = num;
	}
}

/******************************************************************************
 *  DDA Callback functions
 *****************************************************************************/

static void emac_status_update(struct emac_dev *dev, emac_status *cmd_arg)
{

	/* cmd_arg will point to status structure, param not used */
	struct net_device *netdev = dev->owner;

	emac_status *status = &dev->ddc_status;
	dev->ddc_status = *cmd_arg;
	if ((status->hw_status & EMAC_TX_HOST_ERROR) ==
			EMAC_TX_HOST_ERROR) {
		ERR("TX Host Error. "
				"Transmit Stopped %s\n", netdev->name);
	}
	if ((status->hw_status & EMAC_RX_HOST_ERROR) ==
			EMAC_RX_HOST_ERROR) {
		ERR("RX Host Error. "
				"Receive Stopped %s\n", netdev->name);
	}
	if (status->phy_linked) {
		/* link ON */
		if (!netif_carrier_ok(netdev))
			netif_carrier_on(netdev);

		dev->link_speed =
			((status->phy_speed == 100) ? 100000000 : 10000000);
		dev->link_mode =
			((status->phy_duplex == PHY_DUPLEX_FULL) ?
				PHY_DUPLEX_FULL : PHY_DUPLEX_HALF);

		/* reactivate the transmit queue if it
		 * is stopped */
		if (netif_running(netdev) && netif_queue_stopped(netdev))
			netif_wake_queue(netdev);
	} else {
		/* link OFF */
		if (netif_carrier_ok(netdev)) {
			/* do we need to register
			 * synchronization issues with
			 * stats here. */
			dev->link_speed = 100000000;
			dev->link_mode = 1;
			netif_carrier_off(netdev);
		}
		if (!netif_queue_stopped(netdev))
			/* so that kernel does not
			 * keep on xmiting pkts. */
			netif_stop_queue(netdev);
	}

	pr_debug("%s, PhyNum %d,  %s, %s, %s\n",
			dev->owner->name,
			status->phy_num,
			((status->phy_duplex == PHY_DUPLEX_FULL) ?
			 "Full Duplex" : "Half Duplex"),
			((status->phy_speed == 100) ?
			 "100 Mbps" : "10 Mbps"),
			((status->phy_linked) ?
			 "Linked" : "NO LINK"));
}

static void emac_timer_start(struct emac_dev *dev, u32 cmd_arg)
{
	/* cmd will directly have the timer period of the
	 * periodic timer, param not used */

	/* asks for milli_secs. so calculate ticks
	 * from ticks per 1000 m_sec */
	struct timer_list *p_timer = &dev->mib_timer;

	dev->mib_ticks = (EMAC_TICKS_PER_SEC * cmd_arg) / 1000;
	p_timer->expires = jiffies + dev->mib_ticks;
	add_timer(p_timer);
	dev->mib_timer_active = TRUE;
}

static void emac_timer_stop(struct emac_dev *dev)
{
	/* cmd_arg and param not used */
	if (dev->mib_timer_active == TRUE) {
		del_timer_sync(&dev->mib_timer);
		dev->mib_timer_active = FALSE;
	}
}

/*****************************************************************************
 *
 * emacEndGetConfig - Extract configuration for given unit number/instance
 *
 * This function gets the configuration information from the
 * configuration service or by some means for the given unit
 * number/emac instance
 *
 * Note: For debug/default, static information is obtained from the
 * header file
 *
 * RETURNS: OK or ERROR.
 */
static void emac_net_get_config(struct emac_dev *dev)
{
	struct emac_init_config *i_cfg = &dev->init_cfg;
	emac_ch_info *tx_ch_cfg = &dev->tx_ch_info[0];
	emac_ch_info *rx_ch_cfg = &dev->rx_ch_info[0];

	i_cfg->rx_cfg.promiscous_enable = 0;
	i_cfg->rx_cfg.broadcast_enable = 1;
	i_cfg->rx_cfg.multicast_enable = 1;

	i_cfg->rx_cfg.max_rx_pkt_length = EMAC_DEFAULT_MAX_FRAME_SIZE;

	tx_ch_cfg->num_bd = EMAC_DEFAULT_TX_NUM_BD;
	tx_ch_cfg->service_max = EMAC_DEFAULT_TX_MAX_SERVICE;
	rx_ch_cfg->num_bd = EMAC_DEFAULT_RX_NUM_BD;
	rx_ch_cfg->service_max = EMAC_DEFAULT_RX_MAX_SERVICE;

	i_cfg->mdio_tick_msec = 10;

	i_cfg->mib64cnt_msec = CONFIG_EMAC_MIB_TIMER_TIMEOUT;
	rx_ch_cfg->buf_size = i_cfg->rx_cfg.max_rx_pkt_length;
	dev->rx_buf_offset = EMAC_L3_ALIGN(EMAC_DEFAULT_EXTRA_RXBUF_SIZE) +
			     EMAC_DEFAULT_EXTRA_RXBUF_SIZE;
	dev->rx_buf_size = (rx_ch_cfg->buf_size + dev->rx_buf_offset);

	/*
	 * Align skb's on 4 byte boundry - no hard requirement currently
	 * - done for future use
	 */
	dev->rx_buf_size += EMAC_4BYTE_ALIGN(dev->rx_buf_size);

	/* determine phy speed/duplex mode - to be built as per MDIO
	 * module requirements */
	if (g_link_speed == CONFIG_EMAC_NOPHY) {
		i_cfg->phy_mode = SNWAY_NOPHY;
	} else {
		if ((g_link_speed == 0) && (g_link_mode == PHY_DUPLEX_AUTO)) {
			i_cfg->phy_mode = SNWAY_AUTOALL;
		} else if (g_link_speed == 10) {
			if (g_link_mode == PHY_DUPLEX_HALF)
				i_cfg->phy_mode = SNWAY_HD10;
			else if (g_link_mode == PHY_DUPLEX_FULL)
				i_cfg->phy_mode = SNWAY_FD10;
			else
				i_cfg->phy_mode = SNWAY_HD10 | SNWAY_FD10;
		} else if (g_link_speed == 100) {
			if (g_link_mode == PHY_DUPLEX_HALF)
				i_cfg->phy_mode = SNWAY_HD100;
			else if (g_link_mode == PHY_DUPLEX_FULL)
				i_cfg->phy_mode = SNWAY_FD100;
			else
				i_cfg->phy_mode = SNWAY_HD100 | SNWAY_FD100;
		} else if (g_link_speed == 1000) {
			if (g_link_mode == PHY_DUPLEX_HALF)
				i_cfg->phy_mode = SNWAY_HD1000;
			else if (g_link_mode == PHY_DUPLEX_FULL)
				i_cfg->phy_mode = SNWAY_FD1000;
			else
				i_cfg->phy_mode = SNWAY_HD1000 | SNWAY_FD1000;
		} else {
			if (g_link_mode == PHY_DUPLEX_FULL) {
				i_cfg->phy_mode = SNWAY_FD10 | SNWAY_FD100;
				if (dev->init_cfg.gigabit)
					i_cfg->phy_mode |= SNWAY_FD1000;

			} else {
				i_cfg->phy_mode = SNWAY_HD10 | SNWAY_HD100;
				if (dev->init_cfg.gigabit)
					i_cfg->phy_mode |= SNWAY_HD1000;
			}
		}
	}

	i_cfg->num_tx_channels = EMAC_DEFAULT_NUM_TX_CHANNELS;
	i_cfg->num_rx_channels = EMAC_DEFAULT_NUM_RX_CHANNELS;
	i_cfg->m_link_mask = EMAC_DEFAULT_MLINK_MASK;
	i_cfg->rx_cfg.pass_crc = EMAC_DEFAULT_PASS_CRC;
	i_cfg->rx_cfg.qos_enable = EMAC_DEFAULT_QOS_ENABLE;
	i_cfg->rx_cfg.no_buffer_chaining = EMAC_DEFAULT_NO_BUFFER_CHAINING;
	i_cfg->rx_cfg.copy_maccontrol_frames_enable =
			EMAC_DEFAULT_COPY_MAC_CONTROL_FRAMES_ENABLE;
	i_cfg->rx_cfg.copy_short_frames_enable =
			EMAC_DEFAULT_COPY_SHORT_FRAMES_ENABLE;
	i_cfg->rx_cfg.copy_error_frames_enable =
			EMAC_DEFAULT_COPY_ERROR_FRAMES_ENABLE;
	i_cfg->rx_cfg.promiscous_channel = EMAC_DEFAULT_PROMISCOUS_CHANNEL;
	i_cfg->rx_cfg.broadcast_channel = EMAC_DEFAULT_BROADCAST_CHANNEL;
	i_cfg->rx_cfg.multicast_channel = EMAC_DEFAULT_MULTICAST_CHANNEL;
	i_cfg->rx_cfg.buffer_offset = EMAC_DEFAULT_BUFFER_OFFSET;

	tx_ch_cfg->ch_num = EMAC_DEFAULT_TX_CHANNEL;
	tx_ch_cfg->ch_dir = NET_CH_DIR_TX;
	tx_ch_cfg->ch_state = NET_CH_UNINITIALIZED;
	rx_ch_cfg->ch_num = EMAC_DEFAULT_RX_CHANNEL;
	rx_ch_cfg->ch_dir = NET_CH_DIR_RX;
	rx_ch_cfg->ch_state = NET_CH_UNINITIALIZED;

}

/* detect manual config */
static int __init emac_p_detect_manual_cfg(int link_speed, char *link_mode)
{
	if ((link_speed == 0) && (!strcmp(link_mode, "auto"))) {
		/* Auto negotiation */
		g_link_speed = 0;
		g_link_mode = PHY_DUPLEX_AUTO;
		pr_debug("auto negotiation selected\n");
	}

	if (!link_speed || (link_speed != 10 && link_speed != 100
				&& link_speed != 1000)) {
		g_link_speed = 0;
		g_link_mode = PHY_DUPLEX_AUTO;
		pr_debug("Invalid or No value of link speed specified,"
				"defaulting to auto negotiation .\n");
	}

	if (!link_mode || (!strcmp(link_mode, "fd")
			&& !strcmp(link_mode, "hd"))) {
		g_link_speed = 0;
		g_link_mode = PHY_DUPLEX_AUTO;
		pr_debug("Invalid or No value of link mode specified,"
				"defaulting to auto mode.\n");
	}

	if ((link_speed == 10) && (!strcmp(link_mode, "fd"))) {
		g_link_speed = 10;
		g_link_mode = PHY_DUPLEX_FULL;
	} else if ((link_speed == 10) && (!strcmp(link_mode, "hd"))) {
		g_link_speed = 10;
		g_link_mode = PHY_DUPLEX_HALF;
	} else if ((link_speed == 100) && (!strcmp(link_mode, "fd"))) {
		g_link_speed = 100;
		g_link_mode = PHY_DUPLEX_FULL;
	} else if ((link_speed == 100) && (!strcmp(link_mode, "hd"))) {
		g_link_speed = 100;
		g_link_mode = PHY_DUPLEX_HALF;
	} else if ((link_speed == 1000) && (!strcmp(link_mode, "fd"))) {
		g_link_speed = 1000;
		g_link_mode = PHY_DUPLEX_FULL;
	} else {
		g_link_speed = 0;
		g_link_mode = PHY_DUPLEX_AUTO;
	}

	pr_debug("Link is set to the speed of"
	    "%s speed and %s mode.\n",
	    ((g_link_speed == 0) ? "auto" : ((g_link_speed == 100) ? "100" :
		((g_link_speed == 1000) ? "1000" : "10"))),
	    ((g_link_mode == 0) ? "auto" :
		((g_link_mode == PHY_DUPLEX_HALF) ? "half" : "full")));

	return (0);
}

static int emac_p_read_link(char *buf, char **start, off_t offset, int count,
		int *eof, void *data)
{
	struct list_head *pos, *tmp;
	int i;
	int len = 0;

	list_for_each_safe(pos, tmp, &emac_devices) {
		struct emac_dev *dev =
			list_entry(pos, struct emac_dev, emac_devices);
		struct net_device *netdev = dev->owner;

		for (i = 0; i < PHY_MAX_ADDR; i++) {
			if (dev->phydev[i] == NULL)
				continue;

			if (!dev->phydev[i]->link) {
				len +=
				    sprintf(buf + len, "%s: Link State: DOWN  "
					    "Phy %d\n", netdev->name, i);
				continue;

			}
			len +=
			    sprintf(buf + len, "%s: Link State: UP    "
				    "Phy %d, Speed = %d, Duplex = %s\n",
				    netdev->name, i, dev->phydev[i]->speed,
				    dev->phydev[i]->duplex == DUPLEX_HALF ?
						"Half" : "Full");
		}
	}

	return len;
}

static int emac_dump_config(char *buf, char **start, off_t offset, int count,
		int *eof, void *data)
{
	int len = 0;

	struct list_head *pos, *tmp;

	list_for_each_safe(pos, tmp, &emac_devices) {
		struct emac_dev *dev =
			list_entry(pos, struct emac_dev, emac_devices);
		struct net_device *netdev = dev->owner;

		len += sprintf(buf + len,
				"\nEMAC Driver Internal Config Info for %s\n",
				netdev->name);
		len += sprintf(buf + len, "rxBufSize          = %d\n",
				dev->rx_buf_size);
		len += sprintf(buf + len, "rxBufOffset        = %d\n",
				dev->rx_buf_offset);
		len += sprintf(buf + len, "numTxChannels      = %d\n",
				dev->init_cfg.num_tx_channels);
		len += sprintf(buf + len, "numRxChannels      = %d\n",
				dev->init_cfg.num_rx_channels);
		len += sprintf(buf + len, "emacBusFrequency  = %d\n",
				dev->init_cfg.emac_bus_frequency);
		len += sprintf(buf + len, "resetLine          = %d\n",
				dev->init_cfg.reset_line);
		len += sprintf(buf + len, "mdioBaseAddress    = %08lX\n",
				dev->mdio_base_address);
		len += sprintf(buf + len, "mdioResetLine      = %d\n",
				dev->init_cfg.mdio_reset_line);
		len += sprintf(buf + len, "MLinkMask          = %08X\n",
				dev->init_cfg.m_link_mask);
		len += sprintf(buf + len, "MdioBusFrequency   = %d\n",
				dev->init_cfg.mdio_bus_frequency);
		len += sprintf(buf + len, "MdioClockFrequency = %d\n",
				dev->init_cfg.mdio_clock_frequency);
		len += sprintf(buf + len, "MdioTickMSec       = %d\n",
				dev->init_cfg.mdio_tick_msec);
		len += sprintf(buf + len, "phyMode            = %d\n",
				dev->init_cfg.phy_mode);
		len += sprintf(buf + len, "passCRC            = %d\n",
				dev->init_cfg.rx_cfg.pass_crc);
		len += sprintf(buf + len, "qosEnable          = %d\n",
				dev->init_cfg.rx_cfg.qos_enable);
		len += sprintf(buf + len, "noBufferChaining   = %d\n",
				dev->init_cfg.rx_cfg.no_buffer_chaining);
		len += sprintf(buf + len, "copyMACCntrlFrsEne = %d\n",
			dev->init_cfg.rx_cfg.copy_maccontrol_frames_enable);
		len += sprintf(buf + len, "copyShortFramesEn  = %d\n",
				dev->init_cfg.rx_cfg.copy_short_frames_enable);
		len += sprintf(buf + len, "copyErrorFramesEn  = %d\n",
				dev->init_cfg.rx_cfg.copy_error_frames_enable);
		len += sprintf(buf + len, "promiscousEnable   = %d\n",
				dev->init_cfg.rx_cfg.promiscous_enable);
		len += sprintf(buf + len, "promiscousChannel  = %d\n",
				dev->init_cfg.rx_cfg.promiscous_channel);
		len += sprintf(buf + len, "broadcastEnable    = %d\n",
				dev->init_cfg.rx_cfg.broadcast_enable);
		len += sprintf(buf + len, "broadcastChannel   = %d\n",
				dev->init_cfg.rx_cfg.broadcast_channel);
		len += sprintf(buf + len, "multicastEnable    = %d\n",
				dev->init_cfg.rx_cfg.multicast_enable);
		len += sprintf(buf + len, "multicastChannel   = %d\n",
				dev->init_cfg.rx_cfg.multicast_channel);
		len += sprintf(buf + len, "maxRxPktLength     = %d\n",
				dev->init_cfg.rx_cfg.max_rx_pkt_length);
		len += sprintf(buf + len, "bufferOffset       = %d\n",
				dev->init_cfg.rx_cfg.buffer_offset);
	}

	return len;
}

/* read stats */
static int emac_p_read_stats(char *buf, char **start, off_t offset, int count,
		int *eof, void *data)
{
	int len = 0;
	int limit = count - 80;

	struct list_head *pos, *tmp;

	list_for_each_safe(pos, tmp, &emac_devices) {
		struct emac_dev *dev =
			list_entry(pos, struct emac_dev, emac_devices);
		struct net_device *netdev = dev->owner;
		emac_hw_statistics *p_device_mib = &dev->device_mib;

		emac_p_update_statistics(dev->owner, NULL, 0, NULL);
		/* transmit stats */
		if (len <= limit)
			len += sprintf(buf + len, "\nCpmac %s, Address %lx\n",
					netdev->name, netdev->base_addr);
		if (len <= limit)
			len += sprintf(buf + len, " Transmit Stats\n");
		if (len <= limit)
			len += sprintf(buf + len,
					"   Tx Valid Bytes Sent        :%u\n",
					p_device_mib->if_out_octets);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Good Tx Frames (Hardware)  :%u\n",
				    p_device_mib->if_out_good_frames);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Good Tx Frames (Software)  :%lu\n",
					dev->net_dev_stats.tx_packets);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Good Tx Broadcast Frames   :%u\n",
					p_device_mib->if_out_broadcasts);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Good Tx Multicast Frames   :%u\n",
					p_device_mib->if_out_multicasts);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Pause Frames Sent          :%u\n",
					p_device_mib->if_out_pause_frames);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Collisions                 :%u\n",
					p_device_mib->if_collision_frames);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Tx Error Frames            :%lu\n",
					dev->net_dev_stats.tx_errors);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Carrier Sense Errors       :%u\n",
					p_device_mib->if_carrier_sense_errors);
		if (len <= limit)
			len += sprintf(buf + len, "\n");

		/* receive stats */
		if (len <= limit)
			len += sprintf(buf + len, "\nCpmac %s, Address %lx\n",
					netdev->name, netdev->base_addr);
		if (len <= limit)
			len += sprintf(buf + len, " Receive Stats\n");
		if (len <= limit)
			len += sprintf(buf + len,
					"   Rx Valid Bytes Received    :%u\n",
					p_device_mib->if_in_octets);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Good Rx Frames (Hardware)  :%u\n",
					p_device_mib->if_in_good_frames);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Good Rx Frames (Software)  :%lu\n",
					dev->net_dev_stats.rx_packets);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Good Rx Broadcast Frames   :%u\n",
					p_device_mib->if_in_broadcasts);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Good Rx Multicast Frames   :%u\n",
					p_device_mib->if_in_multicasts);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Pause Frames Received      :%u\n",
					p_device_mib->if_in_pause_frames);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Rx CRC Errors              :%u\n",
					p_device_mib->if_in_crcerrors);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Rx Align/Code Errors       :%u\n",
					p_device_mib->if_in_align_code_errors);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Rx Jabbers                 :%u\n",
					p_device_mib->if_in_oversized_frames);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Rx Filtered Frames         :%u\n",
					p_device_mib->if_in_filtered_frames);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Rx Fragments               :%u\n",
					p_device_mib->if_in_fragments);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Rx Undersized Frames       :%u\n",
					p_device_mib->if_in_undersized_frames);
		if (len <= limit)
			len += sprintf(buf + len,
					"   Rx Overruns                :%u\n",
					p_device_mib->if_rx_dmaoverruns);
	}

	return len;
}

/* write stats */
static int emac_p_write_stats(struct file *fp, const char *buf,
		unsigned long count, void *data)
{
	struct list_head *pos, *tmp;

	char local_buf[31];

	if (count > 30) {
		printk("Error : Buffer Overflow\n");
		printk("Use \"echo 0 > emac_stat\" to reset the statistics\n");
		return -EFAULT;
	}

	copy_from_user(local_buf, buf, count);
	local_buf[count - 1] = '\0';	/* ignoring last \n char */
	if (strcmp("0", local_buf)) {
		printk("Error: Unknown operation on emac statistics\n");
		printk("Use \"echo 0 > emac_stats\" to reset the statistics\n");
		return -EFAULT;
	}

	pr_info("Resetting statistics for EMAC interface.\n");

	list_for_each_safe(pos, tmp, &emac_devices) {
		struct emac_dev *dev =
			list_entry(pos, struct emac_dev, emac_devices);
		emac_p_reset_statistics(dev->owner);
	}

	return count;
}

/* update RFC2665 statistics */
static int emac_p_read_rfc2665_stats(char *buf, char **start, off_t offset,
		int count, int *eof, void *data)
{
	int limit = count - 80;
	int len = 0;
	struct net_device *netdev = (struct net_device *)data;

	emac_p_update_statistics(netdev, buf, limit, &len);
	*eof = 1;

	return len;
}

/* reset statistics */
static int emac_p_reset_statistics(struct net_device *netdev)
{
	struct emac_dev *dev = NETDEV_PRIV(netdev);

	memset(&dev->device_mib, 0, sizeof(emac_hw_statistics));
	memset(&dev->device_stats, 0, sizeof(emac_drv_stats));
	memset(&dev->net_dev_stats, 0, sizeof(struct net_device_stats));

	/* clear statistics */
	if (emac_control(dev, EMAC_IOCTL_CLR_STATISTICS, NULL, NULL)
			!= EMAC_SUCCESS) {
		ERR("Error clearing statistics in DDC for %s\n", netdev->name);

		return (-1);
	}

	return (0);
}

/* update statistics */
static int emac_p_update_statistics(struct net_device *netdev, char *buf,
		int limit, int *p_len)
{
	unsigned long rx_hal_errors = 0;
	unsigned long rx_hal_discards = 0;
	unsigned long tx_hal_errors = 0;
	unsigned long if_out_discards = 0;
	unsigned long if_in_discards = 0;
	unsigned long if_out_errors = 0;
	unsigned long if_in_errors = 0;
	struct emac_dev *dev = NETDEV_PRIV(netdev);
	emac_hw_statistics *p_device_mib = &dev->device_mib;
	emac_drv_stats *p_stats = &dev->device_stats;
	emac_hw_statistics local_mib;
	emac_hw_statistics *p_local_mib = &local_mib;
	struct net_device_stats *p_net_dev_stats = &dev->net_dev_stats;
	int len = 0;
	int dev_mib_elem_count = 0;

	/* do not access the hardware if it is in the reset state. */
	if (!test_bit(0, &dev->set_to_close)) {
		/* get hardware statistics from DDC */
		if (emac_control(dev, EMAC_IOCTL_GET_STATISTICS,
				(void *)p_local_mib, NULL) != EMAC_SUCCESS) {
			ERR("Error getting statistics for %s\n", netdev->name);

			return (-1);
		}

		dev_mib_elem_count =
			sizeof(emac_hw_statistics) / sizeof(unsigned long);

		/* Update the history of the stats. This takes care of
		 * any reset of the device and stats that might have
		 * taken place during the life time of the driver.
		 */
		while (dev_mib_elem_count--) {
			*((unsigned long *)p_device_mib + dev_mib_elem_count) =
				*((unsigned long *)p_local_mib +
					dev_mib_elem_count);
		}
	}

	/* RFC2665, section 3.2.7, page 9 */
	rx_hal_errors =
		p_device_mib->if_in_fragments +
		p_device_mib->if_in_crcerrors +
		p_device_mib->if_in_align_code_errors +
		p_device_mib->if_in_jabber_frames;

	/* RFC2233 */
	rx_hal_discards = p_device_mib->if_rx_dmaoverruns;

	/* RFC2665, section 3.2.7, page 9 */
	tx_hal_errors =
		p_device_mib->if_excessive_collision_frames +
		p_device_mib->if_late_collisions +
		p_device_mib->if_carrier_sense_errors +
		p_device_mib->if_out_underrun;

	/*
	 * If not set, the short frames (< 64 bytes) are considered as
	 * errors
	 */
	if (dev->init_cfg.rx_cfg.copy_short_frames_enable == FALSE)
		rx_hal_errors += p_device_mib->if_in_undersized_frames;

	/* All frames greater than max rx frame length set in hardware
	 * should be considered error frames RFC2665, section 3.2.7,
	 * page 9. */
	rx_hal_errors += p_device_mib->if_in_oversized_frames;

	/* if not in promiscous, then non addr matching frames are discarded */
	/* EMAC 2.0 manual section 2.8.1.14 */
	if (dev->init_cfg.rx_cfg.promiscous_enable == FALSE) {
		if_in_discards += p_device_mib->if_in_filtered_frames;
	}

	/* total rx discards = hal discards */
	if_in_discards = rx_hal_discards;
	p_net_dev_stats->rx_dropped = rx_hal_discards;
	p_net_dev_stats->rx_crc_errors = p_device_mib->if_in_crcerrors;
	p_net_dev_stats->rx_frame_errors =
		p_device_mib->if_in_align_code_errors;
	p_net_dev_stats->multicast = p_device_mib->if_in_multicasts;
	if_in_errors = rx_hal_errors;
	if_out_errors = tx_hal_errors;
	if_out_discards = p_net_dev_stats->tx_dropped;

	/* let us update the net device stats struct. to be updated in
	   the later releases. */
	dev->net_dev_stats.rx_errors = if_in_errors;
	dev->net_dev_stats.collisions = p_device_mib->if_collision_frames;
	dev->net_dev_stats.tx_carrier_errors =
		p_device_mib->if_carrier_sense_errors;

	if (buf == NULL || limit == 0) {
		return (0);
	}

	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifSpeed",
				dev->link_speed);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n",
				"dot3StatsDuplexStatus", dev->link_mode);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifAdminStatus",
			    (netdev->flags & IFF_UP ? 1 : 2));
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifOperStatus",
				(((netdev->flags & IFF_UP)
				&& netif_carrier_ok(netdev)) ? 1 : 2));
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %lu\n", "ifLastChange",
				p_stats->start_tick);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %lu\n", "ifInDiscards",
				if_in_discards);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %lu\n", "ifInErrors",
				if_in_errors);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %lu\n", "ifOutDiscards",
				if_out_discards);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %lu\n", "ifOutErrors",
				if_out_errors);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifInGoodFrames",
				p_device_mib->if_in_good_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifInBroadcasts",
				p_device_mib->if_in_broadcasts);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifInMulticasts",
				p_device_mib->if_in_multicasts);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifInPauseFrames",
				p_device_mib->if_in_pause_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifInCRCErrors",
				p_device_mib->if_in_crcerrors);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifInAlignCodeErrors",
				p_device_mib->if_in_align_code_errors);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifInOversizedFrames",
				p_device_mib->if_in_oversized_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifInJabberFrames",
				p_device_mib->if_in_jabber_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifInUndersizedFrames",
				p_device_mib->if_in_undersized_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifInFragments",
				p_device_mib->if_in_fragments);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifInFilteredFrames",
				p_device_mib->if_in_filtered_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n",
				"ifInQosFilteredFrames",
				p_device_mib->if_in_qos_filtered_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifInOctets",
				p_device_mib->if_in_octets);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifOutGoodFrames",
				p_device_mib->if_out_good_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifOutBroadcasts",
				p_device_mib->if_out_broadcasts);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifOutMulticasts",
				p_device_mib->if_out_multicasts);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifOutPauseFrames",
				p_device_mib->if_out_pause_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n",
				"ifDeferredTransmissions",
				p_device_mib->if_deferred_transmissions);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifCollisionFrames",
				p_device_mib->if_collision_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n",
				"ifSingleCollisionFrames",
				p_device_mib->if_single_collision_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n",
				"ifMultipleCollisionFrames",
				p_device_mib->if_multiple_collision_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n",
				"ifExcessiveCollisionFrames",
				p_device_mib->if_excessive_collision_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifLateCollisions",
				p_device_mib->if_late_collisions);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifOutUnderrun",
				p_device_mib->if_out_underrun);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifCarrierSenseErrors",
				p_device_mib->if_carrier_sense_errors);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifOutOctets",
				p_device_mib->if_out_octets);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "if64OctetFrames",
				p_device_mib->if64octet_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n",
				"if65To127POctetFrames",
				p_device_mib->if65to127octet_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n",
				"if128To255OctetFrames",
				p_device_mib->if128to255octet_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n",
				"if256To511OctetFrames",
				p_device_mib->if256to511octet_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n",
				"if512To1023OctetFrames",
				p_device_mib->if512to1023octet_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n",
				"if1024ToUpOctetFrames",
				p_device_mib->if1024to_upoctet_frames);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifNetOctets",
				p_device_mib->if_net_octets);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifRxSofOverruns",
				p_device_mib->if_rx_sof_overruns);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifRxMofOverruns",
				p_device_mib->if_rx_mof_overruns);
	if (len <= limit)
		len += sprintf(buf + len, "%-35s: %u\n", "ifRxDMAOverruns",
				p_device_mib->if_rx_dmaoverruns);
	*p_len = len;

	return (0);
}

/* version info */
static int emac_p_get_version(char *buf, char **start, off_t offset, int count,
		int *eof, void *data)
{
	int len = 0;

	len += sprintf(buf + len, "Texas Instruments : %s\n",
			emac_version_string);
	return len;
}

/* mib timer */
static void emac_p_mib_timer_expiry(struct emac_dev *dev)
{
	struct timer_list *p_timer = &dev->mib_timer;

	if (test_bit(0, &dev->set_to_close)) {
		return;
	}

	if (dev->mib_timer_active == TRUE) {
		emac_control(dev, EMAC_IOCTL_IF_PARAMS_UPDT, NULL, NULL);

		/* restart the timer */
		p_timer->expires = jiffies + dev->mib_ticks;
		add_timer(p_timer);
	}
}

/******************************************************************************
 *  Net Device functions
 *****************************************************************************/

/* get statistics */
static struct net_device_stats *emac_dev_get_net_stats(
		struct net_device *netdev)
{
	struct emac_dev *dev = NETDEV_PRIV(netdev);

	emac_p_update_statistics(netdev, NULL, 0, NULL);
	return &dev->net_dev_stats;
}

/* set multicast address in the driver */
static void emac_dev_mcast_set(struct net_device *netdev)
{
	struct emac_dev *dev = NETDEV_PRIV(netdev);

	if (netdev->flags & IFF_PROMISC) {
		/* enable promiscous mode */
		dev->init_cfg.rx_cfg.promiscous_enable = TRUE;

		emac_control(dev, EMAC_IOCTL_SET_RXCFG,
				(void *)&dev->init_cfg.rx_cfg, NULL);
	} else if ((netdev->flags & IFF_ALLMULTI)
			|| (netdev->mc_count
				> EMAC_DEFAULT_MAX_MULTICAST_ADDRESSES)) {
		/* enable multicast - disable promiscous */
		dev->init_cfg.rx_cfg.promiscous_enable = FALSE;
		dev->init_cfg.rx_cfg.multicast_enable = TRUE;
		emac_control(dev, EMAC_IOCTL_SET_RXCFG,
				(void *)&dev->init_cfg.rx_cfg, NULL);

		/* enable all multicast addresses */
		emac_control(dev, EMAC_IOCTL_ALL_MULTI,
				(void *)EMAC_ALL_MULTI_SET, NULL);
	} else if (netdev->mc_count == 0) {
		/* only unicast mode to be set - clear promiscous and
		   clear multicast modes */
		emac_control(dev, EMAC_IOCTL_ALL_MULTI,
				(void *)EMAC_ALL_MULTI_CLR, NULL);

		/* disable promiscous and multicast modes */
		dev->init_cfg.rx_cfg.promiscous_enable = FALSE;
		dev->init_cfg.rx_cfg.multicast_enable = FALSE;
		emac_control(dev, EMAC_IOCTL_SET_RXCFG,
				(void *)&dev->init_cfg.rx_cfg, NULL);
	} else if (netdev->mc_count) {
		struct dev_mc_list *mc_ptr;

		/* clear multicast list first */
		emac_control(dev, EMAC_IOCTL_ALL_MULTI,
				(void *)EMAC_ALL_MULTI_CLR, NULL);

		/* enable multicast - disable promiscous */
		dev->init_cfg.rx_cfg.promiscous_enable = FALSE;
		dev->init_cfg.rx_cfg.multicast_enable = TRUE;
		emac_control(dev, EMAC_IOCTL_SET_RXCFG,
				(void *)&dev->init_cfg.rx_cfg, NULL);

		/* program multicast address list into EMAC hardware */
		for (mc_ptr = netdev->mc_list; mc_ptr; mc_ptr = mc_ptr->next) {
			emac_control(dev, EMAC_IOCTL_MULTICAST_ADDR,
					(void *)EMAC_MULTICAST_ADD,
					(void *)mc_ptr->dmi_addr);
		}
	} else {
		pr_debug("%s:No Multicast address to set.\n", netdev->name);
	}
}

static int emac_dev_set_mac_addr(struct net_device *netdev, void *addr)
{
	int ret_code;
	emac_address_params address_params;
	struct sockaddr *sa = addr;
	struct emac_dev *dev = NETDEV_PRIV(netdev);

	address_params.channel = EMAC_DEFAULT_RX_CHANNEL;
	address_params.mac_address = sa->sa_data;

	ret_code = emac_control(dev, EMAC_IOCTL_SET_MAC_ADDRESS,
			(emac_address_params *)&address_params, NULL);

	if (ret_code != EMAC_SUCCESS) {
		ERR("%s error: Error %08X from EMAC TX Channel Open()\n",
				netdev->name, ret_code);

		return -EIO;
	}
	memcpy(dev->mac_addr, sa->sa_data, netdev->addr_len);
	memcpy(netdev->dev_addr, sa->sa_data, netdev->addr_len);
	return 0;
}

static void emac_tx_timeout(struct net_device *netdev)
{
	int ret_code;
	struct emac_dev *dev = NETDEV_PRIV(netdev);

	printk("EMAC Tx Timeout: Closing TX channel\n");
	emac_ch_close(dev, dev->tx_ch_info[0].ch_num,
			dev->tx_ch_info[0].ch_dir, 0);

	printk("EMAC Tx Timeout: Opening TX channel\n");
	ret_code = emac_ch_open(dev, &dev->tx_ch_info[0], NULL);

	if (ret_code != EMAC_SUCCESS) {
		ERR("%s error: Error %08X from EMAC TX Channel Open()\n",
				netdev->name, ret_code);
	} else {
		ERR("EMAC Tx Timeout: "
				"successfully closed and opened channels\n");
	}

	/* update interface statistics */
	dev->net_dev_stats.tx_errors++;
}

static int
emac_ethtool_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	struct emac_dev *dev = netdev_priv(netdev);

	if (dev->init_cfg.phy_mode & SNWAY_NOPHY) {
		cmd->speed = SPEED_100;
		cmd->duplex = DUPLEX_FULL;
		cmd->port = PORT_MII;
		cmd->phy_address = 0;
		cmd->transceiver = XCVR_EXTERNAL;
		cmd->autoneg = 0;
		cmd->supported = SUPPORTED_100baseT_Full;
		cmd->advertising = SUPPORTED_100baseT_Full;
		return 0;
	}

	return phy_ethtool_gset(dev->phydev[dev->phy_active], cmd);
}

static int
emac_ethtool_set_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	struct emac_dev *dev = netdev_priv(netdev);

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	if (dev->init_cfg.phy_mode & SNWAY_NOPHY)
		return -EOPNOTSUPP;

	if ((cmd->phy_address >= PHY_MAX_ADDR) ||
	    (dev->phydev[cmd->phy_address] == NULL))
		return -EINVAL;

	dev->phy_active = cmd->phy_address;

	return phy_ethtool_sset(dev->phydev[dev->phy_active], cmd);
}

static void emac_ethtool_get_drvinfo(struct net_device *netdev,
		struct ethtool_drvinfo *info)
{
	strcpy(info->driver, emac_version_string);
	strcpy(info->version, EMAC_MODULE_VERSION);
	info->fw_version[0] = '\0';
	strcpy(info->bus_info, emac_version_string);
	info->regdump_len = 0;
}

static struct ethtool_ops emac_ethtool_ops = {
	.get_settings = emac_ethtool_get_settings,
	.set_settings = emac_ethtool_set_settings,
	.get_drvinfo = emac_ethtool_get_drvinfo,
	.get_link = ethtool_op_get_link,
};

/***************************************************************
 *  emac_dev_init
 *
 *  Returns:
 *      0 on success, error code otherwise.
 *  Parms:
 *      dev The structure of the device to be
 *          init'ed.
 *
 *  This function completes the initialization of the
 *  device structure and driver.  It reserves the IO
 *  addresses and assignes the device's methods.
 *
 **************************************************************/

static int emac_dev_init(struct net_device *netdev)
{
	int cnt;
	struct emac_dev *dev = NETDEV_PRIV(netdev);
	struct platform_device *pdev =
		to_platform_device(netdev->class_dev.dev);
	unsigned char mac_eeprom[20];
	struct resource *res;
	int err = 0;

	if (davinci_get_macaddr(mac_eeprom)) {
		pr_err("Error getting board specific MAC address\n");
		pr_err("Assuming default MAC address\n");
		emac_str_to_ethaddr(dev->mac_addr, "08.00.28.32.06.08");
	} else {
		mac_eeprom[17] = 0;
		pr_info("MAC address is %s\n", mac_eeprom);
		emac_str_to_ethaddr(dev->mac_addr, mac_eeprom);
	}

	for (cnt = 0; cnt <= ETH_ALEN; cnt++) {
		netdev->dev_addr[cnt] = dev->mac_addr[cnt];
	}

	dev->set_to_close = 1;

	emac_net_get_config(dev);

	/* init spin lock */
	spin_lock_init(&dev->tx_lock);
	spin_lock_init(&dev->rx_lock);

	/* set as per RFC 2665 */
	dev->link_speed = 100000000;
	dev->link_mode = 1;

	/*
	 * get resources
	 */

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ctrl_regs");
	if (!res) {
		err = -ENODEV;
		goto emac_dev_init_exit;
	}
	dev->regs = (struct emac_regs *)IO_ADDRESS(res->start);
	netdev->base_addr = res->start;

	res = platform_get_resource_byname(pdev,
			IORESOURCE_MEM, "wrapper_ctrl_regs");
	if (!res) {
		err = -ENODEV;
		goto emac_dev_init_exit;
	}
	dev->ewrap_regs = (volatile union ewrap_regs *)IO_ADDRESS(res->start);

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "wrapper_ram");
	if (!res) {
		err = -ENODEV;
		goto emac_dev_init_exit;
	}
	dev->ewrap_ram = (char *)IO_ADDRESS(res->start);
	dev->ewrap_ram_size = res->end - res->start + 1;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		err = -ENODEV;
		goto emac_dev_init_exit;
	}
	netdev->irq = res->start;

	dev->drv_state = DRV_CREATED;

	init_timer(&dev->mib_timer);
	dev->mib_timer_active = FALSE;
	dev->mib_timer.data = (unsigned long)dev;
	dev->mib_timer.function = (timer_tick_func) emac_p_mib_timer_expiry;

	/* populate the device structure */
	netdev->addr_len = 6;
	netdev->open = emac_dev_open;	/* i.e. start device */
	netdev->do_ioctl = emac_ioctl;
	netdev->hard_start_xmit = emac_dev_tx;
	netdev->stop = emac_dev_close;
	netdev->get_stats = emac_dev_get_net_stats;
	netdev->set_multicast_list = emac_dev_mcast_set;
	netdev->tx_timeout = emac_tx_timeout;
	netdev->set_mac_address = emac_dev_set_mac_addr;
	netdev->poll = emac_poll;
	SET_ETHTOOL_OPS(netdev, &emac_ethtool_ops);
#ifdef CONFIG_NET_POLL_CONTROLLER
	netdev->poll_controller = emac_poll_controller;
#endif
	netdev->weight = EMAC_DEFAULT_RX_MAX_SERVICE;

	/* reset the broadcast and multicast flags and enable them
	   based upon configuration of driver */
	netdev->flags &= ~(IFF_PROMISC | IFF_BROADCAST | IFF_MULTICAST);
	if (dev->init_cfg.rx_cfg.broadcast_enable == TRUE)
		netdev->flags |= IFF_BROADCAST;
	if (dev->init_cfg.rx_cfg.multicast_enable == TRUE)
		netdev->flags |= IFF_MULTICAST;

	netif_carrier_off(netdev);

	return (0);

emac_dev_init_exit:
	return err;
}

/******************************************************************************
 *  Device Open/Close functions
 *****************************************************************************/

int davinci_mdio_read(struct mii_bus *bus, int mii_id, int regnum);

#define PHY_ID_MASK	0xfffffff0
#define LXT971_ID	0x001378e0
#define KSZ8893MQL_ID	0x00221430

static int emac_phy_init(struct emac_dev *dev, u8 phy_num, u8 *phy_cnt,
			 u8 *phy_max)
{
	char phy_id[BUS_ID_SIZE];

	snprintf(phy_id, BUS_ID_SIZE, PHY_ID_FMT, 1, phy_num);

	dev->phydev[phy_num] = phy_connect(dev->owner, phy_id,
		&emac_update_phy_status, 0, PHY_INTERFACE_MODE_MII);
	if (IS_ERR(dev->phydev[phy_num])) {
		if (dev->phydev[phy_num] == ERR_PTR(-ENODEV))
			dev->phydev[phy_num] = NULL;
		return PTR_ERR(dev->phydev[phy_num]);
	}

	dev->phydev[phy_num]->supported = PHY_BASIC_FEATURES;

	if (dev->init_cfg.gigabit)
		dev->phydev[phy_num]->supported |= SUPPORTED_1000baseT_Full;

	dev->phydev[phy_num]->advertising = dev->phydev[phy_num]->supported;

	if ((dev->phydev[phy_num]->phy_id & PHY_ID_MASK) == LXT971_ID) {
		/*
		 * CRITICAL: Fix for increasing PHY signal drive strength for
		 * TX lockup issue. On DaVinci EVM, the Intel LXT971 PHY
		 * signal strength was low causing TX to fail randomly. The
		 * fix is to Set bit 11 (Increased MII drive strength) of PHY
		 * register 26 (Digital Config register) on this phy.
		 */
		int control;

		pr_info("applying Intel LTX971 workaround..\n");

		control = phy_read(dev->phydev[phy_num], 26);
		phy_write(dev->phydev[phy_num], 26, (control | 0x800));
		phy_read(dev->phydev[phy_num], 26);
	} else if ((dev->phydev[phy_num]->phy_id & PHY_ID_MASK) ==
		    KSZ8893MQL_ID) {
		/*
		 * KSZ8893MQL is a layer 2 switch that connects to EMAC via
		 * MII.  The MII setting is required to be fixed with autoneg
		 * off, 100Mbps, and full duplex.  This happens to be the
		 * settings and behavior when phy_mode is set to SNWAY_NOPHY.
		 * Therefore, we set the phy_mode to SNWAY_NOPHY whenever
		 * KSZ8893MQL is detected.
		 */
		dev->init_cfg.phy_mode = SNWAY_NOPHY;

		/* The third phy is used for MII, so only expect 2 phys */
		*phy_max = 2;
	}

	*phy_cnt = *phy_cnt + 1;
	phy_start(dev->phydev[phy_num]);

	return 0;
}

/* open the adapter */
static int emac_dev_open(struct net_device *netdev)
{
	u8 phy_cnt, phy_max;
  	struct emac_dev *dev = NETDEV_PRIV(netdev);
	struct platform_device *pdev =
		to_platform_device(netdev->class_dev.dev);
	struct resource *res;
	int err;
  	int i;
	int ret_code;

	dev->set_to_close = 0;

	/* create a TX channel */
	ret_code = emac_ch_open(dev, &dev->tx_ch_info[0], NULL);

	if (ret_code != EMAC_SUCCESS) {
		ERR("%s error: Error %08X from EMAC TX Channel Open()\n",
				netdev->name, ret_code);

		return -EINVAL;
  	}

	/* create a RX channel - note that last param contains mac address */
	ret_code = emac_ch_open(dev, &dev->rx_ch_info[0],
			(void *)&dev->mac_addr[0]);
	if (ret_code != EMAC_SUCCESS) {
		ERR("%s error: Error %08X from EMAC RX Channel Open()\n",
				netdev->name, ret_code);

		return -EINVAL;
	}

	/* open DDC instance */
	ret_code = emac_open(dev, NULL);
	if (ret_code != EMAC_SUCCESS) {
		ERR("%s error: Error %08X from EMAC Open()\n",
				netdev->name, ret_code);

		return -EINVAL;
	}

	/* we can have multiple interrupt lines running to the CPU */
	for (i = 0; 1; i++) {

		res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
		if (!res) {
			if (i)
				break;
			else
				goto error;
		}

		err = request_irq(res->start, emac_hal_isr, SA_INTERRUPT,
				res->name, dev);
  		if (err) {
			pr_err("Failed to register the irq %d for %s\n",
					res->start, res->name);
			goto error;
		}
	}

	phy_max = 1;
	phy_cnt = 0;
	for (i = 1; i < PHY_MAX_ADDR; i++) {
		err = emac_phy_init(dev, i, &phy_cnt, &phy_max);
		if (dev->phydev[i] != NULL) {
			if (err)
				break;

			/*
			 * Default active phy to first phy detected.
			 * The value can be changed via ethtool
			 */
			if (dev->phy_active == 0)
				dev->phy_active = i;
		}

		/* break when we found all the phys we expected */
		if (phy_cnt == phy_max)
			break;

		err = 0;
	}

	/* if no phys are found, scan for broadcast phy */
	if (phy_cnt == 0)
		err = emac_phy_init(dev, 0, &phy_cnt, &phy_max);

	if (err)
		goto error;

	if (netif_carrier_ok(netdev))
		netif_start_queue(netdev);
	else
		netif_stop_queue(netdev);

	dev->device_stats.start_tick = jiffies;
	pr_debug("Started the network queue for %s.\n", netdev->name);

	return 0;

error:
	while (--i >= 0) {
		res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
		if (!res)
			break;

		free_irq(res->start, dev);
	}

	return err;
}

/* close the adapter */
static int emac_dev_close(struct net_device *netdev)
{
	struct emac_dev *dev = NETDEV_PRIV(netdev);
	struct platform_device *pdev =
		to_platform_device(netdev->class_dev.dev);
	struct resource *res;
	int ret_code;
	int i;

	/* inform the upper layers. */
	netif_stop_queue(dev->owner);

	for (i = 0; i < PHY_MAX_ADDR; i++)
		if (dev->phydev[i] != NULL)
			phy_disconnect(dev->phydev[i]);

	/* prepare to close */
	set_bit(0, &dev->set_to_close);

	/* closing the DDC instance will close all channels also */
	ret_code = emac_close(dev, NULL);

	if (ret_code != EMAC_SUCCESS) {
		ERR("%s error: Error %08X from EMAC Close()\n",
				netdev->name, ret_code);
		return -EIO;
	} else {
		pr_debug("Device %s Closed.\n", netdev->name);
		dev->device_stats.start_tick = jiffies;
		dev->link_speed = 100000000;
		dev->link_mode = 1;
		netif_carrier_off(netdev);
	}

	/* free ISR */
	/* we can have multiple interrupt lines running to the CPU */
	for (i = 0; 1 ; i++) {

		res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
		if (!res)
			break;

		free_irq(res->start, dev);
	}

	return 0;
}

/* ioctl function */
static int emac_ioctl(struct net_device *netdev, struct ifreq *rq, int cmd)
{
	emac_drv_priv_ioctl priv_ioctl;
	struct emac_dev *dev = NETDEV_PRIV(netdev);

	if (cmd == SIOCDEVPRIVATE) {
		/* copy user data */
		if (copy_from_user((char *)&priv_ioctl, (char *)rq->ifr_data,
					sizeof(emac_drv_priv_ioctl)))
			return -EFAULT;

		switch (priv_ioctl.cmd) {
			/* program type 2/3 address filter */
		case EMAC_PRIV_FILTERING:
			{
				emac_type2_3_addr_filter_params filter_params;

				if (copy_from_user((char *)&filter_params,
					(char *)priv_ioctl.data,
					sizeof(emac_type2_3_addr_filter_params))
				)

					return -EFAULT;

				if (emac_control(dev,
					EMAC_IOCTL_TYPE2_3_FILTERING,
					(emac_type2_3_addr_filter_params *)
						&filter_params, NULL)
					!= EMAC_SUCCESS) {

					ERR("Failed to read params");
					return -EFAULT;
				}
				break;
			}

			/* read PHY register via MII interface */
		case EMAC_PRIV_MII_READ:
			{
				emac_phy_params phy_params;
				unsigned long irq_flags;

				/* copy user data into local variable */
				if (copy_from_user((char *)&phy_params,
						(char *)priv_ioctl.data,
						sizeof(emac_phy_params)))
					return -EFAULT;

				/* make sure this function does not
				 * clash with mii access during tick
				 * function */
				local_irq_save(irq_flags);

				if (emac_control(dev, EMAC_IOCTL_READ_PHY_REG,
						(void *)&phy_params, NULL)
						!= EMAC_SUCCESS) {
					ERR("Failed to read params");
					return -EFAULT;
				}

				/* copy the local data to user space */
				if (copy_to_user((char *)priv_ioctl.data,
						(char *)&phy_params,
						sizeof(emac_phy_params)))
					return -EFAULT;

				/* enable tick timer to access phy now
				   if required */
				local_irq_restore(irq_flags);
			}

			break;

			/* write PHY register via MII interface */
		case EMAC_PRIV_MII_WRITE:
			{
				emac_phy_params phy_params;
				unsigned long irq_flags;

				/* copy user data into local variable */
				if (copy_from_user((char *)&phy_params,
						(char *)priv_ioctl.data,
						sizeof(emac_phy_params)))
					return -EFAULT;

				/* make sure this function does not
				   clash with mii access during tick
				   function */
				local_irq_save(irq_flags);

				if (emac_control(dev, EMAC_IOCTL_WRITE_PHY_REG,
						 (void *)&phy_params,
						 NULL) != EMAC_SUCCESS) {
					ERR("Failed to read params");
					return -EFAULT;
				}

				/* enable tick timer to access phy now
				   if required */
				local_irq_restore(irq_flags);
			}
			break;

			/* get statistics */
		case EMAC_PRIV_GET_STATS:
			{
				emac_hw_statistics stats;

				/* Caller provides memory for HW stats
				   structure via "data" pointer */
				if (emac_control(dev, EMAC_IOCTL_GET_STATISTICS,
							(void *)&stats, NULL)
						!= EMAC_SUCCESS) {
					ERR("Failed to get statistics");
					return (EMAC_INTERNAL_FAILURE);
				}

				/* copy the local data to user space */
				if (copy_to_user((char *)priv_ioctl.data,
						(char *)&stats,
						sizeof(emac_hw_statistics)))
					return -EFAULT;
				break;
			}

			/* clear statistics */
		case EMAC_PRIV_CLR_STATS:
			{
				if (emac_control(dev, EMAC_IOCTL_CLR_STATISTICS,
							NULL, NULL)
						!= EMAC_SUCCESS) {
					ERR("Failed to clear statistics");
					return (EMAC_INTERNAL_FAILURE);
				}
				break;
			}
		default:
			return -EFAULT;
			break;
		}
	}

	else if (cmd == SIOTIMIB2) {
		TI_SNMP_CMD_T ti_snmp_cmd;

		/* now copy the user data */
		if (copy_from_user((char *)&ti_snmp_cmd, (char *)rq->ifr_data,
					sizeof(TI_SNMP_CMD_T)))
			return -EFAULT;

		switch (ti_snmp_cmd.cmd) {
		case TI_SIOCGINTFCOUNTERS:
			{
				struct mib2_if_counters mib_counter;

				/* Caller provides memory for HW stats
				   structure via "data" pointer */
				if (emac_control(dev, EMAC_IOCTL_IF_COUNTERS,
						(void *)&mib_counter, NULL)
						!= EMAC_SUCCESS) {
					ERR("Failed to get statistics");
					return (EMAC_INTERNAL_FAILURE);
				}

				/* copy the local data to user space */
				if (copy_to_user((char *)ti_snmp_cmd.data,
					(char *)&mib_counter,
					sizeof(struct mib2_if_counters)))

					return -EFAULT;
				break;
			}
		case TI_SIOCGINTFPARAMS:
			{
				struct mib2_if_params local_params;

				local_params.if_speed = dev->link_speed;
				local_params.if_high_speed =
					(local_params.if_speed) / 1000000;
				local_params.if_oper_status =
					((netdev->flags & IFF_UP)
					 ? MIB2_STATUS_UP : MIB2_STATUS_DOWN);
				local_params.if_promiscuous_mode =
					((netdev->flags & IFF_PROMISC)
					 ? TRUE : FALSE);

				/* now copy the counters to the user data */
				if (copy_to_user((char *)ti_snmp_cmd.data,
						(char *)&local_params,
						sizeof(struct mib2_if_params)))
					return -EFAULT;
			}
			break;

		case TI_SIOCGETHERCOUNTERS:
			{
				struct mib2_phy_counters phy_counter;

				/* Caller provides memory for HW stats
				   structure via "data" pointer */
				if (emac_control(dev, EMAC_IOCTL_ETHER_COUNTERS,
						(void *)&phy_counter, NULL)
						!= EMAC_SUCCESS) {
					ERR("Failed to get statistics");

					return (EMAC_INTERNAL_FAILURE);
				}

				/* copy the local data to user space */
				if (copy_to_user((char *)ti_snmp_cmd.data,
					(char *)&phy_counter,
					sizeof(struct mib2_phy_counters)))

					return -EFAULT;
				break;
			}

		case TI_SIOCGETHERPARAMS:
			{
				struct mib2_eth_params local_params;

				local_params.eth_duplex_status =
					((dev->link_mode == PHY_DUPLEX_FULL)
					 ? MIB2_FULL_DUPLEX : MIB2_HALF_DUPLEX);

				/* now copy the counters to the user data */
				if (copy_to_user((char *)ti_snmp_cmd.data,
						(char *)&local_params,
						sizeof(struct mib2_eth_params)))
					return -EFAULT;
				break;
			}

		default:
			return -EFAULT;
		}
	} else {
		return -EFAULT;
	}

	return (0);
}

/************************ HASH SUPPORT FUNCTIONS ************************/

/* get hash value using mechainsm in specs */
static u32 hash_get(u8 *addr)
{
	u32 hash;
	u8 tmpval;
	int cnt;
	hash = 0;

	for (cnt = 0; cnt < 2; cnt++) {
		tmpval = *addr++;
		hash ^= (tmpval >> 2) ^ (tmpval << 4);
		tmpval = *addr++;
		hash ^= (tmpval >> 4) ^ (tmpval << 2);
		tmpval = *addr++;
		hash ^= (tmpval >> 6) ^ (tmpval);
	}

	return (hash & 0x3F);
}

/**
 * Hash Table Add
 * - Adds mac address to hash table and upates hash bits in hardware
 * - Returns negative if error, 0 if no change to registers, >0 if
 *   hash registers need to change
 */
static int hash_add(struct emac_dev *dev, u8 *mac_address)
{
	u32 hash_value;
	u32 hash_bit;
	u32 status = 0;
	hash_value = hash_get(mac_address);

	if (hash_value >= EMAC_NUM_MULTICAST_BITS) {
		LOGERR("Invalid Hash Value=%d. Should not be greater than %d",
				hash_value, (EMAC_NUM_MULTICAST_BITS - 1));
		return (EMAC_INVALID_PARAM);
	}

	/* set the hash bit only if not previously set */
	if (dev->multicast_hash_cnt[hash_value] == 0) {
		status = 1;
		if (hash_value < 32) {
			hash_bit = (1 << hash_value);
			dev->mac_hash1 |= hash_bit;
		} else {
			hash_bit = (1 << (hash_value - 32));
			dev->mac_hash2 |= hash_bit;
		}
	}

	/*
	 * increment counter to maintain number of multicast address
	 * that map to this hash bit
	 */
	++dev->multicast_hash_cnt[hash_value];

	return (status);
}

/**
 * Hash Table Del
 * - Deletes a mac address from hash table and updates hash register bits
 * - Returns negative if error, 0 if no change to registers, >0 if
 *   hash registers need to change
 */
static int hash_del(struct emac_dev *dev, u8 *mac_address)
{
	u32 hash_value;
	u32 hash_bit;

	hash_value = hash_get(mac_address);
	if (dev->multicast_hash_cnt[hash_value] > 0) {
		/*
		 * decrement counter to reduce number of multicast
		 * address that map to this hash bit
		 */
		--dev->multicast_hash_cnt[hash_value];
	}

	/* if counter still > 0, at least one multicast address refers
	 * to this hash bit. so return 0 */
	if (dev->multicast_hash_cnt[hash_value] > 0) {
		return (0);
	}

	if (hash_value < 32) {
		hash_bit = (1 << hash_value);
		dev->mac_hash1 &= ~hash_bit;
	} else {
		hash_bit = (1 << (hash_value - 32));
		dev->mac_hash2 &= ~hash_bit;
	}

	/* return 1 to indicate change in mac_hash registers reqd */
	return (1);
}

/* updates hash register bits with single multicast address add/delete
 * operation */
static void emac_single_multi(struct emac_dev *dev, emac_single_multi_oper oper,
		u8 *addr)
{
	int status = -1;

	switch (oper) {
	case EMAC_MULTICAST_ADD:
		status = hash_add(dev, addr);
		break;

	case EMAC_MULTICAST_DEL:
		status = hash_del(dev, addr);
		break;

	default:
		LOGERR("Unhandled Single Multicast operation %d", oper);
		break;
	}

	/* write to the hardware only if the register status chances */
	if (status > 0) {
		dev->regs->mac_hash1 = dev->mac_hash1;
		dev->regs->mac_hash2 = dev->mac_hash2;
	}
}

/* updates hash register bits for all multi operation (set/clear) */
static void emac_all_multi(struct emac_dev *dev, emac_all_multi_oper oper)
{
	switch (oper) {
	case EMAC_ALL_MULTI_SET:
		dev->mac_hash1 = EMAC_ALL_MULTI_REG_VALUE;
		dev->mac_hash2 = EMAC_ALL_MULTI_REG_VALUE;
		break;
	case EMAC_ALL_MULTI_CLR:
		dev->mac_hash1 = 0;
		dev->mac_hash2 = 0;
		memset(&(dev->multicast_hash_cnt[0]), 0 ,
		sizeof(dev->multicast_hash_cnt[0]) * EMAC_NUM_MULTICAST_BITS);
		break;
	default:
		LOGERR("Unhandled All multi operation %d", oper);
		break;
	}

	dev->regs->mac_hash1 = dev->mac_hash1;
	dev->regs->mac_hash2 = dev->mac_hash2;
}

/************************ PHY related functions ************************/

/* Cpmac Update Phy Status - updates phy status variables in hDDC->status "CpmacDDCStatus" structure */
static void _emac_update_phy_status(struct net_device *netdev, int phy_num)
{
	struct emac_dev *dev = NETDEV_PRIV(netdev);

	/* no phy condition */
	if (dev->init_cfg.phy_mode & SNWAY_NOPHY) {
		/* no phy condition, always linked */
		dev->status.phy_linked = 1;
		dev->status.phy_speed = 100;
		dev->status.phy_duplex = PHY_DUPLEX_FULL;
		dev->status.phy_num = 0xFFFFFFFF;	/* no phy */

		dev->mac_control |= EMAC_MACCONTROL_FULLDUPLEXEN_MASK |
					EMAC_MACCONTROL_RMIISPEED_MASK;

		/* write mac control register from stored value */
		dev->regs->mac_control = dev->mac_control;

		emac_status_update(dev, &dev->status);
		return;
	}
	/* if loopback set in hardware, set link to ON */
	if (dev->mac_control & EMAC_MACCONTROL_LOOPBKEN_MASK) {
		dev->status.phy_linked = 1;
		return;
	}

	if (dev->phydev[phy_num]->link) {
		dev->status.phy_linked = dev->phydev[phy_num]->link;

		/*  retreive duplex and speed and the phy number  */
		dev->status.phy_duplex = dev->phydev[phy_num]->duplex ?
			PHY_DUPLEX_FULL : PHY_DUPLEX_HALF;
		dev->status.phy_speed = dev->phydev[phy_num]->speed;
		dev->status.phy_num = dev->phydev[phy_num]->addr;

		if (dev->status.phy_speed == 100)
			dev->mac_control |= EMAC_MACCONTROL_RMIISPEED_MASK;
		else
			dev->mac_control &= ~EMAC_MACCONTROL_RMIISPEED_MASK;

		/* set the duplex bit in maccontrol */
		if (dev->status.phy_duplex == PHY_DUPLEX_FULL) {
			dev->mac_control |= EMAC_MACCONTROL_FULLDUPLEXEN_MASK;
		} else {
			dev->mac_control &= ~EMAC_MACCONTROL_FULLDUPLEXEN_MASK;
		}

		if (dev->status.phy_speed == 1000)
			dev->mac_control |=
				((0x1 << EMAC_MACCONTROL_GIGABITEN_SHIFT) |
				 (0x1 << EMAC_MACCONTROL_GIG_FORCE_SHIFT));
		else
			dev->mac_control &=
			      ~((0x1 << EMAC_MACCONTROL_GIGABITEN_SHIFT) |
				(0x1 << EMAC_MACCONTROL_GIG_FORCE_SHIFT));
	}
	if (dev->status.phy_speed == 1000)
		dev->mac_control |= EMAC_MACCONTROL_GIGABITEN_MASK;
	else
		dev->mac_control &= ~EMAC_MACCONTROL_GIGABITEN_MASK;

	/* write mac control register from stored value */
	dev->regs->mac_control = dev->mac_control;

	emac_status_update(dev, &dev->status);

}

static void emac_update_phy_status(struct net_device *netdev)
{
	struct emac_dev *dev = NETDEV_PRIV(netdev);
	int i;

	/* verify proper device state */
	if (dev->drv_state != DRV_OPENED)
		return;

	for (i = 0; i < PHY_MAX_ADDR; i++)
		if (dev->phydev[i] != NULL)
			_emac_update_phy_status(netdev, i);
}

/* set phy mode */
static int emac_set_phy_mode(struct emac_dev *dev)
{
	u32 set_phy_mode;
	u32 phy_mode;

	LOGMSG(EMAC_DEBUG_BUSY_FUNCTION_ENTRY, "");

	/* verify proper device state */
	if (dev->drv_state != DRV_OPENED) {
		LOGERR("Device NOT Open");
		return (EMAC_ERR_DEV_NOT_OPEN);
	}

	set_phy_mode = dev->init_cfg.phy_mode;
	phy_mode = 0;
	if (set_phy_mode & SNWAY_AUTO)
		phy_mode |= NWAY_AUTO;
	if (set_phy_mode & SNWAY_FD10)
		phy_mode |= NWAY_FD10;
	if (set_phy_mode & SNWAY_FD100)
		phy_mode |= NWAY_FD100;
	if (set_phy_mode & SNWAY_FD1000)
		phy_mode |= NWAY_FD1000;
	if (set_phy_mode & SNWAY_HD10)
		phy_mode |= NWAY_HD10;
	if (set_phy_mode & SNWAY_HD100)
		phy_mode |= NWAY_HD100;
	if (set_phy_mode & SNWAY_HD1000)
		phy_mode |= NWAY_HD1000;
	if (set_phy_mode & SNWAY_LPBK)
		phy_mode |= NWAY_LPBK;
	if (set_phy_mode & SNWAY_AUTOMDIX)
		phy_mode |= NWAY_AUTOMDIX;
	/* check for EMAC bus frequency for correct speed operation */
	if ((set_phy_mode & SNWAY_FD10) || (set_phy_mode & SNWAY_HD10)) {
		if (dev->init_cfg.emac_bus_frequency
				<= EMAC_MIN_FREQUENCY_FOR_10MBPS)
			LOGERR("Bus speedemacSetPhyMode: CpmacFreq(%d) "
					"is less than required %d freq for "
					"10Mbps support. CANNOT SUPPORT 10Mbps",
					dev->init_cfg.emac_bus_frequency,
					EMAC_MIN_FREQUENCY_FOR_10MBPS);
	}

	else if ((set_phy_mode & SNWAY_FD100) || (set_phy_mode & SNWAY_HD100)) {
		if (dev->init_cfg.emac_bus_frequency
				<= EMAC_MIN_FREQUENCY_FOR_100MBPS)

			LOGERR("freq(%d) is less than required %d freq for "
				"100Mbps support. CANNOT SUPPORT 100Mbps",
				dev->init_cfg.emac_bus_frequency,
				EMAC_MIN_FREQUENCY_FOR_100MBPS);

	} else if ((set_phy_mode & SNWAY_FD1000)
			|| (set_phy_mode & SNWAY_HD1000)) {
		if (dev->init_cfg.emac_bus_frequency
				<= EMAC_MIN_FREQUENCY_FOR_1000MBPS)

			LOGERR("freq(%d) is less than required %d freq for "
				"1000Mbps support. CANNOT SUPPORT 1000Mbps",
				dev->init_cfg.emac_bus_frequency,
				EMAC_MIN_FREQUENCY_FOR_1000MBPS);
	}

	LOGMSG(EMAC_DEBUG_PORT_UPDATE,
			"MdioPhyMode=%08X, PhyMode=%08d, Auto:%d, FD10:%d, "
			"HD10:%d, FD100:%d, HD100:%d",
			set_phy_mode, phy_mode, (phy_mode & NWAY_AUTO),
			(phy_mode & NWAY_FD10), (phy_mode & NWAY_HD10),
			(phy_mode & NWAY_FD100), (phy_mode & NWAY_HD100));
	emac_update_phy_status(dev->owner);
	LOGMSG(EMAC_DEBUG_BUSY_FUNCTION_EXIT, "");

	return (EMAC_SUCCESS);
}

/***************** MAC ADDRESSING MODE SUPPORT FUNCTIONS ********************/

/* this function sets / clears the unicast flag in hardware */
static void emac_rx_uni_cast(struct emac_dev *dev, u32 channel, bool enable)
{
	/* update local copy of register to save cycles in reading the
	 * register */
	if (enable == TRUE) {
		dev->rx_unicast_set |= (1 << channel);
		dev->rx_unicast_clear &= ~(1 << channel);
	} else {
		/* disable unicast channel setting */
		dev->rx_unicast_clear |= (1 << channel);
		dev->rx_unicast_set &= ~(1 << channel);
	}

	/* write to hardware if device is open */
	if (dev->drv_state == DRV_OPENED) {
		dev->regs->rx_unicast_set = dev->rx_unicast_set;
		dev->regs->rx_unicast_clear = dev->rx_unicast_clear;
	}
}

/**
 * emac_set_type0addr: Set EMAC Type0 mac address
 * @priv: The DaVinci EMAC private adapter structure
 * @ch: RX channel number
 * @mac_addr: MAC address to set in device
 *
 * Called internally to set Type0 mac address of the adapter (Device)
 */
static void emac_set_type0addr(struct emac_dev *dev, u32 channel,
				char *mac_address)
{
	dev->regs->mac_src_addr_lo = (mac_address[0] << 8) | (mac_address[1]);
	dev->regs->mac_src_addr_hi = (mac_address[2] << 24) |
		(mac_address[3] << 16) | (mac_address[4] << 8) |
		(mac_address[5]);

	/* enable unicast */
	emac_rx_uni_cast(dev, channel, TRUE);
}

/**
 * emac_set_type1addr: Set EMAC Type1 mac address
 * @priv: The DaVinci EMAC private adapter structure
 * @ch: RX channel number
 * @mac_addr: MAC address to set in device
 *
 * Called internally to set Type1 mac address of the adapter (Device)
 */
static void emac_set_type1addr(struct emac_dev *dev, u32 channel,
				char *mac_address)
{
	/* set mac_index register with channel number */
	dev->regs->mac_index = channel;

	/* set mac_addr_lo register */
	dev->regs->mac_addr_lo = ((mac_address[5] << 8) | mac_address[4]);

	/* set mac_addr_hi register */
	dev->regs->mac_addr_hi = (mac_address[3] << 24) | (mac_address[2] << 16)
		| (mac_address[1] << 8) | (mac_address[0]);

	/* As per discussion with hardware folks, it is mandatory to
	   set the source address of the mac, else correct behaviour
	   is not guaranteed */
	emac_set_type0addr(dev, channel, mac_address);
}

/**
 * emac_set_type2addr: Set EMAC Type2 mac address
 * @priv: The DaVinci EMAC private adapter structure
 * @ch: RX channel number
 * @mac_addr: MAC address to set in device
 * @index: index into RX address entries
 * @match: match parameter for RX address matching logic
 *
 * Called internally to set Type2 mac address of the adapter (Device)
 */
static void emac_set_type2addr(struct emac_dev *dev, u32 channel,
		char *mac_address, int index, int match)
{
	dev->regs->mac_index = index;

	dev->regs->mac_addr_hi = (mac_address[3] << 24) | (mac_address[2] << 16)
		| (mac_address[1] << 8) | (mac_address[0]);

	dev->regs->mac_addr_lo = ((mac_address[5] << 8) | mac_address[4]
			| ((channel & 0x7) << 16) | (match << 19) | (1 << 20));

	emac_set_type0addr(dev, channel, mac_address);
}

/**
 * emac_setmac: Set mac address in the adapter (internal function)
 * @priv: The DaVinci EMAC private adapter structure
 * @ch: RX channel number
 * @mac_addr: MAC address to set in device
 *
 * Called internally to set the mac address of the adapter (Device)
 */
static void emac_setmac(struct emac_dev *dev, u32 ch, char *mac_addr)
{
	if (dev->rx_addr_type == 0) {
		emac_set_type0addr(dev, ch, mac_addr);
	} else if (dev->rx_addr_type == 1) {
		u32 cnt;
		for (cnt = 0; cnt < EMAC_MAX_TXRX_CHANNELS; cnt++)
			emac_set_type1addr(dev, ch, mac_addr);
	} else if (dev->rx_addr_type == 2) {
		emac_set_type2addr(dev, ch, mac_addr, ch, 1);
		emac_set_type0addr(dev, ch, mac_addr);
	}
}


/************************ HARDWARE CONFIGURATION SUPPORT FUNCTIONS ************************/

/* set RX hardware configuration */
void emac_set_rx_hw_cfg(struct emac_dev *dev)
{
	struct emac_rx_config *rx_cfg;
	u32 rx_mbp_enable;

	if (dev->drv_state != DRV_OPENED) {
		LOGERR("Function called when device is NOT in open state");
		return;
	}

	rx_cfg = &dev->init_cfg.rx_cfg;

	/* set RX MBP enable register */
	rx_mbp_enable = ((rx_cfg->pass_crc & 0x1) << EMAC_RXMBP_PASSCRC_SHIFT)
		| ((rx_cfg->qos_enable & 0x1) << EMAC_RXMBP_QOSEN_SHIFT)
		| ((rx_cfg->no_buffer_chaining & 0x1)
				<< EMAC_RXMBP_NOCHAIN_SHIFT)
		| ((rx_cfg->copy_maccontrol_frames_enable & 0x1)
				<< EMAC_RXMBP_CMFEN_SHIFT)
		| ((rx_cfg->copy_short_frames_enable & 0x1)
				<< EMAC_RXMBP_CSFEN_SHIFT)
		| ((rx_cfg->copy_error_frames_enable & 0x1)
				<< EMAC_RXMBP_CEFEN_SHIFT)
		| ((rx_cfg->promiscous_enable & 0x1) << EMAC_RXMBP_CAFEN_SHIFT)
		| ((rx_cfg->promiscous_channel & EMAC_RXMBP_CHMASK)
				<< EMAC_RXMBP_PROMCH_SHIFT)
		| ((rx_cfg->broadcast_enable & 0x1) << EMAC_RXMBP_BROADEN_SHIFT)
		| ((rx_cfg->broadcast_channel & EMAC_RXMBP_CHMASK)
				<< EMAC_RXMBP_BROADCH_SHIFT)
		| ((rx_cfg->multicast_enable & 0x1) << EMAC_RXMBP_MULTIEN_SHIFT)
		| ((rx_cfg->multicast_channel & EMAC_RXMBP_CHMASK)
				<< EMAC_RXMBP_MULTICH_SHIFT);

	if (rx_cfg->promiscous_enable) {
		/* disable mcast bcast and unicast:  H/W limitation */
		rx_mbp_enable &= ~(0x1 << EMAC_RXMBP_BROADEN_SHIFT);
		rx_mbp_enable &= ~(0x1 << EMAC_RXMBP_MULTIEN_SHIFT);

		/* disable unicast - warning!! assuming only one
		 * channel open */
		emac_rx_uni_cast(dev, (dev->rx_cppi[0])->ch_info.ch_num,
				 FALSE);
	} else {
		/* enable unicast - warning!! assuming only one
		 * channel open */
		emac_rx_uni_cast(dev, (dev->rx_cppi[0])->ch_info.ch_num, TRUE);
	}

	if (dev->rx_MBP_enable != rx_mbp_enable) {
		dev->rx_MBP_enable = rx_mbp_enable;
		dev->regs->rx_MBP_enable = rx_mbp_enable;
	}

	/* set max rx packet length */
	dev->regs->rx_maxlen =
		(rx_cfg->max_rx_pkt_length & EMAC_RX_MAX_LEN_MASK);

	/* set rx buffer offset */
	dev->regs->rx_buffer_offset =
		(rx_cfg->buffer_offset & EMAC_RX_BUFFER_OFFSET_MASK);

	LOGMSG(EMAC_DEBUG_FUNCTION_EXIT,
			"Rx_MBP_Enable = 0x%08x\n", rx_mbp_enable);
}

/**
 * EMAC DDC Open
 *  - Brings module out of reset
 *  - Open's CSL, programs mandatory hardware init registers
 *  - Open's MII_MDIO module and enable poll timer via DDA
 *  - Enables earlier created TX/RX channels
 *  - Enables TX/RX operation in hardware
 *
 * "param" not used in this implementation
 */
static int emac_open(struct emac_dev *dev, void *param)
{
	u32 channel;
	int ret_val;
	struct emac_init_config *init_cfg;
        volatile struct ewrap_dm646x_regs *dm646x_wrap = NULL;
	u32 clk_div;

	LOGMSG(EMAC_DEBUG_FUNCTION_ENTRY, "");

	if (dev->drv_state == DRV_OPENED) {
		LOGERR("Device already open");
		return (EMAC_ERR_DEV_ALREADY_OPEN);
	}

	/* get init config info structure pointer for easy access */
	init_cfg = &dev->init_cfg;

	if (!dev->init_cfg.registers_old) {
		/* bring EMAC out of reset - for clean implementation, reset
		 * and then unreset the module */
		/* for EMAC 2.6 and beyond, reset is internal to the module */
		dm646x_wrap = &dev->ewrap_regs->wrapper_646x_regs;
		dm646x_wrap->SOFTRST = 1;
		while (dm646x_wrap->SOFTRST) {
			/* wait for reset to complete */
		}
        }

	dev->regs->soft_reset = 1;
	while (dev->regs->soft_reset) {
		/* wait for reset to complete - do nothing */
	}

	/*
	 * reinitialize mdio after reset--rather bad idea;
	 * review init code instead
	 */
	clk_div = ((dev->init_cfg.mdio_bus_frequency /
			dev->init_cfg.mdio_clock_frequency) - 1) & 0xff;

	__raw_writel(clk_div | MDIO_CONTROL_ENABLE,
			dev->mdio_base_address + MDIO_CONTROL);

	/* program TX/RX HDP's to 0 */
	for (channel = 0; channel < EMAC_MAX_TXRX_CHANNELS; channel++) {
		dev->regs->tx_HDP[channel] = 0;

		/* initialize the completion pointers to 0 */
		dev->regs->tx_CP[channel] = 0;
	}

	for (channel = 0; channel < EMAC_MAX_TXRX_CHANNELS; channel++) {
		dev->regs->rx_HDP[channel] = 0;

		/* initialize the completion pointers to 0 */
		dev->regs->rx_CP[channel] = 0;
	}

	/* enable TX/RX DMA */
	dev->regs->tx_control |= EMAC_TX_CONTROL_TX_ENABLE_VAL;
	dev->regs->rx_control |= EMAC_RX_CONTROL_RX_ENABLE_VAL;

	/* enable adapter check interrupts - disable stats interupt */
	dev->regs->mac_int_mask_set = EMAC_MAC_HOST_ERR_INTMASK_VAL;

	if (!dev->init_cfg.registers_old) {
		/* update wrapper book-keeping fields */
		dev->misc_en = (EMAC_MAC_HOST_ERR_INTMASK_VAL);
		dm646x_wrap->C0_MISCEN = EMAC_MAC_HOST_ERR_INTMASK_VAL;
	}

	/* set device state - opened - useful when opening channels */
	dev->drv_state = DRV_OPENED;

	/* set the mac_control register */
	dev->mac_control = (1 << EMAC_MACCONTROL_TXPTYPE_SHIFT);
	dev->regs->mac_control = dev->mac_control;

	/* set the PHY to a given mode - as per config parameters and
	 * update DDA layer */
	emac_set_phy_mode(dev);

	/* enable opened TX channels */
	for (channel = 0; channel < EMAC_MAX_TXRX_CHANNELS; channel++) {
		if (dev->tx_cppi[channel] != NULL) {
			ret_val = emac_enable_channel(dev, channel,
					NET_CH_DIR_TX);
			if (ret_val != EMAC_SUCCESS) {
				LOGERR("Error enabling TX channel %d", channel);

				/* TODECIDE: should we return from
				 * here or continue enabling other
				 * channels */
				return (ret_val);
			}
		}
	}

	/* set filter low threshold - not supported, hence set to 0 */
	dev->regs->rx_filter_low_thresh = 0;

	/* disable unicast on all channels first - enabled if channel
	 * is configured & enabled below */
	dev->regs->rx_unicast_clear = EMAC_RX_UNICAST_CLEAR_ALL;

	/* set MAC hash register */
	dev->regs->mac_hash1 = dev->mac_hash1;
	dev->regs->mac_hash2 = dev->mac_hash2;

	/* RX MBP, RX pkt length and RX buffer offset registers taken
	 * care by this function */
	emac_set_rx_hw_cfg(dev);

	/* read RX address matching/filtering type (0/1/2) */
	dev->rx_addr_type = (dev->regs->mac_cfig >> 8) & 0xFF;

	/* enable opened RX channels */
	for (channel = 0; channel < EMAC_MAX_TXRX_CHANNELS; channel++) {
		if (dev->rx_cppi[channel] != NULL) {
			ret_val = emac_enable_channel(dev, channel,
					NET_CH_DIR_RX);
			if (ret_val != EMAC_SUCCESS) {
				LOGERR("Error enabling RX channel %d", channel);

				/* TODECIDE: should we return from
				 * here or continue enabling other
				 * channels */
				return (ret_val);
			}
		}

		/* since flow threshold and free buffer feature is not
		 * supported, set it to 0 */
		dev->regs->rx_flow_thresh[channel] = 0;
		dev->regs->rx_free_buffer[channel] = 0;
	}

	/* finally set MAC control register, enable MII */
	dev->mac_control |= (1 << EMAC_MACCONTROL_MIIEN_SHIFT);
	dev->regs->mac_control = dev->mac_control;

	/* start the MIB cnt tick timer via DDA */
	emac_timer_start(dev, init_cfg->mib64cnt_msec);

	/* enable interrupts via module control (wrapper) */
	emac_enable_interrupt(dev, 1);

	LOGMSG(EMAC_DEBUG_FUNCTION_EXIT, "");

	return (EMAC_SUCCESS);

}

/**
 * EMAC DDC Close
 *  - Disables poll timer via DDA
 *  - Disable and Close all open TX/RX channels
 *  - Closes CSL
 *  - Puts module in reset
 *
 * "param" not used in this implementation
 */
static int emac_close(struct emac_dev *dev, void *param)
{
	int ret_val;
	int err_val = EMAC_SUCCESS;
	u32 channel;

	LOGMSG(EMAC_DEBUG_FUNCTION_ENTRY, "");

	if (dev->drv_state == DRV_CLOSED) {
		LOGERR("Device already closed");
		return (EMAC_ERR_DEV_ALREADY_CLOSED);
	}

	/* stop the mib timer via DDA */
	emac_timer_stop(dev);
	/* close TX channels */
	for (channel = 0; channel < EMAC_MAX_TXRX_CHANNELS; channel++) {
		if (dev->tx_cppi[channel] != NULL) {
			ret_val = emac_ch_close(dev, channel, NET_CH_DIR_TX,
					NULL);
			if (ret_val != EMAC_SUCCESS) {
				LOGERR("Error closing TX channel %d", channel);

				/* instead of returning immediatley on
				 * error, we close all possible
				 * channels */
				err_val = ret_val;
			}
		}
	}

	/* close RX channels */
	for (channel = 0; channel < EMAC_MAX_TXRX_CHANNELS; channel++) {
		if (dev->rx_cppi[channel] != NULL) {
			ret_val = emac_ch_close(dev, channel, NET_CH_DIR_RX,
					NULL);

			if (ret_val != EMAC_SUCCESS) {
				LOGERR("Error closing RX channel %d", channel);

				/* instead of returning immediatley on
				 * error, we close all possible
				 * channels */
				err_val = ret_val;	/* return (ret_val); */
			}
		}
	}

	/* disable interrupts via module control (wrapper) */
	emac_disable_interrupt(dev);

	/* put EMAC in reset */
	dev->regs->soft_reset = 1;

	/* put MDIO in reset - not required for davinci */

	LOGMSG(EMAC_DEBUG_FUNCTION_EXIT, "");

	/* If closed all channels successfully. mark the DDC as closed */
	if (err_val == EMAC_SUCCESS)
		dev->drv_state = DRV_CLOSED;

	return (err_val);
}

/**
 * EMAC DDC Ioctl
 *  - Get Software (DDC) and Hardware Versions
 *  - Set/Modify RX and MAC configuration
 *  - Get DDC/module status
 *  - Read/Write MII registers (via PHY)
 *  - Get/Clr Statistics (hardware counters)
 *  - Add/Del/ Multicast operations AllMulti Set/Clear operations
 *  - Type2/3 Filtering operation
 *
 * "param" not used in this implementation
 */
static int
emac_control(struct emac_dev *dev, int cmd, void *cmd_arg, void *param)
{
	/* sanity check */
	if (dev->drv_state != DRV_OPENED) {
		LOGERR("ioctl called when device is NOT open");
		return (EMAC_ERR_DEV_NOT_OPEN);
	}

	switch (cmd) {
	case EMAC_IOCTL_GET_HWVER:
		/*
		 * Read hardware versions only if device is in open
		 * state
		 */
		/*
		 * Cmd is a ptr to an integer that will contain Tx Id
		 * ver and param is a pointer to an integer that will
		 * contain rx id ver after this call
		 */
		if (dev->drv_state == DRV_OPENED) {
			*((u32 *) cmd_arg) = dev->regs->tx_id_ver;
			*((u32 *) param) = dev->regs->rx_id_ver;
		} else {
			return (EMAC_ERR_DEV_NOT_OPEN);
		}
		break;

	case EMAC_IOCTL_SET_RXCFG:
		/* rx configuration structure passed in structure
		 * pointed by cmd_arg, params not used */
		if (cmd_arg != NULL) {
			dev->init_cfg.rx_cfg =
				*((struct emac_rx_config *) cmd_arg);
			emac_set_rx_hw_cfg(dev);
		} else {
			return (EMAC_INVALID_PARAM);
		}
		break;

	case EMAC_IOCTL_GET_STATUS:
		/* returns emac_status structure back in cmd_arg
		 * pointer pointed structure */
		{
			emac_status *status = (emac_status *) cmd_arg;
			*status = dev->status;	/* structure copy */
		}
		break;
	case EMAC_IOCTL_READ_PHY_REG:
		/* cmd = pointer to CpmacPhyParams struct. data read
		 * back into "data" parameter in the structure */
		{
			/* \warning: Read to the phy registers - Note
			   that this code loops on a completion bit in
			   the phy so there are chances of hanging" */
			emac_phy_params *phy_params =
				(emac_phy_params *) cmd_arg;

			phy_params->data =
				phy_read(dev->phydev[phy_params->phy_num],
					phy_params->reg_addr);
		}
		break;

	case EMAC_IOCTL_WRITE_PHY_REG:
		/* cmd = pointer to CpmacPhyParams struct. data to be
		 * written is in "data" parameter in the structure */
		{
			emac_phy_params *phy_params =
				(emac_phy_params *) cmd_arg;

			/* \warning: Write to the phy registers - Note
			   that this code loops on a completion bit in
			   the phy so there are chances of hanging" */
			phy_write(dev->phydev[phy_params->phy_num],
					phy_params->reg_addr,
					phy_params->data);
		}
		break;
	case EMAC_IOCTL_GET_STATISTICS:
		/* cmd_arg points to the user provided structure for
		 * statistics which match with hardware 36 regs, param
		 * is not used */
		{
			u32 cnt;
			u32 *user_stats = (u32 *) cmd_arg;
			volatile u32 *addr =
				(u32 *)&dev->regs->rx_good_frames;

			for (cnt = 0; cnt < EMAC_NUM_STAT_REGS;
					cnt++, user_stats++, addr++)
				*user_stats = *addr;
		}

		break;

	case EMAC_IOCTL_CLR_STATISTICS:
		/* cmd_arg or param is not used */
		{
			u32 cnt;
			volatile u32 *addr =
				(u32 *)&dev->regs->rx_good_frames;

			for (cnt = 0; cnt < EMAC_NUM_STAT_REGS; cnt++, addr++)
				*addr = EMAC_STAT_CLEAR; /* 0xFFFFFFFF value */
			emac_ddcifcnt_clear(dev);
		}
		break;

	case EMAC_IOCTL_MULTICAST_ADDR:
		/* cmd_arg= emac_multicast_oper enum, param = pointer
		 * to multicast address - u8 */
		{
			u8 *addr = (u8 *) param;
			emac_single_multi(dev, (emac_single_multi_oper)cmd_arg,
					addr);
		}
		break;

	case EMAC_IOCTL_ALL_MULTI:
		/* cmd_arg= emac_all_multi_oper enum, param=not used */
		emac_all_multi(dev, (emac_all_multi_oper) cmd_arg);
		break;

	case EMAC_IOCTL_TYPE2_3_FILTERING:
		{
			/* cmd_arg = pointer to
			 * emac_type2_3_addr_filter_params structure,
			 * param=not used */
			emac_type2_3_addr_filter_params *addr_params;

			addr_params =
				(emac_type2_3_addr_filter_params *)cmd_arg;
			emac_set_type2addr(dev, addr_params->channel,
					addr_params->mac_address,
					addr_params->index, addr_params->match);
		}
		break;

	case EMAC_IOCTL_SET_MAC_ADDRESS:
		{
			/* cmd_arg = pointer to
			 * emac_type2_3_addr_filter_params structure,
			 * param=not used */
			emac_address_params *addr_params;
			emac_rx_cppi_ch *rx_cppi;
			int cnt;

			if (dev->drv_state != DRV_OPENED) {
				LOGERR("EMAC_IOCTL_TYPE2_3_FILTERING Ioctl "
						"called when device is NOT "
						"in open state");
				return (EMAC_ERR_DEV_NOT_OPEN);
			}

			addr_params = (emac_address_params *) cmd_arg;
			rx_cppi = dev->rx_cppi[addr_params->channel];
			if (rx_cppi == NULL) {
				LOGERR("Invalid Channel %d. RX CPPI "
						"structure NULL",
						addr_params->channel);
				return (EMAC_ERR_RX_CH_INVALID);
			}

			for (cnt = 0; cnt < 6; cnt++)
				rx_cppi->mac_addr[cnt] =
					addr_params->mac_address[cnt];

			/* set interface MAC address */
			emac_setmac(dev, addr_params->channel,
					addr_params->mac_address);
		}
		break;

	case EMAC_IOCTL_IF_COUNTERS:
		emac_ddcifcnt_updt(dev);

		memcpy(cmd_arg, &dev->mib2if_hccounter.mib2if_counter,
				sizeof(struct mib2_if_counters));
		break;

	case EMAC_IOCTL_ETHER_COUNTERS:
		emac_ddcphycnt(dev, cmd_arg);
		break;

	case EMAC_IOCTL_IF_PARAMS_UPDT:
		emac_ddcifcnt_updt(dev);
		break;

	default:
		LOGERR("Unhandled ioctl code %d", cmd);
		break;
	}

	return (EMAC_SUCCESS);
}

/**
 * EMAC DDC Channel Open
 *  - Verify channel info (range checking etc)
 *  - Allocate memory for the channel
 *  - Book-keep operations for the channel - ready to be enabled in hardware
 *
 * 1. If DDC instance is in "Opened" state, the channel is enabled in hardware
 *       2. "chOpenArgs" is used only for opening RX channel
 */
static int emac_ch_open(struct emac_dev *dev, emac_ch_info *ch_info,
			void *ch_open_args)
{
	int ret_val;

	LOGMSG(EMAC_DEBUG_FUNCTION_ENTRY, "ChannelNo=%d, Dir=%s",
			ch_info->ch_num,
			((ch_info->ch_dir == NET_CH_DIR_TX) ? "TX" : "RX"));

	/* if the channel state is not NET_CH_UNINITIALIZED, return error */
	if (ch_info->ch_state != NET_CH_UNINITIALIZED) {
		LOGERR("%s channel %d  should be in NET_CH_UNINITIALIZED state",
		     ((ch_info->ch_dir == NET_CH_DIR_TX) ? "TX" : "RX"),
		     ch_info->ch_num);
		return (EMAC_INVALID_PARAM);
	}

	/* init channel */
	if (ch_info->ch_dir == NET_CH_DIR_TX) {
		if (ch_info->ch_num >= dev->init_cfg.num_tx_channels) {
			LOGERR("Invalid TX Channel=%d specified",
					ch_info->ch_num);
			return (EMAC_ERR_TX_CH_INVALID);
		}

		if (dev->tx_is_created[ch_info->ch_num] == TRUE) {
			LOGERR("TX Channel %d already open", ch_info->ch_num);
			return (EMAC_ERR_TX_CH_ALREADY_INIT);
		}

		/* allocate channel memory and perform other book-keep
		 * functions for the channel */
		ret_val = emac_init_tx_channel(dev, ch_info, ch_open_args);
		if (ret_val != EMAC_SUCCESS) {
			LOGERR("Error in initializing TX channel %d",
					ch_info->ch_num);
			return (ret_val);
		}
	} else if (ch_info->ch_dir == NET_CH_DIR_RX) {
		if (ch_info->ch_num >= dev->init_cfg.num_rx_channels) {
			LOGERR("Invalid RX Channel=%d specified",
					ch_info->ch_num);
			return (EMAC_ERR_RX_CH_INVALID);
		}

		if (dev->rx_is_created[ch_info->ch_num] == TRUE) {
			LOGERR("RX Channel %d already open", ch_info->ch_num);
			return (EMAC_ERR_RX_CH_ALREADY_INIT);
		}

		/* allocate channel memory and perform other book-keep
		 * functions for the channel */
		ret_val = emac_init_rx_channel(dev, ch_info, ch_open_args);

		if (ret_val != EMAC_SUCCESS) {
			LOGERR("Error in initializing RX channel %d",
					ch_info->ch_num);
			return (ret_val);
		}
	}

	/* if device is opened already, enable this channel for use */
	if (dev->drv_state == DRV_OPENED) {
		ret_val = emac_enable_channel(dev, ch_info->ch_num,
				ch_info->ch_dir);
		if (ret_val != EMAC_SUCCESS) {
			LOGERR("Error enabling channel %d in %d direction",
					ch_info->ch_num, ch_info->ch_dir);
			return (ret_val);
		}
	}

	LOGMSG(EMAC_DEBUG_FUNCTION_EXIT, "ChannelNo=%d, Dir=%s",
			ch_info->ch_num,
			((ch_info->ch_dir == NET_CH_DIR_TX) ? "TX" : "RX"));

	return (EMAC_SUCCESS);
}

/**
 * EMAC DDC Channel Close
 *  - If DDC instance is in "Opened" state, disable the channel in hardware
 *  - Un-initialize the channel (free memory previously allocated)
 */
static int emac_ch_close(struct emac_dev *dev, int channel,
			 int direction, void *ch_close_args)
{
	int ret_val;

	LOGMSG(EMAC_DEBUG_FUNCTION_ENTRY, "ChannelNo=%d, Dir=%s", channel,
			((direction == NET_CH_DIR_TX) ? "TX" : "RX"));

	/* disable this channel */
	if (dev->drv_state == DRV_OPENED) {
		ret_val = emac_disable_channel(dev, channel, direction);
		if (ret_val != EMAC_SUCCESS) {
			LOGERR("Error disabling channel %d in %s direction",
				channel,
				((direction == NET_CH_DIR_TX) ? "TX" : "RX"));
			return (ret_val);
		}
	}

	/* un_init channel */
	if (direction == NET_CH_DIR_TX) {
		ret_val = emac_un_init_tx_channel(dev, channel, ch_close_args);
		if (ret_val != EMAC_SUCCESS) {
			LOGERR("Error in UnInit of TX channel %d", channel);
			return (ret_val);
		}
	}

	else if (direction == NET_CH_DIR_RX) {
		ret_val = emac_un_init_rx_channel(dev, channel, ch_close_args);
		if (ret_val != EMAC_SUCCESS) {
			LOGERR("Error in UnInit of TX channel %d", channel);
			return (ret_val);
		}
	}

	LOGMSG(EMAC_DEBUG_FUNCTION_EXIT, "ChannelNo=%d, Dir=%s", channel,
			((direction == NET_CH_DIR_TX) ? "TX" : "RX"));

	return (EMAC_SUCCESS);
}

/**
 * Init Tx Channel
 *  - Allocates memory for TX Ch Control structure, Buffer descriptors
 *  - Initialize the above data structures as per channel configuration
 *  - Chain the TX BD list ready to be given to hardware
 *
 * 1. "chOpenArgs" not used in this implementation
 *
 * 2. This function assumes that the channel number passed is valid
 * and the hDDC->txCppi[channel] pointer is NULL. This function will
 * not do any error check on these parameters to avoid duplicate error
 * checks (done in caller function).
 */
static int emac_init_tx_channel(struct emac_dev *dev,
				emac_ch_info * ch_info, void *ch_open_args)
{
	u32 cnt, bd_size;
	char *alloc_mem;
	emac_tx_bd *curr_bd;
	emac_tx_cppi_ch *tx_cppi = NULL;

	LOGMSG(EMAC_DEBUG_FUNCTION_ENTRY, "ChannelNo=%d", ch_info->ch_num);

	/* allocate memory for TX CPPI channel and set to 0 */
	emac_malloc(sizeof(emac_tx_cppi_ch), (void **)&tx_cppi);

	/* update the channel control structure in DDC */
	dev->tx_cppi[ch_info->ch_num] = tx_cppi;

	/* populate channel info */
	tx_cppi->ch_info = *ch_info;	/* structure copy */
	tx_cppi->ch_info.ch_state = NET_CH_INITIALIZED;
	tx_cppi->active_queue_head = 0;
	tx_cppi->active_queue_tail = 0;
	tx_cppi->queue_active = FALSE;
	dev->tx_teardown_pending[ch_info->ch_num] = FALSE;

#ifdef EMAC_MULTIPACKET_TX_COMPLETE_NOTIFY
	/* allocate memory for TX CPPI channel on a 4 byte boundry */
	emac_malloc((ch_info->service_max * sizeof(u32)),
			(void **)&tx_cppi->tx_complete);
#endif

	/* allocate buffer descriptor pool align every BD on four word
	 * boundry for future requirements */
	bd_size = (sizeof(emac_tx_bd) + 0xF) & ~0xF;
	tx_cppi->alloc_size = (((bd_size * ch_info->num_bd) + 0xF) & ~0xF);

	/* alloc TX BD memory */
	tx_cppi->bd_mem = dev->ewrap_ram;
	memzero(tx_cppi->bd_mem, tx_cppi->alloc_size);

	/* initialize the BD linked list */
	alloc_mem = (char *)(((u32) tx_cppi->bd_mem + 0xF) & ~0xF);

	tx_cppi->bd_pool_head = 0;
	for (cnt = 0; cnt < ch_info->num_bd; cnt++) {
		curr_bd = (emac_tx_bd *) (alloc_mem + (cnt * bd_size));
		curr_bd->next = tx_cppi->bd_pool_head;
		tx_cppi->bd_pool_head = curr_bd;
	}

	/* reset statistics counters */
	tx_cppi->out_of_tx_bd = 0;
	tx_cppi->no_active_pkts = 0;
	tx_cppi->active_queue_count = 0;
	dev->tx_is_created[ch_info->ch_num] = TRUE;
	LOGMSG(EMAC_DEBUG_FUNCTION_EXIT, "ChannelNo=%d", ch_info->ch_num);

	return (EMAC_SUCCESS);
}

/**
 * Un-Init Tx Channel
 *
 * - Frees memory previously allocated for Ch Control structure,
 *     Buffer descriptors
 *
 * 1. "chCloseArgs" not used in this implementation
 * 2. This function assumes that the channel number passed is valid
 * and this function will not do any error check to avoid duplicate
 * error checks (done in caller function).
 */
static int emac_un_init_tx_channel(struct emac_dev *dev, u32 channel,
		void *ch_close_args)
{
	emac_tx_cppi_ch *tx_cppi;

	LOGMSG(EMAC_DEBUG_FUNCTION_ENTRY, "ChannelNo=%d", channel);

	/* check if channel structure is already de-allocated */
	if (dev->tx_is_created[channel] == FALSE) {
		LOGERR("TX CPPI Channel %d structure already freed", channel);
		return (EMAC_ERR_TX_CH_ALREADY_CLOSED);
	}

	tx_cppi = dev->tx_cppi[channel];

	/* free the buffer descriptors memory */
	if (tx_cppi->bd_mem != NULL) {
		tx_cppi->bd_mem = NULL;
	}
#ifdef EMAC_MULTIPACKET_TX_COMPLETE_NOTIFY
	/* free the TX complete queue */
	emac_free(tx_cppi->tx_complete);
#endif

	/* free the TX channel structure */
	emac_free(tx_cppi);
	dev->tx_cppi[channel] = NULL;
	dev->tx_is_created[channel] = FALSE;
	LOGMSG(EMAC_DEBUG_FUNCTION_EXIT, "ChannelNo=%d", channel);

	return (EMAC_SUCCESS);
}

/**
 * Init Rx Channel
 *  - Allocates memory for RX Ch Control structure, Buffer descriptors
 *  - Initialize the above data structures as per channel configuration
 * - Allocate receive buffers from DDA and chain the RX BD list ready
 *     to be given to hardware
 *
 * 1. "chOpenArgs" Points to MAC address for this channel
 * 2. This function assumes that the channel number passed is valid
 * and the hDDC->rxCppi[channel] pointer is NULL. This function will
 * not do any error check on these parameters to avoid duplicate error
 * checks (done in caller function).
 */
static int emac_init_rx_channel(struct emac_dev *dev,
				emac_ch_info * ch_info, void *ch_open_args)
{
	u32 cnt, bd_size;
	char *alloc_mem;
	emac_rx_bd *curr_bd;
	emac_rx_cppi_ch *rx_cppi = NULL;

	LOGMSG(EMAC_DEBUG_FUNCTION_ENTRY, "ChannelNo=%d", ch_info->ch_num);

	/* allocate memory for RX CPPI channel */
	emac_malloc(sizeof(emac_rx_cppi_ch), (void **)&rx_cppi);
	/* update the channel control structure in DDC */
	dev->rx_cppi[ch_info->ch_num] = rx_cppi;

	rx_cppi->ch_info = *ch_info;	/* structure copy */
	rx_cppi->ch_info.ch_state = NET_CH_INITIALIZED;
	dev->rx_teardown_pending[ch_info->ch_num] = FALSE;

	/* save mac address */
	alloc_mem = (char *)ch_open_args;
	for (cnt = 0; cnt < 6; cnt++)
		rx_cppi->mac_addr[cnt] = alloc_mem[cnt];

	/* allocate buffer descriptor pool align every BD on four word
	 * boundry for future requirements */
	bd_size = (sizeof(emac_rx_bd) + 0xF) & ~0xF;
	rx_cppi->alloc_size = (((bd_size * ch_info->num_bd) + 0xF) & ~0xF);

	/* alloc RX BD memory */
	rx_cppi->bd_mem = dev->ewrap_ram + (dev->ewrap_ram_size / 2);
	memzero(rx_cppi->bd_mem, rx_cppi->alloc_size);

	rx_cppi->pkt_queue.buf_list = &rx_cppi->buf_queue[0];

	/* allocate RX buffer and initialize the BD linked list */
	alloc_mem = (char *)(((u32) rx_cppi->bd_mem + 0xF) & ~0xF);
	rx_cppi->active_queue_head = 0;
	rx_cppi->active_queue_tail = (emac_rx_bd *) alloc_mem;
	for (cnt = 0; cnt < ch_info->num_bd; cnt++) {
		curr_bd = (emac_rx_bd *) (alloc_mem + (cnt * bd_size));

		/* for potential future use the last parameter
		 * contains the BD ptr */
		curr_bd->data_ptr = (void *)(emac_net_alloc_rx_buf(dev,
				ch_info->buf_size,
				(emac_net_data_token *)&curr_bd->buf_token, 0,
				(void *)curr_bd));
		if (curr_bd->data_ptr == NULL) {
			LOGERR("Error in RX Buffer allocation for channel %d",
					ch_info->ch_num);
			return (EMAC_ERR_RX_BUFFER_ALLOC_FAIL);
		}

		/* populate the hardware descriptor */
		curr_bd->h_next = EMAC_VIRT_TO_PHYS(rx_cppi->active_queue_head);
		curr_bd->buff_ptr = EMAC_VIRT_TO_PHYS(curr_bd->data_ptr);
		curr_bd->off_b_len = ch_info->buf_size;
		curr_bd->mode = EMAC_CPPI_OWNERSHIP_BIT;

		/* write back to hardware memory */
		BD_CACHE_WRITEBACK_INVALIDATE((u32) curr_bd,
				EMAC_BD_LENGTH_FOR_CACHE);
		curr_bd->next = (void *)rx_cppi->active_queue_head;
		rx_cppi->active_queue_head = curr_bd;
	}

	/* At this point rxCppi->activeQueueHead points to the first
	   RX BD ready to be given to RX HDP and
	   rx_cppi->active_queue_tail points to the last RX BD */
	dev->rx_is_created[ch_info->ch_num] = TRUE;

	LOGMSG(EMAC_DEBUG_FUNCTION_EXIT, "ChannelNo=%d", ch_info->ch_num);

	return (EMAC_SUCCESS);
}

/**
 * Un-Init Rx Channel
 * - Frees memory previously allocated for Ch Control structure,
 *     Buffer descriptors
 * - Returns (Frees) back receive buffers to DDA layer
 *
 * 1. "chCloseArgs" not used in this implementation
 * 2. This function assumes that the channel number passed is valid
 * and this function will not do any error check to avoid duplicate
 * error checks (done in caller function).
 */
static int emac_un_init_rx_channel(struct emac_dev *dev, u32 channel,
		void *ch_close_args)
{
	emac_rx_cppi_ch *rx_cppi;
	emac_rx_bd *curr_bd;

	LOGMSG(EMAC_DEBUG_FUNCTION_ENTRY, "ChannelNo=%d", channel);

	/* check if channel structure is already de-allocated */
	if (dev->rx_is_created[channel] == FALSE) {
		LOGERR("RX CPPI Channel %d structure already freed", channel);
		return (EMAC_ERR_RX_CH_ALREADY_CLOSED);
	}

	rx_cppi = dev->rx_cppi[channel];

	/* free the receive buffers previously allocated */
	curr_bd = rx_cppi->active_queue_head;
	while (curr_bd) {
		if (emac_net_free_rx_buf(dev,
					 curr_bd->data_ptr,
					 (emac_net_data_token) curr_bd->
					 buf_token, 0, NULL) != EMAC_SUCCESS) {
			LOGERR("Failed to free RX buffer Ch %d", channel);
		}
		curr_bd = curr_bd->next;
	}

	/* free the buffer descriptors memory */
	if (rx_cppi->bd_mem != NULL) {
		rx_cppi->bd_mem = NULL;
	}

	/* free the RX channel structure */
	emac_free(rx_cppi);
	dev->rx_cppi[channel] = NULL;
	dev->rx_is_created[channel] = FALSE;
	LOGMSG(EMAC_DEBUG_FUNCTION_EXIT, "ChannelNo=%d", channel);

	return (EMAC_SUCCESS);
}

/**
 * Enable TX/RX Channel
 * Functionality provided:
 * - Channel is enabled in hardware. Data transfer can occur on this
 *     channel after this.
 *
 * 1. It is assumed that the channel is already "initialized"
 * 2. To enable a channel after its disabled, it needs to be initialized again
 */
static int emac_enable_channel(struct emac_dev *dev, u32 channel, u32 direction)
{
	LOGMSG(EMAC_DEBUG_FUNCTION_ENTRY, "ChannelNo=%d, Direction=%s",
			channel, ((direction == NET_CH_DIR_TX) ? "TX" : "RX"));

	if (direction == NET_CH_DIR_TX) {
		emac_tx_cppi_ch *tx_cppi;

		tx_cppi = dev->tx_cppi[channel];
		if (tx_cppi == NULL) {
			LOGERR("Invalid Channel %d. TX CPPI structure NULL",
					channel);

			return (EMAC_ERR_TX_CH_INVALID);
		}

		/* init head descriptor pointer */
		dev->regs->tx_HDP[channel] = 0;

		/* mark channel open */
		dev->tx_is_open[channel] = TRUE;
		tx_cppi->ch_info.ch_state = NET_CH_OPENED;
	}

	else if (direction == NET_CH_DIR_RX) {
		emac_rx_cppi_ch *rx_cppi;

		rx_cppi = dev->rx_cppi[channel];
		if (rx_cppi == NULL) {
			LOGERR("Invalid Channel %d. RX CPPI structure NULL",
					channel);

			return (EMAC_ERR_RX_CH_INVALID);
		}

		/* set interface MAC address */
		emac_setmac(dev, channel, rx_cppi->mac_addr);

		/* enable channel interrupt */
		dev->regs->rx_int_mask_set = (1 << channel);
		/* disable channel interrupt */
		dev->regs->tx_int_mask_clear = (1 << channel);
		if (!dev->init_cfg.registers_old) {
			/* update wrapper book-keeping fields */
			dev->tx_en &= (~(1 << channel));
			((volatile struct ewrap_dm646x_regs)
			 dev->ewrap_regs->wrapper_646x_regs).
				C0_TXINTEN = dev->tx_en;
		}
		dev->tx_interrupt_disable = TRUE;
		dev->tx_int_threshold[channel] =
			dev->tx_cppi[channel]->ch_info.service_max;

		if (!dev->init_cfg.registers_old) {
		/* update wrapper book-keeping fields */
			dev->rx_en = (1 << channel);
			((volatile struct ewrap_dm646x_regs)dev->
				ewrap_regs->wrapper_646x_regs).
				C0_RXINTEN = (1 << channel);
		}

		/* mark queue active */
		rx_cppi->queue_active = TRUE;

		/* enable DMA */
		dev->regs->rx_HDP[channel] =
			EMAC_VIRT_TO_PHYS(rx_cppi->active_queue_head);

		/* mark channel open */
		dev->rx_is_open[channel] = TRUE;

		rx_cppi->ch_info.ch_state = NET_CH_OPENED;

	}

	LOGMSG(EMAC_DEBUG_FUNCTION_EXIT, "ChannelNo=%d, Direction=%s",
			channel, ((direction == NET_CH_DIR_TX) ? "TX" : "RX"));

	return (EMAC_SUCCESS);

}

/**
 * Disable TX/RX Channel
 * Functionality provided:
 * - Channel is disabled in hardware. No data transfer can occur on
 *    this channel after this.
 *
 * 1. It is assumed that the channel number passed is valid
 * 2. Resources for the channel will be released only when its closed
 */
static int emac_disable_channel(struct emac_dev *dev, u32 channel,
				net_ch_dir direction)
{
	LOGMSG(EMAC_DEBUG_FUNCTION_ENTRY, "ChannelNo=%d, Direction=%s",
			channel, ((direction == NET_CH_DIR_TX) ? "TX" : "RX"));

	if (direction == NET_CH_DIR_TX) {

		/* set the TX teardown pending flag */
		dev->tx_teardown_pending[channel] = TRUE;

		/* initiate teardown of TX channel */
		dev->regs->tx_teardown = channel;

		/* wait for teardown complete */
		if (emac_wait_for_teardown_complete(dev, channel, direction,
					TRUE) != EMAC_SUCCESS) {

			LOGERR("Failed to teardown TX channel %d", channel);

			/* instead of quitting on error immediately,
			 * we continue so as to cleanup the channel */
		}

		dev->tx_teardown_pending[channel] = FALSE;

		/* disable interrupt */
		dev->regs->tx_int_mask_clear = (1 << channel);
		if (!dev->init_cfg.registers_old) {
		/* update wrapper book-keeping fields */
			dev->tx_en &= (~(1 << channel));
			((volatile struct ewrap_dm646x_regs)dev->
				ewrap_regs->wrapper_646x_regs).
				C0_TXINTEN = dev->tx_en;
		}

		/* disable DMA */

		/* mark channel closed */
		dev->tx_is_open[channel] = FALSE;
	}

	else if (direction == NET_CH_DIR_RX) {
		dev->rx_teardown_pending[channel] = TRUE;

		/* initiate teardown of TX channel */
		dev->regs->rx_teardown = channel;

		/* wait for teardown complete */
		if (emac_wait_for_teardown_complete(dev, channel, direction,
					TRUE) != EMAC_SUCCESS) {
			LOGERR("Failed to teardown RX channel %d", channel);
		}
		dev->rx_teardown_pending[channel] = FALSE;

		/* disable interrupt */
		dev->regs->rx_int_mask_clear = (1 << channel);

		/* mark channel closed */
		dev->rx_is_open[channel] = FALSE;
	}

	LOGMSG(EMAC_DEBUG_FUNCTION_EXIT, "ChannelNo=%d, Direction=%s",
			channel, ((direction == NET_CH_DIR_TX) ? "TX" : "RX"));

	return (EMAC_SUCCESS);
}

/**
 * Wait for Teardown Complete
 *  - This function waits (blocking mode) for teardown completion.
 *  - blocking = TRUE ( waits on OS timer wait untill teardown complete),
 *             = FALSE (returns immediately) - NOT SUPPORTED
 * As of now this function supports blocking mode in polled mode only
 */
static int emac_wait_for_teardown_complete(struct emac_dev *dev, u32 channel,
		net_ch_dir direction, bool blocking)
{
	volatile unsigned int teardown_cnt = 0xFFFFFFF0;

	if (direction == NET_CH_DIR_TX) {
		emac_tx_bd *curr_bd;
		emac_tx_cppi_ch *tx_cppi;

		LOGMSG(EMAC_DEBUG_FUNCTION_ENTRY, "ChannelNo=%d, Direction=%s",
				channel,
				((direction == NET_CH_DIR_TX) ? "TX" : "RX"));

		while ((dev->regs->tx_CP[channel] & EMAC_TEARDOWN_VALUE)
				!= EMAC_TEARDOWN_VALUE) {
			/* wait here for tx teardown completion
			 * interrupt to occur */

			/* A task delay can be called here to pend
			 * rather than occupying CPU cycles - anyway
			 * it has been found that the teardown takes
			 * very few cpu cycles and does not affect
			 * functionality
			 */
			--teardown_cnt;
			if (teardown_cnt) {
				printk("Tx teardown aborted\n");
				break;
			}
		}

		/* write to the completion pointer */
		dev->regs->tx_CP[channel] = EMAC_TEARDOWN_VALUE;

		/* TX teardown complete - process sent packets and
		 * return sent packets to DDA */
		tx_cppi = dev->tx_cppi[channel];
		if (tx_cppi->queue_active == TRUE) {
			curr_bd = tx_cppi->active_queue_head;
			while (curr_bd != NULL) {
				emac_net_tx_complete(dev, &(curr_bd->buf_token),
						1, channel);

				if (curr_bd != tx_cppi->active_queue_tail) {
					curr_bd = curr_bd->next;
				} else {
					break;
				}
			}
			tx_cppi->bd_pool_head = tx_cppi->active_queue_head;
			tx_cppi->active_queue_head = 0;
			tx_cppi->active_queue_tail = 0;
		}

		/* At this stage all TX BD's are available linked with
		 * "bdPoolHead" and can be freed */
		LOGMSG(EMAC_DEBUG_FUNCTION_EXIT, "ChannelNo=%d, Direction=%s",
				channel,
				((direction == NET_CH_DIR_TX) ? "TX" : "RX"));
	} else if (direction == NET_CH_DIR_RX) {
		LOGMSG(EMAC_DEBUG_FUNCTION_ENTRY, "ChannelNo=%d, Direction=%s",
				channel,
				((direction == NET_CH_DIR_TX) ? "TX" : "RX"));

		while ((dev->regs->rx_CP[channel] & EMAC_TEARDOWN_VALUE)
				!= EMAC_TEARDOWN_VALUE) {

			/* wait here for rx teardown completion
			 * interrupt to occur */

			/* A task delay can be called here to pend
			 * rather than occupying CPU cycles - anyway
			 * it has been found that the teardown takes
			 * very few cpu cycles and does not affect
			 * functionality
			 */
			--teardown_cnt;
			if (teardown_cnt) {
				printk("Rx teardown aborted\n");
				break;
			}
		}

		/* write to the completion pointer */
		dev->regs->rx_CP[channel] = EMAC_TEARDOWN_VALUE;

		/* At this stage all TX BD's are available linked with
		 * "activeQueueHead" and can be freed */
		LOGMSG(EMAC_DEBUG_FUNCTION_EXIT, "ChannelNo=%d, Direction=%s",
				channel,
				((direction == NET_CH_DIR_TX) ? "TX" : "RX"));
	}

	return (EMAC_SUCCESS);
}

static void emac_ddcphycnt(struct emac_dev *dev, u32 *cmd_arg)
{
	int result;
	emac_hw_statistics stats;
	struct mib2_phy_counters *mib2phy_counters =
		(struct mib2_phy_counters *)cmd_arg;

	result = emac_control(dev, EMAC_IOCTL_GET_STATISTICS, (u32 *)&stats,
			NULL);

	if (result != 0) {
		LOGERR("Error from ioctl for EMAC_IOCTL_GET_STATISTICS \n");
		return;
	}

	mib2phy_counters->eth_alignment_errors = stats.if_in_align_code_errors;
	mib2phy_counters->eth_fcserrors = stats.if_in_crcerrors;
	mib2phy_counters->eth_single_collisions =
		stats.if_single_collision_frames;
	mib2phy_counters->eth_multiple_collisions =
		stats.if_multiple_collision_frames;
	mib2phy_counters->eth_sqetest_errors = 0;
	mib2phy_counters->eth_deferred_tx_frames =
		stats.if_deferred_transmissions;
	mib2phy_counters->eth_late_collisions = stats.if_late_collisions;
	mib2phy_counters->eth_excessive_collisions =
		stats.if_excessive_collision_frames;
	mib2phy_counters->eth_internal_mac_tx_errors = 0;
	mib2phy_counters->eth_carrier_sense_errors =
		stats.if_carrier_sense_errors;
	mib2phy_counters->eth_too_long_rx_frames =
		stats.if_in_oversized_frames;
	mib2phy_counters->eth_internal_mac_rx_errors = 0;
	mib2phy_counters->eth_symbol_errors = 0;

	return;
}

static void emac_ddcifcnt_clear(struct emac_dev *dev)
{
	memzero((char *)&dev->mib2if_hccounter, sizeof(dev->mib2if_hccounter));
}

static void emac_ddcifcnt_updt(struct emac_dev *dev)
{
	int result;
	emac_hw_statistics stats;

	result = emac_control(dev, EMAC_IOCTL_GET_STATISTICS, (u32 *)&stats,
			NULL);

	if (result != 0) {
		LOGERR("Error from ioctl for DDC EMAC_IOCTL_GET_STATISTICS \n");
		return;
	}

	if (stats.if_in_octets >= dev->mib2if_hccounter.in_bytes)
		dev->mib2if_hccounter.in_bytes_hc +=
			(stats.if_in_octets - dev->mib2if_hccounter.in_bytes);
	else
		dev->mib2if_hccounter.in_bytes_hc += 0xffffffff
			- (dev->mib2if_hccounter.in_bytes - stats.if_in_octets);

	dev->mib2if_hccounter.in_bytes = stats.if_in_octets;
	if (stats.if_in_good_frames >= dev->mib2if_hccounter.in_multicast_pkts
			+ dev->mib2if_hccounter.in_broadcast_pkts
			+ dev->mib2if_hccounter.in_unicast_pkts)
		dev->mib2if_hccounter.in_unicast_pkts_hc +=
			((stats.if_in_good_frames - (stats.if_in_broadcasts
						     + stats.if_in_multicasts))
			 - dev->mib2if_hccounter.in_unicast_pkts);
	else
		dev->mib2if_hccounter.in_unicast_pkts_hc += 0xffffffff
			- (dev->mib2if_hccounter.in_unicast_pkts
				- (stats.if_in_good_frames
					- (stats.if_in_broadcasts
						+ stats.if_in_multicasts)));

	dev->mib2if_hccounter.in_unicast_pkts = (stats.if_in_good_frames
			- (stats.if_in_broadcasts + stats.if_in_multicasts));
	if (stats.if_in_multicasts >= dev->mib2if_hccounter.in_multicast_pkts)
		dev->mib2if_hccounter.in_multicast_pkts_hc +=
			(stats.if_in_multicasts
			 - dev->mib2if_hccounter.in_multicast_pkts);
	else
		dev->mib2if_hccounter.in_multicast_pkts_hc += 0xffffffff -
			(dev->mib2if_hccounter.in_multicast_pkts
			 - stats.if_in_multicasts);

	dev->mib2if_hccounter.in_multicast_pkts = stats.if_in_multicasts;
	if (stats.if_in_broadcasts >= dev->mib2if_hccounter.in_broadcast_pkts)
		dev->mib2if_hccounter.in_broadcast_pkts_hc +=
			(stats.if_in_broadcasts
			 - dev->mib2if_hccounter.in_broadcast_pkts);

	else
		dev->mib2if_hccounter.in_broadcast_pkts_hc += 0xffffffff -
			(dev->mib2if_hccounter.in_broadcast_pkts
			 - stats.if_in_broadcasts);

	dev->mib2if_hccounter.in_broadcast_pkts = stats.if_in_broadcasts;
	if (stats.if_out_octets >= dev->mib2if_hccounter.out_bytes)
		dev->mib2if_hccounter.out_bytes_hc +=
			(stats.if_out_octets - dev->mib2if_hccounter.out_bytes);
	else
		dev->mib2if_hccounter.out_bytes_hc += 0xffffffff -
			(dev->mib2if_hccounter.out_bytes - stats.if_out_octets);

	dev->mib2if_hccounter.out_bytes = stats.if_out_octets;
	if (stats.if_out_good_frames
			>= dev->mib2if_hccounter.out_multicast_pkts
			+ dev->mib2if_hccounter.out_broadcast_pkts
			+ dev->mib2if_hccounter.out_unicast_pkts)
		dev->mib2if_hccounter.out_unicast_pkts_hc +=
			((stats.if_out_good_frames
			  - (stats.if_out_broadcasts
				  + stats.if_out_multicasts))
			 - dev->mib2if_hccounter.out_unicast_pkts);

	else
		dev->mib2if_hccounter.out_unicast_pkts_hc += 0xffffffff -
			(dev->mib2if_hccounter.out_unicast_pkts
			 - (stats.if_out_good_frames
				 - (stats.if_out_broadcasts
					 + stats.if_out_multicasts)));

	dev->mib2if_hccounter.out_unicast_pkts = (stats.if_out_good_frames
			- (stats.if_out_broadcasts + stats.if_out_multicasts));

	if (stats.if_out_multicasts >= dev->mib2if_hccounter.out_multicast_pkts)
		dev->mib2if_hccounter.out_multicast_pkts_hc +=
			(stats.if_out_multicasts
			 - dev->mib2if_hccounter.out_multicast_pkts);
	else
		dev->mib2if_hccounter.out_multicast_pkts_hc += 0xffffffff -
			(dev->mib2if_hccounter.out_multicast_pkts
			 - stats.if_out_multicasts);

	dev->mib2if_hccounter.out_multicast_pkts = stats.if_out_multicasts;
	if (stats.if_out_broadcasts >= dev->mib2if_hccounter.out_broadcast_pkts)
		dev->mib2if_hccounter.out_broadcast_pkts_hc +=
			(stats.if_out_broadcasts
			 - dev->mib2if_hccounter.out_broadcast_pkts);
	else
		dev->mib2if_hccounter.out_broadcast_pkts_hc += 0xffffffff -
			(dev->mib2if_hccounter.out_broadcast_pkts
			 - stats.if_out_broadcasts);
	dev->mib2if_hccounter.out_broadcast_pkts = stats.if_out_broadcasts;

	/* low 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.in_bytes_low =
		(unsigned long)dev->mib2if_hccounter.in_bytes_hc;

	/* high 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.in_bytes_high =
		(dev->mib2if_hccounter.in_bytes_hc >> 32);

	/* low 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.in_unicast_pkts_low =
		(unsigned long)dev->mib2if_hccounter.in_unicast_pkts_hc;

	/* high 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.in_unicast_pkts_high =
		(dev->mib2if_hccounter.in_unicast_pkts_hc >> 32);

	/* low 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.in_multicast_pkts_low =
		(unsigned long)dev->mib2if_hccounter.in_multicast_pkts_hc;

	/* high 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.in_multicast_pkts_high =
		dev->mib2if_hccounter.in_multicast_pkts_hc >> 32;

	/* low 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.in_broadcast_pkts_low =
		(unsigned long)dev->mib2if_hccounter.in_broadcast_pkts_hc;

	/* high 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.in_broadcast_pkts_high =
		dev->mib2if_hccounter.in_broadcast_pkts_hc >> 32;

	/* packets discarded due to resource limit */
	dev->mib2if_hccounter.mib2if_counter.in_discard_pkts =
		stats.if_rx_dmaoverruns + stats.if_rx_mof_overruns
		+ stats.if_rx_sof_overruns + stats.if_in_crcerrors
		+ stats.if_in_align_code_errors + stats.if_in_jabber_frames
		+ stats.if_in_fragments + stats.if_in_oversized_frames
		+ stats.if_in_undersized_frames + stats.if_in_filtered_frames
		+ stats.if_in_qos_filtered_frames;

	/* packets discarded due to format errors */
	dev->mib2if_hccounter.mib2if_counter.in_error_pkts =
		stats.if_in_crcerrors + stats.if_in_align_code_errors
		+ stats.if_in_jabber_frames + stats.if_in_fragments;

	dev->mib2if_hccounter.mib2if_counter.in_unknown_prot_pkts = 0;

	/* low 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.out_bytes_low =
		(unsigned long)dev->mib2if_hccounter.out_bytes_hc;

	/* high 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.out_bytes_high =
		dev->mib2if_hccounter.out_bytes_hc >> 32;

	/* low 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.out_unicast_pkts_low =
		(unsigned long)dev->mib2if_hccounter.out_unicast_pkts_hc;

	/* high 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.out_unicast_pkts_high =
		dev->mib2if_hccounter.out_unicast_pkts_hc >> 32;

	/* low 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.out_multicast_pkts_low =
		(unsigned long)dev->mib2if_hccounter.out_multicast_pkts_hc;

	/* high 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.out_multicast_pkts_high =
		dev->mib2if_hccounter.out_multicast_pkts_hc >> 32;

	/* low 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.out_broadcast_pkts_low =
		(unsigned long)dev->mib2if_hccounter.out_broadcast_pkts_hc;

	/* high 32-bit of total octets received from media */
	dev->mib2if_hccounter.mib2if_counter.out_broadcast_pkts_high =
		dev->mib2if_hccounter.out_broadcast_pkts_hc >> 32;

	/* packets discarded due to format errors */
	dev->mib2if_hccounter.mib2if_counter.out_error_pkts =
		(stats.if_excessive_collision_frames
		 + stats.if_late_collisions + stats.if_carrier_sense_errors);

	/* packets discarded due to resource limit */
	dev->mib2if_hccounter.mib2if_counter.out_discard_pkts =
		stats.if_out_underrun
		+ dev->mib2if_hccounter.mib2if_counter.out_error_pkts;

	return;
}

#define emac_min_val(a,b) ((a > b) ? b : a)

#ifdef EMAC_DEBUG		/* used only for debug printing */
/* static global strings */
static char *emac_tx_host_error_codes[16] = {
	/* 0000 */ "No error",
	/* 0001 */ "SOP error",
	/* 0010 */ "Ownership bit not set in SOP buffer",
	/* 0011 */ "Zero Next Buffer Descriptor Pointer Without EOP",
	/* 0100 */ "Zero Buffer Pointer",
	/* 0101 */ "Zero Buffer Length",
	/* 0110 */ "Packet Length Error",
	/* 0111 */ "Reserved",
	/* 1000 */ "Reserved",
	/* 1001 */ "Reserved",
	/* 1010 */ "Reserved",
	/* 1011 */ "Reserved",
	/* 1100 */ "Reserved",
	/* 1101 */ "Reserved",
	/* 1110 */ "Reserved",
	/* 1111 */ "Reserved"
};

static char *emac_rx_host_error_codes[16] = {
	/* 0000 */ "No error",
	/* 0001 */ "Reserved",
	/* 0010 */ "Ownership bit not set in input buffer",
	/* 0011 */ "Reserved",
	/* 0100 */ "Zero Buffer Pointer",
	/* 0101 */ "Reserved",
	/* 0110 */ "Reserved",
	/* 0111 */ "Reserved",
	/* 1000 */ "Reserved",
	/* 1001 */ "Reserved",
	/* 1010 */ "Reserved",
	/* 1011 */ "Reserved",
	/* 1100 */ "Reserved",
	/* 1101 */ "Reserved",
	/* 1110 */ "Reserved",
	/* 1111 */ "Reserved"
};

#endif	/* EMAC_DEBUG */

/**
 * EMAC DDC Packet processing function
 *  - Detects if there are host errors and invokes the DDA callback to inform
 *    the DDA layer about the hardware error.
 *
 */
static void emac_process_host_error(struct emac_dev *dev)
{
	u32 channel = 0;
	u32 vector = 0;
	u32 status = 0;

	/* the mac_status register bits starting from rx error channel
	 * have been mapped to hw_err_info LSB 16 bits */
	status = dev->regs->mac_status;

	/* TX: reading the channel and cause */
	channel = ((status & EMAC_MACSTATUS_TXERRCH_MASK)
			>> EMAC_MACSTATUS_TXERRCH_SHIFT);

	dev->status.hw_err_info = channel << 16;

	vector = (status & EMAC_MACSTATUS_TXERRCODE_MASK)
		>> EMAC_MACSTATUS_TXERRCODE_SHIFT;

	if (vector) {
		dev->status.hw_status = EMAC_TX_HOST_ERROR;
		LOGERR("Ch=%d, EMAC_TX_HOST_ERROR. Cause=%s",
				dev->status.hw_err_info,
				&emac_tx_host_error_codes[vector][0]);
	}

	/* RX: reading the channel and cause (vector variable being
	 * re-used) */
	channel = ((status & EMAC_MACSTATUS_RXERRCH_MASK)
			>> EMAC_MACSTATUS_RXERRCH_SHIFT);

	dev->status.hw_err_info |= channel;
	vector = (status & EMAC_MACSTATUS_RXERRCODE_MASK)
		>> EMAC_MACSTATUS_RXERRCODE_SHIFT;
	if (vector) {
		dev->status.hw_status = EMAC_RX_HOST_ERROR;
		LOGERR("Ch=%d, EMAC_RX_HOST_ERROR. Cause=%s",
				dev->status.hw_err_info,
				&emac_rx_host_error_codes[vector][0]);
	}

	/* inform DDA layer about this critical failure */
	emac_status_update(dev, &dev->status);
}

/**
 * EMAC DDC Packet processing function

 * - Reads the device interrupt status and invokes TX/RX BD processing
 *   function
 * - Also detects if there are host errors and invokes the
 *   callback to inform  about the hardware error.
 *
 * "pkts_pending" will contain number of packets still to be processed
 * (TX + RX)
 */
static int emac_pkt_process(struct emac_dev *dev, int *pkts_pending,
		void *pkt_args)
{
	u32 channel = 0;
	u32 vector = 0;
	u32 handle_pkts_and_status = 0;
	u32 vector_channel = 0;
	int pkts_processed = 0;
	u32 mac_in_vector_tx_int_vec;
	u32 mac_in_vector_tx_int_shift;
	u32 mac_in_vector_rx_int_vec;
	u32 mac_in_vector_rx_int_shift;
	u32 mac_in_vector_host_int_vec;

	if (!dev->init_cfg.registers_old) {
		mac_in_vector_tx_int_vec = EMAC_DM646X_MAC_IN_VECTOR_TX_INT_VEC;
		mac_in_vector_tx_int_shift =
			EMAC_DM646X_MAC_IN_VECTOR_TX_INT_SHIFT;
		mac_in_vector_rx_int_vec = EMAC_DM646X_MAC_IN_VECTOR_RX_INT_VEC;
		mac_in_vector_host_int_vec = EMAC_DM646X_MAC_IN_VECTOR_HOST_INT;
		mac_in_vector_rx_int_shift =
			EMAC_DM646X_MAC_IN_VECTOR_RX_INT_SHIFT;
	} else {
		mac_in_vector_tx_int_vec = EMAC_MAC_IN_VECTOR_TX_INT_VEC;
		mac_in_vector_tx_int_shift = 0;
		mac_in_vector_rx_int_vec = EMAC_MAC_IN_VECTOR_RX_INT_VEC;
		mac_in_vector_rx_int_shift =
			EMAC_MAC_IN_VECTOR_RX_INT_SHIFT;
		mac_in_vector_host_int_vec = EMAC_MAC_IN_VECTOR_HOST_INT;
	}

	/* disable interrupts via module control (wrapper) */
	emac_disable_interrupt(dev);
	vector = dev->regs->mac_in_vector;

	/* handle packet transmit completion */
	if (vector & mac_in_vector_tx_int_vec) {
		bool is_eoq;

		vector_channel = ((vector & mac_in_vector_tx_int_vec)
				>> mac_in_vector_tx_int_shift);

		for (channel = 0; channel < 8; channel++) {
			if (vector_channel & 0x1)
				break;

			vector_channel >>= 1;
		}

		handle_pkts_and_status =
			dev->tx_cppi[channel]->ch_info.service_max;
		if (pkt_args)
			handle_pkts_and_status =
				emac_min_val(((rx_tx_params *)pkt_args)
						->tx_pkts,
						handle_pkts_and_status);

		pkts_processed = emac_tx_bdproc(dev, channel,
				&handle_pkts_and_status, &is_eoq);
		if (pkt_args)
			((rx_tx_params *) pkt_args)->ret_tx_pkts =
				pkts_processed;

		if (dev->tx_interrupt_disable == TRUE) {
			/* status */
			if (!handle_pkts_and_status && is_eoq)
				/* disable channel interrupt */
				dev->regs->tx_int_mask_clear = (1 << channel);
				if (!dev->init_cfg.registers_old) {
					/* update wrapper book-keeping fields */
					dev->tx_en &= (~(1 << channel));
					((volatile struct ewrap_dm646x_regs)dev->
						ewrap_regs->wrapper_646x_regs).
							C0_TXINTEN = dev->tx_en;
					}
		}
		*pkts_pending = handle_pkts_and_status;	/* status. */
	}

	/* Handle RX packets first - the thought process in this is
	 * that the received packets will be handled immediately
	 * reducing the latency (- but an equally opposite argument
	 * can also be made)
	 */
	if (vector & mac_in_vector_rx_int_vec) {
	vector_channel = ((vector & mac_in_vector_rx_int_vec) >>
				mac_in_vector_rx_int_shift);
		for (channel = 0; channel < 8; channel++) {
			if (vector_channel & 0x1)
				break;
			vector_channel >>= 1;
		}

		handle_pkts_and_status =
			dev->rx_cppi[channel]->ch_info.service_max;
		if (pkt_args)
			handle_pkts_and_status =
				emac_min_val(((rx_tx_params *)pkt_args)
						->rx_pkts,
						handle_pkts_and_status);

		pkts_processed = emac_rx_bdproc(dev, channel,
				&handle_pkts_and_status);

		if (pkt_args)
			((rx_tx_params *) pkt_args)->ret_rx_pkts =
				pkts_processed;

		*pkts_pending |= handle_pkts_and_status;	/* status */
	}

	/* handle host errors - being handled last does not mean its
	 * of least priority */
	if (vector & mac_in_vector_host_int_vec)
		emac_process_host_error(dev);

	return (EMAC_SUCCESS);
}

/**
 * EMAC DDC Signal Packet processing end to hardware
 * - programs the EOI vector register so that if there are pending
 *   packets in hardware queue an interrupt can be generated by the
 *   hardware
 */
/* packet processing end */
static int emac_pkt_process_end(struct emac_dev *dev, void *proc_args)
{
	/* enable interrupts via module control (wrapper) */
	emac_enable_interrupt(dev, 1); /*TODO: ack_eoi=0,1?*/

	return (EMAC_SUCCESS);
}

#ifdef EMAC_MULTIFRAGMENT

#error "EMAC Multi fragment Not supported"

#else

/*************************************
 * SINGLE-FRAGMENT SUPPORT HERE
 *************************************/

/**
 * EMAC DDC Send/Transmit function
 *  - Queues the packet provided by DDA into hardware queue
 *  - If the queue is stalled due to sync issues, re-trigger the hardware
 *
 * If "sendArgs" is TRUE (non zero) CRC is calculated by DDA or upper
 * layer and not by hardware and is part of the packet data send to
 * this function
 */

static int emac_send(struct emac_dev *dev, net_pkt_obj *pkt,
		int channel, bool send_args)
{
	unsigned long flags;
	int ret_val = EMAC_SUCCESS;
	emac_tx_bd *curr_bd;
	emac_tx_cppi_ch *tx_cppi;
	net_buf_obj *buf_list;

	/* verify proper device state */
	if (dev->drv_state != DRV_OPENED)
		return (EMAC_ERR_DEV_NOT_OPEN);

	/* validate channel number and get channel control structure */
	if (channel > EMAC_MAX_TXRX_CHANNELS)
		return (EMAC_ERR_TX_CH_INVALID);

	if (dev->tx_is_open[channel] != TRUE)
		return (EMAC_ERR_TX_CH_NOT_OPEN);

	/* check ethernet link state. if not linked, return error */
	if (!dev->status.phy_linked)
		return (EMAC_ERR_TX_NO_LINK);

	tx_cppi = dev->tx_cppi[channel];
	buf_list = pkt->buf_list;	/* get handle to the buffer array */

	/* check packet size and if < EMAC_MIN_ETHERNET_PKT_SIZE, pad it up */
	if (pkt->pkt_length < EMAC_MIN_ETHERNET_PKT_SIZE) {
		buf_list->length +=
			(EMAC_MIN_ETHERNET_PKT_SIZE - pkt->pkt_length);
		pkt->pkt_length = EMAC_MIN_ETHERNET_PKT_SIZE;
	}
	spin_lock_irqsave(&dev->tx_lock, flags);

	/* only one tx BD for the packet to be sent */
	curr_bd = tx_cppi->bd_pool_head;
	if (curr_bd == NULL) {
#ifdef EMAC_GETSTATS
		tx_cppi->out_of_tx_bd++;
#endif
		ret_val = EMAC_ERR_TX_OUT_OF_BD;
		goto exit_emac_send;
	}

	tx_cppi->bd_pool_head = curr_bd->next;

	/* populate the BD contents to be added to the TX list */
	curr_bd->buf_token = buf_list->buf_token;
	curr_bd->buff_ptr = EMAC_VIRT_TO_PHYS((int *)buf_list->data_ptr);
	curr_bd->off_b_len = buf_list->length;
	curr_bd->h_next = 0;
	curr_bd->next = 0;
	curr_bd->mode =
		(EMAC_CPPI_SOP_BIT | EMAC_CPPI_OWNERSHIP_BIT
		 | EMAC_CPPI_EOP_BIT | pkt->pkt_length);

	if ((bool) send_args == TRUE)
		curr_bd->mode |= EMAC_CPPI_PASS_CRC_BIT;

	/* flush the packet from cache if write back cache is present */
	BD_CACHE_WRITEBACK_INVALIDATE(curr_bd, EMAC_BD_LENGTH_FOR_CACHE);

	/* send the packet */
	if (tx_cppi->active_queue_head == 0) {
		tx_cppi->active_queue_head = curr_bd;
		tx_cppi->active_queue_tail = curr_bd;
		if (tx_cppi->queue_active != TRUE) {
			dev->regs->tx_HDP[channel] = EMAC_VIRT_TO_PHYS(curr_bd);
			tx_cppi->queue_active = TRUE;
		}
#ifdef EMAC_GETSTATS
		++tx_cppi->queue_reinit;
#endif
	} else {
		register volatile emac_tx_bd *tail_bd;
		register u32 frame_status;

		tail_bd = tx_cppi->active_queue_tail;
		tail_bd->next = curr_bd;
		tx_cppi->active_queue_tail = curr_bd;
		tail_bd = EMAC_VIRT_NOCACHE(tail_bd);
		tail_bd->h_next = (int)EMAC_VIRT_TO_PHYS(curr_bd);
		frame_status = tail_bd->mode;
		if (frame_status & EMAC_CPPI_EOQ_BIT) {
			dev->regs->tx_HDP[channel] = EMAC_VIRT_TO_PHYS(curr_bd);
			frame_status &= ~(EMAC_CPPI_EOQ_BIT);
			tail_bd->mode = frame_status;
#ifdef EMAC_GETSTATS
			++tx_cppi->end_of_queue_add;
#endif
		} else {
			if (dev->tx_interrupt_disable == TRUE) {
				/* enable channel interrupt */
				dev->regs->tx_int_mask_set = (1 << channel);
				if (!dev->init_cfg.registers_old) {
					volatile struct ewrap_dm646x_regs *dm646x_wrap;
					dm646x_wrap = &dev->ewrap_regs->
					wrapper_646x_regs;
					/* update wrapper book-keeping fields*/
					dev->tx_en = (1 << channel);
					dm646x_wrap->C0_TXINTEN =
						(1 << channel);
				}
			}
		}
	}
#ifdef EMAC_GETSTATS
	tx_cppi->active_queue_count++;
#endif

exit_emac_send:
	spin_unlock_irqrestore(&dev->tx_lock, flags);

	if (dev->tx_interrupt_disable == TRUE) {
		if (--dev->tx_int_threshold[channel] <= 0) {
			bool is_eoq;
			u32 handle_pkts_and_status;

			handle_pkts_and_status =
				dev->tx_cppi[channel]->ch_info.service_max;
			emac_tx_bdproc(dev, channel, &handle_pkts_and_status,
					&is_eoq);
			dev->tx_int_threshold[channel] =
				dev->tx_cppi[channel]->ch_info.service_max;
		}
	}

	return (ret_val);
}

/**
 * EMAC DDC TX Buffer Descriptor processing
 *  - processes transmit completed packets and returns the handles to DDA layer
 *  - If the queue is stalled due to sync issues, re-trigger the hardware
 *
 * returns number of pkts processed and 1 in morePkts if pkt
 * completion processing pending
 */
static int emac_tx_bdproc(struct emac_dev *dev, u32 channel,
		u32 *handle_pkts_and_status, bool *is_eoq)
{
	unsigned long flags;
	emac_tx_bd *curr_bd;
	emac_tx_cppi_ch *tx_cppi;
	volatile u32 frame_status;
	u32 pkts_processed = 0;
	u32 pkts_to_process = *handle_pkts_and_status;
#ifdef EMAC_MULTIPACKET_TX_COMPLETE_NOTIFY
	u32 tx_complete_cnt = 0;
	u32 *tx_complete_ptr;
#endif

	*handle_pkts_and_status = 0;	/* status. */
	*is_eoq = TRUE;

	/* Here no need to validate channel number, since it is taken
	   from the interrupt register instead channel structure
	   should be validated */
	if (dev->tx_is_open[channel] == FALSE)
		return (EMAC_ERR_TX_CH_NOT_OPEN);

	if (dev->tx_teardown_pending[channel] == TRUE) {
		return (EMAC_SUCCESS);	/* dont handle any pkt completions */
	}

	tx_cppi = dev->tx_cppi[channel];
#ifdef EMAC_MULTIPACKET_TX_COMPLETE_NOTIFY
	tx_complete_ptr = &tx_cppi->tx_complete[0];
#endif
#ifdef EMAC_GETSTATS
	++tx_cppi->proc_count;
#endif
	spin_lock_irqsave(&dev->tx_lock, flags);

	/* get first BD to process */
	curr_bd = tx_cppi->active_queue_head;
	if (curr_bd == 0) {
		dev->regs->tx_CP[channel] =
			EMAC_VIRT_TO_PHYS(tx_cppi->last_hw_bdprocessed);
#ifdef EMAC_GETSTATS
		tx_cppi->no_active_pkts++;
#endif
		spin_unlock_irqrestore(&dev->tx_lock, flags);

		return (EMAC_SUCCESS);
	}

	/* invalidate BD */
	BD_CACHE_INVALIDATE(curr_bd, EMAC_BD_LENGTH_FOR_CACHE);

	frame_status = curr_bd->mode;
	while ((curr_bd) && ((frame_status & EMAC_CPPI_OWNERSHIP_BIT) == 0)
			&& (pkts_processed < pkts_to_process)) {

		dev->regs->tx_CP[channel] = EMAC_VIRT_TO_PHYS(curr_bd);
		tx_cppi->active_queue_head = curr_bd->next;
		if (frame_status & EMAC_CPPI_EOQ_BIT) {
			if (curr_bd->next) {	/* misqueued packet */
				dev->regs->tx_HDP[channel] = curr_bd->h_next;
#ifdef EMAC_GETSTATS
				++tx_cppi->mis_queued_packets;
#endif
			} else {	/* end of queue */
				tx_cppi->queue_active = FALSE;
			}
		}
#ifdef EMAC_MULTIPACKET_TX_COMPLETE_NOTIFY
		*tx_complete_ptr = (u32) curr_bd->buf_token;
		++tx_complete_ptr;
		++tx_complete_cnt;
#else
		/* single packet TX complete notify - this function is
		 * called in the send critical section context */
		emac_net_tx_complete(dev, &curr_bd->buf_token, 1,
				(void *)channel);
#endif
		curr_bd->next = tx_cppi->bd_pool_head;
		tx_cppi->bd_pool_head = curr_bd;
#ifdef EMAC_GETSTATS
		--tx_cppi->active_queue_count;
#endif
		pkts_processed++;
		tx_cppi->last_hw_bdprocessed = curr_bd;
		curr_bd = tx_cppi->active_queue_head;
		if (curr_bd) {
			BD_CACHE_INVALIDATE(curr_bd, EMAC_BD_LENGTH_FOR_CACHE);
			frame_status = curr_bd->mode;
		}
	}			/* end of while loop */

	if ((curr_bd) && ((frame_status & EMAC_CPPI_OWNERSHIP_BIT) == 0)) {
		*handle_pkts_and_status = 1;
	}
	/*
	 * this check is same as check for EOQ i.e framestatus and
	 * EMAC_CPPI_EOQ_BIT
	 */
	if (curr_bd)
		*is_eoq = FALSE;

#ifdef EMAC_MULTIPACKET_TX_COMPLETE_NOTIFY
	/* multiple packet TX complete notify - this function is NOT
	 * called in the send critical section context */
	emac_net_tx_complete(dev,
			(emac_net_data_token *)&tx_cppi->tx_complete[0],
			tx_complete_cnt, channel);
#endif
	spin_unlock_irqrestore(&dev->tx_lock, flags);

	return (pkts_processed);
}

/**
 * EMAC DDC Add Buffer to RX queue function
 *  - returns the BD to the Receive queue
 *  - If the queue is stalled due to sync issues, re-trigger the hardware
 */
static void emac_add_bdto_rx_queue(struct emac_dev *dev,
		emac_rx_cppi_ch *rx_cppi, emac_rx_bd *curr_bd, char *buffer,
		emac_net_data_token buf_token)
{
	/* populate the hardware descriptor */
	curr_bd->h_next = 0;
	curr_bd->buff_ptr = EMAC_VIRT_TO_PHYS(buffer);
	curr_bd->off_b_len = rx_cppi->ch_info.buf_size;
	curr_bd->mode = EMAC_CPPI_OWNERSHIP_BIT;
	curr_bd->next = 0;
	curr_bd->data_ptr = buffer;
	curr_bd->buf_token = buf_token;

	/* write back */
	BD_CACHE_WRITEBACK_INVALIDATE(curr_bd, EMAC_BD_LENGTH_FOR_CACHE);
	if (rx_cppi->active_queue_head == 0) {
		rx_cppi->active_queue_head = curr_bd;
		rx_cppi->active_queue_tail = curr_bd;
		if (rx_cppi->queue_active != FALSE) {
			dev->regs->rx_HDP[rx_cppi->ch_info.ch_num] =
				EMAC_VIRT_TO_PHYS(rx_cppi->active_queue_head);
			rx_cppi->queue_active = TRUE;
		}
	} else {
		emac_rx_bd *tail_bd;
		u32 frame_status;

		tail_bd = rx_cppi->active_queue_tail;
		rx_cppi->active_queue_tail = curr_bd;
		tail_bd->next = (void *)curr_bd;
		tail_bd = EMAC_VIRT_NOCACHE(tail_bd);
		tail_bd->h_next = EMAC_VIRT_TO_PHYS(curr_bd);
		frame_status = tail_bd->mode;
		if (frame_status & EMAC_CPPI_EOQ_BIT) {
			dev->regs->rx_HDP[rx_cppi->ch_info.ch_num] =
				EMAC_VIRT_TO_PHYS(curr_bd);
			frame_status &= ~(EMAC_CPPI_EOQ_BIT);
			tail_bd->mode = frame_status;
#ifdef EMAC_GETSTATS
			++rx_cppi->end_of_queue_add;
#endif
		}

	}

#ifdef EMAC_GETSTATS
	/* maintain statistics of how many BD's were queued back - recycled */
	++rx_cppi->recycled_bd;
#endif
}

/**
 * EMAC DDC RX Buffer Descriptor processing
 *  - processes received packets and passes them to DDA layer
 *  - requeues the buffer descriptor to the receive pool
 *  - If the queue is stalled due to sync issues, re-trigger the hardware
 */
static int emac_rx_bdproc(struct emac_dev *dev, u32 channel,
		int *handle_pkts_and_status)
{
	unsigned long flags;
	emac_rx_cppi_ch *rx_cppi;
	emac_rx_bd *curr_bd, *last_bd;
	u32 frame_status;
	char *new_buffer;
	emac_net_data_token new_buf_token;
	net_buf_obj *rx_buf_obj;
	u32 pkts_processed;
	net_pkt_obj *curr_pkt,pkt_obj;
	net_buf_obj buf_obj;
	u32 pkts_to_be_processed = *handle_pkts_and_status;

	/* Here no need to validate channel number, since it is taken
	   from the interrupt register instead channel structure
	   should be validated */
	if (dev->rx_is_open[channel] == FALSE) {
		*handle_pkts_and_status = 0;
		return (EMAC_ERR_RX_CH_NOT_OPEN);
	}

	/* check if channel teardown pending */
	rx_cppi = dev->rx_cppi[channel];
	if (dev->rx_teardown_pending[channel] == TRUE) {
		*handle_pkts_and_status = 0;
		return (0);
	}
#ifdef EMAC_GETSTATS
	++rx_cppi->proc_count;
#endif
	*handle_pkts_and_status = 0;
	pkts_processed = 0;

	spin_lock_irqsave(&dev->rx_lock, flags);

	pkt_obj.buf_list = &buf_obj;
	curr_pkt = &pkt_obj;
	curr_bd = rx_cppi->active_queue_head;
	BD_CACHE_INVALIDATE(curr_bd, EMAC_BD_LENGTH_FOR_CACHE);
	frame_status = curr_bd->mode;

	while ((curr_bd) && ((frame_status & EMAC_CPPI_OWNERSHIP_BIT) == 0)
			&& (pkts_processed < pkts_to_be_processed)) {

		/* allocate new buffer */
		new_buffer = (void *)(emac_net_alloc_rx_buf(dev,
					rx_cppi->ch_info.buf_size,
					&new_buf_token, 0, NULL));
		if (new_buffer == NULL) {
			/* no buffer available. return error with packets pending */
#ifdef EMAC_GETSTATS
			++rx_cppi->out_of_rx_buffers;
#endif
			goto end_emac_rx_bdproc;
		}

		/* populate received packet data structure */
		rx_buf_obj = &curr_pkt->buf_list[0];
		rx_buf_obj->data_ptr = (char *)curr_bd->data_ptr;
		rx_buf_obj->length = curr_bd->off_b_len & EMAC_RX_BD_BUF_SIZE;
		rx_buf_obj->buf_token = curr_bd->buf_token;
		curr_pkt->pkt_token = curr_pkt->buf_list->buf_token;
		curr_pkt->num_bufs = 1;
		curr_pkt->pkt_length =
			(frame_status & EMAC_RX_BD_PKT_LENGTH_MASK);
		/* acknowledge RX interrupt for the channel */
		dev->regs->rx_CP[channel] = EMAC_VIRT_TO_PHYS(curr_bd);
#ifdef EMAC_GETSTATS
		++rx_cppi->processed_bd;
#endif
		last_bd = curr_bd;
		curr_bd = last_bd->next;
		rx_cppi->active_queue_head = curr_bd;

		/* check if end of RX queue ? */
		if (frame_status & EMAC_CPPI_EOQ_BIT) {
			if (curr_bd) {	/* misqueued packet */
#ifdef EMAC_GETSTATS
				++rx_cppi->mis_queued_packets;
#endif
				dev->regs->rx_HDP[channel] =
					EMAC_VIRT_TO_PHYS(curr_bd);
			} else {	/* end of queue */
#ifdef EMAC_GETSTATS
				++rx_cppi->end_of_queue;
#endif
				rx_cppi->queue_active = FALSE;	/* clear software RX queue */
			}
		}

		/* recycle BD */
		emac_add_bdto_rx_queue(dev, rx_cppi, last_bd, new_buffer,
				new_buf_token);

		/* return the packet to the user - BD ptr passed in
		 * last parameter for potential *future* use */
		spin_unlock_irqrestore(&dev->rx_lock, flags);
		emac_net_rx_cb(dev,curr_pkt,(void*)channel);
		spin_lock_irqsave(&dev->rx_lock, flags);

		curr_bd = rx_cppi->active_queue_head;
		if (curr_bd) {
			BD_CACHE_INVALIDATE(curr_bd, EMAC_BD_LENGTH_FOR_CACHE);
			frame_status = curr_bd->mode;
		}
		++pkts_processed;
	}

	if ((curr_bd) && ((frame_status & EMAC_CPPI_OWNERSHIP_BIT) == 0)) {
		*handle_pkts_and_status = 1;
	}

end_emac_rx_bdproc:
	spin_unlock_irqrestore(&dev->rx_lock, flags);
	return (pkts_processed);
}

#endif	/* !EMAC_MULTIFRAGMENT */

/* Linux 2.6 Kernel Ethernet Poll function Call only RX processing in
 * the poll function - TX is taken care of in interrupt context
 */
static int emac_poll(struct net_device *netdev, int *budget)
{
	struct emac_dev *dev = netdev_priv(netdev);
	unsigned int work = min(netdev->quota, *budget);
	unsigned int pkts_pending = 0;
	/* this is used to pass the rx packets to be processed and
	 * return the number of rx packets processed */
	rx_tx_params *napi_params = &dev->napi_rx_tx;

	if (!dev->set_to_close) {
		napi_params->rx_pkts = work;
		napi_params->tx_pkts = EMAC_DEFAULT_TX_MAX_SERVICE;

		/* process packets - call the DDC packet processing function */
		emac_pkt_process(dev, &pkts_pending, napi_params);

		/* if more packets reschedule the tasklet or call
		 * pkt_process_end */
		if (!pkts_pending) {
			if (test_bit(__LINK_STATE_RX_SCHED, &netdev->state)) {
				netif_rx_complete(netdev);
			}
			emac_pkt_process_end(dev, NULL);
			return 0;
		} else if (!test_bit(0, &dev->set_to_close)) {
			*budget -= napi_params->ret_rx_pkts;
			netdev->quota -= napi_params->ret_rx_pkts;
			return 1;
		}
	}

	/* we are closing down, so dont process anything */
	return 0;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
/*
 * Polling receive - used by netconsole and other diagnostic tools
 * to allow network i/o with interrupts disabled.
 */
void emac_poll_controller(struct net_device *netdev)
{
	struct emac_dev *dev = NETDEV_PRIV(netdev);

	disable_irq(netdev->irq);
	emac_hal_isr(netdev->irq, dev, NULL);
	enable_irq(netdev->irq);
}
#endif

/* allocate RX buffer */
void *emac_net_alloc_rx_buf(struct emac_dev *dev, int buf_size,
		emac_net_data_token *data_token, u32 channel, void *alloc_args)
{
	struct net_device *netdev = dev->owner;
	struct sk_buff *p_skb;

	p_skb = dev_alloc_skb(dev->rx_buf_size);
	if (p_skb == NULL) {
#ifdef EMAC_DEBUG
		ERR("emac_net_alloc_rx_buf:Failed to allocate skb for %s.\n",
				netdev->name);
#endif
		return (NULL);
	}

	/* set device pointer in skb and reserve space for extra bytes */
	p_skb->dev = netdev;
	skb_reserve(p_skb, dev->rx_buf_offset);

	/* set the data token */
	*data_token = (emac_net_data_token) p_skb;
#ifdef EMAC_CACHE_INVALIDATE_FIX
	/* invalidate buffer */
	EMAC_CACHE_INVALIDATE((unsigned long)p_skb->data, buf_size);
#endif

	return p_skb->data;
}

/* free RX buffer */
static int emac_net_free_rx_buf(struct emac_dev *dev, void *buffer,
				emac_net_data_token data_token,
				u32 channel, void *free_args)
{
	dev_kfree_skb_any((struct sk_buff *)data_token);
	return (EMAC_SUCCESS);
}


/*
 * Packet receive notification
 *
 * This function gets received packet via the netPktList and
 * it queues the packet into the higher layer queue
 *
 * Note that rxArgs contains "channel" and is ignored for this
 * implementation
*/
static int emac_net_rx_cb(struct emac_dev *dev, net_pkt_obj *net_pkt_list,
		void *rx_args)
{

	struct sk_buff *p_skb;

	p_skb = (struct sk_buff *)net_pkt_list->pkt_token;

	/* set length of packet */
	skb_put(p_skb, net_pkt_list->pkt_length);
#ifndef EMAC_CACHE_INVALIDATE_FIX
	/* invalidate cache */
	EMAC_CACHE_INVALIDATE((unsigned long)p_skb->data, p_skb->len);
#endif
	p_skb->protocol = eth_type_trans(p_skb, dev->owner);
	p_skb->dev->last_rx = jiffies;
	netif_receive_skb(p_skb);
	dev->net_dev_stats.rx_bytes += net_pkt_list->pkt_length;
	dev->net_dev_stats.rx_packets++;

	return (0);
}


/* transmit complete callback */
static int emac_net_tx_complete(struct emac_dev *dev,
				emac_net_data_token *net_data_tokens,
				int num_tokens, u32 channel)
{
	u32 cnt;

	if (num_tokens && netif_queue_stopped(dev->owner)) {
		printk("EMAC: TX Complete: Starting queue\n");
		netif_start_queue(dev->owner);
	}

	for (cnt = 0; cnt < num_tokens; cnt++) {
		struct sk_buff *skb = (struct sk_buff *)net_data_tokens[cnt];
		if (skb == NULL)
			continue;

		dev->net_dev_stats.tx_packets++;
		dev->net_dev_stats.tx_bytes += skb->len;
		dev_kfree_skb_any(skb);
	}

	return (0);
}

/******************************************************************************
 *  Interrupt functions
 *****************************************************************************/

irqreturn_t emac_hal_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	struct emac_dev *dev = (struct emac_dev *) dev_id;

	++dev->isr_count;
	if (!dev->set_to_close) {
		/* NAPI support */
		netif_rx_schedule(dev->owner);
	} else {
		/* we are closing down, so dont process anything */
	}

	return IRQ_HANDLED;
}

/* transmit function - only single fragment supported */
static int emac_dev_tx(struct sk_buff *skb, struct net_device *netdev)
{
	int ret_code;
	net_buf_obj tx_buf;	/* buffer object - only single frame support */
	net_pkt_obj tx_packet;	/* packet object */
	struct emac_dev *dev = NETDEV_PRIV(netdev);
	/* ANANT HACK: unsigned long flags; */

	/* Build the buffer and packet objects - Since only single fragment is
	 * supported, need not set length and token in both packet & object.
	 * Doing so for completeness sake & to show that this needs to be done
	 * in multifragment case
	 */
	tx_packet.buf_list = &tx_buf;
	tx_packet.num_bufs = 1;	/* only single fragment supported */
	tx_packet.pkt_length = skb->len;
	tx_packet.pkt_token = (emac_net_data_token) skb;
	tx_buf.length = skb->len;
	tx_buf.buf_token = (emac_net_data_token) skb;
	tx_buf.data_ptr = skb->data;

	/* flush data buffer if write back mode */
	EMAC_CACHE_WRITEBACK((unsigned long)skb->data, skb->len);
	netdev->trans_start = jiffies;

	/* ANANT_HACK: Need to lock TX so that there is no contention
	   spin_lock_irqsave(&hDDA->lock, flags);
	 */

	/* DDC send : last param FALSE so that hardware calculates CRC */
	ret_code = emac_send(dev, &tx_packet, EMAC_DEFAULT_TX_CHANNEL, FALSE);

	/* ANANT_HACK: Need to un-lock TX so that there is no contention
	   between two processes
	   spin_unlock_irqrestore(&hDDA->lock, flags);
	 */

	if (ret_code != EMAC_SUCCESS) {
		if (ret_code == EMAC_ERR_TX_OUT_OF_BD) {
			ERR("WARN: emac_dev_tx: Out of TX BD's\n");
			netif_stop_queue(dev->owner);
		}
		dev->net_dev_stats.tx_dropped++;
		goto emac_dev_tx_drop_pkt;
	}
	return (0);

emac_dev_tx_drop_pkt:
	dev->net_dev_stats.tx_dropped++;

	return (-1);
}

/* emac_disable_interrupt : Turns of the interrupts from the
 * peripheral to the CPU. we have four interrupt lines
 * running to the CPU, we handle them as a set*/
static void emac_disable_interrupt(struct emac_dev *dev)
{
	if (!dev->init_cfg.registers_old) {
		unsigned long flags;
		volatile struct ewrap_dm646x_regs *dm646x_wrap;

		dm646x_wrap = &dev->ewrap_regs->wrapper_646x_regs;
		local_irq_save(flags);
		/* Program C0_Int_En to zero to turn off
		 * interrupts to the CPU */
		dm646x_wrap->C0_RXTHRESHEN = 0x0;
		dm646x_wrap->C0_RXINTEN = 0x0;
		dm646x_wrap->C0_TXINTEN = 0x0;
		dm646x_wrap->C0_MISCEN = 0x0;

		local_irq_restore(flags);
	} else {
		volatile struct ewrap_dm644x_regs *dm644x_wrap;

		dm644x_wrap = &dev->ewrap_regs->wrapper_644x_regs;
		dm644x_wrap->EWCTL = 0x0;
	}
}

static void emac_enable_interrupt(struct emac_dev *dev, int ack_eoi)
{
	if (!dev->init_cfg.registers_old) {
		unsigned long flags;
		volatile struct ewrap_dm646x_regs *dm646x_wrap;

                dm646x_wrap = &dev->ewrap_regs->wrapper_646x_regs;

		local_irq_save(flags);

		/* Program C0_Int_En to zero to appropriate values */
		dm646x_wrap->C0_RXTHRESHEN = dev->rx_thresh_en;
		dm646x_wrap->C0_RXINTEN = dev->rx_en;
		dm646x_wrap->C0_TXINTEN = dev->tx_en;
		dm646x_wrap->C0_MISCEN = dev->misc_en;

		if (ack_eoi) {

			/*
			 * In addition to turning on interrupt Enable, we need
			 * ack by writing appropriate values to the EOI
			 * register
			 */
			/* ack rxthresh- only then a new pulse is generated */
			dev->regs->mac_EOI_vector =
				EMAC_DM646X_MAC_EOI_C0_RXTHRESH;
			/*ack rxen only then a new pulse will be generated */
			dev->regs->mac_EOI_vector =
				EMAC_DM646X_MAC_EOI_C0_RXEN;
			/*ack txen- only then a new pulse will be generated */
			dev->regs->mac_EOI_vector =
				EMAC_DM646X_MAC_EOI_C0_TXEN;
			/*ack macen- only then a new pulse will be generated */
			dev->regs->mac_EOI_vector =
				EMAC_DM646X_MAC_EOI_C0_MISCEN;
		}

		local_irq_restore(flags);
	} else {
		volatile struct ewrap_dm644x_regs *dm644x_wrap;

		dm644x_wrap = &dev->ewrap_regs->wrapper_644x_regs;
		dm644x_wrap->EWCTL = 0x1;
	}
}

/*
 * proc/sysfs stuff
 */

static int init_proc(void)
{
	struct proc_dir_entry *gp_stats_file;

	create_proc_read_entry("net/emac_link", 0, NULL, emac_p_read_link,
			NULL);
	create_proc_read_entry("net/emac_ver", 0, NULL, emac_p_get_version,
			NULL);
	create_proc_read_entry("net/emac_config", 0, NULL, emac_dump_config,
			NULL);
	gp_stats_file = create_proc_entry("net/emac_stats", 0644, NULL);
	if (gp_stats_file) {
		gp_stats_file->read_proc = emac_p_read_stats;
		gp_stats_file->write_proc = emac_p_write_stats;
	}

	return 0;
}

static void exit_proc(void)
{
	remove_proc_entry("net/emac_link", NULL);
	remove_proc_entry("net/emac_ver", NULL);
	remove_proc_entry("net/emac_config", NULL);
	remove_proc_entry("net/emac_stats", NULL);
}

static void format_proc_name(char *buf, int len, struct emac_dev *dev)
{
	strncpy(buf, "net/emac_", len);
	buf[len - 1] = 0;
	strncat(buf, ((struct net_device *)dev->owner)->name, len - strlen(buf));
	strncat(buf, "_rfc2665_stats", len - strlen(buf));
}

#define PROC_NAME_MAX_LEN 50

static int create_proc_entries_for_dev(struct emac_dev *dev)
{
	char proc_name[PROC_NAME_MAX_LEN];

	format_proc_name(proc_name, PROC_NAME_MAX_LEN, dev);

	create_proc_read_entry(proc_name, 0, NULL,
			emac_p_read_rfc2665_stats, dev->owner);

	return 0;
}

static void delete_proc_entries_for_dev(struct emac_dev *dev)
{
	char proc_name[PROC_NAME_MAX_LEN];

	format_proc_name(proc_name, PROC_NAME_MAX_LEN, dev);

	remove_proc_entry(proc_name, NULL);
}

static ssize_t emac_show_version(struct device_driver *drv, char *buf)
{
	return emac_p_get_version(buf, NULL, 0, 4096, NULL, NULL);
}

static DRIVER_ATTR(version, S_IRUGO, emac_show_version, NULL);

/*
 * mdio bus stuff
 */

static inline void wait_for_user_access(struct emac_dev *dev)
{
	unsigned int value;

	do {
		value = __raw_readl(dev->mdio_base_address + MDIO_USERACCESS0);
	} while (value & MDIO_USERACCESS_GO);
}

int davinci_mdio_write(struct mii_bus *bus, int mii_id, int regnum, u16 value)
{
	struct emac_dev *dev = container_of(bus, struct emac_dev, mdio_bus);

	__raw_writel(MDIO_USERACCESS_GO | MDIO_USERACCESS_WRITE |
		       ((regnum << 21) & MDIO_USERACCESS_REGADR) |
		       ((mii_id << 16) & MDIO_USERACCESS_PHYADR) |
			value,
		    dev->mdio_base_address + MDIO_USERACCESS0);

	wait_for_user_access(dev);

	return 0;
}

int davinci_mdio_read(struct mii_bus *bus, int mii_id, int regnum)
{
	struct emac_dev *dev = container_of(bus, struct emac_dev, mdio_bus);

	wait_for_user_access(dev);

	__raw_writel(MDIO_USERACCESS_GO | MDIO_USERACCESS_READ
			| ((regnum << 21) & MDIO_USERACCESS_REGADR)
			| ((mii_id << 16) & MDIO_USERACCESS_PHYADR),
			dev->mdio_base_address + MDIO_USERACCESS0);

	wait_for_user_access(dev);

	return __raw_readl(dev->mdio_base_address + MDIO_USERACCESS0) &
		MDIO_USERACCESS_DATA;
}

/*
 * probe/remove stuff
 */

static int probe_mii_bus(struct emac_dev *dev)
{
	struct net_device *netdev = dev->owner;
	struct platform_device *pdev =
		to_platform_device(netdev->class_dev.dev);
	struct emac_init_config *pdata = pdev->dev.platform_data;
	struct resource *res;
	u32 clk_div;
	int err = 0;

	memzero(&dev->mdio_bus, sizeof(dev->mdio_bus));

	dev->mdio_bus.name = "Davinci EMAC MII Bus",
	dev->mdio_bus.id = pdev->id;

	dev->mdio_bus.read = &davinci_mdio_read,
	dev->mdio_bus.write = &davinci_mdio_write,

	dev->mdio_bus.dev = &pdev->dev;
	dev->mdio_bus.irq = pdata->mdio_irqs;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mdio_regs");
	if (!res)
		return -ENODEV;

	dev->mdio_base_address = IO_ADDRESS(res->start);

	clk_div = ((dev->init_cfg.mdio_bus_frequency /
			dev->init_cfg.mdio_clock_frequency) - 1) & 0xff;

	__raw_writel(clk_div | MDIO_CONTROL_ENABLE,
			dev->mdio_base_address + MDIO_CONTROL);

	err = mdiobus_register(&dev->mdio_bus);
	if (0 != err) {
		printk(KERN_ERR "%s: Cannot register as MDIO bus\n",
				dev->mdio_bus.name);
		return err;
	}

	return 0;
}

static inline void remove_mii_bus(struct emac_dev *dev)
{
	mdiobus_unregister(&dev->mdio_bus);
}

static int __init davinci_emac_probe(struct platform_device *pdev)
{
	struct emac_init_config *pdata = pdev->dev.platform_data;
	struct net_device *netdev;
	struct emac_dev *dev;
	int err = 0;

	/* obtain clock rate from kernel clock API's */
	emac_clk = clk_get(0, "EMACCLK");
	if (IS_ERR(emac_clk)) {
		printk("TI DAVINCI EMAC: Failed to get clock. Exiting\n");
		return -1;
	}
	clk_enable(emac_clk);

	netdev = alloc_etherdev(sizeof(struct emac_dev));
	if (!netdev) {
		printk ("TI DaVinci EMAC: Etherdev alloc failed (1).\n");

		err = -ENOMEM;
		/* if error, free EMAC clock */
		clk_disable(emac_clk);
		goto err;
	}

	dev = NETDEV_PRIV(netdev);
	dev->owner = netdev;
	netdev->init = emac_dev_init;
	SET_NETDEV_DEV(netdev, &(pdev->dev));
	memcpy(&dev->init_cfg, pdata, sizeof(struct emac_init_config));

	/*
	 * Check for settings in platform data
	 */
	if (pdata->phy_mode == -1)
		g_link_speed = CONFIG_EMAC_NOPHY;
	else
		emac_p_detect_manual_cfg(cfg_link_speed, cfg_link_mode);

	platform_set_drvdata(pdev, netdev);

	probe_mii_bus(dev);

	/* register the network device with linux */
	err = register_netdev(netdev);
	if (err) {
		ERR("Could not register device: %d\n", err);

		clk_disable(emac_clk);

		FREE_NETDEV(netdev);
	}

	create_proc_entries_for_dev(dev);

	list_add(&dev->emac_devices, &emac_devices);

	printk("%s\n", emac_version_string);

err:
	return err;
}

/* frees the EMAC device structures */
static int __devexit davinci_emac_remove(struct platform_device *pdev)
{
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct emac_dev *dev = NETDEV_PRIV(netdev);

 	list_del(&dev->emac_devices);
 
	/* free EMAC clock */
	clk_disable(emac_clk);

	/* delete the proc entry */
	delete_proc_entries_for_dev(dev);

	/* release memory region and unregister the device */
	unregister_netdev(netdev);

	remove_mii_bus(dev);

	if (netdev)
		FREE_NETDEV(netdev);

	return 0;
}

/*
 * init/exit stuff
 */
static struct platform_driver davinci_emac_driver = {
	.driver	= {
		.name = "emac_davinci",
		.owner = THIS_MODULE,
	},
	.probe = davinci_emac_probe,
	.remove = __devexit_p(davinci_emac_remove),
};

static int __init davinci_emac_init(void)
{
	int err = platform_driver_register(&davinci_emac_driver);
	if (err)
		return err;

	driver_create_file(&davinci_emac_driver.driver, &driver_attr_version);

	init_proc();
	return 0;
}

static void __exit davinci_emac_exit(void)
{
	exit_proc();

	driver_remove_file(&davinci_emac_driver.driver, &driver_attr_version);
	platform_driver_unregister(&davinci_emac_driver);
}

module_init(davinci_emac_init);
module_exit(davinci_emac_exit);

MODULE_AUTHOR("Texas Instruments");
MODULE_DESCRIPTION("TI DaVinci EMAC Driver");
MODULE_LICENSE("GPL");
