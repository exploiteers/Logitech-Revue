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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/vlynq/vlynq_os.h>
#include <linux/vlynq/vlynq_dev.h>
#include <linux/vlynq/vlynq_ioctl.h>

#include <asm/arch/hardware.h>
#include <asm/arch/irqs.h>

#define VLYNQ_IVEC_OFFSET(line)         (line*8)

#define VLYNQ_IVEC_POL_BIT_MAP          (1 << 5)
#define VLYNQ_IVEC_POL_MASK(line)       (VLYNQ_IVEC_POL_BIT_MAP << (line*8))
#define VLYNQ_IVEC_POL_VAL(val)         ((val & 0x1) << 5)
#define VLYNQ_IVEC_POL_DE_VAL(val)      ((val >>  5) & 1)

#define VLYNQ_IVEC_IRQ_BIT_MAP          (0x1f)
#define VLYNQ_IVEC_IRQ_MASK(line)       (VLYNQ_IVEC_IRQ_BIT_MAP << (line*8))
#define VLYNQ_IVEC_IRQ_VAL(val)         (val & 0x1f)
#define VLYNQ_IVEC_IRQ_DE_VAL(val)      (val & 0x1f)

#define VLYNQ_IVEC_TYPE_BIT_MAP         (1 << 6)
#define VLYNQ_IVEC_TYPE_MASK(line)      (VLYNQ_IVEC_TYPE_BIT_MAP << (line*8))
#define VLYNQ_IVEC_TYPE_VAL(val)        ((val & 0x1) << 6)
#define VLYNQ_IVEC_TYPE_DE_VAL(val)     ((val >>  6) & 1)

#define VLYNQ_IVEC_EN_BIT_MAP           (1 << 7)
#define VLYNQ_IVEC_EN_MASK(line)        (VLYNQ_IVEC_EN_BIT_MAP << (line*8))
#define VLYNQ_IVEC_EN_VAL(val)          ((val & 0x1) << 7)
#define VLYNQ_IVEC_EN_DE_VAL(val)       ((val >> 7) & 1)

#define VLYNQ_IVEC_HW_LINE_ADJUST(line) ((line > 3) ? line - 4: line)


/* Vlynq 2.4 register set */
#define VLYNQ_REV	0x00
#define VLYNQ_CNTL	0x04
#define VLYNQ_STAT	0x08
#define VLYNQ_INTPRI	0x0C
#define VLYNQ_INTSTAT	0x10
#define VLYNQ_INTPEND	0x14
#define VLYNQ_INTPTR	0x18
#define VLYNQ_TXMAP	0x1C
#define VLYNQ_RXSET	0x20

#define VLYNQ_CHIPVER	0x40
#define VLYNQ_AUTONEGN	0x44
#define VLYNQ_MANNEGN	0x48
#define VLYNQ_NEGNSTAT	0x4C

#define VLYNQ_ENDIAN	0x5C
#define VLYNQ_IVR30	0x60
#define VLYNQ_IVR74	0x64

#define VLYNQ_RXSIZE(x)		(VLYNQ_RXSET + (x << 3))
#define VLYNQ_RXOFFSET(x)	(VLYNQ_RXSET + (x << 3) + 4)

/*------------------------------------------------------------------------------
 * Forward declaration of data structures.
 *----------------------------------------------------------------------------*/
struct vlynq_irq_map_t;
struct vlynq_irq_info_t;
struct vlynq_isr_info;

/*-----------------------------------------------------------------------------
 * The VLYNQ Information. Priavate structures and API(s).
 *----------------------------------------------------------------------------*/

struct vlynq_region_info {
	s8 owner_dev_index;
	u8 owner_dev_locale;
};

struct vlynq_module {
	u32 base;
	u8 soc;
	u32 vlynq_version;
	u32 timeout_ms;
	struct vlynq_module *next;
	struct vlynq_module *prev;
	void *local_dev[MAX_DEV_PER_VLYNQ];
	void *peer_dev[MAX_DEV_PER_VLYNQ];
	struct vlynq_region_info local_region_info[MAX_VLYNQ_REGION];
	struct vlynq_region_info remote_region_info[MAX_VLYNQ_REGION];
	struct vlynq_irq_map_t *root_irq_map;
	struct vlynq_irq_info_t *root_irq_info;
	struct vlynq_isr_info *local_isr_info;
	struct vlynq_isr_info *peer_isr_info;
	u8 *irq_swap_map;
	u32 backup_local_cntl_word;
	u32 backup_local_intr_ptr;
	u32 backup_local_tx_map;
	u32 backup_local_endian;
	u32 backup_peer_cntl_word;
	u32 backup_peer_intr_ptr;
	u32 backup_peer_tx_map;
	u32 backup_peer_endian;
	s8 local_irq;
	s8 peer_irq;
	u8 local_swap;
	u8 peer_swap;
};

static struct vlynq_module vlynq_module_arr[MAX_VLYNQ_COUNT];
static struct vlynq_module *free_vlynq_list;
static struct vlynq_module *root_vlynq[MAX_ROOT_VLYNQ];

static struct vlynq_module *vlynq_get_free_vlynq(void);
static void vlynq_free_vlynq(struct vlynq_module *);

#define LOCAL_REGS(p_vlynq) (p_vlynq->base)
#define PEER_REGS(p_vlynq)  (p_vlynq->base + 0x80)

static void vlynq_param_init(struct vlynq_module *p_vlynq)
{
	int j;

	for (j = 0; j < MAX_DEV_PER_VLYNQ; j++) {
		p_vlynq->local_dev[j] = NULL;
		p_vlynq->peer_dev[j] = NULL;
	}

	for (j = 0; j < MAX_VLYNQ_REGION; j++) {
		p_vlynq->local_region_info[j].owner_dev_index = -1;
		p_vlynq->remote_region_info[j].owner_dev_index = -1;
	}

	p_vlynq->local_isr_info = NULL;
	p_vlynq->peer_isr_info = NULL;
	p_vlynq->root_irq_map = NULL;
	p_vlynq->root_irq_info = NULL;
	p_vlynq->irq_swap_map = NULL;
	p_vlynq->prev = NULL;
	p_vlynq->next = NULL;
	p_vlynq->local_swap = 0u;
	p_vlynq->peer_swap = 0u;
}

static int vlynq_misc_init(void)
{
	struct vlynq_module *p_vlynq;
	u32 root_count = 0, i;

	for (i = 0; i < MAX_VLYNQ_COUNT; i++) {
		p_vlynq = &vlynq_module_arr[i];

		vlynq_param_init(p_vlynq);

		p_vlynq->next = free_vlynq_list;
		free_vlynq_list = p_vlynq;
	}

	while (root_count < MAX_ROOT_VLYNQ)
		root_vlynq[root_count++] = NULL;

	return (VLYNQ_OK);
}

static int vlynq_add_root(struct vlynq_module *p_vlynq)
{
	int i;
	int ret = VLYNQ_INTERNAL_ERROR;

	for (i = 0; i < MAX_ROOT_VLYNQ; i++) {
		if (!root_vlynq[i]) {
			root_vlynq[i] = p_vlynq;
			ret = 0;
			break;
		}
	}

	return (ret);
}

static int vlynq_remove_root(struct vlynq_module *p_vlynq)
{
	int i;
	int ret = VLYNQ_INTERNAL_ERROR;

	for (i = 0; i < MAX_ROOT_VLYNQ; i++) {
		if (root_vlynq[i]) {
			if (root_vlynq[i] == p_vlynq) {
				ret = 0;
				root_vlynq[i] = NULL;
				break;
			}
		}
	}

	return (ret);
}

static struct vlynq_module *vlynq_get_free_vlynq(void)
{
	struct vlynq_module *p_vlynq = free_vlynq_list;

	if (p_vlynq) {
		free_vlynq_list = free_vlynq_list->next;
		p_vlynq->next = NULL;
	}

	return (p_vlynq);
}

static void vlynq_free_vlynq(struct vlynq_module *p_vlynq)
{
	vlynq_param_init(p_vlynq);

	p_vlynq->next = free_vlynq_list;
	free_vlynq_list = p_vlynq;
}

static struct vlynq_module *vlynq_find_root_vlynq(struct vlynq_module *p_vlynq)
{
	while (p_vlynq->prev)
		p_vlynq = p_vlynq->prev;

	return (p_vlynq->soc ? p_vlynq : NULL);
}

static int vlynq_find_dev_index(struct vlynq_module *p_vlynq,
				void *vlynq_dev,
				u8 *peer, int *index)
{
	int dev_index = 0;
	int ret_val = VLYNQ_INTERNAL_ERROR;

	while (dev_index < MAX_DEV_PER_VLYNQ) {
		if (p_vlynq->peer_dev[dev_index] == vlynq_dev) {
			ret_val = 0;
			*peer = 1;	/* 1u */
			break;
		}

		if (p_vlynq->local_dev[dev_index] == vlynq_dev) {
			ret_val = 0;
			*peer = 0;	/* 0u */
			break;
		}

		dev_index++;
	}

	*index = dev_index;

	return (ret_val);
}

/******************************************************************************
 * Interrupts galore: data structures and private functions.
 ******************************************************************************/
#if (VLYNQ_DEV_ISR_PARM_NUM == 0)
#define VLYNQ_DEV_ISR_SIGNATURE (p)
#elif (VLYNQ_DEV_ISR_PARM_NUM == 1)
#define VLYNQ_DEV_ISR_SIGNATURE(p) (p->arg1)
#elif (VLYNQ_DEV_ISR_PARM_NUM == 2)
#define VLYNQ_DEV_ISR_SIGNATURE(p) (p->arg1, p->arg2)
#elif (VLYNQ_DEV_ISR_PARM_NUM == 3)
#define VLYNQ_DEV_ISR_SIGNATURE(p) (p->arg1, p->arg2, p->arg3)
#elif (VLYNQ_DEV_ISR_PARM_NUM == 4)
#define VLYNQ_DEV_ISR_SIGNATURE(p) (p->arg1, p->arg2, p->arg3, p->arg4)
#elif (VLYNQ_DEV_ISR_PARM_NUM == 5)
#define VLYNQ_DEV_ISR_SIGNATURE(p) (p->arg1, p->arg2, p->arg3, p->arg4, \
				p->arg5)
#elif (VLYNQ_DEV_ISR_PARM_NUM == 6)
#define VLYNQ_DEV_ISR_SIGNATURE(p) (p->arg1, p->arg2, p->arg3, p->arg4, \
				p->arg5, p->arg6)
#elif (VLYNQ_DEV_ISR_PARM_NUM == 7)
#define VLYNQ_DEV_ISR_SIGNATURE(p) (p->arg1, p->arg2, p->arg3, p->arg4, \
				p->arg5, p->arg6, p->arg7)
#elif (VLYNQ_DEV_ISR_PARM_NUM == 8)
#define VLYNQ_DEV_ISR_SIGNATURE(p) (p->arg1, p->arg2, p->arg3, p->arg4, \
				p->arg5, p->arg6, p->arg7, p->arg8)
#elif (VLYNQ_DEV_ISR_PARM_NUM == 9)
#define VLYNQ_DEV_ISR_SIGNATURE(p) (p->arg1, p->arg2, p->arg3, p->arg4, \
				p->arg5, p->arg6, p->arg7, p->arg8, p->arg9)
#elif (VLYNQ_DEV_ISR_PARM_NUM == 10)
#define VLYNQ_DEV_ISR_SIGNATURE(p) (p->arg1, p->arg2, p->arg3, p->arg4, \
				p->arg5, p->arg6, p->arg7, p->arg8, \
				p->arg9, p->arg10)
#else
#error "Please define the number of parameters in the ISR for the OS."
#endif

/*------------------------------------------------------------------------------
 * Endian Private functions.
 *
 * Following structure and the associated semantics with the structure is work
 * around to keep certain "endian specific" devices ticking on the VLYNQ chain.
 * Such devices can either work in "big endian" or "little endian" mode and are
 * not capable of the switching to the host specific endianness because of some
 * inherent shortcomings or design. Such devices are exceptions and are treated
 * with bare minimal support.
 *
 * One strong assumption that is made for a chain in which such devices are
 * present is that they should not share interrupts with any other devices in
 * the chain.
 *----------------------------------------------------------------------------*/

/*
 * Interprets the IRQ to the indented value as dispatched by the peer vlynq,
 * overcoming the endian mismatch.
 */
u8 irq_val_swap[] = { 24, 25, 26, 27, 28, 29, 30, 31,
	16, 17, 18, 19, 20, 21, 22, 23,
	8, 9, 10, 11, 12, 13, 14, 15,
	0, 1, 2, 3, 4, 5, 6, 7
};

struct irq_swap_fixup_flag_list {
	/* Do not change the position of the first element in the structure. */
	u8 irq_swap_fixup_flag[MAX_IRQ_PER_CHAIN];
	struct irq_swap_fixup_flag_list *next;

};

static struct irq_swap_fixup_flag_list irq_swap_flag_fixup_list[MAX_ROOT_VLYNQ];
static struct irq_swap_fixup_flag_list *free_irq_swap_flag_fixup_list;

static void vlynq_irq_swap_fixup_list_init(void)
{
	int i = 0;
	struct irq_swap_fixup_flag_list *p_fixup_list;

	while (i < MAX_ROOT_VLYNQ) {
		int j = 0;

		p_fixup_list = &irq_swap_flag_fixup_list[i];

		while (j < MAX_IRQ_PER_CHAIN)
			p_fixup_list->irq_swap_fixup_flag[j++] = 0u;

		p_fixup_list->next = free_irq_swap_flag_fixup_list;
		free_irq_swap_flag_fixup_list = p_fixup_list;
		i++;
	}
}

static u8 *vlynq_get_free_irq_swap_fixup(void)
{
	struct irq_swap_fixup_flag_list *p_fixup_list =
						free_irq_swap_flag_fixup_list;

	if (p_fixup_list) {
		free_irq_swap_flag_fixup_list =
		    free_irq_swap_flag_fixup_list->next;
		p_fixup_list->next = NULL;
	}

	return ((u8 *) p_fixup_list);
}

static void vlynq_free_irq_swap_fixup(u8 *p_irq_swap)
{
	struct irq_swap_fixup_flag_list *p_fixup_list =
	    (struct irq_swap_fixup_flag_list *) p_irq_swap;

	p_fixup_list->next = free_irq_swap_flag_fixup_list;
	free_irq_swap_flag_fixup_list = p_fixup_list;

	return;
}

static void vlynq_set_intern_irq_swap_fixup(struct vlynq_module *p_vlynq)
{
	u8 irq = p_vlynq->peer_irq;
	struct vlynq_module *p_root_vlynq = vlynq_find_root_vlynq(p_vlynq);

	if (p_vlynq->peer_swap == 1u)
		irq = irq_val_swap[irq];
	p_root_vlynq->irq_swap_map[irq] |= p_vlynq->peer_swap;

	irq = p_vlynq->local_irq;
	if (p_vlynq->local_swap == 1u)
		irq = irq_val_swap[irq];

	p_root_vlynq->irq_swap_map[irq] |= p_vlynq->local_swap;

	return;
}

static void vlynq_unset_intern_irq_swap_fixup(struct vlynq_module *p_vlynq)
{
	u8 irq = p_vlynq->peer_irq;
	struct vlynq_module *p_root_vlynq = vlynq_find_root_vlynq(p_vlynq);

	if (p_vlynq->peer_swap == 1u)
		irq = irq_val_swap[irq];
	p_root_vlynq->irq_swap_map[irq] = 0u;

	irq = p_vlynq->local_irq;
	if (p_vlynq->local_swap == 1u)
		irq = irq_val_swap[irq];

	p_root_vlynq->irq_swap_map[irq] = 0u;

	return;
}

static void vlynq_set_irq_swap_fixup(struct vlynq_module *p_vlynq, u8 peer,
				     u8 irq, u8 *p_irq_swap_map)
{
	switch (peer) {
	case 1u:
		if (p_vlynq->peer_swap == 1u)
			irq = irq_val_swap[irq];
		p_irq_swap_map[irq] |= p_vlynq->peer_swap;
		break;

	case 0u:
		if (p_vlynq->local_swap == 1u)
			irq = irq_val_swap[irq];
		p_irq_swap_map[irq] |= p_vlynq->local_swap;
		break;

	default:
		break;
	}

	return;
}

