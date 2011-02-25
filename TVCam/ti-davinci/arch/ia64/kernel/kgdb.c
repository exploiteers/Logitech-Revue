/*
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */

/*
 * Copyright (C) 2000-2001 VERITAS Software Corporation.
 * (c) Copyright 2005 Hewlett-Packard Development Company, L.P.
 *     Bob Picco <bob.picco@hp.com>
 */
/*
 *  Contributor:     Lake Stevens Instrument Division$
 *  Written by:      Glenn Engel $
 *  Updated by:	     Amit Kale<akale@veritas.com>
 *  Modified for 386 by Jim Kingdon, Cygnus Support.
 *  Origianl kgdb, compatibility with 2.1.xx kernel by David Grothe <dave@gcom.com>
 */

#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <asm/system.h>
#include <asm/ptrace.h>		/* for linux pt_regs struct */
#include <asm/unwind.h>
#include <asm/rse.h>
#include <linux/kgdb.h>
#include <linux/init.h>
#include <asm/cacheflush.h>
#include <asm/kdebug.h>

#define NUM_REGS 590
#define REGISTER_BYTES (NUM_REGS*8+128*8)
#define REGISTER_BYTE(N) (((N) * 8)                                    \
	+ ((N) <= IA64_FR0_REGNUM ?                                     \
	0 : 8 * (((N) > IA64_FR127_REGNUM) ? 128 : (N) - IA64_FR0_REGNUM)))
#define REGISTER_SIZE(N)                                               \
	(((N) >= IA64_FR0_REGNUM && (N) <= IA64_FR127_REGNUM) ? 16 : 8)
#define IA64_GR0_REGNUM         0
#define IA64_FR0_REGNUM         128
#define IA64_FR127_REGNUM       (IA64_FR0_REGNUM+127)
#define IA64_PR0_REGNUM         256
#define IA64_BR0_REGNUM         320
#define IA64_VFP_REGNUM         328
#define IA64_PR_REGNUM          330
#define IA64_IP_REGNUM          331
#define IA64_PSR_REGNUM         332
#define IA64_CFM_REGNUM         333
#define IA64_AR0_REGNUM         334
#define IA64_NAT0_REGNUM        462
#define IA64_NAT31_REGNUM       (IA64_NAT0_REGNUM+31)
#define IA64_NAT32_REGNUM       (IA64_NAT0_REGNUM+32)
#define IA64_RSC_REGNUM		(IA64_AR0_REGNUM+16)
#define IA64_BSP_REGNUM		(IA64_AR0_REGNUM+17)
#define IA64_BSPSTORE_REGNUM	(IA64_AR0_REGNUM+18)
#define IA64_RNAT_REGNUM	(IA64_AR0_REGNUM+19)
#define IA64_FCR_REGNUM		(IA64_AR0_REGNUM+21)
#define IA64_EFLAG_REGNUM	(IA64_AR0_REGNUM+24)
#define IA64_CSD_REGNUM		(IA64_AR0_REGNUM+25)
#define IA64_SSD_REGNUM		(IA64_AR0_REGNUM+26)
#define IA64_CFLG_REGNUM	(IA64_AR0_REGNUM+27)
#define IA64_FSR_REGNUM		(IA64_AR0_REGNUM+28)
#define IA64_FIR_REGNUM		(IA64_AR0_REGNUM+29)
#define IA64_FDR_REGNUM		(IA64_AR0_REGNUM+30)
#define IA64_CCV_REGNUM		(IA64_AR0_REGNUM+32)
#define IA64_UNAT_REGNUM	(IA64_AR0_REGNUM+36)
#define IA64_FPSR_REGNUM	(IA64_AR0_REGNUM+40)
#define IA64_ITC_REGNUM		(IA64_AR0_REGNUM+44)
#define IA64_PFS_REGNUM		(IA64_AR0_REGNUM+64)
#define IA64_LC_REGNUM		(IA64_AR0_REGNUM+65)
#define IA64_EC_REGNUM		(IA64_AR0_REGNUM+66)

#define	REGISTER_INDEX(N)	(REGISTER_BYTE(N) / sizeof (unsigned long))
#define BREAK_INSTR_ALIGN	(~0xfULL)

#define	ptoff(V)	((unsigned int) &((struct pt_regs *)0x0)->V)
struct reg_to_ptreg_index {
	unsigned int reg;
	unsigned int ptregoff;
};

