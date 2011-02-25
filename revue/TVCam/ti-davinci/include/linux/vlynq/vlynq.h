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

#ifndef __VLYNQ_H__
#define __VLYNQ_H__

#include <linux/kernel.h>

#include "vlynq_os.h"

#define LATEST_VLYNQ_REV 0x00010206

#ifndef NULL
#define  NULL 0
#endif

/*
 * The parameters:
 *
 * #define MAX_ROOT_VLYNQ       The number of vlynq ports on the SoC.
 * #define MAX_VLYNQ_COUNT      The maximum number of vlynq interfaces with
 *                              in the system.
 *
 * are system specific and should be defined outside this module.
 *
 */

#ifndef MAX_ROOT_VLYNQ
#error "Please define MAX_ROOT_VLYNQ - The number of vlynq ports on the SoC."
#endif

#ifndef MAX_VLYNQ_COUNT
#error  "Please define MAX_VLYNQ_COUNT - The maximum number of vlynq " \
	"interfaces with in the system."
#endif

#define MAX_VLYNQ_REGION             4
#define MAX_DEV_PER_VLYNQ            MAX_VLYNQ_REGION
#define MAX_DEV_COUNT                12
#define TYPICAL_NUM_ISR_PER_IRQ      1
#define MAX_IRQ_PER_CHAIN            32
#define MAX_IRQ_PER_VLYNQ            8

enum vlynq_clock_dir {
	VLYNQ_CLK_DIR_IN = 0,	/* Sink   */
	VLYNQ_CLK_DIR_OUT = 1	/* Source */
};

enum vlynq_endian {
	VLYNQ_ENDIAN_IGNORE = 0,
	VLYNQ_ENDIAN_LITTLE = 0x2f2f2f2f,
	VLYNQ_ENDIAN_BIG = 0xf2f2f2f2
};

enum vlynq_rtm_cfg {
	VLYNQ_NO_RTM_CGF = 0,
	VLYNQ_RTM_AUTO_SELECT_SAMPLE_VAL = 1,
	VLYNQ_RTM_FORCE_SAMPLE_VAL = 2
};

enum vlynq_init_err {
	VLYNQ_INIT_SUCCESS = 0,
	VLYNQ_INIT_NO_LINK_ON_RESET,
	VLYNQ_INIT_NO_MEM,
	VLYNQ_INIT_CLK_CFG,
	VLYNQ_INIT_NO_LINK_ON_CLK_CFG,
	VLYNQ_INIT_INTERNAL_PROBLEM,
	VLYNQ_INIT_INVALID_PARAM,
	VLYNQ_INIT_LOCAL_HIGH_REV,
	VLYNQ_INIT_PEER_HIGH_REV,
	VLYNQ_INIT_RTM_CFG,
	VLYNQ_INIT_NO_LINK_ON_RTM_CFG
};

struct vlynq_config {
	/*
	 * 1 => the VLYNQ module is on the SOC running the code, 0 =>
	 * otherwise.
	 */
	u8 on_soc;

	enum vlynq_init_err error_status;

	char error_msg[50];

	/* Virtual Base Address of the module. */
	u32 base_addr;

	/*
	 * The number of millsecs that the software should allow for
	 * initialization to complete.
	 */
	u32 init_timeout_in_ms;

	/* The clock divisor for the local VLYNQ module. */
	u8 local_clock_div;

	/* The clock direction; sink or source for the local VLYNQ module. */
	enum vlynq_clock_dir local_clock_dir;

	/*
	 * 1 => interrupts are being handled locally or 0 => sent over the
	 * bus.
	 */
	u8 local_intr_local;

	/*
	 * The IRQ vector# to be used on the local VLYNQ module. Valid values
	 * are 0 to 31. Should be unique.
	 */
	u8 local_intr_vector;

	/* 1 => enable the local irq vector or => disable. */
	u8 local_intr_enable;

	/* Valid only if intr_local is set.
	   1 => write to the local VLYNQ interrupt pending register;
	   0 => write to the location refered by local address pointer. */
	u8 local_int2cfg;

	/*
	 * Address to which the irq should be written to; valid only if
	 * int2cfg is not set.
	 */
	u32 local_intr_pointer;

	/* Endianess of the local VLYNQ module. */
	enum vlynq_endian local_endianness;

	/* The physical portal address of the local VLYNQ. */
	u32 local_tx_addr;

	/* The RTM configuration for the local VLYNQ. */
	enum vlynq_rtm_cfg local_rtm_cfg_type;

	/*
	 * The RTM sample value for the local VLYNQ, valid only if forced
	 * rtm cfg type is selected.
	 */
	u8 local_rtm_sample_value;

	/* TX Fast path 0 => slow path, 1 => fast path. */
	u8 local_tx_fast_path;

	/* The clock divisor for the peer VLYNQ module. */
	u8 peer_clock_div;

	/* The clock direction; sink or source for the peer VLYNQ module. */
	enum vlynq_clock_dir peer_clock_dir;

	/*
	 * 1 => interrupts are being handled by peer VLYNQ or 0 => sent
	 * over the bus.
	 */
	u8 peer_intr_local;

	/* 1 => enable the peer irq vector or => disable. */
	u8 peer_intr_enable;