static void vlynq_unset_irq_swap_fixup(struct vlynq_module *p_vlynq, u8 peer,
				       u8 irq, u8 *p_irq_swap_map)
{
	switch (peer) {
	case 1u:
		if (p_vlynq->peer_swap == 1u)
			irq = irq_val_swap[irq];
		p_irq_swap_map[irq] = 0u;
		break;

	case 0u:
		if (p_vlynq->local_swap == 1u)
			irq = irq_val_swap[irq];
		p_irq_swap_map[irq] = 0u;
		break;

	default:
		break;
	}

	return;
}

u32 vlynq_endian_swap(u32 val)
{
	return ((val << 24) | (val >> 24) | ((val >> 8) & 0xff00) |
		((val << 8) & 0xff0000));
}

static u8 need_swap(struct vlynq_module *p_vlynq, u8 peer)
{
	if ((p_vlynq->peer_swap == 1u) && peer)
		return 1u;
	else if ((p_vlynq->local_swap == 1u) && (!peer))
		return 1u;
	else
		return 0u;
}

static void vlynq_wr_reg(struct vlynq_module *p_vlynq, u8 peer,
			 u32 reg_addr, u32 val)
{
	if (need_swap(p_vlynq, peer))
		val = vlynq_endian_swap(val);

	__raw_writel(val, (u32)reg_addr);
}

static u32 vlynq_rd_reg(struct vlynq_module *p_vlynq, u8 peer,
			u32 reg_addr)
{
	u32 val = __raw_readl(reg_addr);

	if (need_swap(p_vlynq, peer))
		val = vlynq_endian_swap(val);

	return (val);
}

/*-----------------------------------------------------------------------------
 *   ISR Information: Takes care of user's drivers and error handling.
 *----------------------------------------------------------------------------*/
#define MAX_NUM_ISR             (TYPICAL_NUM_ISR_PER_IRQ*MAX_IRQ_PER_CHAIN)

typedef int (*vlynq_intern_isr_fn) (struct vlynq_module *);

struct vlynq_intern_isr_info {
	/* Do not change the position of the first element in the structure. */
	vlynq_intern_isr_fn isr;
	struct vlynq_module *vlynq;

};

struct vlynq_dev_isr_info {
	void (*isr) (void *arg1);
	struct vlynq_dev_isr_param_grp_t *isr_param;

};

struct vlynq_isr_info {
	/* Let this be first function in this structure. */
	union one_of_isr_info {
		struct vlynq_dev_isr_info dev_isr_info;
		struct vlynq_intern_isr_info intern_isr_info;

	} one_of_isr_info;

	struct vlynq_isr_info *next;
	s8 irq_type;		/* device, internal. */

};

static struct vlynq_isr_info vlynq_isr_info_list[MAX_NUM_ISR];
static struct vlynq_isr_info *free_vlynq_isr_info_list;

static int vlynq_isr_info_init(void)
{
	struct vlynq_isr_info *p_isr_info;
	int i;

	for (i = 0; i < MAX_NUM_ISR; i++) {
		p_isr_info = &vlynq_isr_info_list[i];
		p_isr_info->irq_type = -1;
		p_isr_info->next = free_vlynq_isr_info_list;
		free_vlynq_isr_info_list = p_isr_info;
	}

	return (VLYNQ_OK);
}

static struct vlynq_isr_info *vlynq_get_free_isr_info(void)
{
	struct vlynq_isr_info *p_isr_info = free_vlynq_isr_info_list;

	if (p_isr_info) {
		free_vlynq_isr_info_list = free_vlynq_isr_info_list->next;
		p_isr_info->next = NULL;
	}

	return (p_isr_info);
}

static void vlynq_free_isr_info(struct vlynq_isr_info *p_isr_info)
{
/*    p_isr_info->irq          = -1; */
	p_isr_info->irq_type = -1;
	p_isr_info->next = free_vlynq_isr_info_list;
	free_vlynq_isr_info_list = p_isr_info;
}

/*-----------------------------------------------------------------------------
 * IRQ Information. House keeping and maintains information for multiple ISR(s).
 *---------------------------------------------------------------------------*/

struct vlynq_irq_info_t {
	struct vlynq_isr_info *isr_info;
	u32 count;
};

struct vlynq_irq_info_list_t {
	/* Do not change the position of the first element in the structure. */
	struct vlynq_irq_info_t irq_info[MAX_IRQ_PER_CHAIN];
	struct vlynq_irq_info_list_t *next;

};

static struct vlynq_irq_info_list_t vlynq_irq_info_list[MAX_ROOT_VLYNQ];
static struct vlynq_irq_info_list_t *free_vlynq_irq_info_list;

static int vlynq_irq_info_init(void)
{
	struct vlynq_irq_info_list_t *p_irq_info_list;
	struct vlynq_irq_info_t *p_irq_info;
	int i, j;

	for (i = 0; i < MAX_ROOT_VLYNQ; i++) {
		p_irq_info_list = &vlynq_irq_info_list[i];
		p_irq_info = (struct vlynq_irq_info_t *)p_irq_info_list;

		for (j = 0; j < MAX_IRQ_PER_CHAIN; j++) {
			p_irq_info->isr_info = NULL;
			p_irq_info->count = 0;	/* SSI: what about irq. ? */
			p_irq_info++;
		}

		p_irq_info_list->next = free_vlynq_irq_info_list;
		free_vlynq_irq_info_list = p_irq_info_list;
	}

	return (VLYNQ_OK);
}

static struct vlynq_irq_info_t *vlynq_get_free_irq_info(void)
{
	struct vlynq_irq_info_list_t *p_irq_info_list =
	    free_vlynq_irq_info_list;

	if (p_irq_info_list) {
		free_vlynq_irq_info_list = free_vlynq_irq_info_list->next;
		p_irq_info_list->next = NULL;
	}

	return ((struct vlynq_irq_info_t *)p_irq_info_list);
}

static void vlynq_free_irq_info(struct vlynq_irq_info_t *p_irq_info)
{
	struct vlynq_irq_info_list_t *p_irq_info_list =
	    (struct vlynq_irq_info_list_t *)p_irq_info;
	int i;

	for (i = 0; i < MAX_IRQ_PER_CHAIN; i++) {
		p_irq_info->isr_info = NULL;
		p_irq_info->count = 0;
		p_irq_info++;
	}

	p_irq_info_list->next = free_vlynq_irq_info_list;
	free_vlynq_irq_info_list = p_irq_info_list;

	return;
}

/*-----------------------------------------------------------------------------
 * IRQ Mapping: maps hw line to irq.
 *---------------------------------------------------------------------------*/
struct vlynq_irq_map_t {
	struct vlynq_module *vlynq;
	s8 hw_intr_line;
	u8 peer;
};

struct vlynq_irq_map_list_t {
	/* Do not change the position of the first element in the structure. */
	struct vlynq_irq_map_t irq_map[MAX_IRQ_PER_CHAIN];
	struct vlynq_irq_map_list_t *next;
};

static struct vlynq_irq_map_list_t vlynq_irq_map_list[MAX_ROOT_VLYNQ];
static struct vlynq_irq_map_list_t *free_vlynq_irq_map_list;

static int vlynq_irq_map_init(void)
{
	struct vlynq_irq_map_list_t *p_irq_map_list;
	struct vlynq_irq_map_t *p_irq_map;
	int i, j;

	for (i = 0; i < MAX_ROOT_VLYNQ; i++) {
		p_irq_map_list = &vlynq_irq_map_list[i];
		p_irq_map = (struct vlynq_irq_map_t *)p_irq_map_list;

		for (j = 0; j < MAX_IRQ_PER_CHAIN; j++) {
			p_irq_map->vlynq = NULL;
			p_irq_map->hw_intr_line = -1;
			p_irq_map++;
		}

		p_irq_map_list->next = free_vlynq_irq_map_list;
		free_vlynq_irq_map_list = p_irq_map_list;
	}

	return (VLYNQ_OK);
}

static struct vlynq_irq_map_t *vlynq_get_free_irq_map(void)
{
	struct vlynq_irq_map_list_t *p_irq_map_list = free_vlynq_irq_map_list;

	if (p_irq_map_list) {
		free_vlynq_irq_map_list = free_vlynq_irq_map_list->next;
		p_irq_map_list->next = NULL;
	}

	return ((struct vlynq_irq_map_t *)p_irq_map_list);
}

static void vlynq_free_irq_map(struct vlynq_irq_map_t *p_irq_map)
{
	struct vlynq_irq_map_list_t *p_irq_map_list =
	    (struct vlynq_irq_map_list_t *)p_irq_map;
	int i;

	for (i = 0; i < MAX_IRQ_PER_CHAIN; i++) {
		p_irq_map->vlynq = NULL;
		p_irq_map->hw_intr_line = -1;
		p_irq_map++;
	}

	p_irq_map_list->next = free_vlynq_irq_map_list;
	free_vlynq_irq_map_list = p_irq_map_list;

	return;
}

static
struct vlynq_irq_map_t *vlynq_get_map_for_irq(struct vlynq_module *p_vlynq,
					      u8 irq)
{
	struct vlynq_irq_map_t *p_irq_map;
	struct vlynq_module *p_root_vlynq = vlynq_find_root_vlynq(p_vlynq);

	if (!p_root_vlynq || !p_root_vlynq->root_irq_map)
		return (NULL);

	p_irq_map = p_root_vlynq->root_irq_map + irq;

	if (p_irq_map->hw_intr_line < 0)
		return (NULL);

	return (p_irq_map);
}

/*
 * VLYNQ portal Detect Logic Begins.
 *
 * These detection logic has been implemented to take care of cascaded chain of
 * same SoC(s). Since, this SW is a generic one and target to cater all types
 * of cascaded needs, we have a little bit complex logic. Simpler logic would
 * work as well but for configurations where you may not have more than 2
 * instances of the same SoC(s) cascaded (pretty much next to each other.
 */
#define CLK_DIV_TO_CTRL_REG_MAP(reg, clk) ((reg & ~(0x7 << 16)) | \
					((clk & 0x7) << 16))
#define CTRL_REG_MAP_TO_CLK_DIV(reg)      ((reg >> 16) & 0x7)

static void vlynq_sys_wr_reg(u8 swap, u32 reg_addr, u32 val)
{
	if (swap)
		val = vlynq_endian_swap(val);

	__raw_writel(val, reg_addr);
}

static u32 vlynq_sys_rd_reg(u8 swap, u32 reg_addr)
{
	if (swap)
		return vlynq_endian_swap(__raw_readl(reg_addr));
	else
		return __raw_readl(reg_addr);
}

static void vlynq_sys_set_reg(u8 swap, u32 reg_addr, u32 val)
{
	u32 tmp = vlynq_sys_rd_reg(swap, reg_addr);
	vlynq_sys_wr_reg(swap, reg_addr, tmp | val);
}

void vlynq_delay_wait(u32 count)
{
	/*
	 * The original code assumed 360Mhz processor and 23 instructions
	 * for each iteration in the loop.  Each iterations through the loop
	 * taks about 1/360Mhz * 23 (about 64.4ns).  The code is expect to
	 * loop through (count/23) + 1 times, so the delay becomes
	 * (count/23 + 1) * 1/360Mhz * 23 ~= (count * 2.8 + 64.4)ns
	 * in the code, delay computations is round up to
	 * count * 3 and "+ 64.4" is removed.  Also 1024 instead of 1000
	 * to convert between ns, us, and ms to speed up computation.
	 * multiplication factor from 2.8 to 3.0 more than make up
	 * for this difference in conversion.
	 */
	count = ((count * 3) >> 10) + 0x1;
	if (count > 1024)
		mdelay(count >> 10);
	else
		udelay(count);
}

static int vlynq_detect_mapped_clk_div_equal(u32 peer_base,
					     u32 clk_div, u8 endian_swap)
{
	/*
	 * We need to have shadow of the VLYNQ register in which clk div was
	 * set.
	 */
	u32 stat_val = vlynq_sys_rd_reg(endian_swap, peer_base + VLYNQ_STAT);
	u32 ctrl_val;

	if (!(stat_val & 0x1))
		return (VLYNQ_INTERNAL_ERROR);

	peer_base += 0x80;;
	ctrl_val = vlynq_sys_rd_reg(endian_swap, peer_base + VLYNQ_CNTL);

	if ((CTRL_REG_MAP_TO_CLK_DIV(ctrl_val) != clk_div))
		return (VLYNQ_INTERNAL_ERROR);

	return (VLYNQ_OK);
}

static int vlynq_detect_peer_portal_when_sinking_clk(u32 base,
						     u32 peer_hvlynq_virt_addr,
						     u32 peer_lvlynq_virt_addr,
						     u8 endian_swap,
						     u32 *got_vlynq_virt_addr)
{
	u8 first_selection = 1u;
	u32 clk_div = 0x4;
	u32 peer_base = peer_hvlynq_virt_addr;

	/* The algorithm:
	 * [a] Set the clock div of local VLYNQ to a known value.
	 * [b] Try reading the value of the clock div in the remote VLYNQ of the
	 *     mapped of one of the two VLYNQ module of the peer VLYNQ SoC.
	 * [c] If the values in [a] and [b] match, then, chances are that local
	 *     VLYNQ is connected to that mapped VLYNQ module. If not try
	 *     doing [b] with the other mapped VLYNQ module.
	 * [d] After the first match try out [a] & [b] two more times, with
	 *     unique values of clock div between the local VLYNQ and
	 *     the selected peer mapped VLYNQ module.
	 */
	do {
		u32 rd_val = vlynq_sys_rd_reg(endian_swap, base + VLYNQ_CNTL);
		rd_val = CLK_DIV_TO_CTRL_REG_MAP(rd_val, clk_div);
		vlynq_sys_wr_reg(endian_swap, base + VLYNQ_CNTL, rd_val);
		vlynq_delay_wait(0xfff);

		rd_val = vlynq_sys_rd_reg(endian_swap, base + VLYNQ_STAT);
		if (!(rd_val & 0x1))
			return (VLYNQ_INTERNAL_ERROR);	/* No Link. */

		if (first_selection) {
			first_selection = 0;
			if (!vlynq_detect_mapped_clk_div_equal
			    (peer_base, clk_div, endian_swap))
				continue;

			peer_base = peer_lvlynq_virt_addr;
		}

		if (vlynq_detect_mapped_clk_div_equal
		    (peer_base, clk_div, endian_swap))
			return (VLYNQ_INTERNAL_ERROR);

	} while (clk_div >>= 1);

	*got_vlynq_virt_addr = peer_base;
	return (VLYNQ_OK);
}

static int vlynq_detect_set_mapped_clk_div(u32 base,
					   u32 clk_div, u8 endian_swap)
{
	u32 val = vlynq_sys_rd_reg(endian_swap, base + VLYNQ_CNTL);

	val = CLK_DIV_TO_CTRL_REG_MAP(val, clk_div);
	vlynq_sys_wr_reg(endian_swap, base + VLYNQ_CNTL, val);
	vlynq_delay_wait(0xfff);

	return (VLYNQ_OK);
}