static struct reg_to_ptreg_index gr_reg_to_ptreg_index[] = {
	{IA64_GR0_REGNUM + 1, ptoff(r1)},
	{IA64_GR0_REGNUM + 2, ptoff(r2)},
	{IA64_GR0_REGNUM + 3, ptoff(r3)},
	{IA64_GR0_REGNUM + 8, ptoff(r8)},
	{IA64_GR0_REGNUM + 9, ptoff(r9)},
	{IA64_GR0_REGNUM + 10, ptoff(r10)},
	{IA64_GR0_REGNUM + 11, ptoff(r11)},
	{IA64_GR0_REGNUM + 12, ptoff(r12)},
	{IA64_GR0_REGNUM + 13, ptoff(r13)},
	{IA64_GR0_REGNUM + 14, ptoff(r14)},
	{IA64_GR0_REGNUM + 15, ptoff(r15)},
	{IA64_GR0_REGNUM + 16, ptoff(r16)},
	{IA64_GR0_REGNUM + 17, ptoff(r17)},
	{IA64_GR0_REGNUM + 18, ptoff(r18)},
	{IA64_GR0_REGNUM + 19, ptoff(r19)},
	{IA64_GR0_REGNUM + 20, ptoff(r20)},
	{IA64_GR0_REGNUM + 21, ptoff(r21)},
	{IA64_GR0_REGNUM + 22, ptoff(r22)},
	{IA64_GR0_REGNUM + 23, ptoff(r23)},
	{IA64_GR0_REGNUM + 24, ptoff(r24)},
	{IA64_GR0_REGNUM + 25, ptoff(r25)},
	{IA64_GR0_REGNUM + 26, ptoff(r26)},
	{IA64_GR0_REGNUM + 27, ptoff(r27)},
	{IA64_GR0_REGNUM + 28, ptoff(r28)},
	{IA64_GR0_REGNUM + 29, ptoff(r29)},
	{IA64_GR0_REGNUM + 30, ptoff(r30)},
	{IA64_GR0_REGNUM + 31, ptoff(r31)},
};

static struct reg_to_ptreg_index br_reg_to_ptreg_index[] = {
	{IA64_BR0_REGNUM, ptoff(b0)},
	{IA64_BR0_REGNUM + 6, ptoff(b6)},
	{IA64_BR0_REGNUM + 7, ptoff(b7)},
};

static struct reg_to_ptreg_index ar_reg_to_ptreg_index[] = {
	{IA64_PFS_REGNUM, ptoff(ar_pfs)},
	{IA64_UNAT_REGNUM, ptoff(ar_unat)},
	{IA64_RNAT_REGNUM, ptoff(ar_rnat)},
	{IA64_BSPSTORE_REGNUM, ptoff(ar_bspstore)},
	{IA64_RSC_REGNUM, ptoff(ar_rsc)},
	{IA64_CSD_REGNUM, ptoff(ar_csd)},
	{IA64_SSD_REGNUM, ptoff(ar_ssd)},
	{IA64_FPSR_REGNUM, ptoff(ar_fpsr)},
	{IA64_CCV_REGNUM, ptoff(ar_ccv)},
};

extern atomic_t cpu_doing_single_step;

static int kgdb_gr_reg(int regnum, struct unw_frame_info *info,
	unsigned long *reg, int rw)
{
	char nat;

	if ((regnum >= IA64_GR0_REGNUM && regnum <= (IA64_GR0_REGNUM + 1)) ||
		(regnum >= (IA64_GR0_REGNUM + 4) &&
		regnum <= (IA64_GR0_REGNUM + 7)))
		return !unw_access_gr(info, regnum - IA64_GR0_REGNUM,
		reg, &nat, rw);
	else
		return 0;
}
static int kgdb_gr_ptreg(int regnum, struct pt_regs * ptregs,
	struct unw_frame_info *info, unsigned long *reg, int rw)
{
	int i, result = 1;
	char nat;

	if (!((regnum >= (IA64_GR0_REGNUM + 2) &&
		regnum <= (IA64_GR0_REGNUM + 3)) ||
		(regnum >= (IA64_GR0_REGNUM + 8) &&
		regnum <= (IA64_GR0_REGNUM + 15)) ||
		(regnum >= (IA64_GR0_REGNUM + 16) &&
		regnum <= (IA64_GR0_REGNUM + 31))))
		return 0;
	else if (rw && ptregs) {
		for (i = 0; i < ARRAY_SIZE(gr_reg_to_ptreg_index); i++)
			if (gr_reg_to_ptreg_index[i].reg == regnum) {
				*((unsigned long *)(((void *)ptregs) +
				gr_reg_to_ptreg_index[i].ptregoff)) = *reg;
				break;
			}
	} else if (!rw && ptregs) {
		for (i = 0; i < ARRAY_SIZE(gr_reg_to_ptreg_index); i++)
			if (gr_reg_to_ptreg_index[i].reg == regnum) {
				*reg = *((unsigned long *)
				(((void *)ptregs) +
				 gr_reg_to_ptreg_index[i].ptregoff));
				break;
			}
	} else
		result = !unw_access_gr(info, regnum - IA64_GR0_REGNUM,
					reg, &nat, rw);
	return result;
}

static int kgdb_br_reg(int regnum, struct pt_regs * ptregs,
	struct unw_frame_info *info, unsigned long *reg, int rw)
{
	int i, result = 1;

	if (!(regnum >= IA64_BR0_REGNUM && regnum <= (IA64_BR0_REGNUM + 7)))
		return 0;

