/* ==========================================================================
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================== */

#ifndef __DWC_OTG_PLAT_H__
#define __DWC_OTG_PLAT_H__

#include <linux/slab.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/io.h>

#include "dwc_otg_regs.h"

/**
 * @file
 *
 * This file contains the Platform Specific constants, interfaces
 * (functions and macros) for Linux.
 *
 */
#if !defined(CONFIG_4xx)
#error "The contents of this file are AMCC 4xx processor specific!!!"
#endif


/*
 * Debugging support vanishes in non-debug builds.
 */


/**
 * The Debug Level bit-mask variable.
 */
extern uint32_t g_dbg_lvl;

/**
 * Set the Debug Level variable.
 */
static inline uint32_t SET_DEBUG_LEVEL(const uint32_t _new)
{
	uint32_t old = g_dbg_lvl;
	g_dbg_lvl = _new;
	return old;
}

/** When debug level has the DBG_CIL bit set, display CIL Debug messages. */
#define DBG_CIL		(0x2)
/** When debug level has the DBG_CILV bit set, display CIL Verbose debug
 * messages */
#define DBG_CILV	(0x20)
/**  When debug level has the DBG_PCD bit set, display PCD (Device) debug
 *  messages */
#define DBG_PCD		(0x4)
/** When debug level has the DBG_PCDV set, display PCD (Device) Verbose debug
 * messages */
#define DBG_PCDV	(0x40)
/** When debug level has the DBG_HCD bit set, display Host debug messages */
#define DBG_HCD		(0x8)
/** When debug level has the DBG_HCDV bit set, display Verbose Host debug
 * messages */
#define DBG_HCDV	(0x80)
/** When debug level has the DBG_HCD_URB bit set, display enqueued URBs in host
 *  mode. */
#define DBG_HCD_URB	(0x800)

/** When debug level has any bit set, display debug messages */
#define DBG_ANY		(0xFF)

/** All debug messages off */
#define DBG_OFF		0

/** Prefix string for DWC_DEBUG print macros. */
#define USB_DWC "dwc_otg: "

/**
 * Print a debug message when the Global debug level variable contains
 * the bit defined in <code>lvl</code>.
 *
 * @param[in] lvl - Debug level, use one of the DBG_ constants above.
 * @param[in] x - like printf
 *
 *    Example:<p>
 * <code>
 *      DWC_DEBUGPL(DBG_ANY, "%s(%p)\n", __func__, _reg_base_addr);
 * </code>
 * <br>
 * results in:<br>
 * <code>
 * usb-DWC_otg: dwc_otg_cil_init(ca867000)
 * </code>
 */
#ifdef DEBUG
# define DWC_DEBUGPL(lvl, x...)				\
	do {						\
		if ((lvl)&g_dbg_lvl)			\
			printk(KERN_DEBUG USB_DWC x);	\
	} while (0)

# define DWC_DEBUGP(x...)	DWC_DEBUGPL(DBG_ANY, x)

# define CHK_DEBUG_LEVEL(level) ((level) & g_dbg_lvl)

#else

# define DWC_DEBUGPL(lvl, x...) do {} while (0)
# define DWC_DEBUGP(x...)

# define CHK_DEBUG_LEVEL(level) (0)

#endif /*DEBUG*/

/**
 * Print an Error message.
 */
#define DWC_ERROR(x...) printk(KERN_ERR USB_DWC x)
/**
 * Print a Warning message.
 */
#define DWC_WARN(x...) printk(KERN_WARNING USB_DWC x)
/**
 * Print a notice (normal but significant message).
 */
#define DWC_NOTICE(x...) printk(KERN_NOTICE USB_DWC x)
/**
 *  Basic message printing.
 */
#define DWC_PRINT(x...) printk(KERN_INFO USB_DWC x)


#ifdef DEBUG
static inline void dump_msg(const u8 *buf, unsigned int length)
{
	unsigned int start, num, i;
	char line[52], *p;

	if (length >= 512)
		return;

	start = 0;
	while (length > 0) {
		num = min(length, 16u);
		p = line;
		for (i = 0; i < num; ++i) {
			if (i == 8)
				*p++ = ' ';
			sprintf(p, " %02x", buf[i]);
			p += 3;
		}
		*p = 0;
		DWC_PRINT("%6x: %s\n", start, line);
		buf += num;
		start += num;
		length -= num;
	}
}


#else	/* DEBUG */
static inline void dump_msg(const u8 *buf, unsigned int length)
{
}
#endif	/* DEBUG */