static int vlynq_detect_peer_portal_when_sourcing_clk(u32 base,
						      u32 peer_hvlynq_virt_addr,
						      u32 peer_lvlynq_virt_addr,
						      u8 endian_swap,
						      u32 *got_vlynq_phy_addr)
{
	u8 first_selection = 1u;
	u32 clk_div = 0x4;
	u32 peer_base = peer_hvlynq_virt_addr;

	/* The algorithm:
	 * [a] Set the clock div of one of the mapped VLYNQ module to a known
	 *     value.
	 * [b] Try reading the value of the clock div in the local VLYNQ.
	 * [c] If the values in [a] and [b] match, then, chances are that local
	 *     VLYNQ is connected to that mapped VLYNQ module. If not try
	 *     doing [a] with the other mapped VLYNQ module.
	 * [d] After the first match try out [a] & [b] two more times, with
	 *     unique values of clock div between the local VLYNQ and the
	 *     selected peer mapped VLYNQ module.
	 */
	do {
		u32 rd_val;

		if (!(vlynq_sys_rd_reg(endian_swap, base + VLYNQ_STAT) & 0x1))
			return (VLYNQ_INTERNAL_ERROR);	/* No Link. */

		if (vlynq_detect_set_mapped_clk_div
		    (peer_base, clk_div, endian_swap))
			return (VLYNQ_INTERNAL_ERROR);

		base += 0x80;

		if (first_selection) {
			first_selection = 0u;
			rd_val = vlynq_sys_rd_reg(endian_swap,
						  base + VLYNQ_CNTL);
			if (CTRL_REG_MAP_TO_CLK_DIV(rd_val) == clk_div)
				continue;

			peer_base = peer_lvlynq_virt_addr;
			if (vlynq_detect_set_mapped_clk_div
			    (peer_base, clk_div, endian_swap))
				return (VLYNQ_INTERNAL_ERROR);
		}

		rd_val = vlynq_sys_rd_reg(endian_swap, base + VLYNQ_CNTL);
		if (CTRL_REG_MAP_TO_CLK_DIV(rd_val) != clk_div)
			return (VLYNQ_INTERNAL_ERROR);

	} while (clk_div >>= 1);

	*got_vlynq_phy_addr = peer_base;
	return (VLYNQ_OK);
}

int vlynq_detect_peer_portal(u32 local_vlynq_base,
			     u32 local_tx_phy_addr, u32 local_tx_virt_addr,
			     u32 peer_lvlynq_phy_addr, u32 peer_hvlynq_phy_addr,
			     u8 endian_swap, u32 *got_vlynq_addr)
{
	u32 base = local_vlynq_base;
	u32 rd_val = vlynq_sys_rd_reg(endian_swap, base + VLYNQ_STAT);
	u8 clk_dir =
	    (vlynq_sys_rd_reg(endian_swap, base + VLYNQ_CNTL) >> 15) & 0x1;
	u32 vlynq_reg_map_size = 0x100;	/* 256 bytes. */
	int ret_val = VLYNQ_OK;
	u32 peer_lvlynq_virt_addr = local_tx_virt_addr;
	u32 peer_hvlynq_virt_addr = peer_lvlynq_virt_addr + vlynq_reg_map_size;
	u32 lvlynq_id, hvlynq_id;

	if (!(rd_val & 0x1))
		return (VLYNQ_INTERNAL_ERROR);	/* Could not detect any Link. */

	/* clear spur intr. */
	vlynq_sys_wr_reg(endian_swap, base + VLYNQ_STAT, rd_val | 0x180);
	vlynq_sys_wr_reg(endian_swap, base + VLYNQ_TXMAP, local_tx_phy_addr);

	base += 0x80;

	vlynq_sys_wr_reg(endian_swap, base + VLYNQ_RXOFFSET(0),
			 peer_lvlynq_phy_addr);
	vlynq_sys_wr_reg(endian_swap, base + VLYNQ_RXSIZE(0),
			 vlynq_reg_map_size);
	vlynq_sys_wr_reg(endian_swap, base + VLYNQ_RXOFFSET(1),
			 peer_hvlynq_phy_addr);
	vlynq_sys_wr_reg(endian_swap, base + VLYNQ_RXSIZE(1),
			 vlynq_reg_map_size);

	lvlynq_id =
	    (vlynq_sys_rd_reg(endian_swap, peer_lvlynq_virt_addr) &
	     0xFFFF0000) >> 16;
	hvlynq_id =
	    (vlynq_sys_rd_reg(endian_swap, peer_hvlynq_virt_addr) &
	     0xFFFF0000) >> 16;

#define VLYNQ_MODULE_ID 0x1

	/*
	 * On many devices, associated portals do NOT master (cannot access)
	 * control modules.
	 */
	if ((lvlynq_id == VLYNQ_MODULE_ID) && (hvlynq_id != VLYNQ_MODULE_ID)) {
		/*
		 * High portal mastering low control module, we are connected
		 * to high portal.
		 */
		*got_vlynq_addr = peer_hvlynq_phy_addr;
	} else if ((lvlynq_id != VLYNQ_MODULE_ID)
		   && (hvlynq_id == VLYNQ_MODULE_ID)) {
		/*
		 * Low portal mastering high control module, we are connected
		 * to low portal.
		 */
		*got_vlynq_addr = peer_lvlynq_phy_addr;
	} else if (clk_dir) {
		/*
		 * Since, both the controls are visible to us, let us detect
		 * the connected portal.
		 */
		ret_val =
		    vlynq_detect_peer_portal_when_sourcing_clk(local_vlynq_base,
							peer_hvlynq_virt_addr,
							peer_lvlynq_virt_addr,
							endian_swap,
							got_vlynq_addr);

		*got_vlynq_addr = (*got_vlynq_addr == peer_lvlynq_virt_addr) ?
		    peer_lvlynq_phy_addr : peer_hvlynq_phy_addr;
	} else {
		ret_val =
		    vlynq_detect_peer_portal_when_sinking_clk(local_vlynq_base,
							peer_hvlynq_virt_addr,
							peer_lvlynq_virt_addr,
							endian_swap,
							got_vlynq_addr);

		*got_vlynq_addr = (*got_vlynq_addr == peer_lvlynq_virt_addr) ?
		    peer_lvlynq_phy_addr : peer_hvlynq_phy_addr;
	}

	vlynq_sys_wr_reg(endian_swap, base + VLYNQ_RXOFFSET(0), 0x00000000);
	vlynq_sys_wr_reg(endian_swap, base + VLYNQ_RXSIZE(0), 0x00000000);
	vlynq_sys_wr_reg(endian_swap, base + VLYNQ_RXOFFSET(1), 0x00000000);
	vlynq_sys_wr_reg(endian_swap, base + VLYNQ_RXSIZE(1), 0x00000000);

	return (ret_val);
}
EXPORT_SYMBOL(vlynq_detect_peer_portal);

static int vlynq_sys_soft_reset(u32 base, u8 local_swap)
{
	u32 val = vlynq_sys_rd_reg(local_swap, base + VLYNQ_REV);

	if (val >= 0x00010200) {
		val = vlynq_sys_rd_reg(local_swap, base + VLYNQ_CNTL);
		vlynq_sys_wr_reg(local_swap, base + VLYNQ_CNTL, val | 0x1);

		/*
		 * Provide sufficient time for reset. Refer 2.4.2 of 2.6 VLYNQ
		 * specifications.
		 */
		vlynq_delay_wait(0xffffff);

		val = vlynq_sys_rd_reg(local_swap, base + VLYNQ_CNTL);
		vlynq_sys_wr_reg(local_swap, base + VLYNQ_CNTL,
				 val & (~0x1));
		vlynq_delay_wait(0xffffff);
	}

	return (VLYNQ_OK);
}

static int vlynq_enable_HW_default_clock(u32 base, u8 local_swap,
					 u8 peer_swap)
{
	int num_try = 3;
	int num_reset = 2;
	u32 clk_div;
	u32 val;

	/*
	 * Find the default clock directions. The qualifying
	 * factor for the default clock directions would be the link detection.
	 */
reset_sink_default_link:
	vlynq_sys_soft_reset(base, local_swap);

try_sink_default_link:
	/* Lets see if have link on reset. We are sinking the clock. */
	val = vlynq_sys_rd_reg(local_swap, base + VLYNQ_STAT) & 0x1;
	if (val) {
		if (--num_try) {
			vlynq_sys_wr_reg(local_swap, base + VLYNQ_STAT,
					 val | 0x180);
			vlynq_delay_wait(0xffffff);
			goto try_sink_default_link;
		}

		num_try = 3;
		if (--num_reset)
			goto reset_sink_default_link;

		return (VLYNQ_OK);	/* Yes, we have the link. */
	}

	/* So, we did not find default clock direction as yet. Let us set the
	 * clock in the source mode.
	 */
	for (clk_div = 4; clk_div < 9; clk_div++) {
		num_try = 3;
		num_reset = 2;

reset_source_default_link:
		vlynq_sys_soft_reset(base, local_swap);

		/* Set up a reducing frequency to detect the link in each
			iteration. */
		val = vlynq_sys_rd_reg(local_swap, base + VLYNQ_CNTL);
		val |= (0x1 << 15);
		val &= ~(0x7 << 16);
		val |= ((clk_div - 1) << 16);
		vlynq_sys_wr_reg(local_swap, base + VLYNQ_CNTL, val);

try_source_default_link:
		vlynq_delay_wait(0xffffff);

		val = vlynq_sys_rd_reg(local_swap, base + VLYNQ_STAT) & 0x1;
		if (val) {
			if (--num_try) {
				vlynq_sys_wr_reg(local_swap,
						 base + VLYNQ_STAT,
						 val | 0x180);
				goto try_source_default_link;
			}

			num_try = 3;
			if (--num_reset)
				goto reset_source_default_link;

			return (VLYNQ_OK);	/* Yes, we have the link. */
		}
		/* Not yet, let us try getting the link at a lower frequency. */
	}

	return (VLYNQ_INTERNAL_ERROR);
}

int vlynq_detect_peer(u32 base, u8 endian_swap, u32 *rev_word, u32 *cnt_word)
{
	if (vlynq_enable_HW_default_clock(base, endian_swap, endian_swap))
		return (VLYNQ_INTERNAL_ERROR);

	*cnt_word = vlynq_sys_rd_reg(endian_swap, base + VLYNQ_CNTL);

	base += 0x80;
	*rev_word = vlynq_sys_rd_reg(endian_swap, base + VLYNQ_CHIPVER);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_detect_peer);

/*-----------------------------------------------------------------------------
 * VLYNQ portal Detect Logic Ends.
 *---------------------------------------------------------------------------*/

int vlynq_init(void)
{
	/* SSI, need to fill up. */
	vlynq_isr_info_init();
	vlynq_irq_info_init();
	vlynq_irq_map_init();
	vlynq_irq_swap_fixup_list_init();
	vlynq_misc_init();

	vlynq_dev_init();

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_init);

static int vlynq_add_intern_isr(struct vlynq_module *p_root_vlynq, u8 irq,
				struct vlynq_isr_info *p_isr_info)
{
	struct vlynq_irq_info_t *p_irq_info = p_root_vlynq->root_irq_info + irq;

	p_isr_info->next = p_irq_info->isr_info;
	p_irq_info->isr_info = p_isr_info;

	return (VLYNQ_OK);
}

static int vlynq_remove_intern_isr(struct vlynq_module *p_root_vlynq, u8 irq,
				   struct vlynq_isr_info *isr_info)
{
	struct vlynq_irq_info_t *p_irq_info = p_root_vlynq->root_irq_info + irq;
	struct vlynq_isr_info *p_isr_info = p_irq_info->isr_info;
	struct vlynq_isr_info *p_last_isr_info = NULL;
	int ret_val = VLYNQ_INTERNAL_ERROR;

	for (p_isr_info = p_irq_info->isr_info;
	     p_isr_info != NULL;
	     p_last_isr_info = p_isr_info, p_isr_info = p_isr_info->next) {
		if (p_isr_info == isr_info) {
			if (!p_last_isr_info)
				p_irq_info->isr_info = p_last_isr_info;
			else
				p_last_isr_info->next = p_isr_info->next;

			ret_val = VLYNQ_OK;
			break;
		}
	}

	return (ret_val);
}

int vlynq_add_device(void *vlynq, void *vlynq_dev, u8 peer)
{
	struct vlynq_module *p_vlynq = (struct vlynq_module *) vlynq;
	int dev_count = MAX_DEV_PER_VLYNQ;
	int ret_val = VLYNQ_INTERNAL_ERROR;
	void **pp_vlynq_dev;

	if (!vlynq || !vlynq_dev)
		return (VLYNQ_INVALID_PARAM);

	pp_vlynq_dev = (void **)
	    (peer ? &p_vlynq->peer_dev : &p_vlynq->local_dev);

	while (dev_count--) {
		if (!(*pp_vlynq_dev)) {
			*pp_vlynq_dev = (void *) vlynq_dev;
			ret_val = VLYNQ_OK;
			break;
		} else
			pp_vlynq_dev++;
	}

	return (ret_val);
}
EXPORT_SYMBOL(vlynq_add_device);

int vlynq_remove_device(void *vlynq, void *vlynq_dev)
{
	struct vlynq_module *p_vlynq = (struct vlynq_module *) vlynq;
	int ret_val = VLYNQ_OK;
	u8 peer;
	int index;

	void **pp_vlynq_dev;

	if (!vlynq || !vlynq_dev)
		return (VLYNQ_INVALID_PARAM);

	if (vlynq_find_dev_index(p_vlynq, vlynq_dev, &peer, &index))
		return (VLYNQ_INTERNAL_ERROR);

	pp_vlynq_dev = (void **)
	    (peer ? &p_vlynq->peer_dev : &p_vlynq->local_dev);

	pp_vlynq_dev[index] = NULL;

	return (ret_val);
}
EXPORT_SYMBOL(vlynq_remove_device);

int vlynq_unmap_region(void *vlynq, u8 remote, u32 region_id,
		       void *vlynq_dev)
{
	void *p_vlynq_dev = vlynq_dev;
	struct vlynq_module *p_vlynq = vlynq;

	u8 peer;
	u32 index;
	struct vlynq_region_info *p_region, *p_local_region, *p_remote_region;
	u32 base;
	u32 p_remote_reg, p_local_reg;

	if (!p_vlynq_dev || !p_vlynq)
		return (VLYNQ_INVALID_PARAM);

	if (vlynq_find_dev_index(p_vlynq, p_vlynq_dev, &peer, &index))
		return (VLYNQ_INTERNAL_ERROR);

	if (region_id > 3)
		return (VLYNQ_INVALID_PARAM);

	p_local_region = p_vlynq->local_region_info + region_id;
	p_remote_region = p_vlynq->remote_region_info + region_id;

	p_local_reg = LOCAL_REGS(p_vlynq);
	p_remote_reg = PEER_REGS(p_vlynq);

	/*
	 * local vlynq <----------------bus/remoteness--------------> peer vlynq
	 *
	 * Interpretation has to be made in the perspective of the device and
	 * its location i.e. association of the device with the local vlynq or
	 * peer vlynq.
	 *
	 * A true value for 'remote' means that a device wants to export the
	 * local memory to other devices, otherwise, the device intends to
	 * import portion of external memory into it.
	 *
	 * A device connected to 'peer' vlynq exports memory region by
	 * programming peer vlynq rx registers and imports memory region by
	 * programming the rx registers of local vlynq.
	 *
	 * A device connected to local vlynq exports memory region by
	 * programming local vlynq rx registers and imports memory region by
	 * programming the rx registers of peer vlynq.
	 *
	 * Complex eh ? Yep, seems so.
	 */
	if (peer) {
		p_region = remote ? p_remote_region : p_local_region;
		base = remote ? p_remote_reg : p_local_reg;

	} else {
		p_region = remote ? p_local_region : p_remote_region;
		base = remote ? p_local_reg : p_remote_reg;

	}

	if (p_region->owner_dev_index == -1)
		return (VLYNQ_INTERNAL_ERROR);	/* Region Not allocated. */

	vlynq_sys_wr_reg(0, base + VLYNQ_RXSIZE(region_id), 0);
	vlynq_sys_wr_reg(0, base + VLYNQ_RXOFFSET(region_id), 0);

	p_region->owner_dev_locale = peer;
	p_region->owner_dev_index = -1;

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_unmap_region);