	switch (regnum) {
	case IA64_BR0_REGNUM:
	case IA64_BR0_REGNUM + 6:
	case IA64_BR0_REGNUM + 7:
		if (rw) {
			for (i = 0; i < ARRAY_SIZE(br_reg_to_ptreg_index); i++)
				if (br_reg_to_ptreg_index[i].reg == regnum) {
					*((unsigned long *)
					(((void *)ptregs) +
					br_reg_to_ptreg_index[i].ptregoff)) =
					*reg;
					break;
				}
		} else
			for (i = 0; i < ARRAY_SIZE(br_reg_to_ptreg_index); i++)
				if (br_reg_to_ptreg_index[i].reg == regnum) {
						*reg = *((unsigned long *)
						(((void *)ptregs) +
						br_reg_to_ptreg_index[i].
						ptregoff));
						break;
				}
		break;
	case IA64_BR0_REGNUM + 1:
	case IA64_BR0_REGNUM + 2:
	case IA64_BR0_REGNUM + 3:
	case IA64_BR0_REGNUM + 4:
	case IA64_BR0_REGNUM + 5:
		result = !unw_access_br(info, regnum - IA64_BR0_REGNUM,
				reg, rw);
		break;
	}

	return result;
}

static int kgdb_fr_reg(int regnum, char *inbuffer, struct pt_regs * ptregs,
	struct unw_frame_info *info, unsigned long *reg,
	struct ia64_fpreg *freg, int rw)
{
	int result = 1;

	if (!(regnum >= IA64_FR0_REGNUM && regnum <= (IA64_FR0_REGNUM + 127)))
		return 0;

	switch (regnum) {
	case IA64_FR0_REGNUM + 6:
	case IA64_FR0_REGNUM + 7:
	case IA64_FR0_REGNUM + 8:
	case IA64_FR0_REGNUM + 9:
	case IA64_FR0_REGNUM + 10:
	case IA64_FR0_REGNUM + 11:
	case IA64_FR0_REGNUM + 12:
		if (rw) {
			char *ptr = inbuffer;

			freg->u.bits[0] = *reg;
			kgdb_hex2long(&ptr, &freg->u.bits[1]);
			*(&ptregs->f6 + (regnum - (IA64_FR0_REGNUM + 6))) =
				*freg;
			break;
		} else if (!ptregs)
			result = !unw_access_fr(info, regnum - IA64_FR0_REGNUM,
				freg, rw);
		else
			*freg =
			*(&ptregs->f6 + (regnum - (IA64_FR0_REGNUM + 6)));
		break;
	default:
		if (!rw)
			result = !unw_access_fr(info, regnum - IA64_FR0_REGNUM,
				freg, rw);
		else
			result = 0;
		break;
	}

	return result;
}

static int kgdb_ar_reg(int regnum, struct pt_regs * ptregs,
	struct unw_frame_info *info, unsigned long *reg, int rw)
{
	int result = 0, i;

	if (!(regnum >= IA64_AR0_REGNUM && regnum <= IA64_EC_REGNUM))
		return 0;

	if (rw && ptregs) {
		for (i = 0; i < ARRAY_SIZE(ar_reg_to_ptreg_index); i++)
			if (ar_reg_to_ptreg_index[i].reg == regnum) {
				*((unsigned long *) (((void *)ptregs) +
				ar_reg_to_ptreg_index[i].ptregoff)) =
					*reg;
				result = 1;
				break;
			}
	} else if (ptregs) {
		for (i = 0; i < ARRAY_SIZE(ar_reg_to_ptreg_index); i++)
			if (ar_reg_to_ptreg_index[i].reg == regnum) {
				*reg = *((unsigned long *) (((void *)ptregs) +
					ar_reg_to_ptreg_index[i].ptregoff));
					result = 1;
				break;
			}
	}

	if (result)
		return result;

       result = 1;

	switch (regnum) {
	case IA64_CSD_REGNUM:
		result = !unw_access_ar(info, UNW_AR_CSD, reg, rw);
		break;
	case IA64_SSD_REGNUM:
		result = !unw_access_ar(info, UNW_AR_SSD, reg, rw);
		break;
	case IA64_UNAT_REGNUM:
		result = !unw_access_ar(info, UNW_AR_RNAT, reg, rw);
		break;
		case IA64_RNAT_REGNUM:
		result = !unw_access_ar(info, UNW_AR_RNAT, reg, rw);
		break;
	case IA64_BSPSTORE_REGNUM:
		result = !unw_access_ar(info, UNW_AR_RNAT, reg, rw);
		break;
	case IA64_PFS_REGNUM:
		result = !unw_access_ar(info, UNW_AR_RNAT, reg, rw);
		break;
	case IA64_LC_REGNUM:
		result = !unw_access_ar(info, UNW_AR_LC, reg, rw);
		break;
	case IA64_EC_REGNUM:
		result = !unw_access_ar(info, UNW_AR_EC, reg, rw);
		break;
	case IA64_FPSR_REGNUM:
		result = !unw_access_ar(info, UNW_AR_FPSR, reg, rw);
		break;
	case IA64_RSC_REGNUM:
		result = !unw_access_ar(info, UNW_AR_RSC, reg, rw);
		break;
	case IA64_CCV_REGNUM:
		result = !unw_access_ar(info, UNW_AR_CCV, reg, rw);
		break;
	default:
		result = 0;
	}

	return result;
}