	/*
	 * The IRQ vector# to be used on the peer VLYNQ module. Valid values
	 * are 0 to 31. Should be unique.
	 */
	u8 peer_intr_vector;

	/* Valid only if intr_local is set.
	   1 => write to the local VLYNQ interrupt pending register;
	   0 => write to the location refered by local address pointer. */
	u8 peer_int2cfg;

	/*
	 * Address to which the irq should be written to; valid only if
	 * int2cfg is not set.
	 */
	u32 peer_intr_pointer;

	/* Endianess of the local VLYNQ module. */
	enum vlynq_endian peer_endianness;

	/* The physical portal address of the peer VLYNQ. */
	u32 peer_tx_addr;

	/* The RTM configuration for the peer VLYNQ. */
	enum vlynq_rtm_cfg peer_rtm_cfg_type;

	/*
	 * The RTM sample value for the local VLYNQ, valid only if forced
	 * rtm cfg type is selected.
	 */
	u8 peer_rtm_sample_value;

	/* TX Fast path 0 => slow path, 1 => fast path. */
	u8 peer_tx_fast_path;

	/* Is the Vlynq module in Endian swapped state to begin with. */
	u8 init_swap_flag;
};

enum vlynq_irq_pol {
	VLYNQ_IRQ_POL_HIGH = 0,
	VLYNQ_IRQ_POL_LOW
};

enum vlynq_irq_type {
	VLYNQ_IRQ_TYPE_LEVEL = 0,
	VLYNQ_IRQ_TYPE_EDGE
};

/* Error Codes */

#define VLYNQ_OK                    0
#define VLYNQ_INVALID_PARAM        -1
#define VLYNQ_INTERNAL_ERROR       -2

extern u8 vlynq_swap_hack;

int vlynq_init(void);

int vlynq_detect_peer_portal(u32 local_vlynq_base,
			     u32 local_tx_phy_addr,
			     u32 local_tx_virt_addr,
			     u32 peer_lvlynq_phy_addr,
			     u32 peer_hvlynq_phy_addr,
			     u8 endian_swap, u32 *got_vlynq_addr);

int vlynq_detect_peer(u32 vlynq_base_addr, u8 endian_swap, u32 *rev_word,
		      u32 *cnt_word);

int vlynq_add_device(void *vlynq, void *vlynq_dev, u8 peer);

int vlynq_remove_device(void *vlynq, void *vlynw_dev);

int vlynq_map_region(void *vlynq, u8 remote, u32 region_id,
		     u32 region_offset, u32 region_size,
		     void *vlynq_dev);

int vlynq_unmap_region(void *vlynq, u8 remote, u32 region_id,
		       void *vlynq_dev);

int vlynq_get_dev_base(void *vlynq, u32 offset, u32 *base_addr,
		       void *vlynq_dev);

u8 vlynq_get_link_status(void *vlynq);

int vlynq_get_num_root(void);

void *vlynq_get_root(int index);

void *vlynq_get_next(void *this);

int vlynq_is_last(void *this);

int vlynq_get_chain_length(void *this);

int vlynq_get_base_addr(void *vlynq, u32 *base);

void *vlynq_get_root_at_base(u32 base_addr);

void *vlynq_get_root_vlynq(void *vlynq);

int vlynq_chain_append(void *this, void *to);

int vlynq_chain_unappend(void *this, void *from);

int vlynq_map_irq(void *vlynq, u32 hw_intr_line, u32 irq,
		  void *vlynq_dev);

int vlynq_unmap_irq(void *vlynq, u32 irq, void *vlynq_dev);

int vlynq_add_isr(void *vlynq, u32 irq, void (*dev_isr) (void *arg1),
		  struct vlynq_dev_isr_param_grp_t *isr_param);

int vlynq_remove_isr(void *vlynq, u32 irq,
		     struct vlynq_dev_isr_param_grp_t *isr_param);

int vlynq_root_isr(void *p_vlynq);

int vlynq_get_irq_count(void *vlynq, u32 irq, u32 *count);

void *vlynq_get_for_irq(void *root, u32 irq);

int vlynq_set_irq_pol(void *vlynq, u32 irq, enum vlynq_irq_pol pol);

int vlynq_get_irq_pol(void *vlynq, u32 irq, enum vlynq_irq_pol *pol);

int vlynq_set_irq_type(void *vlynq, u32 irq, enum vlynq_irq_type type);

int vlynq_get_irq_type(void *vlynq, u32 irq, enum vlynq_irq_type *type);

int vlynq_disable_irq(void *vlynq, u32 irq);

int vlynq_enable_irq(void *vlynq, u32 irq);

int vlynq_config_clock(void *p_vlynq,
		       enum vlynq_clock_dir local_clock_dir,
		       enum vlynq_clock_dir peer_clock_dir,
		       u8 local_clock_div, u8 peer_clock_div);

void *vlynq_init_soc(struct vlynq_config *pal_vlynq_config);

int vlynq_cleanup(void *vlynq);

int vlynq_dump(void *vlynq, u32 dump_type, char *buf, int limit,
	       int *eof);

int vlynq_ioctl(void *vlynq_hnd, u32 cmd, u32 cmd_val);

#endif				/* #ifndef __VLYNQ_H__ */