int vlynq_map_region(void *vlynq, u8 remote, u32 region_id,
		     u32 region_offset, u32 region_size,
		     void *vlynq_dev)
{
	void *p_vlynq_dev = vlynq_dev;
	struct vlynq_module *p_vlynq = vlynq;

	u8 peer;
	u32 index;
	struct vlynq_region_info *p_region, *p_local_region, *p_remote_region;
	u32 base;
	u32 p_remote_reg, p_local_reg;
	u8 peer_reg_flag;

	if (!p_vlynq_dev || !p_vlynq)
		return (VLYNQ_INVALID_PARAM);

	if (vlynq_find_dev_index(p_vlynq, p_vlynq_dev, &peer, &index))
		return (VLYNQ_INTERNAL_ERROR);

	if (region_id > 3)
		return (VLYNQ_INVALID_PARAM);

	p_local_region = p_vlynq->local_region_info + region_id;
	p_remote_region = p_vlynq->remote_region_info + region_id;

	p_local_reg = LOCAL_REGS(p_vlynq);
	p_remote_reg = PEER_REGS(p_vlynq);

	/*
	 * local vlynq <----------------bus/remoteness--------------> peer vlynq
	 *
	 * Interpretation has to be made in the perspective of the device and
	 * its location i.e. association of the device with the local vlynq or
	 * peer vlynq.
	 *
	 * A true value for 'remote' means that a device wants to export the
	 * local memory to other devices, otherwise, the device intends to
	 * import portion of external memory into it.
	 *
	 * A device connected to 'peer' vlynq exports memory region by
	 * programming peer vlynq rx registers and imports memory region by
	 * programming the rx registers of local vlynq.
	 *
	 * A device connected to local vlynq exports memory region by
	 * programming local vlynq rx registers and imports memory region by
	 * programming the rx registers of peer vlynq.
	 *
	 * Complex eh ? Yep, seems so.
	 */
	if (peer) {
		p_region = remote ? p_remote_region : p_local_region;
		base = remote ? p_remote_reg : p_local_reg;
		peer_reg_flag = remote ? 1u : 0u;
	} else {
		p_region = remote ? p_local_region : p_remote_region;
		base = remote ? p_local_reg : p_remote_reg;
		peer_reg_flag = remote ? 0u : 1u;
	}

	if (p_region->owner_dev_index != -1)
		return (VLYNQ_INTERNAL_ERROR);	/* Region already allocated. */

	vlynq_wr_reg(p_vlynq, peer_reg_flag, base + VLYNQ_RXSIZE(region_id),
		     region_size);
	vlynq_wr_reg(p_vlynq, peer_reg_flag, base + VLYNQ_RXOFFSET(region_id),
		     region_offset);

	p_region->owner_dev_locale = peer;
	p_region->owner_dev_index = index;

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_map_region);

static int vlynq_calculate_base_addr(struct vlynq_module *p_vlynq, u32 offset,
				     u32 *base_addr, u32 index)
{
	u32 p_local_reg = LOCAL_REGS(p_vlynq);
	u32 base = PEER_REGS(p_vlynq);
	u32 i, offset_diff, size = 0;
	struct vlynq_region_info *p_remote_region = p_vlynq->remote_region_info;

	for (i = 0; i < MAX_VLYNQ_REGION; i++) {

		offset_diff = offset -
			vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_RXOFFSET(i));

		if (offset_diff < 0)
			return (VLYNQ_INTERNAL_ERROR);

		if (offset_diff <=
		    vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_RXSIZE(i))) {
			*base_addr =
			    offset_diff + size + vlynq_rd_reg(p_vlynq, 0u,
							      p_local_reg +
							      VLYNQ_TXMAP);
			break;
		}

		size += vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_RXSIZE(i));

		p_remote_region++;
	}

	return (VLYNQ_OK);
}

int vlynq_get_dev_base(void *vlynq,
		       u32 offset, u32 *base_addr, void *vlynq_dev)
{
	void *p_vlynq_dev = vlynq_dev;
	struct vlynq_module *p_vlynq = vlynq;
	u32 index;
	u8 peer;
	int ret_val = 0;

	if (!p_vlynq || !p_vlynq_dev)
		return (VLYNQ_INVALID_PARAM);

	if (vlynq_find_dev_index(p_vlynq, p_vlynq_dev, &peer, &index))
		return (VLYNQ_INTERNAL_ERROR);

	/*
	 * Not sure why would one require to calculate base address for the
	 * device which is not associated with the peer VLYNQ.
	 */
#if 1
	if (!peer)
		return (VLYNQ_INTERNAL_ERROR);
#endif

	/* Let us now calculate the base address back up to the root. */
	while (p_vlynq) {
		ret_val =
		    vlynq_calculate_base_addr(p_vlynq, offset, base_addr,
					      index);
		if (ret_val)
			break;

		offset = *base_addr;
		p_vlynq = p_vlynq->prev;
	}

	return (ret_val);
}
EXPORT_SYMBOL(vlynq_get_dev_base);

/******************************************************************************
 * VLYNQ Configuration API(s): Private ones.
 *****************************************************************************/
static int vlynq_validate_param_and_find_ivr_info(struct vlynq_module *p_vlynq,
						  u32 irq,
						  u32 *p_reg,
						  u8 *p_hw_intr_line)
{
	struct vlynq_irq_map_t *p_irq_map;
	u32 base;

	if (!p_vlynq)
		return (VLYNQ_INVALID_PARAM);

	if (irq > 31)
		return (VLYNQ_INVALID_PARAM);

	p_irq_map = vlynq_get_map_for_irq(p_vlynq, irq);
	if (!p_irq_map)
		return (VLYNQ_INTERNAL_ERROR);

	base =
	    (p_irq_map->peer == 1u) ? PEER_REGS(p_vlynq) : LOCAL_REGS(p_vlynq);
	*p_hw_intr_line = p_irq_map->hw_intr_line;

	*p_reg =
	    (p_irq_map->hw_intr_line <
	     4) ? base + VLYNQ_IVR30 : base + VLYNQ_IVR74;

	return (VLYNQ_OK);
}

static int vlynq_root_isr_local_error(struct vlynq_module *p_vlynq)
{
	u32 base = LOCAL_REGS(p_vlynq);
	void **pp_dev = p_vlynq->local_dev;
	u32 val = vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_STAT);
	int i = 0;

	val |= 0x80;
	vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_STAT, val);

	while (i++ < MAX_DEV_PER_VLYNQ)
		vlynq_dev_handle_event(*pp_dev++, VLYNQ_EVENT_LOCAL_ERROR, val);

	return (VLYNQ_OK);
}

static int vlynq_root_isr_peer_error(struct vlynq_module *p_vlynq)
{
	u32 base = LOCAL_REGS(p_vlynq);
	void **pp_dev = p_vlynq->peer_dev;
	u32 val = vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_STAT);
	int i = 0;

	val |= 0x100;
	vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_STAT, val);

	while (i++ < MAX_DEV_PER_VLYNQ)
		vlynq_dev_handle_event(*pp_dev++, VLYNQ_EVENT_PEER_ERROR, val);

	return (VLYNQ_OK);
}

static int vlynq_validate_init_params(struct vlynq_config *vlynq_cfg)
{
	if (!vlynq_cfg)
		return (VLYNQ_INVALID_PARAM);

	/* Both peers of the VLYNQ bridge can not be sourcing. */
	if (vlynq_cfg->local_clock_dir == VLYNQ_CLK_DIR_OUT &&
	    vlynq_cfg->peer_clock_dir == VLYNQ_CLK_DIR_OUT)
		return (VLYNQ_INVALID_PARAM);

	/* We can not accept zero divisors. */
	if (vlynq_cfg->local_clock_div == 0 || vlynq_cfg->peer_clock_div == 0)
		return (VLYNQ_INVALID_PARAM);

	/* This vlynq has to handle irqs locally and peer should
	 * be sending the irq to the local VLYNQ. */
	if (!vlynq_cfg->local_intr_local || vlynq_cfg->peer_intr_local)
		return (VLYNQ_INVALID_PARAM);

	/* Non-on chip VLYNQ(s) have to write to a specific
	 * register to get the interrupts to the root vlynq. */
	if (vlynq_cfg->on_soc && !vlynq_cfg->local_int2cfg)
		return (VLYNQ_INVALID_PARAM);

	/* Validate the clock divisor values. */
	if (vlynq_cfg->local_clock_div > 8 || vlynq_cfg->peer_clock_div > 8)
		return (VLYNQ_INVALID_PARAM);

	/* Validate the interrupt vector values. */
	if (vlynq_cfg->local_intr_vector > 31 ||
	    vlynq_cfg->peer_intr_vector > 31)
		return (VLYNQ_INVALID_PARAM);

	return (VLYNQ_OK);
}

static u8 vlynq_detect_link_status(struct vlynq_module *p_vlynq)
{
	u32 base = LOCAL_REGS(p_vlynq);

	return ((vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_STAT) & 0x1) ? 1u : 0u);
}

static u8 vlynq_touch_vlynq_through_chain(struct vlynq_module *p_vlynq)
{
	/*
	 * We need to go back to the root vlynq to ensure that the link exists
	 * from the root to the vlynq under consideration i.e. p_vlynq.
	 *
	 * If we find any of the intermediate links as failed, then we cannot
	 * access the p_vlynq and we return that the link is down.
	 *
	 * This shall avoid a possible system hang.
	 */

	struct vlynq_module *p_temp_vlynq = vlynq_find_root_vlynq(p_vlynq);
	u8 link_status = 0;

	while (p_temp_vlynq) {
		if (p_temp_vlynq == p_vlynq) {
			link_status = vlynq_detect_link_status(p_vlynq);
			break;
		}

		p_temp_vlynq = p_temp_vlynq->next;
	}

	return (link_status);
}

u8 vlynq_get_link_status(void *vlynq)
{
	u8 ret_val;

	ret_val =
		vlynq_touch_vlynq_through_chain((struct vlynq_module *) vlynq);

	return (ret_val);
}
EXPORT_SYMBOL(vlynq_get_link_status);

static int vlynq_config_bridge_endianness(struct vlynq_module *p_vlynq,
					  enum vlynq_endian local_endianness,
					  enum vlynq_endian peer_endianness)
{
	/* The following implementation has been hacked for 1350, the correct
	 * statement should be (... < 0x00010205) instead (... <= 0x00010206) */

	u32 base = LOCAL_REGS(p_vlynq);

	if ((vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_REV) >= 0x00010200) &&
	    (vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_REV) <= 0x00010206) &&
	    local_endianness)
		vlynq_sys_wr_reg(0, base + VLYNQ_ENDIAN, local_endianness);

	base = PEER_REGS(p_vlynq);

	if ((vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_REV) >= 0x00010200) &&
	    (vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_REV) <= 0x00010206) &&
	    peer_endianness)
		vlynq_sys_wr_reg(0, base + VLYNQ_ENDIAN, peer_endianness);

	if (local_endianness != peer_endianness)
		p_vlynq->peer_swap = (p_vlynq->peer_swap == 1u) ? 0u : 1u;

	return (VLYNQ_OK);
}

static void vlynq_wr_config_clock(struct vlynq_module *p_vlynq,
				  u32 base,
				  u8 flag,
				  enum vlynq_clock_dir clock_dir, u8 clock_div)
{
	u32 val;

	val = vlynq_rd_reg(p_vlynq, flag, base + VLYNQ_CNTL);
	val &= ~(0x1 << 15);
	val &= ~(0x7 << 16);
	val |= ((clock_div - 1) << 16) | (clock_dir << 15);
	vlynq_wr_reg(p_vlynq, flag, base + VLYNQ_CNTL, val);

	vlynq_delay_wait(0xffffff);
}

static int vlynq_config_bridge_clock(struct vlynq_module *p_vlynq,
				     enum vlynq_clock_dir local_clock_dir,
				     enum vlynq_clock_dir peer_clock_dir,
				     u8 local_clock_div, u8 peer_clock_div)
{
	u32 base;

	/* Clock configuration is hardware driven. We do some basic
	 * configuration to aid the hardware setup. Lest, the software
	 * configuration is aligned with the hardware setup, expect the
	 * the ultimate, "the hang". Read on.....
	 */

	if (local_clock_dir == VLYNQ_CLK_DIR_OUT) {
		base = LOCAL_REGS(p_vlynq);
		vlynq_wr_config_clock(p_vlynq, base, 0u,
				      local_clock_dir, local_clock_div);

		/* We must have the Link at this stage. We have to go over the
		 * link for peer clock.*/
		if (!vlynq_detect_link_status(p_vlynq)) {
			printk(KERN_INFO "VLYNQ ID from config_bridge = %x\n",
				vlynq_sys_rd_reg(0, base + VLYNQ_REV));
			printk(KERN_INFO "Link not detected 1\n");
			return (VLYNQ_INTERNAL_ERROR);
		}

		base = PEER_REGS(p_vlynq);
		vlynq_wr_config_clock(p_vlynq, base, 1u,
				      peer_clock_dir, peer_clock_div);
	} else {
		/* We must have the Link at this stage. We have to go over the
		 * link for peer clock.*/
		if (!vlynq_detect_link_status(p_vlynq))
			return (VLYNQ_INTERNAL_ERROR);

		base = PEER_REGS(p_vlynq);
		vlynq_wr_config_clock(p_vlynq, base, 1u,
				      peer_clock_dir, peer_clock_div);

		base = LOCAL_REGS(p_vlynq);
		vlynq_wr_config_clock(p_vlynq, base, 0u,
				      local_clock_dir, local_clock_div);
	}

	/* We must have the Link at this stage */
	if (!vlynq_detect_link_status(p_vlynq)) {
		printk(KERN_INFO "Link not detected 2\n");
		return (VLYNQ_INTERNAL_ERROR);
	}

	return (VLYNQ_OK);
}

static int vlynq_uninstall_intern_isr(struct vlynq_module *p_vlynq)
{
	struct vlynq_isr_info *p_isr_info;
	u32 base = PEER_REGS(p_vlynq);

	vlynq_wr_reg(p_vlynq, 1u, base + VLYNQ_CNTL,
		     vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_CNTL) & (~0x7f80));

	if (p_vlynq->peer_isr_info) {
		p_isr_info = p_vlynq->peer_isr_info;
		p_isr_info->one_of_isr_info.intern_isr_info.isr = NULL;
		p_isr_info->one_of_isr_info.intern_isr_info.vlynq = NULL;

		vlynq_free_isr_info(p_isr_info);
	}

	base = PEER_REGS(p_vlynq);

	vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_CNTL,
		     vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_CNTL) & (~0x7f80));

	if (p_vlynq->local_isr_info) {
		p_isr_info = p_vlynq->local_isr_info;
		p_isr_info->one_of_isr_info.intern_isr_info.isr = NULL;
		p_isr_info->one_of_isr_info.intern_isr_info.vlynq = NULL;

		vlynq_free_isr_info(p_isr_info);
	}

	return (VLYNQ_OK);
}

static struct vlynq_isr_info
*vlynq_config_intern_error_intr(struct vlynq_module *p_vlynq,
				u8 intr_local,
				u8 intr_enable,
				u8 intr_vec,
				u8 int2cfg,
				u32 intr_pointer,
				u8 peer)
{
	struct vlynq_isr_info *p_isr_info;
	u32 val = 0;
	u32 base = (peer == 1u) ? PEER_REGS(p_vlynq) : LOCAL_REGS(p_vlynq);

	p_isr_info = vlynq_get_free_isr_info();
	if (!p_isr_info)
		return (NULL);

	if (peer)
		p_isr_info->one_of_isr_info.intern_isr_info.isr =
		    vlynq_root_isr_peer_error;
	else
		p_isr_info->one_of_isr_info.intern_isr_info.isr =
		    vlynq_root_isr_local_error;

	p_isr_info->one_of_isr_info.intern_isr_info.vlynq = p_vlynq;
	p_isr_info->irq_type = 0x1;

	if (int2cfg)
		vlynq_wr_reg(p_vlynq, peer, base + VLYNQ_INTPTR, 0x14);
	else
		vlynq_wr_reg(p_vlynq, peer, base + VLYNQ_INTPTR, intr_pointer);

	val = vlynq_rd_reg(p_vlynq, peer, base + VLYNQ_CNTL);
	val &= ~(0x7f80);
	val |= intr_local << 14;
	val |= intr_enable << 13;
	val |= intr_vec << 8;
	val |= int2cfg << 7;
	vlynq_wr_reg(p_vlynq, peer, base + VLYNQ_CNTL, val);

	return (p_isr_info);
}

static int vlynq_install_intern_isr(struct vlynq_module *p_vlynq,
				    struct vlynq_config *p_vlynq_config)
{
	struct vlynq_isr_info *p_intern_isr = NULL;
	int ret_val = 0;

	p_intern_isr = vlynq_config_intern_error_intr(p_vlynq,
						      p_vlynq_config->
						      local_intr_local,
						      p_vlynq_config->
						      local_intr_enable,
						      p_vlynq_config->
						      local_intr_vector,
						      p_vlynq_config->
						      local_int2cfg,
						      p_vlynq_config->
						      local_intr_pointer, 0);
	if (!p_intern_isr)
		goto vlynq_install_intern_isr_local_error;