void kgdb_get_reg(char *outbuffer, int regnum, struct unw_frame_info *info,
	struct pt_regs *ptregs)
{
	unsigned long reg, size = 0, *mem = &reg;
	struct ia64_fpreg freg;

	if (kgdb_gr_reg(regnum, info, &reg, 0) ||
		kgdb_gr_ptreg(regnum, ptregs, info, &reg, 0) ||
		kgdb_br_reg(regnum, ptregs, info, &reg, 0) ||
		kgdb_ar_reg(regnum, ptregs, info, &reg, 0))
			size = sizeof(reg);
	else if (kgdb_fr_reg(regnum, NULL, ptregs, info, &reg, &freg, 0)) {
		size = sizeof(freg);
		mem = (unsigned long *)&freg;
	} else if (regnum == IA64_IP_REGNUM) {
		if (!ptregs) {
			unw_get_ip(info, &reg);
			size = sizeof(reg);
		} else {
			reg = ptregs->cr_iip;
			size = sizeof(reg);
		}
	} else if (regnum == IA64_CFM_REGNUM) {
		if (!ptregs)
			unw_get_cfm(info, &reg);
		else
			reg = ptregs->cr_ifs;
		size = sizeof(reg);
	} else if (regnum == IA64_PSR_REGNUM) {
		if (!ptregs && kgdb_usethread)
			ptregs = (struct pt_regs *)
			((unsigned long)kgdb_usethread +
			IA64_STK_OFFSET) - 1;
		if (ptregs)
			reg = ptregs->cr_ipsr;
		size = sizeof(reg);
	} else if (regnum == IA64_PR_REGNUM) {
		if (ptregs)
			reg = ptregs->pr;
		else
			unw_access_pr(info, &reg, 0);
		size = sizeof(reg);
	} else if (regnum == IA64_BSP_REGNUM) {
		unw_get_bsp(info, &reg);
		size = sizeof(reg);
	}

	if (size) {
		kgdb_mem2hex((char *) mem, outbuffer, size);
		outbuffer[size*2] = 0;
	}
	else
		strcpy(outbuffer, "E0");

	return;
}

void kgdb_put_reg(char *inbuffer, char *outbuffer, int regnum,
		  struct unw_frame_info *info, struct pt_regs *ptregs)
{
	unsigned long reg;
	struct ia64_fpreg freg;
	char *ptr = inbuffer;

	kgdb_hex2long(&ptr, &reg);
	strcpy(outbuffer, "OK");

	if (kgdb_gr_reg(regnum, info, &reg, 1) ||
		kgdb_gr_ptreg(regnum, ptregs, info, &reg, 1) ||
		kgdb_br_reg(regnum, ptregs, info, &reg, 1) ||
		kgdb_fr_reg(regnum, inbuffer, ptregs, info, &reg, &freg, 1) ||
		kgdb_ar_reg(regnum, ptregs, info, &reg, 1)) ;
	else if (regnum == IA64_IP_REGNUM)
		ptregs->cr_iip = reg;
	else if (regnum == IA64_CFM_REGNUM)
		ptregs->cr_ifs = reg;
	else if (regnum == IA64_PSR_REGNUM)
		ptregs->cr_ipsr = reg;
	else if (regnum == IA64_PR_REGNUM)
		ptregs->pr = reg;
	else
		strcpy(outbuffer, "E01");
	return;
}

void regs_to_gdb_regs(unsigned long *gdb_regs, struct pt_regs *regs)
{
}

void sleeping_thread_to_gdb_regs(unsigned long *gdb_regs, struct task_struct *p)
{
}

void gdb_regs_to_regs(unsigned long *gdb_regs, struct pt_regs *regs)
{

}

#define	MAX_HW_BREAKPOINT	(20)
long hw_break_total_dbr, hw_break_total_ibr;
#define	HW_BREAKPOINT	(hw_break_total_dbr + hw_break_total_ibr)
#define	WATCH_INSTRUCTION	0x0
#define WATCH_WRITE		0x1
#define	WATCH_READ		0x2
#define	WATCH_ACCESS		0x3

#define	HWCAP_DBR	((1 << WATCH_WRITE) | (1 << WATCH_READ))
#define	HWCAP_IBR	(1 << WATCH_INSTRUCTION)
struct hw_breakpoint {
	unsigned enabled;
	unsigned long capable;
	unsigned long type;
	unsigned long mask;
	unsigned long addr;
} *breakinfo;

static struct hw_breakpoint hwbreaks[MAX_HW_BREAKPOINT];

enum instruction_type { A, I, M, F, B, L, X, u };