/**
 * The following parameters may be specified when starting the module. These
 * parameters define how the DWC_otg controller should be configured.
 * Parameter values are passed to the CIL initialization function
 * dwc_otg_cil_init.
 */
typedef struct dwc_otg_core_params {
	int32_t opt;
#define dwc_param_opt_default 1

	/**
	 * Specifies the OTG capabilities. The driver will automatically
	 * detect the value for this parameter if none is specified.
	 * 0 - HNP and SRP capable (default)
	 * 1 - SRP Only capable
	 * 2 - No HNP/SRP capable
	 */
	int32_t otg_cap;
#define DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE 0
#define DWC_OTG_CAP_PARAM_SRP_ONLY_CAPABLE 1
#define DWC_OTG_CAP_PARAM_NO_HNP_SRP_CAPABLE 2
#define dwc_param_otg_cap_default DWC_OTG_CAP_PARAM_HNP_SRP_CAPABLE

	/**
	 * Specifies whether to use slave or DMA mode for accessing the data
	 * FIFOs. The driver will automatically detect the value for this
	 * parameter if none is specified.
	 * 0 - Slave
	 * 1 - DMA (default, if available)
	 */
	int32_t dma_enable;
#define dwc_param_dma_enable_default 1

	/** The DMA Burst size (applicable only for External DMA
	 * Mode). 1, 4, 8 16, 32, 64, 128, 256 (default 32)
	 */
	int32_t dma_burst_size;	 /* Translate this to GAHBCFG values */
#define dwc_param_dma_burst_size_default 32

	/**
	 * Specifies the maximum speed of operation in host and device mode.
	 * The actual speed depends on the speed of the attached device and
	 * the value of phy_type. The actual speed depends on the speed of the
	 * attached device.
	 * 0 - High Speed (default)
	 * 1 - Full Speed
	 */
	int32_t speed;
#define dwc_param_speed_default 0
#define DWC_SPEED_PARAM_HIGH 0
#define DWC_SPEED_PARAM_FULL 1

	/** Specifies whether low power mode is supported when attached
	 *	to a Full Speed or Low Speed device in host mode.
	 * 0 - Don't support low power mode (default)
	 * 1 - Support low power mode
	 */
	int32_t host_support_fs_ls_low_power;
#define dwc_param_host_support_fs_ls_low_power_default 0

	/** Specifies the PHY clock rate in low power mode when connected to a
	 * Low Speed device in host mode. This parameter is applicable only if
	 * HOST_SUPPORT_FS_LS_LOW_POWER is enabled. If PHY_TYPE is set to FS
	 * then defaults to 6 MHZ otherwise 48 MHZ.
	 *
	 * 0 - 48 MHz
	 * 1 - 6 MHz
	 */
	int32_t host_ls_low_power_phy_clk;
#define dwc_param_host_ls_low_power_phy_clk_default 0
#define DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_48MHZ 0
#define DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_6MHZ 1

	/**
	 * 0 - Use cC FIFO size parameters
	 * 1 - Allow dynamic FIFO sizing (default)
	 */
	int32_t enable_dynamic_fifo;
#define dwc_param_enable_dynamic_fifo_default 1

	/** Total number of 4-byte words in the data FIFO memory. This
	 * memory includes the Rx FIFO, non-periodic Tx FIFO, and periodic
	 * Tx FIFOs.
	 * 32 to 32768 (default 8192)
	 * Note: The total FIFO memory depth in the FPGA configuration is 8192.
	 */
	int32_t data_fifo_size;
#define dwc_param_data_fifo_size_default 8192

	/** Number of 4-byte words in the Rx FIFO in device mode when dynamic
	 * FIFO sizing is enabled.
	 * 16 to 32768 (default 1064)
	 */
	int32_t dev_rx_fifo_size;
#define dwc_param_dev_rx_fifo_size_default 1064

	/** Number of 4-byte words in the non-periodic Tx FIFO in device mode
	 * when dynamic FIFO sizing is enabled.
	 * 16 to 32768 (default 1024)
	 */
	int32_t dev_nperio_tx_fifo_size;
#define dwc_param_dev_nperio_tx_fifo_size_default 1024

	/** Number of 4-byte words in each of the periodic Tx FIFOs in device
	 * mode when dynamic FIFO sizing is enabled.
	 * 4 to 768 (default 256)
	 */
	uint32_t dev_perio_tx_fifo_size[MAX_PERIO_FIFOS];
#define dwc_param_dev_perio_tx_fifo_size_default 256

	/** Number of 4-byte words in the Rx FIFO in host mode when dynamic
	 * FIFO sizing is enabled.
	 * 16 to 32768 (default 1024)
	 */
	int32_t host_rx_fifo_size;
#define dwc_param_host_rx_fifo_size_default 1024

	/** Number of 4-byte words in the non-periodic Tx FIFO in host mode
	 * when Dynamic FIFO sizing is enabled in the core.
	 * 16 to 32768 (default 1024)
	 */
	int32_t host_nperio_tx_fifo_size;
#define dwc_param_host_nperio_tx_fifo_size_default 1024

	/** Number of 4-byte words in the host periodic Tx FIFO when dynamic
	 * FIFO sizing is enabled.
	 * 16 to 32768 (default 1024)
	 */
	int32_t host_perio_tx_fifo_size;
#define dwc_param_host_perio_tx_fifo_size_default 1024

	/** The maximum transfer size supported in bytes.
	 * 2047 to 65,535  (default 65,535)
	 */
	int32_t max_transfer_size;
#define dwc_param_max_transfer_size_default 65535

	/** The maximum number of packets in a transfer.
	 * 15 to 511  (default 511)
	 */
	int32_t max_packet_count;
#define dwc_param_max_packet_count_default 511

	/** The number of host channel registers to use.
	 * 1 to 16 (default 12)
	 * Note: The FPGA configuration supports a maximum of 12 host channels.
	 */
	int32_t host_channels;
#define dwc_param_host_channels_default 12

	/** The number of endpoints in addition to EP0 available for device
	 * mode operations.
	 * 1 to 15 (default 6 IN and OUT)
	 * Note: The FPGA configuration supports a maximum of 6 IN and OUT
	 * endpoints in addition to EP0.
	 */
	int32_t dev_endpoints;
#define dwc_param_dev_endpoints_default 6

	/**
	 * Specifies the type of PHY interface to use. By default, the driver
	 * will automatically detect the phy_type.
	 *
	 * 0 - Full Speed PHY
	 * 1 - UTMI+ (default)
	 * 2 - ULPI
	 */
	int32_t phy_type;
#define DWC_PHY_TYPE_PARAM_FS 0
#define DWC_PHY_TYPE_PARAM_UTMI 1
#define DWC_PHY_TYPE_PARAM_ULPI 2
#define dwc_param_phy_type_default DWC_PHY_TYPE_PARAM_UTMI

	/**
	 * Specifies the UTMI+ Data Width.	This parameter is
	 * applicable for a PHY_TYPE of UTMI+ or ULPI. (For a ULPI
	 * PHY_TYPE, this parameter indicates the data width between
	 * the MAC and the ULPI Wrapper.) Also, this parameter is
	 * applicable only if the OTG_HSPHY_WIDTH cC parameter was set
	 * to "8 and 16 bits", meaning that the core has been
	 * configured to work at either data path width.
	 *
	 * 8 or 16 bits (default 16)
	 */
	int32_t phy_utmi_width;
#define dwc_param_phy_utmi_width_default 16

	/**
	 * Specifies whether the ULPI operates at double or single
	 * data rate. This parameter is only applicable if PHY_TYPE is
	 * ULPI.
	 *
	 * 0 - single data rate ULPI interface with 8 bit wide data
	 * bus (default)
	 * 1 - double data rate ULPI interface with 4 bit wide data
	 * bus
	 */
	int32_t phy_ulpi_ddr;
#define dwc_param_phy_ulpi_ddr_default 0

	/**
	 * Specifies whether to use the internal or external supply to
	 * drive the vbus with a ULPI phy.
	 */
	int32_t phy_ulpi_ext_vbus;
#define DWC_PHY_ULPI_INTERNAL_VBUS 0
#define DWC_PHY_ULPI_EXTERNAL_VBUS 1
#define dwc_param_phy_ulpi_ext_vbus_default DWC_PHY_ULPI_INTERNAL_VBUS

	/**
	 * Specifies whether to use the I2Cinterface for full speed PHY. This
	 * parameter is only applicable if PHY_TYPE is FS.
	 * 0 - No (default)
	 * 1 - Yes
	 */
	int32_t i2c_enable;
#define dwc_param_i2c_enable_default 0

	int32_t ulpi_fs_ls;
#define dwc_param_ulpi_fs_ls_default 0

	int32_t ts_dline;
#define dwc_param_ts_dline_default 0

	/**
	 * Specifies whether dedicated transmit FIFOs are
	 * enabled for non periodic IN endpoints in device mode
	 * 0 - No
	 * 1 - Yes
	 */
	 int32_t en_multiple_tx_fifo;
#define dwc_param_en_multiple_tx_fifo_default 1

	/** Number of 4-byte words in each of the Tx FIFOs in device
	 * mode when dynamic FIFO sizing is enabled.
	 * 4 to 768 (default 256)
	 */
	uint32_t dev_tx_fifo_size[MAX_TX_FIFOS];
#define dwc_param_dev_tx_fifo_size_default 256

	/** Thresholding enable flag-
	 * bit 0 - enable non-ISO Tx thresholding
	 * bit 1 - enable ISO Tx thresholding
	 * bit 2 - enable Rx thresholding
	 */
	uint32_t thr_ctl;
#define dwc_param_thr_ctl_default 0

	/** Thresholding length for Tx
	 *	FIFOs in 32 bit DWORDs
	 */
	uint32_t tx_thr_length;
#define dwc_param_tx_thr_length_default 64

	/** Thresholding length for Rx
	 *	FIFOs in 32 bit DWORDs
	 */
	uint32_t rx_thr_length;
#define dwc_param_rx_thr_length_default 64

	/** Read-only feature flags
	 *      Set automatically
	 *      at driver start-up
	 */
	uint32_t feature;

	/** Read-only device EP transfer
	 *  size register mask
	 *      Set automatically
	 *      at driver start-up
	 */
	uint32_t deptsiz_mask;
} dwc_otg_core_params_t;