	p_vlynq->local_isr_info = p_intern_isr;
	p_vlynq->local_irq = p_vlynq_config->local_intr_vector;

	p_intern_isr = vlynq_config_intern_error_intr(p_vlynq,
						      p_vlynq_config->
						      peer_intr_local,
						      p_vlynq_config->
						      peer_intr_enable,
						      p_vlynq_config->
						      peer_intr_vector,
						      p_vlynq_config->
						      peer_int2cfg,
						      p_vlynq_config->
						      peer_intr_pointer, 1);
	if (!p_intern_isr)
		goto vlynq_install_intern_isr_peer_error;

	p_vlynq->peer_isr_info = p_intern_isr;
	p_vlynq->peer_irq = p_vlynq_config->peer_intr_vector;

	return (ret_val);

vlynq_install_intern_isr_peer_error:
	vlynq_free_isr_info(p_vlynq->local_isr_info);
	p_vlynq->local_isr_info = NULL;

vlynq_install_intern_isr_local_error:
	ret_val = VLYNQ_INTERNAL_ERROR;

	return (ret_val);
}

int vlynq_get_num_root(void)
{
	int root_count = 0, index = 0;

	while (index < MAX_ROOT_VLYNQ)
		if (root_vlynq[index++])
			root_count++;

	return (root_count);
}
EXPORT_SYMBOL(vlynq_get_num_root);

void *vlynq_get_root(int index)
{
	struct vlynq_module *p_vlynq = NULL;

	if (index < MAX_ROOT_VLYNQ)
		p_vlynq = root_vlynq[index];

	return ((void *) p_vlynq);
}
EXPORT_SYMBOL(vlynq_get_root);

void *vlynq_get_next(void *this)
{
	struct vlynq_module *p_vlynq = (struct vlynq_module *) this;

	if (p_vlynq)
		p_vlynq = p_vlynq->next;

	return ((void *) p_vlynq);
}
EXPORT_SYMBOL(vlynq_get_next);

int vlynq_is_last(void *this)
{
	struct vlynq_module *p_vlynq = (struct vlynq_module *) this;
	int ret_val = VLYNQ_INTERNAL_ERROR;

	if (p_vlynq)
		ret_val = p_vlynq->next ? 0 : 1;

	return (ret_val);
}
EXPORT_SYMBOL(vlynq_is_last);

int vlynq_get_chain_length(void *this)
{
	struct vlynq_module *p_vlynq;
	int length;

	for (p_vlynq = (struct vlynq_module *) this, length = 0;
	     p_vlynq != NULL; p_vlynq = p_vlynq->next)
		length++;

	return (this ? length : -1);
}
EXPORT_SYMBOL(vlynq_get_chain_length);

int vlynq_get_base_addr(void *vlynq, u32 *base)
{
	struct vlynq_module *p_vlynq = vlynq;

	if (!p_vlynq)
		return (VLYNQ_INVALID_PARAM);

	*base = p_vlynq->base;

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_get_base_addr);

void *vlynq_get_root_at_base(u32 base_addr)
{
	struct vlynq_module *p_ret_vlynq = NULL;
	int i;

	for (i = 0; i < MAX_ROOT_VLYNQ; i++) {
		if (root_vlynq[i]) {
			if (root_vlynq[i]->base == base_addr) {
				p_ret_vlynq = root_vlynq[i];
				break;
			}
		}
	}

	return (p_ret_vlynq);
}
EXPORT_SYMBOL(vlynq_get_root_at_base);

void *vlynq_get_root_vlynq(void *vlynq)
{
	if (vlynq)
		return (vlynq_find_root_vlynq(vlynq));

	return (NULL);
}
EXPORT_SYMBOL(vlynq_get_root_vlynq);

int vlynq_chain_append(void *this, void *to)
{
	struct vlynq_module *this_vlynq = (struct vlynq_module *) this;
	struct vlynq_module *to_vlynq = (struct vlynq_module *) to;
	struct vlynq_module *root_vlynq = NULL;
/*   struct vlynq_irq_map_t *p_irq_map; */

	if (!this_vlynq || !to_vlynq)
		return (VLYNQ_INVALID_PARAM);

	if (to_vlynq->next != NULL)
		return (VLYNQ_INVALID_PARAM);

	root_vlynq = vlynq_find_root_vlynq(to_vlynq);
	if (!root_vlynq || !root_vlynq->root_irq_info)
		return (VLYNQ_INTERNAL_ERROR);

	/* Load the local isr information into the root vlynq. */
	vlynq_add_intern_isr(root_vlynq, this_vlynq->local_irq,
			     this_vlynq->local_isr_info);

	/* Load the peer isr information into the root vlynq. */
	vlynq_add_intern_isr(root_vlynq, this_vlynq->peer_irq,
			     this_vlynq->peer_isr_info);

	to_vlynq->next = this_vlynq;
	this_vlynq->prev = to_vlynq;

	if (to_vlynq->peer_swap == 1u)
		this_vlynq->local_swap = 1u;

	vlynq_set_intern_irq_swap_fixup(this_vlynq);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_chain_append);

int vlynq_chain_unappend(void *this, void *from)
{
	struct vlynq_module *this_vlynq = (struct vlynq_module *) this;
	struct vlynq_module *from_vlynq = (struct vlynq_module *) from;
	struct vlynq_module *root_vlynq;

	if (!this_vlynq || !from_vlynq)
		return (VLYNQ_INVALID_PARAM);	/* NULL input pointers */

	if (this_vlynq->next)
		return (VLYNQ_INVALID_PARAM);

	if (from_vlynq->next != this_vlynq)
		return (VLYNQ_INTERNAL_ERROR);

	root_vlynq = vlynq_find_root_vlynq(from_vlynq);
	if (!root_vlynq || !root_vlynq->root_irq_info)
		return (VLYNQ_INTERNAL_ERROR);

	vlynq_unset_intern_irq_swap_fixup(this_vlynq);

	vlynq_remove_intern_isr(root_vlynq, this_vlynq->local_irq,
				this_vlynq->local_isr_info);

	vlynq_remove_intern_isr(root_vlynq, this_vlynq->peer_irq,
				this_vlynq->peer_isr_info);

	from_vlynq->next = this_vlynq->next;
	this_vlynq->prev = NULL;

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_chain_unappend);

int vlynq_map_irq(void *vlynq, u32 hw_intr_line,
		  u32 irq, void *vlynq_dev)
{
	void *p_vlynq_dev = (void *) vlynq_dev;
	struct vlynq_module *p_vlynq = vlynq, *p_root_vlynq;
	struct vlynq_irq_map_t *p_irq_map;
	u32 p_reg;
	u32 val;
	u32 base;
	int index;
	u8 peer;

	if (!p_vlynq_dev || !p_vlynq)
		return (VLYNQ_INVALID_PARAM);

	if (irq > 31 || hw_intr_line > 7)
		return (VLYNQ_INVALID_PARAM);

	/* Has this irq been already mapped. */
	if (vlynq_get_map_for_irq((struct vlynq_module *) vlynq, irq))
		return (VLYNQ_INTERNAL_ERROR);

	p_root_vlynq = vlynq_find_root_vlynq((struct vlynq_module *) vlynq);
	if (!p_root_vlynq || !p_root_vlynq->root_irq_map)
		return (VLYNQ_INTERNAL_ERROR);

	p_irq_map = p_root_vlynq->root_irq_map + irq;

	if (vlynq_find_dev_index(p_vlynq, p_vlynq_dev, &peer, &index))
		return (VLYNQ_INTERNAL_ERROR);

	/* Add information into the device. */
	if (vlynq_dev_ioctl(p_vlynq_dev, VLYNQ_DEV_ADD_IRQ, irq))
		return (VLYNQ_INTERNAL_ERROR);

	/* Update the VLYNQ hardware. */
	base = (peer == 1u) ? PEER_REGS(p_vlynq) : LOCAL_REGS(p_vlynq);

	p_reg = (hw_intr_line < 4) ? base + VLYNQ_IVR30 : base + VLYNQ_IVR74;

	p_irq_map->hw_intr_line = hw_intr_line;
	p_irq_map->vlynq = p_vlynq;
	p_irq_map->peer = peer;

	hw_intr_line = VLYNQ_IVEC_HW_LINE_ADJUST(hw_intr_line);

	val = vlynq_rd_reg(p_vlynq, peer, p_reg);
	val &= ~VLYNQ_IVEC_IRQ_MASK(hw_intr_line);
	val |= VLYNQ_IVEC_IRQ_VAL(irq) << VLYNQ_IVEC_OFFSET(hw_intr_line);
	vlynq_wr_reg(p_vlynq, peer, p_reg, val);

	vlynq_set_irq_swap_fixup(p_vlynq, peer, irq,
				 p_root_vlynq->irq_swap_map);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_map_irq);

int vlynq_unmap_irq(void *vlynq, u32 irq, void *vlynq_dev)
{
	void *p_vlynq_dev = (void *) vlynq_dev;
	struct vlynq_module *p_vlynq = (struct vlynq_module *) vlynq;
	struct vlynq_irq_map_t *p_irq_map;
	struct vlynq_module *p_root_vlynq = NULL;
	int index;
	u8 peer;

	if (!p_vlynq_dev || !p_vlynq)
		return (VLYNQ_INVALID_PARAM);

	if (irq > 31)
		return (VLYNQ_INVALID_PARAM);

	p_irq_map = vlynq_get_map_for_irq(p_vlynq, irq);
	if (!p_irq_map)
		return (VLYNQ_INTERNAL_ERROR);

	/* Was it mapped by this VLYNQ. */
	if (p_irq_map->vlynq != vlynq)
		return (VLYNQ_INTERNAL_ERROR);

	p_root_vlynq = vlynq_find_root_vlynq(p_vlynq);
	if (!p_root_vlynq)
		return (VLYNQ_INTERNAL_ERROR);

	/* Still have ISR attached. */
	if ((p_root_vlynq->root_irq_info[irq].isr_info))
		return (VLYNQ_INTERNAL_ERROR);

	if (vlynq_find_dev_index(p_vlynq, p_vlynq_dev, &peer, &index))
		return (VLYNQ_INTERNAL_ERROR);

	/* Add information into the device. */
	if (vlynq_dev_ioctl(p_vlynq_dev, VLYNQ_DEV_REMOVE_IRQ, irq))
		return (VLYNQ_INTERNAL_ERROR);

	p_irq_map->hw_intr_line = -1;
	p_irq_map->vlynq = NULL;

	vlynq_unset_irq_swap_fixup(p_vlynq, peer, irq,
				   p_root_vlynq->irq_swap_map);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_unmap_irq);

int vlynq_add_isr(void *vlynq, u32 irq,
		  void (*dev_isr) (void *arg1),
		  struct vlynq_dev_isr_param_grp_t *isr_param)
{
	struct vlynq_module *p_root_vlynq;
	struct vlynq_isr_info *p_isr_info;
	unsigned long flags;

	if (!vlynq || !dev_isr || !isr_param)
		return (VLYNQ_INVALID_PARAM);

	if (irq > 31)
		return (VLYNQ_INVALID_PARAM);

	local_irq_save(flags);

	/* Ascertain whether the IRQ has been already mapped. */
	if (!vlynq_get_map_for_irq((struct vlynq_module *) vlynq, irq)) {
		local_irq_restore(flags);
		return (VLYNQ_INTERNAL_ERROR);
	}

	p_root_vlynq = vlynq_find_root_vlynq((struct vlynq_module *) vlynq);
	if (!p_root_vlynq || !p_root_vlynq->root_irq_info) {
		local_irq_restore(flags);
		return (VLYNQ_INTERNAL_ERROR);
	}

	/* Get a place holder for ISR information. */
	p_isr_info = vlynq_get_free_isr_info();
	if (!p_isr_info) {
		local_irq_restore(flags);
		return (VLYNQ_INTERNAL_ERROR);
	}

	/* House keeping; you see. */
	p_isr_info->one_of_isr_info.dev_isr_info.isr = dev_isr;
	p_isr_info->one_of_isr_info.dev_isr_info.isr_param = isr_param;
	p_isr_info->irq_type = 0x0;

	p_isr_info->next = p_root_vlynq->root_irq_info[irq].isr_info;
	p_root_vlynq->root_irq_info[irq].isr_info = p_isr_info;

	local_irq_restore(flags);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_add_isr);

int vlynq_remove_isr(void *vlynq, u32 irq,
		     struct vlynq_dev_isr_param_grp_t *isr_param)
{
	struct vlynq_module *p_root_vlynq;
	struct vlynq_isr_info *p_isr_info, *p_last_isr_info;
	unsigned long flags;

	if (!vlynq || !isr_param)
		return (VLYNQ_INVALID_PARAM);

	if (irq > 31)
		return (VLYNQ_INVALID_PARAM);

	local_irq_save(flags);

	p_root_vlynq = vlynq_find_root_vlynq((struct vlynq_module *) vlynq);
	if (!p_root_vlynq || !p_root_vlynq->root_irq_info) {
		local_irq_restore(flags);
		return (VLYNQ_INTERNAL_ERROR);
	}

	p_isr_info = p_root_vlynq->root_irq_info[irq].isr_info;

	for (p_last_isr_info = NULL;
	     p_isr_info != NULL;
	     p_last_isr_info = p_isr_info, p_isr_info = p_isr_info->next) {
		struct vlynq_dev_isr_info *p_dev_isr_info =
		    (struct vlynq_dev_isr_info *) p_isr_info;

		if (p_dev_isr_info->isr_param == isr_param) {
			if (!p_last_isr_info)
				p_root_vlynq->root_irq_info[irq].isr_info =
				    NULL;
			else
				p_last_isr_info->next = p_isr_info->next;

			vlynq_free_isr_info(p_isr_info);

			local_irq_restore(flags);

			return (VLYNQ_OK);
		}
	}

	local_irq_restore(flags);

	return (VLYNQ_INTERNAL_ERROR);
}
EXPORT_SYMBOL(vlynq_remove_isr);

int vlynq_root_isr(void *vlynq)
{
	int count, irq = 0;

	struct vlynq_module *p_vlynq = (struct vlynq_module *) vlynq;
	u32 base = LOCAL_REGS(p_vlynq);
	u32 irq_status_map = vlynq_sys_rd_reg(0, base + VLYNQ_INTSTAT);
	struct vlynq_isr_info *p_vlynq_isr_info;
	struct vlynq_irq_info_t *p_vlynq_irq_info;
	struct vlynq_dev_isr_info *p_dev_isr_info;
	struct vlynq_intern_isr_info *p_intern_isr_info;

	/* No more than 32 consecutive interrupts for VLYNQ; lest it thrashes
	 * the system; safety measures you see.
	 */
	for (count = 0; count < 32; count++) {
		if (vlynq_sys_rd_reg(0, base + VLYNQ_REV) >= 0x00010200) {
			u32 val = vlynq_sys_rd_reg(0, base + VLYNQ_INTPRI);

			irq = val & 0x1f;

			if (val & 0x80000000)
				break;
		} else {
			u8 val = irq_status_map & 1;
			if (!irq_status_map)
				break;

			irq_status_map >>= 1;
			if (val)
				irq = count;
			else
				continue;
		}

		vlynq_sys_set_reg(0, base + VLYNQ_INTSTAT,  (1 << irq));

		if (p_vlynq->irq_swap_map[irq] == 1u)
			irq = irq_val_swap[irq];

		p_vlynq_irq_info = p_vlynq->root_irq_info + irq;
		p_vlynq_isr_info = p_vlynq_irq_info->isr_info;

		p_vlynq_irq_info->count++;

		while (p_vlynq_isr_info) {
			p_dev_isr_info =
			    (struct vlynq_dev_isr_info *) p_vlynq_isr_info;
			p_intern_isr_info =
			    (struct vlynq_intern_isr_info *) p_vlynq_isr_info;

			if (!p_vlynq_isr_info->irq_type)
				p_dev_isr_info->isr(VLYNQ_DEV_ISR_SIGNATURE
						    (p_dev_isr_info->
						     isr_param));
			else
				p_intern_isr_info->isr(p_intern_isr_info->
						       vlynq);

			p_vlynq_isr_info = p_vlynq_isr_info->next;
		}
	}

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_root_isr);