static enum instruction_type bundle_encoding[32][3] = {
	{M, I, I},		/* 00 */
	{M, I, I},		/* 01 */
	{M, I, I},		/* 02 */
	{M, I, I},		/* 03 */
	{M, L, X},		/* 04 */
	{M, L, X},		/* 05 */
	{u, u, u},		/* 06 */
	{u, u, u},		/* 07 */
	{M, M, I},		/* 08 */
	{M, M, I},		/* 09 */
	{M, M, I},		/* 0A */
	{M, M, I},		/* 0B */
	{M, F, I},		/* 0C */
	{M, F, I},		/* 0D */
	{M, M, F},		/* 0E */
	{M, M, F},		/* 0F */
	{M, I, B},		/* 10 */
	{M, I, B},		/* 11 */
	{M, B, B},		/* 12 */
	{M, B, B},		/* 13 */
	{u, u, u},		/* 14 */
	{u, u, u},		/* 15 */
	{B, B, B},		/* 16 */
	{B, B, B},		/* 17 */
	{M, M, B},		/* 18 */
	{M, M, B},		/* 19 */
	{u, u, u},		/* 1A */
	{u, u, u},		/* 1B */
	{M, F, B},		/* 1C */
	{M, F, B},		/* 1D */
	{u, u, u},		/* 1E */
	{u, u, u},		/* 1F */
};

int kgdb_validate_break_address(unsigned long addr)
{
	int error;
	char tmp_variable[BREAK_INSTR_SIZE];
	error = kgdb_get_mem((char *)(addr & BREAK_INSTR_ALIGN), tmp_variable,
		BREAK_INSTR_SIZE);
	return error;
}

int kgdb_arch_set_breakpoint(unsigned long addr, char *saved_instr)
{
	extern unsigned long _start[];
	unsigned long slot = addr & BREAK_INSTR_ALIGN, bundle_addr;
	unsigned long template;
	struct bundle {
		struct {
			unsigned long long template:5;
			unsigned long long slot0:41;
			unsigned long long slot1_p0:64 - 46;
		} quad0;
		struct {
			unsigned long long slot1_p1:41 - (64 - 46);
			unsigned long long slot2:41;
		} quad1;
	} bundle;
	int ret;

	bundle_addr = addr & ~0xFULL;

	if (bundle_addr == (unsigned long)_start)
		return 0;

	ret = kgdb_get_mem((char *)bundle_addr, (char *)&bundle,
			   BREAK_INSTR_SIZE);
	if (ret < 0)
		return ret;

	if (slot > 2)
		slot = 0;

	memcpy(saved_instr, &bundle, BREAK_INSTR_SIZE);
	template = bundle.quad0.template;

	if (slot == 1 && bundle_encoding[template][1] == L)
		slot = 2;

	switch (slot) {
	case 0:
		bundle.quad0.slot0 = BREAKNUM;
		break;
	case 1:
		bundle.quad0.slot1_p0 = BREAKNUM;
		bundle.quad1.slot1_p1 = (BREAKNUM >> (64 - 46));
		break;
	case 2:
		bundle.quad1.slot2 = BREAKNUM;
		break;
	}

	return kgdb_set_mem((char *)bundle_addr, (char *)&bundle,
			    BREAK_INSTR_SIZE);
}

int kgdb_arch_remove_breakpoint(unsigned long addr, char *bundle)
{
	extern unsigned long _start[];
	
	addr = addr & BREAK_INSTR_ALIGN;
	if (addr == (unsigned long)_start)
		return 0;
	return kgdb_set_mem((char *)addr, (char *)bundle, BREAK_INSTR_SIZE);
}

static int hw_breakpoint_init;

void do_init_hw_break(void)
{
	s64 status;
	int i;

	hw_breakpoint_init = 1;

#ifdef	CONFIG_IA64_HP_SIM
	hw_break_total_ibr = 8;
	hw_break_total_dbr = 8;
	status = 0;
#else
	status = ia64_pal_debug_info(&hw_break_total_ibr, &hw_break_total_dbr);
#endif

	if (status) {
		printk(KERN_INFO "do_init_hw_break: pal call failed %d\n",
		       (int)status);
		return;
	}

	if (HW_BREAKPOINT > MAX_HW_BREAKPOINT) {
		printk(KERN_INFO "do_init_hw_break: %d exceeds max %d\n",
		       (int)HW_BREAKPOINT, (int)MAX_HW_BREAKPOINT);

		while ((HW_BREAKPOINT > MAX_HW_BREAKPOINT)
		       && hw_break_total_ibr != 1)
			hw_break_total_ibr--;
		while (HW_BREAKPOINT > MAX_HW_BREAKPOINT)
			hw_break_total_dbr--;
	}

	breakinfo = hwbreaks;

	memset(breakinfo, 0, HW_BREAKPOINT * sizeof(struct hw_breakpoint));

	for (i = 0; i < hw_break_total_dbr; i++)
		breakinfo[i].capable = HWCAP_DBR;

	for (; i < HW_BREAKPOINT; i++)
		breakinfo[i].capable = HWCAP_IBR;

	return;
}