/* DWC OTG feature flags */
#define DWC_OTG_FTR_460EX		0x1
#define DWC_OTG_FTR_405EX		0x2
#define DWC_OTG_FTR_405EZ		0x4
#define DWC_OTG_FTR_SLAVE		0x10
/* Device only */
#define DWC_OTG_FTR_DEVICE		0x20
/* Host only */
#define DWC_OTG_FTR_HOST		0x40
#define DWC_OTG_FTR_EXT_CHG_PUMP	0x80

#ifdef OTG_EXT_CHG_PUMP
#define DWC_OTG_FTRS_ALWAYS		DWC_OTG_FTR_EXT_CHG_PUMP
#else
#define DWC_OTG_FTRS_ALWAYS		0x0
#endif

#define DWC_OTG_FTRS_POSSIBLE	(DWC_OTG_FTR_460EX |	\
				DWC_OTG_FTR_405EX |	\
				DWC_OTG_FTR_405EZ |	\
				DWC_OTG_FTR_SLAVE |	\
				DWC_OTG_FTR_DEVICE |	\
				DWC_OTG_FTR_HOST |	\
				DWC_OTG_FTR_EXT_CHG_PUMP)

static inline int dwc_otg_has_feature(dwc_otg_core_params_t *params,
					uint32_t feature)
{
	return (DWC_OTG_FTRS_ALWAYS & feature) ||
		(DWC_OTG_FTRS_POSSIBLE & params->feature & feature);
}