int vlynq_get_irq_count(void *vlynq, u32 irq, u32 *count)
{
	struct vlynq_module *p_root_vlynq;
	struct vlynq_irq_info_t *p_irq_info;

	if (!vlynq)
		return (VLYNQ_INVALID_PARAM);

	if (irq > 31)
		return (VLYNQ_INVALID_PARAM);

	p_root_vlynq = vlynq_find_root_vlynq((struct vlynq_module *) vlynq);
	if (!p_root_vlynq || !p_root_vlynq->root_irq_info)
		return (VLYNQ_INTERNAL_ERROR);

	p_irq_info = p_root_vlynq->root_irq_info + irq;
	*count = p_irq_info->count;

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_get_irq_count);

void *vlynq_get_for_irq(void *root, u32 irq)
{
	struct vlynq_irq_map_t *p_irq_map = NULL;

	if (!root || irq > 31)
		return (NULL);

	p_irq_map = vlynq_get_map_for_irq(root, irq);

	return (p_irq_map ? p_irq_map->vlynq : NULL);
}
EXPORT_SYMBOL(vlynq_get_for_irq);

int vlynq_set_irq_pol(void *vlynq, u32 irq, enum vlynq_irq_pol pol)
{
	s8 hw_intr_line;
	u32 p_reg;
	int ret_val;
	u32 val;

	ret_val = vlynq_validate_param_and_find_ivr_info(
						(struct vlynq_module *) vlynq,
						irq, &p_reg,
						&hw_intr_line);
	if (ret_val)
		return (ret_val);

	hw_intr_line = VLYNQ_IVEC_HW_LINE_ADJUST(hw_intr_line);

	val = vlynq_rd_reg((struct vlynq_module *) vlynq, 1u, p_reg);
	val &= ~VLYNQ_IVEC_POL_MASK(hw_intr_line);
	val |= VLYNQ_IVEC_POL_VAL(pol) << VLYNQ_IVEC_OFFSET(hw_intr_line);
	vlynq_wr_reg((struct vlynq_module *) vlynq, 1u, p_reg, val);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_set_irq_pol);

int vlynq_get_irq_pol(void *vlynq, u32 irq, enum vlynq_irq_pol *pol)
{
	s8 hw_intr_line;
	u32 p_reg;
	int ret_val;
	u32 val;

	ret_val = vlynq_validate_param_and_find_ivr_info(
						(struct vlynq_module *) vlynq,
						irq, &p_reg,
						&hw_intr_line);
	if (ret_val)
		return (ret_val);

	hw_intr_line = VLYNQ_IVEC_HW_LINE_ADJUST(hw_intr_line);

	val = vlynq_rd_reg((struct vlynq_module *) vlynq, 1u, p_reg);
	*pol = val & VLYNQ_IVEC_POL_MASK(hw_intr_line);
	*pol = (*pol) >> VLYNQ_IVEC_OFFSET(hw_intr_line);
	*pol = VLYNQ_IVEC_POL_DE_VAL(*pol);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_get_irq_pol);

int vlynq_set_irq_type(void *vlynq, u32 irq, enum vlynq_irq_type type)
{
	s8 hw_intr_line;
	u32 p_reg;
	int ret_val;
	u32 val;

	ret_val = vlynq_validate_param_and_find_ivr_info(
						(struct vlynq_module *) vlynq,
						irq, &p_reg,
						&hw_intr_line);
	if (ret_val)
		return (ret_val);

	hw_intr_line = VLYNQ_IVEC_HW_LINE_ADJUST(hw_intr_line);

	val = vlynq_rd_reg((struct vlynq_module *) vlynq, 1u, p_reg);
	val &= ~(VLYNQ_IVEC_TYPE_MASK(hw_intr_line));
	val |= VLYNQ_IVEC_TYPE_VAL(type) << VLYNQ_IVEC_OFFSET(hw_intr_line);
	vlynq_wr_reg((struct vlynq_module *) vlynq, 1u, p_reg, val);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_set_irq_type);

int vlynq_get_irq_type(void *vlynq, u32 irq, enum vlynq_irq_type *type)
{
	s8 hw_intr_line;
	u32 p_reg;
	int ret_val;
	u32 val;

	ret_val = vlynq_validate_param_and_find_ivr_info(
						(struct vlynq_module *) vlynq,
						irq, &p_reg,
						&hw_intr_line);
	if (ret_val)
		return (ret_val);

	hw_intr_line = VLYNQ_IVEC_HW_LINE_ADJUST(hw_intr_line);

	val = vlynq_rd_reg((struct vlynq_module *) vlynq, 1u, p_reg);
	*type = val & VLYNQ_IVEC_TYPE_MASK(hw_intr_line);
	*type = (*type) >> VLYNQ_IVEC_OFFSET(hw_intr_line);
	*type = VLYNQ_IVEC_TYPE_DE_VAL(*type);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_get_irq_type);

int vlynq_disable_irq(void *vlynq, u32 irq)
{
	s8 hw_intr_line;
	u32 p_reg;
	int ret_val;
	u32 val;

	ret_val = vlynq_validate_param_and_find_ivr_info(
						(struct vlynq_module *) vlynq,
						irq, &p_reg,
						&hw_intr_line);
	if (ret_val)
		return (ret_val);

	hw_intr_line = VLYNQ_IVEC_HW_LINE_ADJUST(hw_intr_line);

	val = vlynq_rd_reg((struct vlynq_module *) vlynq, 1u, p_reg);
	val &= ~(VLYNQ_IVEC_EN_MASK(hw_intr_line));
	vlynq_wr_reg((struct vlynq_module *) vlynq, 1u, p_reg, val);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_disable_irq);

int vlynq_enable_irq(void *vlynq, u32 irq)
{
	s8 hw_intr_line;
	u32 p_reg;
	int ret_val;
	u32 val;

	ret_val = vlynq_validate_param_and_find_ivr_info(
						(struct vlynq_module *) vlynq,
						irq, &p_reg,
						&hw_intr_line);
	if (ret_val)
		return (ret_val);

	hw_intr_line = VLYNQ_IVEC_HW_LINE_ADJUST(hw_intr_line);

	val = vlynq_rd_reg((struct vlynq_module *) vlynq, 1u, p_reg);
	val |= VLYNQ_IVEC_EN_VAL(1) << VLYNQ_IVEC_OFFSET(hw_intr_line);
	vlynq_wr_reg((struct vlynq_module *) vlynq, 1u, p_reg, val);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_enable_irq);

static void vlynq_soft_reset(struct vlynq_module *p_vlynq)
{
	u32 base = LOCAL_REGS(p_vlynq);

	if (vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_REV) >= 0x00010200) {
		vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_CNTL,
			    vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_CNTL) | 0x1);

		/* Provide sufficient time for reset. Refer 2.4.2 of 2.6 VLYNQ
		 *  specifications. */
		vlynq_delay_wait(0xffffff);

		vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_CNTL,
			     vlynq_rd_reg(p_vlynq, 0u,
					  base + VLYNQ_CNTL) & (~0x1));
	}
}

static int vlynq_config_RTM(struct vlynq_module *p_vlynq,
			    u8 peer,
			    enum vlynq_rtm_cfg rtm_cfg_type,
			    u8 rtm_sample_value, u8 tx_fast_path)
{
	/* Let us get on with the RTM configuration first. We will follow it
	   up with the TX Fast path configuration. */

	/* Refer to Section 2.4.2 and 4.3.4 of 2.6 VLYNQ specifications for
	 * clarifications on the configuration aspects of the RTM and Fast Path.
	 * It seems there is a window in the hardware state machine, under
	 * which the configuration of the RTM and Fast Path can cause hang. It
	 * is recommended in the specifications not to set the values when link
	 * or auto negotiation is going on the VLYNQ bridge.
	 *
	 * We should be here once, the link has been established after
	 * sufficient reset time, It is still ok to configure the local VLYNQ
	 * because we can configure it and check for the link. For the peer
	 * VLYNQ, we must have the link to configure on the peer RTM and if link
	 * goes down because of the RTM configuration on the peer VLYNQ, then be
	 *  ready to emberace the ultimate "the hang".
	 */
	u32 val;
	u32 base = (peer == 1u) ? LOCAL_REGS(p_vlynq) : PEER_REGS(p_vlynq);

#define RTM_ENABLE_BIT         0x1

	if (rtm_cfg_type == VLYNQ_RTM_AUTO_SELECT_SAMPLE_VAL) {
		val = vlynq_rd_reg(p_vlynq, peer, base + VLYNQ_CNTL);
		val |= (RTM_ENABLE_BIT << 22);
		vlynq_wr_reg(p_vlynq, peer, base + VLYNQ_CNTL, val);
	} else if (rtm_cfg_type == VLYNQ_RTM_FORCE_SAMPLE_VAL) {
		if (rtm_sample_value > 0x7)
			return (VLYNQ_INVALID_PARAM);

#define RTM_SAMPLE_VAL_MASK    0x7

		/* Clear RTM enable bit & RTM sample value */
		val = vlynq_rd_reg(p_vlynq, peer, base + VLYNQ_CNTL);
		val &= ~((RTM_ENABLE_BIT << 22) | (RTM_SAMPLE_VAL_MASK << 24));

#define RTM_FORCE_VAL_WR       0x1

		/* Populate the new sample value. */
		val |= (RTM_FORCE_VAL_WR << 23) | (rtm_sample_value << 24);
		vlynq_wr_reg(p_vlynq, peer, base + VLYNQ_CNTL, val);
	} else {
		;  /* Nothing for now. We did not mess the RTM configuration. */
	}

#define TX_FAST_PATH       0x1

	val = vlynq_rd_reg(p_vlynq, peer, base + VLYNQ_CNTL);
	val &= ~(TX_FAST_PATH << 21);	/* Clear the value of the fast path */
	val |= (tx_fast_path << 21);	/* Set the new value. */
	vlynq_wr_reg(p_vlynq, peer, base + VLYNQ_CNTL, val);

	return (VLYNQ_OK);
}

static int vlynq_config_bridge_RTM(struct vlynq_module *p_vlynq,
				   enum vlynq_rtm_cfg local_rtm_cfg_type,
				   u8 local_sample_value,
				   u8 local_tx_fast_path,
				   enum vlynq_rtm_cfg peer_rtm_cfg_type,
				   u8 peer_sample_value, u8 peer_tx_fast_path)
{
	int ret_val = VLYNQ_OK;
	u32 base = LOCAL_REGS(p_vlynq);

#define RTM_STATUS_BIT    0x1

	if ((vlynq_sys_rd_reg(0, base + VLYNQ_REV) >= 0x00010205) &&
	    (vlynq_sys_rd_reg(0, base + VLYNQ_STAT) & (RTM_STATUS_BIT << 11))) {
		ret_val =
		    vlynq_config_RTM(p_vlynq, 0u, local_rtm_cfg_type,
				     local_sample_value, local_tx_fast_path);

		if (ret_val != VLYNQ_OK)
			return (ret_val);

		/* Provide time for the bus to settle down. */
		vlynq_delay_wait(0xffffff);
	}

	/* We must have the Link at this stage after we played with the RTM. */
	if (!vlynq_detect_link_status(p_vlynq))
		return (VLYNQ_INTERNAL_ERROR);

	base = PEER_REGS(p_vlynq);

	if ((vlynq_sys_rd_reg(0, base + VLYNQ_REV) >= 0x00010205) &&
	    (vlynq_sys_rd_reg(0, base + VLYNQ_STAT) & (RTM_STATUS_BIT << 11))) {
		ret_val =
		    vlynq_config_RTM(p_vlynq, 1u, peer_rtm_cfg_type,
				     peer_sample_value, peer_tx_fast_path);

		/* Provide time for the bus to settle down. */
		vlynq_delay_wait(0xffffff);
	}

	return (ret_val);
}

int vlynq_config_clock(void *p_vlynq,
		       enum vlynq_clock_dir local_clock_dir,
		       enum vlynq_clock_dir peer_clock_dir,
		       u8 local_clock_div, u8 peer_clock_div)
{
	int ret_val;

	if (!p_vlynq)
		return (VLYNQ_INVALID_PARAM);

	/* Valid clock directions: Either of the peers to source the clock or
	 * both of the peers to sink in external board clock.
	 */
	if (local_clock_dir == VLYNQ_CLK_DIR_OUT &&
	    peer_clock_dir == VLYNQ_CLK_DIR_OUT)
		return (VLYNQ_INVALID_PARAM);

	if ((local_clock_div == 0) || (peer_clock_div == 0))
		return (VLYNQ_INVALID_PARAM);

	if ((local_clock_div > 8) || (peer_clock_div > 8))
		return (VLYNQ_INVALID_PARAM);

	ret_val =
	    vlynq_config_bridge_clock(p_vlynq, local_clock_dir, peer_clock_dir,
				      local_clock_div, peer_clock_div);

	return (ret_val);
}
EXPORT_SYMBOL(vlynq_config_clock);