void kgdb_correct_hw_break(void)
{
	int breakno;

	if (!breakinfo)
		return;

	for (breakno = 0; breakno < HW_BREAKPOINT; breakno++) {
		if (breakinfo[breakno].enabled) {
			if (breakinfo[breakno].capable & HWCAP_IBR) {
				int ibreakno = breakno - hw_break_total_dbr;
				ia64_set_ibr(ibreakno << 1,
					     breakinfo[breakno].addr);
				ia64_set_ibr((ibreakno << 1) + 1,
					     (~breakinfo[breakno].mask &
					      ((1UL << 56UL) - 1)) |
					      (1UL << 56UL) | (1UL << 63UL));
			} else {
				ia64_set_dbr(breakno << 1,
					     breakinfo[breakno].addr);
				ia64_set_dbr((breakno << 1) + 1,
					     (~breakinfo[breakno].
					      mask & ((1UL << 56UL) - 1)) |
					     (1UL << 56UL) |
					     (breakinfo[breakno].type << 62UL));
			}
		} else {
			if (breakinfo[breakno].capable & HWCAP_IBR)
				ia64_set_ibr(((breakno -
					       hw_break_total_dbr) << 1) + 1,
					     0);
			else
				ia64_set_dbr((breakno << 1) + 1, 0);
		}
	}

	return;
}

int hardware_breakpoint(unsigned long addr, int length, int type, int action)
{
	int breakno, found, watch;
	unsigned long mask;
	extern unsigned long _start[];

	if (!hw_breakpoint_init)
		do_init_hw_break();

	if (!breakinfo)
		return 0;
	else if (addr == (unsigned long)_start)
		return 1;

	if (type == WATCH_ACCESS)
		mask = HWCAP_DBR;
	else
		mask = 1UL << type;

	for (watch = 0, found = 0, breakno = 0; breakno < HW_BREAKPOINT;
	     breakno++) {
		if (action) {
			if (breakinfo[breakno].enabled
			    || !(breakinfo[breakno].capable & mask))
				continue;
			breakinfo[breakno].enabled = 1;
			breakinfo[breakno].type = type;
			breakinfo[breakno].mask = length - 1;
			breakinfo[breakno].addr = addr;
			watch = breakno;
		} else if (breakinfo[breakno].enabled &&
			   ((length < 0 && breakinfo[breakno].addr == addr) ||
			    ((breakinfo[breakno].capable & mask) &&
			     (breakinfo[breakno].mask == (length - 1)) &&
			     (breakinfo[breakno].addr == addr)))) {
			breakinfo[breakno].enabled = 0;
			breakinfo[breakno].type = 0UL;
		} else
			continue;
		found++;
		if (type != WATCH_ACCESS)
			break;
		else if (found == 2)
			break;
		else
			mask = HWCAP_IBR;
	}

	if (type == WATCH_ACCESS && found == 1) {
		breakinfo[watch].enabled = 0;
		found = 0;
	}

	mb();
	return found;
}

int kgdb_arch_set_hw_breakpoint(unsigned long addr, int len,
				enum kgdb_bptype type)
{
	return hardware_breakpoint(addr, len, type - '1', 1);
}

int kgdb_arch_remove_hw_breakpoint(unsigned long addr, int len,
				   enum kgdb_bptype type)
{
	return hardware_breakpoint(addr, len, type - '1', 0);
}

int kgdb_remove_hw_break(unsigned long addr)
{
	return hardware_breakpoint(addr, 8, WATCH_INSTRUCTION, 0);

}

void kgdb_remove_all_hw_break(void)
{
	int i;

	for (i = 0; i < HW_BREAKPOINT; i++)
		memset(&breakinfo[i], 0, sizeof(struct hw_breakpoint));
}

int kgdb_set_hw_break(unsigned long addr)
{
	return hardware_breakpoint(addr, 8, WATCH_INSTRUCTION, 1);
}

void kgdb_disable_hw_debug(struct pt_regs *regs)
{
	unsigned long hw_breakpoint_status;

	hw_breakpoint_status = ia64_getreg(_IA64_REG_PSR);
	if (hw_breakpoint_status & IA64_PSR_DB)
		ia64_setreg(_IA64_REG_PSR_L,
			    hw_breakpoint_status ^ IA64_PSR_DB);
}

volatile static struct smp_unw {
	struct unw_frame_info *unw;
	struct task_struct *task;
} smp_unw[NR_CPUS];

static int inline kgdb_get_blocked_state(struct task_struct *p,
					 struct unw_frame_info *unw)
{
	unsigned long ip;
	int count = 0;

	unw_init_from_blocked_task(unw, p);
	ip = 0UL;
	do {
		if (unw_unwind(unw) < 0)
			return -1;
		unw_get_ip(unw, &ip);
		if (!in_sched_functions(ip))
			break;
	} while (count++ < 16);

	if (!ip)
		return -1;
	else
		return 0;
}

static void inline kgdb_wait(struct pt_regs *regs)
{
	unsigned long hw_breakpoint_status = ia64_getreg(_IA64_REG_PSR);
	if (hw_breakpoint_status & IA64_PSR_DB)
		ia64_setreg(_IA64_REG_PSR_L,
			    hw_breakpoint_status ^ IA64_PSR_DB);
	kgdb_nmihook(smp_processor_id(), regs);
	if (hw_breakpoint_status & IA64_PSR_DB)
		ia64_setreg(_IA64_REG_PSR_L, hw_breakpoint_status);

	return;
}

static void inline normalize(struct unw_frame_info *running,
			     struct pt_regs *regs)
{
	unsigned long sp;