extern dwc_otg_core_params_t dwc_otg_module_params;

/* This sets the parameter to the default value if it has not been set by the
 * user */
#define DWC_OTG_PARAM_SET_DEFAULT(_param_)			\
({								\
	int changed = 1;					\
	if (dwc_otg_module_params._param_ == -1) {		\
		changed = 0;					\
		dwc_otg_module_params._param_ =			\
			dwc_param_##_param_##_default;		\
	}							\
	changed;						\
})

/* This checks the macro agains the hardware configuration to see if it is
 * valid.  It is possible that the default value could be invalid. In this
 * case, it will report a module error if the user touched the parameter.
 * Otherwise it will adjust the value without any error. */
#define DWC_OTG_PARAM_CHECK_VALID(_param_, _str_, _is_valid_, _set_valid_)\
({									\
	int changed = DWC_OTG_PARAM_SET_DEFAULT(_param_);		\
	int error = 0;							\
	if (!(_is_valid_)) {						\
		if (changed) {						\
			DWC_ERROR("`%d' invalid for parameter `%s' "	\
				"Check HW configuration.\n", 		\
				dwc_otg_module_params._param_, _str_);	\
			error = 1;					\
		} 							\
		dwc_otg_module_params._param_ = (_set_valid_);		\
	}								\
	error;								\
})