void *vlynq_init_soc(struct vlynq_config *vlynq_cfg)
{
	struct vlynq_module *p_vlynq = NULL;
	u32 base;
	struct vlynq_irq_map_t *p_root_irq_map = NULL;
	struct vlynq_irq_info_t *p_root_irq_info = NULL;
	u8 *p_irq_swap_map = NULL;
	u32 val = 0;

	if (vlynq_validate_init_params(vlynq_cfg)) {
		vlynq_cfg->error_status = VLYNQ_INIT_INVALID_PARAM;
		strcpy(vlynq_cfg->error_msg, "Invalid init params.");
		return (NULL);
	}

	/* We are done with the param validation and are
	 * set for configuring the hardware. Here we go...*/
	p_vlynq = vlynq_get_free_vlynq();
	if (!p_vlynq) {
		vlynq_cfg->error_status = VLYNQ_INIT_NO_MEM;
		strcpy(vlynq_cfg->error_msg, "Mem alloc failed for VLYNQ.");
		return (NULL);
	}

	/* Let us give some more time for the VLYNQ to settle down after the
	 * hardware reset.
	 */
	vlynq_delay_wait(0xffff);

	p_vlynq->base = vlynq_cfg->base_addr;

	if (vlynq_cfg->init_swap_flag) {
		p_vlynq->local_swap = 1u;
		p_vlynq->peer_swap = 1u;
	}

	/* Put the vlynq in reset, if the version is more than
	 * 2.0 for more than 256 serial cycles. Finally bring it out
	 * of reset. */
	vlynq_soft_reset(p_vlynq);

	/* Provide sufficient time for reset to settle down. Refer 2.4.2 of
	 * 2.6 VLYNQ specifications. */
	vlynq_delay_wait(0xffffff);

	/* Do we have the link on the VLYNQ bridge on reset. We must have the
	 * link on reset. It indicates that the hw state machine has  settled
	 * down and hw clocks are working fine. This is must when we are sink
	 * ing the clock. */
	if ((vlynq_cfg->local_clock_dir == VLYNQ_CLK_DIR_IN) &&
	    !vlynq_detect_link_status(p_vlynq)) {
		vlynq_cfg->error_status = VLYNQ_INIT_NO_LINK_ON_RESET;
		strcpy(vlynq_cfg->error_msg, "No link detected on reset.");
		goto pal_vlynq_init_link_error_1;
	}

	if (vlynq_config_bridge_clock(p_vlynq,
				      vlynq_cfg->local_clock_dir,
				      vlynq_cfg->peer_clock_dir,
				      vlynq_cfg->local_clock_div,
				      vlynq_cfg->peer_clock_div)) {

		vlynq_cfg->error_status = VLYNQ_INIT_CLK_CFG;
		strcpy(vlynq_cfg->error_msg, "Error in configuring clocks.");
		goto pal_vlynq_init_clock_config_error;
	}

	/* Let us wait for the Link to settle down. */
	vlynq_delay_wait(0xffffff);

	/* We have just updated the clock, let us check the health of the link.
	 * We must have the link, if not it is over for us. */
	if (!vlynq_detect_link_status(p_vlynq)) {
		vlynq_cfg->error_status = VLYNQ_INIT_NO_LINK_ON_CLK_CFG;
		strcpy(vlynq_cfg->error_msg,
		       "No link detected post clk config.");
		goto pal_vlynq_init_link_error_2;
	}

	/* We must clear the spur interrupts. */
	base = PEER_REGS(p_vlynq);
	vlynq_wr_reg(p_vlynq, 1u, base + VLYNQ_STAT,
		     vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_STAT) | 0x180);

	base = LOCAL_REGS(p_vlynq);
	vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_STAT,
		     vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_STAT) | 0x180);

	/* Are we equipped enough to support the VLYNQ bridge ? */
	base = PEER_REGS(p_vlynq);
	if (vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_REV) > LATEST_VLYNQ_REV) {
		vlynq_cfg->error_status = VLYNQ_INIT_PEER_HIGH_REV;
		strcpy(vlynq_cfg->error_msg,
		       "Peer VLYNQ revision higher than SW.");
		goto pal_vlynq_init_peer_rev_error;
	}

	base = LOCAL_REGS(p_vlynq);
	if (vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_REV) > LATEST_VLYNQ_REV) {
		vlynq_cfg->error_status = VLYNQ_INIT_LOCAL_HIGH_REV;
		strcpy(vlynq_cfg->error_msg,
		       "Local VLYNQ revision higher than SW.");
		goto pal_vlynq_init_local_rev_error;
	}

	/* Let us configure the RTM for VLYNQ 2.5 and higher. */
	if (vlynq_config_bridge_RTM(p_vlynq,
				    vlynq_cfg->local_rtm_cfg_type,
				    vlynq_cfg->local_rtm_sample_value,
				    vlynq_cfg->local_tx_fast_path,
				    vlynq_cfg->peer_rtm_cfg_type,
				    vlynq_cfg->peer_rtm_sample_value,
				    vlynq_cfg->peer_tx_fast_path)) {
		vlynq_cfg->error_status = VLYNQ_INIT_RTM_CFG;
		strcpy(vlynq_cfg->error_msg, "Error in configuring RTM.");
		goto pal_vlynq_init_rtm_config_error;
	}

	/* We have just updated the RTM, let us check the health of the link.
	 * We must have the link, if not it is over for us. */
	if (!vlynq_detect_link_status(p_vlynq)) {
		vlynq_cfg->error_status = VLYNQ_INIT_NO_LINK_ON_RTM_CFG;
		strcpy(vlynq_cfg->error_msg,
		       "No link detected post RTM config.");
		goto pal_vlynq_init_link_error_3;
	}

	/* Again, We must clear the spur interrupts. */
	base = PEER_REGS(p_vlynq);
	vlynq_wr_reg(p_vlynq, 1u, base + VLYNQ_STAT,
		     vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_STAT) | 0x180);

	base = LOCAL_REGS(p_vlynq);
	vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_STAT,
		     vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_STAT) | 0x180);

	/* Let us go ahead and initialize the interal interrupts mechanisms
	 * for the local VLYNQ. */
	if (vlynq_cfg->on_soc) {
		p_root_irq_map = vlynq_get_free_irq_map();
		p_root_irq_info = vlynq_get_free_irq_info();
		p_irq_swap_map = vlynq_get_free_irq_swap_fixup();

		if (!p_root_irq_map || !p_root_irq_info || !p_irq_swap_map) {
			vlynq_cfg->error_status = VLYNQ_INIT_INTERNAL_PROBLEM;
			strcpy(vlynq_cfg->error_msg,
			       "Could not allocate root irq structs.");
			goto pal_vlynq_init_root_irq_map_error;
		}
	}

	p_vlynq->soc = vlynq_cfg->on_soc;
	p_vlynq->root_irq_map = p_root_irq_map;
	p_vlynq->root_irq_info = p_root_irq_info;
	p_vlynq->irq_swap_map = p_irq_swap_map;

	if (vlynq_cfg->on_soc && vlynq_add_root(p_vlynq)) {
		vlynq_cfg->error_status = VLYNQ_INIT_INTERNAL_PROBLEM;
		strcpy(vlynq_cfg->error_msg, "Could not add root vlynq.");
		goto pal_vlynq_init_root_add_error;
	}

	if (vlynq_install_intern_isr(p_vlynq, vlynq_cfg)) {
		vlynq_cfg->error_status = VLYNQ_INIT_INTERNAL_PROBLEM;
		strcpy(vlynq_cfg->error_msg, "Could not install internal isr.");
		goto pal_vlynq_init_install_intern_isr_error;
	}

	if (p_vlynq->soc) {
		if (vlynq_add_intern_isr
		    (p_vlynq, vlynq_cfg->local_intr_vector,
		     p_vlynq->local_isr_info)) {
			vlynq_cfg->error_status = VLYNQ_INIT_INTERNAL_PROBLEM;
			strcpy(vlynq_cfg->error_msg,
			       "Could not add local isr.");
			goto pal_vlynq_init_add_intern_local_isr_error;
		}

		if (vlynq_add_intern_isr
		    (p_vlynq, vlynq_cfg->peer_intr_vector,
		     p_vlynq->peer_isr_info)) {
			vlynq_cfg->error_status = VLYNQ_INIT_INTERNAL_PROBLEM;
			strcpy(vlynq_cfg->error_msg, "Could not add peer isr.");
			goto pal_vlynq_init_add_intern_peer_isr_error;
		}
	}

	base = PEER_REGS(p_vlynq);
	vlynq_wr_reg(p_vlynq, 1u, base + VLYNQ_TXMAP, vlynq_cfg->peer_tx_addr);
	vlynq_wr_reg(p_vlynq, 1u, base + VLYNQ_STAT,
		     vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_STAT) | 0x180);

	val = vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_INTSTAT);
	vlynq_wr_reg(p_vlynq, 1u, base + VLYNQ_INTSTAT, val);

	base = LOCAL_REGS(p_vlynq);
	vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_TXMAP, vlynq_cfg->local_tx_addr);
	vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_STAT,
		     vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_STAT) | 0x180);

	val = vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_INTSTAT);
	vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_INTSTAT, val);

	vlynq_config_bridge_endianness(p_vlynq,
				       vlynq_cfg->local_endianness,
				       vlynq_cfg->peer_endianness);

	/* Lets fixup the effect of endinaness madness on the intern ISR(s). */
	if (p_vlynq->soc)
		vlynq_set_intern_irq_swap_fixup(p_vlynq);

	vlynq_cfg->error_status = VLYNQ_INIT_SUCCESS;
	strcpy(vlynq_cfg->error_msg, "Success");

	return (p_vlynq);

pal_vlynq_init_add_intern_peer_isr_error:

	vlynq_remove_intern_isr(p_vlynq, vlynq_cfg->local_intr_vector,
				p_vlynq->local_isr_info);

pal_vlynq_init_add_intern_local_isr_error:

	vlynq_uninstall_intern_isr(p_vlynq);

pal_vlynq_init_install_intern_isr_error:

	if (vlynq_cfg->on_soc)
		vlynq_remove_root(p_vlynq);

pal_vlynq_init_root_add_error:

	if (vlynq_cfg->on_soc && p_vlynq->root_irq_info)
		vlynq_free_irq_info(p_vlynq->root_irq_info);

	if (vlynq_cfg->on_soc && p_vlynq->root_irq_info)
		vlynq_free_irq_map(p_vlynq->root_irq_map);

pal_vlynq_init_root_irq_map_error:

pal_vlynq_init_link_error_3:
pal_vlynq_init_rtm_config_error:

pal_vlynq_init_local_rev_error:
pal_vlynq_init_peer_rev_error:

pal_vlynq_init_link_error_2:
pal_vlynq_init_clock_config_error:

pal_vlynq_init_link_error_1:
	vlynq_free_vlynq(p_vlynq);
	p_vlynq = NULL;

	return (p_vlynq);
}
EXPORT_SYMBOL(vlynq_init_soc);

int vlynq_cleanup(void *vlynq)
{
	struct vlynq_module *p_vlynq = (struct vlynq_module *) vlynq;
	u32 val;
	u32 base;
	int ret_val = VLYNQ_OK;

	if (!p_vlynq)
		return (VLYNQ_INVALID_PARAM);

	base = PEER_REGS(p_vlynq);

	val = vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_INTSTAT);
	vlynq_wr_reg(p_vlynq, 1u, base + VLYNQ_INTSTAT, val);

	val = vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_STAT);
	val |= 0x180;
	vlynq_wr_reg(p_vlynq, 1u, base + VLYNQ_STAT, val);

	vlynq_sys_wr_reg(0, base + VLYNQ_TXMAP, 0);

	base = LOCAL_REGS(p_vlynq);
	val = vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_INTSTAT);
	vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_INTSTAT, val);

	val = vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_STAT);
	val |= (0x180);
	vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_STAT, val);
	vlynq_sys_wr_reg(0, base + VLYNQ_TXMAP, 0);

	/*
	 * Here, we can not do much about the errors. We simply keep moving on
	 * and return the error status. Thats all we can do.
	 */
	if (p_vlynq->soc) {
		/* Unset the endianness */
		vlynq_unset_intern_irq_swap_fixup(p_vlynq);

		ret_val = vlynq_remove_intern_isr(p_vlynq, p_vlynq->local_irq,
						  p_vlynq->local_isr_info);

		ret_val = vlynq_remove_intern_isr(p_vlynq, p_vlynq->peer_irq,
						  p_vlynq->peer_isr_info);
	}

	ret_val = vlynq_uninstall_intern_isr(p_vlynq);

	if (p_vlynq->soc)
		vlynq_remove_root(p_vlynq);

	if (p_vlynq->soc) {
		vlynq_free_irq_map(p_vlynq->root_irq_map);
		vlynq_free_irq_info(p_vlynq->root_irq_info);
		vlynq_free_irq_swap_fixup(p_vlynq->irq_swap_map);
	}

	base = LOCAL_REGS(p_vlynq);

	/* Put the the local device in reset. */
	if (vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_REV) >= 0x00010200) {
		vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_CNTL,
			    vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_CNTL) | 0x1);
	}

	vlynq_free_vlynq(p_vlynq);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_cleanup);

char *vlynq_get_popular_name(u32 id, char *soc_name)
{
	const char *name = NULL;

	switch (id) {
	case 0x0001:
		name = "avalanche-i";
		break;

	case 0x0002:
		name = "avalanche-d";
		break;

	case 0x0003:
		name = "taos";
		break;

	case 0x0104:
		name = "puma-s";
		break;

	case 0x0204:
		name = "puma-III";
		break;

	case 0x0005:
		name = "sangam";
		break;

	case 0x0006:
		name = "vdsp";
		break;

	case 0x0007:
		name = "titan";
		break;

	case 0x0008:
		name = "vlynq2pci";
		break;

	case 0x0009:
		name = "acx111";
		break;

	case 0x000b:
		name = "apex";
		break;

	case 0x000c:
		name = "vlynq2usb";
		break;

	case 0x000d:
		name = "vlynq2ipsec";
		break;

	case 0x000e:
		name = "omap730";
		break;

	case 0x000f:
		name = "omap1611";
		break;

	case 0x0010:
		name = "omap1710";
		break;

	case 0x0011:
		name = "tnetw1150";
		break;

	case 0x0012:
		name = "omap331";
		break;

	case 0x0013:
		name = "DM320";
		break;

	case 0x0014:
		name = "omap2410";
		break;

	case 0x0015:
		name = "omap2420";
		break;

	case 0x0016:
		name = "huawei-SD705";
		break;

	case 0x0017:
		name = "trinity";
		break;

	case 0x0018:
		name = "ohio";
		break;

	case 0x0019:
		name = "sierra";
		break;

	case 0x0020:
		name = "Psyloke";
		break;

	case 0x0021:
		name = "tnetw1251";
		break;

	case 0x0022:
		name = "himalaya";
		break;

	case 0x0023:
		name = "monticello";
		break;

	case 0x0024:
		name = "DDP3020";
		break;

	case 0x0025:
		name = "Davinci";
		break;

	case 0x0026:
		name = "TUSB6010";
		break;

	case 0x0027:
		name = "tnetw1160";
		break;

	case 0x0028:
		name = "peta";
		break;

	case 0x0029:
		name = "1150v";
		break;

	case 0x002a:
		name = "triveni";
		break;

	case 0x002b:
		name = "ohio250";
		break;

	case 0x002c:
		name = "tomahawk";
		break;

	case 0x002d:
		name = "DM64LC";
		break;

	case 0x002e:
		name = "Jacinto";
		break;

	case 0x002f:
		name = "Aries";
		break;

	case 0x0030:
		name = "Yamuna";
		break;

	case 0x0031:
		name = "Kailash";
		break;

	case 0x0032:
		name = "Passave";
		break;

	case 0x0033:
		name = "Davinci-HD/DM700";
		break;

	default:
		name = "unknown";
		break;
	}

	strcpy(soc_name, name);

	return (soc_name);
}

static int vlynq_dump_raw(struct vlynq_module *p_vlynq, char *buf,
			  u32 limit, u32 *eof, u8 peer)
{
	u8 *p_addr =
	    (peer ==
	     1u) ? (u8 *) PEER_REGS(p_vlynq) : (u8 *) LOCAL_REGS(p_vlynq);
	int i, j, len = 0;

	for (i = 0; i < 16; i++) {
		if (len < limit)
			len += snprintf(buf + len, limit - len, "%08x:",
					(u32) p_addr);
		else
			break;

		for (j = 0; j < 8; j++) {
			if ((j % 4 == 0) && (len < limit))
				len += snprintf(buf + len, limit - len, " ");

			if (len < limit)
				len +=
				    snprintf(buf + len, limit - len, "%02x ",
					    (u32) *p_addr++);
			else
				break;

		}

		if (len < limit)
			len += snprintf(buf + len, limit - len, "\n");
		else
			break;
	}

	return (len);

}

static int vlynq_dump_raw_reg(void *vlynq, char *buf,
			      u32 limit, u32 *eof)
{
	struct vlynq_module *p_vlynq = (struct vlynq_module *) vlynq;
	int len = 0;

	if (!p_vlynq->soc) {
		if (!vlynq_touch_vlynq_through_chain(p_vlynq)) {
			len += snprintf(buf + len, limit - len,
					"Link failure.\n");
			return (len);
		}
	}

	len = vlynq_dump_raw(p_vlynq, buf, limit, eof, 0u);

	if (!vlynq_touch_vlynq_through_chain(p_vlynq)) {
		len += snprintf(buf + len, limit - len,
				"Link to peer failed.\n");
		return (len);
	}

	return (len + vlynq_dump_raw(p_vlynq, buf + len, limit - len, eof, 1u));
}

static int vlynq_get_hop_in_chain(struct vlynq_module *p_root_vlynq,
				  struct vlynq_module *this_vlynq)
{
	struct vlynq_module *p_vlynq = p_root_vlynq;
	int hop = -1, found = 0;

	do {
		hop++;

		if (p_vlynq == this_vlynq) {
			found = 1;
			break;
		}

		p_vlynq = p_vlynq->next;

	} while (p_vlynq);

	return (found ? hop : VLYNQ_INTERNAL_ERROR);
}

static int vlynq_dump_root_vlynq(struct vlynq_module *p_root_vlynq, int root,
				 char *buf, int limit, int *eof)
{
	struct vlynq_irq_map_t *p_irq_map;
	int irq, hop, len = 0;

	len += snprintf(buf + len, limit - len,
		    "\nRoot Vlynq %d @ 0x%08x\n  IRQ Mapping:\n    ",
		    root, p_root_vlynq->base);

	for (irq = 0, p_irq_map = p_root_vlynq->root_irq_map;
	     irq < MAX_IRQ_PER_CHAIN; irq++, p_irq_map++) {
		int disp_count = 0;

		if (p_irq_map->hw_intr_line == -1)
			continue;

		hop = vlynq_get_hop_in_chain(p_root_vlynq, p_irq_map->vlynq);

		if (hop < 0)
			continue;

		if (len < limit) {
			len += snprintf(buf + len, limit - len,
				    "%d for Bridge %d%d %s hw-line %d, ", irq,
				    root, hop,
				    p_irq_map->peer ? "remote" : "local",
				    p_irq_map->hw_intr_line);
			disp_count++;

			if (!(disp_count % 8))
				len += snprintf(buf + len, limit - len, "\n");

			if (!(disp_count % 2))
				len += snprintf(buf + len, limit - len,
							"\n    ");
		}
	}

	len += snprintf(buf + len, limit - len, "\n");

	return (len);
}