	do {
		unw_get_sp(running, &sp);
		if ((sp + 0x10) >= (unsigned long)regs)
			break;
	} while (unw_unwind(running) >= 0);

	return;
}

static void kgdb_init_running(struct unw_frame_info *unw, void *data)
{
	struct pt_regs *regs;

	regs = data;
	normalize(unw, regs);
	smp_unw[smp_processor_id()].unw = unw;
	kgdb_wait(regs);
}

void kgdb_wait_ipi(struct pt_regs *regs)
{
	struct unw_frame_info unw;

	smp_unw[smp_processor_id()].task = current;

	if (user_mode(regs)) {
		smp_unw[smp_processor_id()].unw = (struct unw_frame_info *)1;
		kgdb_wait(regs);
	} else {
		if (current->state == TASK_RUNNING)
			unw_init_running(kgdb_init_running, regs);
		else {
			if (kgdb_get_blocked_state(current, &unw))
				smp_unw[smp_processor_id()].unw =
				    (struct unw_frame_info *)1;
			else
				smp_unw[smp_processor_id()].unw = &unw;
			kgdb_wait(regs);
		}
	}

	smp_unw[smp_processor_id()].unw = NULL;
	return;
}

void kgdb_roundup_cpus(unsigned long flags)
{
	if (num_online_cpus() > 1)
		smp_send_nmi_allbutself();
}

static volatile int kgdb_hwbreak_sstep[NR_CPUS];

static int kgdb_notify(struct notifier_block *self, unsigned long cmd,
	void *ptr)
{
	struct die_args *args = ptr;
	struct pt_regs *regs = args->regs;
	unsigned long err = args->err;

	switch (cmd) {
	default:
		return NOTIFY_DONE;
	case DIE_PAGE_FAULT_NO_CONTEXT:
		if (atomic_read(&debugger_active) && kgdb_may_fault) {
			kgdb_fault_longjmp(kgdb_fault_jmp_regs);
			return NOTIFY_STOP;
		}
		break;
	case DIE_BREAK:
		if (user_mode(regs) || err == 0x80001)
			return NOTIFY_DONE;
		break;
	case DIE_FAULT:
		if (user_mode(regs))
			return NOTIFY_DONE;
		else if (err == 36 && kgdb_hwbreak_sstep[smp_processor_id()]) {
			kgdb_hwbreak_sstep[smp_processor_id()] = 0;
			regs->cr_ipsr &= ~IA64_PSR_SS;
			return NOTIFY_STOP;
		}
	case DIE_MCA_MONARCH_PROCESS:
	case DIE_INIT_MONARCH_PROCESS:
		break;
	}

	kgdb_handle_exception(args->trapnr, args->signr, args->err, regs);
	return NOTIFY_STOP;
}

static struct notifier_block kgdb_notifier = {
	.notifier_call = kgdb_notify,
};

int kgdb_arch_init(void)
{
	atomic_notifier_chain_register(&ia64die_chain, &kgdb_notifier);
	return 0;
}

static void do_kgdb_handle_exception(struct unw_frame_info *, void *data);

struct kgdb_state {
	int e_vector;
	int signo;
	unsigned long err_code;
	struct pt_regs *regs;
	struct unw_frame_info *unw;
	char *inbuf;
	char *outbuf;
	int unwind;
	int ret;
};

static void inline kgdb_pc(struct pt_regs *regs, unsigned long pc)
{
	regs->cr_iip = pc & ~0xf;
	ia64_psr(regs)->ri = pc & 0x3;
	return;
}

int kgdb_arch_handle_exception(int e_vector, int signo,
			       int err_code, char *remcom_in_buffer,
			       char *remcom_out_buffer,
			       struct pt_regs *linux_regs)
{
	struct kgdb_state info;

	info.e_vector = e_vector;
	info.signo = signo;
	info.err_code = err_code;
	info.unw = (void *)0;
	info.inbuf = remcom_in_buffer;
	info.outbuf = remcom_out_buffer;
	info.unwind = 0;
	info.ret = -1;

	if (remcom_in_buffer[0] == 'c' || remcom_in_buffer[0] == 's') {
		info.regs = linux_regs;
		do_kgdb_handle_exception(NULL, &info);
	} else if (kgdb_usethread == current) {
		info.regs = linux_regs;
		info.unwind = 1;
		unw_init_running(do_kgdb_handle_exception, &info);
	} else if (kgdb_usethread->state != TASK_RUNNING) {
		struct unw_frame_info unw_info;

		if (kgdb_get_blocked_state(kgdb_usethread, &unw_info)) {
			info.ret = 1;
			goto bad;
		}
		info.regs = NULL;
		do_kgdb_handle_exception(&unw_info, &info);
	} else {
		int i;

		for (i = 0; i < NR_CPUS; i++)
			if (smp_unw[i].task == kgdb_usethread && smp_unw[i].unw
			    && smp_unw[i].unw != (struct unw_frame_info *)1) {
				info.regs = NULL;
				do_kgdb_handle_exception(smp_unw[i].unw, &info);
				break;
			} else {
				info.ret = 1;
				goto bad;
			}
	}