/* Checks if the parameter is outside of its valid range of values */
#define DWC_OTG_PARAM_TEST(_param_, _low_, _high_)		\
		((dwc_otg_module_params._param_ < (_low_)) ||	\
		 (dwc_otg_module_params._param_ > (_high_)))

/* If the parameter has been set by the user, check that the parameter value is
 * within the value range of values.  If not, report a module error. */
#define DWC_OTG_PARAM_ERR(_param_, _low_, _high_, _string_) 		\
do { 									\
	if (dwc_otg_module_params._param_ != -1) {			\
		if (DWC_OTG_PARAM_TEST(_param_, (_low_), (_high_))) {	\
			DWC_ERROR("`%d' invalid for parameter `%s'\n",	\
			dwc_otg_module_params._param_, _string_);	\
			dwc_otg_module_params._param_ = 		\
				dwc_param_##_param_##_default;		\
			retval++;					\
		}							\
	}								\
} while (0)

/**
 * MMIO Registers operations
 */
#ifdef	CONFIG_405EZ
#define DWC_OTG_REG_BE
#define DWC_OTG_FIFO_LE
#endif

#define SZ_256K  0x00040000
/**
 * Reads the content of a register.
 *
 * @param _reg address of register to read.
 * @return contents of the register.
 *

 * Usage:<br>
 * <code>uint32_t dev_ctl = dwc_read_reg32(&dev_regs->dctl);</code>
 */
static inline uint32_t dwc_read_reg32(uint32_t __iomem *_reg)
{
#ifndef DWC_OTG_REG_BE
	return in_le32(_reg);
#else
	if (dwc_otg_has_feature(&dwc_otg_module_params, DWC_OTG_FTR_405EZ))
		return in_be32(_reg);
	else
		return in_le32(_reg);
#endif
};

/**
 * Writes a register with a 32 bit value.
 *
 * @param _reg address of register to read.
 * @param _value to write to _reg.
 *
 * Usage:<br>
 * <code>dwc_write_reg32(&dev_regs->dctl, 0); </code>
 */
static inline void dwc_write_reg32(uint32_t __iomem *_reg, const uint32_t _value)
{
#ifndef DWC_OTG_REG_BE
	out_le32(_reg, _value);
#else
	if (dwc_otg_has_feature(&dwc_otg_module_params, DWC_OTG_FTR_405EZ))
		out_be32(_reg, _value);
	else
		out_le32(_reg, _value);
#endif
};

/**
 * This function modifies bit values in a register.  Using the
 * algorithm: (reg_contents & ~clear_mask) | set_mask.
 *
 * @param _reg address of register to read.
 * @param _clear_mask bit mask to be cleared.
 * @param _set_mask bit mask to be set.
 *
 * Usage:<br>
 * <code> // Clear the SOF Interrupt Mask bit and <br>
 * // set the OTG Interrupt mask bit, leaving all others as they were.
 *    dwc_modify_reg32(&dev_regs->gintmsk, DWC_SOF_INT, DWC_OTG_INT);</code>
 */
static inline void dwc_modify_reg32(uint32_t __iomem *_reg,
				const uint32_t _clear_mask,
				const uint32_t _set_mask)
{
#ifndef DWC_OTG_REG_BE
	out_le32(_reg, (in_le32(_reg) & ~_clear_mask) | _set_mask);
#else
	if (dwc_otg_has_feature(&dwc_otg_module_params, DWC_OTG_FTR_405EZ))
		out_be32(_reg, (in_be32(_reg) & ~_clear_mask) | _set_mask);
	else
		out_le32(_reg, (in_le32(_reg) & ~_clear_mask) | _set_mask);
#endif
};

static inline void dwc_write_datafifo32(uint32_t __iomem *_reg,
					const uint32_t _value)
{
#ifndef DWC_OTG_FIFO_LE
	out_be32(_reg, _value);
#else
	if (dwc_otg_has_feature(&dwc_otg_module_params, DWC_OTG_FTR_405EZ))
		out_le32(_reg, _value);
	else
		out_be32(_reg, _value);
#endif
};

static inline uint32_t dwc_read_datafifo32(uint32_t __iomem *_reg)
{
#ifndef OTG_FIFO_LE
	return in_be32(_reg);
#else
	if (dwc_otg_has_feature(&dwc_otg_module_params, DWC_OTG_FTR_405EZ))
		return in_le32(_reg);
	else
		return in_be32(_reg);
#endif
};

#endif	/* __DWC_OTG_PLAT_H__ */