int vlynq_dev_dump_dev(void *vlynq_dev, char *buf, int limit,
			      int *eof);

static int vlynq_dump_vlynq(struct vlynq_module *p_vlynq, char *buf, int limit,
			    int *eof)
{
	int index, len = 0;
	u32 p_local_reg = LOCAL_REGS(p_vlynq);
	u32 p_peer_reg = PEER_REGS(p_vlynq);
	void *p_vlynq_dev_hnd;

	len += snprintf(buf + len, limit - len, "  Tx Mapping:\n");

	len += snprintf(buf + len, limit - len,
			"    %22s 0x%08x <-> 0x%08x\n", " ",
		       vlynq_rd_reg(p_vlynq, 0u, p_local_reg + VLYNQ_TXMAP),
		       vlynq_rd_reg(p_vlynq, 1u, p_peer_reg + VLYNQ_TXMAP));

	len += snprintf(buf + len, limit - len, "  Rx Mapping:\n");

	for (index = 0; index < MAX_VLYNQ_REGION; index++) {
		struct vlynq_region_info *p_lregion =
		    &p_vlynq->local_region_info[index];
		struct vlynq_region_info *p_rregion =
		    &p_vlynq->remote_region_info[index];

		len += snprintf(buf + len, limit - len, "    Rx[%d]: ", index);

		if (p_lregion->owner_dev_index != -1) {
			len += snprintf(buf + len, limit - len,
					"S:0x%08x, O:0x%08x <-> ",
					vlynq_rd_reg(p_vlynq, 0u,
					  p_local_reg + VLYNQ_RXSIZE(index)),
					vlynq_rd_reg(p_vlynq, 0u,
					  p_local_reg + VLYNQ_RXOFFSET(index)));
		} else {
			len += snprintf(buf + len, limit - len,
					"                           <-> ");
		}

		if (p_rregion->owner_dev_index != -1) {
			len += snprintf(buf + len, limit - len,
					"S:0x%08x, O:0x%08x",
					vlynq_rd_reg(p_vlynq, 1u,
					  p_peer_reg + VLYNQ_RXSIZE(index)),
					vlynq_rd_reg(p_vlynq, 1u,
					  p_peer_reg + VLYNQ_RXOFFSET(index)));
		}

		len += snprintf(buf + len, limit - len, "\n");
	}

	len += snprintf(buf + len, limit - len, "  Local devices:\n");

	for (index = 0; index < MAX_DEV_PER_VLYNQ; index++) {
		p_vlynq_dev_hnd = p_vlynq->local_dev[index];

		if (p_vlynq_dev_hnd) {
			if (len < limit)
				len +=
				    vlynq_dev_dump_dev(p_vlynq_dev_hnd,
						       buf + len, limit - len,
						       eof);
		}
	}

	len += snprintf(buf + len, limit - len, "\n  Remote devices:\n");

	for (index = 0; index < MAX_DEV_PER_VLYNQ; index++) {
		p_vlynq_dev_hnd = p_vlynq->peer_dev[index];

		if (p_vlynq_dev_hnd) {
			if (len < limit)
				len +=
				    vlynq_dev_dump_dev(p_vlynq_dev_hnd,
						       buf + len, limit - len,
						       eof);
		}
	}

	return (len);
}

static int vlynq_dump_global(char *buf, int limit, int *eof)
{
	int root, len = 0, hop;

	for (root = 0; root < MAX_ROOT_VLYNQ; root++) {
		struct vlynq_module *p_root_vlynq = root_vlynq[root];
		struct vlynq_module *p_vlynq = p_root_vlynq;

		if (!p_root_vlynq)
			continue;

		if (len < limit)
			len +=
			    vlynq_dump_root_vlynq(p_root_vlynq, root, buf + len,
						  limit - len, eof);

		if (!vlynq_detect_link_status(p_root_vlynq)) {
			len += snprintf(buf + len, limit - len,
				    "Link failure for Bridge %d0.\n", root);

			continue;
		}

		hop = 0;

		while (p_vlynq) {
			u32 p_local_reg = LOCAL_REGS(p_vlynq);
			u32 p_peer_reg = PEER_REGS(p_vlynq);
			char lname[30], pname[30];

			len += snprintf(buf + len, limit - len,
					"\nVlynq Bridge %d%d :", root, hop);

			if (!vlynq_detect_link_status(p_vlynq)) {
				len += snprintf(buf + len, limit - len,
						"Link failure "
						"for Bridge %d%d\n",
						root, hop);
				break;
			}

			len += snprintf(buf + len, limit - len,
					"%20s <-> %s\n",
					vlynq_get_popular_name
					  (vlynq_rd_reg(p_vlynq, 0u,
					   p_local_reg + VLYNQ_CHIPVER) &
					   0xffff, lname),
					vlynq_get_popular_name
					  (vlynq_rd_reg(p_vlynq, 1u,
					   p_peer_reg + VLYNQ_CHIPVER) &
					   0xffff, pname));
			if (len < limit)
				len +=
				    vlynq_dump_vlynq(p_vlynq, buf + len,
						     limit - len, eof);

			p_vlynq = p_vlynq->next;
			hop++;
		}
	}

	*eof = 1;
	return (len);

}

u8 vlynq_swap_hack;

int vlynq_dump_ioctl(u32 start_reg, u32 dump_type,
		     char *buf, int limit, u32 vlynq_rev);

int vlynq_dump(void *vlynq, u32 dump_type, char *buf, int limit, int *eof)
{
	int len = 0, valid = 0;
	u32 local_addr = 0, peer_addr = 0;
	u32 base;
	struct vlynq_module *p_vlynq = vlynq;

	if (dump_type == VLYNQ_DUMP_RAW_DATA)
		return (vlynq_dump_raw_reg(p_vlynq, buf, limit, eof));

	if (dump_type == VLYNQ_DUMP_ALL_ROOT)
		return (vlynq_dump_global(buf, limit, eof));

	if (!p_vlynq)
		return (len);

	base = LOCAL_REGS(p_vlynq);

	switch (dump_type) {

	case VLYNQ_DUMP_STS_REG:

		local_addr = base + VLYNQ_STAT;
		base = PEER_REGS(p_vlynq);
		peer_addr = base + VLYNQ_STAT;
		break;

	case VLYNQ_DUMP_CNTL_REG:

		local_addr = base + VLYNQ_CNTL;
		base = PEER_REGS(p_vlynq);
		peer_addr = base + VLYNQ_CNTL;
		break;

	case VLYNQ_DUMP_ALL_REGS:

		local_addr = base;
		base = PEER_REGS(p_vlynq);
		peer_addr = base;
		break;

	default:
		valid = -1;
		break;
	}

	if (valid == -1)
		return (len);

	base = LOCAL_REGS(p_vlynq);

	len += snprintf(buf + len, limit - len, "\nLocal ver#: 0x%08x\n\n",
		       vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_REV));

	vlynq_swap_hack = p_vlynq->local_swap;

	if (len < limit)
		len +=
		    vlynq_dump_ioctl(local_addr, dump_type, buf + len,
				     limit - len, vlynq_rd_reg(p_vlynq, 0u,
							base + VLYNQ_REV));

	if (!vlynq_touch_vlynq_through_chain(p_vlynq)) {
		len += snprintf(buf + len, limit - len,
				"Link to peer failed.\n");
		return (len);
	}

	base = PEER_REGS(p_vlynq);

	len += snprintf(buf + len, limit - len,
			"\nPeer/Remote ver#: 0x%08x\n\n",
			vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_REV));

	vlynq_swap_hack = p_vlynq->peer_swap;

	if (len < limit)
		len +=
		    vlynq_dump_ioctl(peer_addr, dump_type, buf + len,
				     limit - len, vlynq_rd_reg(p_vlynq, 1u,
							base + VLYNQ_REV));
	*eof = 1;

	return (len);
}
EXPORT_SYMBOL(vlynq_dump);

int vlynq_read_write_ioctl(u32 p_start_reg, u32 cmd, u32 val, u32 vlynq_rev);

/* Cmd Code:
 *
 * ----------------------------------------------------------------------------
 * |31: Bitop | 30: R/W* |29: Remote | 28-24:reserved |
 * 23-16 major id of entity | 15-8 reserved|7 - 0 minor id of entity  |
 * ----------------------------------------------------------------------------
 */

int vlynq_non_bit_op_ioctl(struct vlynq_module *p_vlynq, u32 cmd, u32 cmd_val)
{
	u32 base = LOCAL_REGS(p_vlynq);
	u32 p_start_reg = 0;

	int ret_val = 0;
	u32 vlynq_rev;
	u32 val;

	switch (VLYNQ_IOCTL_MAJOR_DE_VAL(cmd)) {
	case VLYNQ_IOCTL_REG_CMD:

		if (cmd & VLYNQ_IOCTL_REMOTE_CMD)
			base = PEER_REGS(p_vlynq);

		p_start_reg = base;
		break;

	case VLYNQ_IOCTL_PREP_LINK_DOWN:

		p_vlynq->backup_local_cntl_word = vlynq_sys_rd_reg(0,
						base + VLYNQ_CNTL);
		p_vlynq->backup_local_intr_ptr = vlynq_sys_rd_reg(0,
						base + VLYNQ_INTPTR);
		p_vlynq->backup_local_tx_map = vlynq_sys_rd_reg(0,
						base + VLYNQ_TXMAP);

		if ((vlynq_sys_rd_reg(0, base + VLYNQ_REV) >= 0x00010200)
					&&
		    (vlynq_sys_rd_reg(0, base + VLYNQ_REV) <= 0x00010206))
		    /* Hack for 1350A */
			p_vlynq->backup_local_endian = vlynq_sys_rd_reg(0,
							 base + VLYNQ_ENDIAN);
		else
			p_vlynq->backup_local_endian = VLYNQ_ENDIAN_LITTLE;

		/* Let us disable the interrupts, spur interrupts are
		 * not invited. */
		val =
		    vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_CNTL) & (~(1 << 13));
		vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_CNTL, val);

		base = PEER_REGS(p_vlynq);

		p_vlynq->backup_peer_cntl_word = vlynq_sys_rd_reg(0,
						base + VLYNQ_CNTL);
		p_vlynq->backup_peer_intr_ptr = vlynq_sys_rd_reg(0,
						base + VLYNQ_INTPTR);
		p_vlynq->backup_peer_tx_map = vlynq_sys_rd_reg(0,
						base + VLYNQ_TXMAP);
		if ((vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_REV) >=
			0x00010200) &&
		    (vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_REV) <=
			0x00010206))	/* Hack for 1350A */
			p_vlynq->backup_peer_endian = vlynq_sys_rd_reg(0,
						base + VLYNQ_ENDIAN);
		else
			p_vlynq->backup_peer_endian = VLYNQ_ENDIAN_LITTLE;

		break;

	case VLYNQ_IOCTL_PREP_LINK_UP:

		/* Clearing the spur interrupts, lest it locks up the bus. */
		val = vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_STAT) | 0x180;
		vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_STAT, val);

		vlynq_sys_wr_reg(0, base + VLYNQ_CNTL,
				 p_vlynq->backup_local_cntl_word);
		vlynq_sys_wr_reg(0, base + VLYNQ_INTPTR,
				 p_vlynq->backup_local_intr_ptr);
		vlynq_sys_wr_reg(0, base + VLYNQ_TXMAP,
				 p_vlynq->backup_local_tx_map);

		if ((vlynq_sys_rd_reg(0, base + VLYNQ_REV) >= 0x00010200)
					&&
		    (vlynq_sys_rd_reg(0, base + VLYNQ_REV) <= 0x00010206))
			/* Hack for 1350A */
			vlynq_sys_wr_reg(0, base + VLYNQ_ENDIAN,
					 p_vlynq->backup_local_endian);

		base = PEER_REGS(p_vlynq);

		p_vlynq->peer_swap = p_vlynq->local_swap;

		/* Clearing the spur interrupts, lest it locks up the bus. */
		val = vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_STAT) | 0x180;
		vlynq_wr_reg(p_vlynq, 1u, base + VLYNQ_STAT, val);

		if ((vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_REV) >=
			0x00010200) &&
		    (vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_REV) <=
			0x00010206))	/* Hack for 1350A */
			vlynq_wr_reg(p_vlynq, 1u, base + VLYNQ_ENDIAN,
				     p_vlynq->backup_peer_endian);

		if (p_vlynq->backup_local_endian != p_vlynq->backup_peer_endian)
			p_vlynq->peer_swap = p_vlynq->peer_swap ? 0u : 1u;

		vlynq_sys_wr_reg(0, base + VLYNQ_CNTL,
				 p_vlynq->backup_peer_cntl_word);
		vlynq_sys_wr_reg(0, base + VLYNQ_INTPTR,
				 p_vlynq->backup_peer_intr_ptr);
		vlynq_sys_wr_reg(0, base + VLYNQ_TXMAP,
				 p_vlynq->backup_peer_tx_map);
		break;

	case VLYNQ_IOCTL_CLEAR_INTERN_ERR:

		base = LOCAL_REGS(p_vlynq);
		val = vlynq_rd_reg(p_vlynq, 0u, base + VLYNQ_STAT) | 0x180;
		vlynq_wr_reg(p_vlynq, 0u, base + VLYNQ_STAT, val);

		base = PEER_REGS(p_vlynq);

		vlynq_delay_wait(0x100);

		val = vlynq_rd_reg(p_vlynq, 1u, base + VLYNQ_STAT) | 0x180;
		vlynq_wr_reg(p_vlynq, 1u, base + VLYNQ_STAT, val);

		break;

	default:
		ret_val = -1;
		break;
	}

	vlynq_rev = vlynq_rd_reg(p_vlynq,
				 (cmd & VLYNQ_IOCTL_REMOTE_CMD) ? 1u : 0u,
				 base + VLYNQ_REV);

	if (p_start_reg) {
		vlynq_swap_hack =
		    (cmd & VLYNQ_IOCTL_REMOTE_CMD) ? p_vlynq->
		    peer_swap : p_vlynq->local_swap;
		ret_val =
		    vlynq_read_write_ioctl(p_start_reg, cmd, cmd_val,
					   vlynq_rev);
	}

	return (VLYNQ_OK);
}

int vlynq_ioctl(void *vlynq_hnd, u32 cmd, u32 cmd_val)
{
	u32 base;
	u32 p_start_reg = 0;

	int ret_val = -1;
	struct vlynq_module *p_vlynq = vlynq_hnd;
	u32 vlynq_rev;
	u8 peer = 1u;

	if (!p_vlynq)
		return (VLYNQ_INVALID_PARAM);

	if (!vlynq_touch_vlynq_through_chain(p_vlynq))
		return (VLYNQ_INTERNAL_ERROR);

	if (cmd & VLYNQ_IOCTL_REMOTE_CMD)
		base = PEER_REGS(p_vlynq);
	else {
		base = LOCAL_REGS(p_vlynq);
		peer = 0u;
	}

	/* Hack for now as we support just one major non bit cmd. */
	if (!(cmd & VLYNQ_IOCTL_BIT_CMD)) {
		ret_val = vlynq_non_bit_op_ioctl(p_vlynq, cmd, cmd_val);
		/* We do not require register access. */
	} else {
		switch (VLYNQ_IOCTL_MAJOR_DE_VAL(cmd)) {
		case VLYNQ_IOCTL_STATUS_REG:
			p_start_reg = base + VLYNQ_STAT;
			break;

		case VLYNQ_IOCTL_CONTROL_REG:
			p_start_reg = base + VLYNQ_CNTL;
			break;

		default:
			break;

		}
	}

	vlynq_rev = vlynq_rd_reg(p_vlynq, peer, base + VLYNQ_REV);

	if (p_start_reg) {
		vlynq_swap_hack =
		    peer ? p_vlynq->peer_swap : p_vlynq->local_swap;
		ret_val =
		    vlynq_read_write_ioctl(p_start_reg, cmd, cmd_val,
					   vlynq_rev);
	}

	return (ret_val);
}
EXPORT_SYMBOL(vlynq_ioctl);