      bad:
	if (info.ret != -1 && remcom_in_buffer[0] == 'p') {
		unsigned long bad = 0xbad4badbadbadbadUL;

		printk("kgdb_arch_handle_exception: p packet bad (%s)\n",
		       remcom_in_buffer);
		kgdb_mem2hex((char *)&bad, remcom_out_buffer, sizeof(bad));
		remcom_out_buffer[sizeof(bad) * 2] = 0;
		info.ret = -1;
	}
	return info.ret;
}

/*
 * This is done because I evidently made an incorrect 'p' encoding
 * when my patch for gdb was committed. It was later corrected. This
 * check supports both my wrong encoding of the register number and
 * the correct encoding. Eventually this should be eliminated and
 * kgdb_hex2long should be demarshalling the regnum.
 */
static inline int check_packet(unsigned int regnum, char *packet)
{
	static int check_done, swap;
	unsigned long reglong;

	if (likely(check_done)) {
		if (swap) {
			kgdb_hex2long(&packet, &reglong);
			regnum = (int) reglong;
		}

	} else {
		if (regnum > NUM_REGS) {
			kgdb_hex2long(&packet, &reglong);
			regnum = (int) reglong;
			swap = 1;
		}
		check_done = 1;
	}
	return regnum;
}

static void do_kgdb_handle_exception(struct unw_frame_info *unw_info,
	void *data)
{
	long addr;
	char *ptr;
	unsigned long newPC;
	int e_vector, signo;
	unsigned long err_code;
	struct pt_regs *linux_regs;
	struct kgdb_state *info;
	char *remcom_in_buffer, *remcom_out_buffer;

	info = data;
	info->unw = unw_info;
	e_vector = info->e_vector;
	signo = info->signo;
	err_code = info->err_code;
	remcom_in_buffer = info->inbuf;
	remcom_out_buffer = info->outbuf;
	linux_regs = info->regs;

	if (info->unwind)
		normalize(unw_info, linux_regs);

	switch (remcom_in_buffer[0]) {
	case 'p':
		{
			unsigned int regnum;

			kgdb_hex2mem(&remcom_in_buffer[1], (char *)&regnum,
				     sizeof(regnum));
			regnum = check_packet(regnum, &remcom_in_buffer[1]);
			if (regnum >= NUM_REGS) {
				remcom_out_buffer[0] = 'E';
				remcom_out_buffer[1] = 0;
			} else
				kgdb_get_reg(remcom_out_buffer, regnum,
					     unw_info, linux_regs);
			break;
		}
	case 'P':
		{
			unsigned int regno;
			long v;
			char *ptr;

			ptr = &remcom_in_buffer[1];
			if ((!kgdb_usethread || kgdb_usethread == current) &&
			    kgdb_hex2long(&ptr, &v) &&
			    *ptr++ == '=' && (v >= 0)) {
				regno = (unsigned int)v;
				regno = (regno >= NUM_REGS ? 0 : regno);
				kgdb_put_reg(ptr, remcom_out_buffer, regno,
					     unw_info, linux_regs);
			} else
				strcpy(remcom_out_buffer, "E01");
			break;
		}
	case 'c':
	case 's':
		if (e_vector == TRAP_BRKPT && err_code == KGDBBREAKNUM) {
			if (ia64_psr(linux_regs)->ri < 2)
				kgdb_pc(linux_regs, linux_regs->cr_iip +
					ia64_psr(linux_regs)->ri + 1);
			else
				kgdb_pc(linux_regs, linux_regs->cr_iip + 16);
		}

		/* try to read optional parameter, pc unchanged if no parm */
		ptr = &remcom_in_buffer[1];
		if (kgdb_hex2long(&ptr, &addr)) {
			linux_regs->cr_iip = addr;
		}
		newPC = linux_regs->cr_iip;

		/* clear the trace bit */
		linux_regs->cr_ipsr &= ~IA64_PSR_SS;

		atomic_set(&cpu_doing_single_step, -1);

		/* set the trace bit if we're stepping or took a hardware break */
		if (remcom_in_buffer[0] == 's' || e_vector == TRAP_HWBKPT) {
			linux_regs->cr_ipsr |= IA64_PSR_SS;
			debugger_step = 1;
			if (kgdb_contthread)
				atomic_set(&cpu_doing_single_step,
					   smp_processor_id());
		}

		kgdb_correct_hw_break();

		/* if not hardware breakpoint, then reenable them */
		if (e_vector != TRAP_HWBKPT)
			linux_regs->cr_ipsr |= IA64_PSR_DB;
		else {
			kgdb_hwbreak_sstep[smp_processor_id()] = 1;
			linux_regs->cr_ipsr &= ~IA64_PSR_DB;
		}

		info->ret = 0;
		break;
	default:
		break;
	}

	return;
}

struct kgdb_arch arch_kgdb_ops = {
	.set_hw_breakpoint = kgdb_arch_set_hw_breakpoint,
	.remove_hw_breakpoint = kgdb_arch_remove_hw_breakpoint,
	.gdb_bpt_instr = {0xcc},
	.flags = KGDB_HW_BREAKPOINT,
};
