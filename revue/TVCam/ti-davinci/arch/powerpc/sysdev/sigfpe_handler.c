/*
 * arch/powerpc/sysdev/sigfpe_handler.c
 *
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Ebony Zhu, ebony.zhu@freescale.com
 *
 * Derived from arch/powerpc/math-emu/math.c
 * Copyright (C) 1999  Eddie C. Dost  (ecd@atecom.com)
 *
 * Description:
 * This file is the exception handler to make E500 SPE instructions
 * fully comply with IEEE-754 floating point standard.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/types.h>
#include <asm/uaccess.h>
#include <asm/reg.h>

#define SPEFUNC(x)	extern int x(void *, void *, void *, void *)
#define efdabs	fabs
#define efdneg	fneg

/* Scalar SPFP functions */
SPEFUNC(efsabs);
SPEFUNC(efsadd);
SPEFUNC(efscfd);
SPEFUNC(efscmpeq);
SPEFUNC(efscmpgt);
SPEFUNC(efscmplt);
SPEFUNC(efsctsf);
SPEFUNC(efsctsi);
SPEFUNC(efsctsiz);
SPEFUNC(efsctuf);
SPEFUNC(efsctui);
SPEFUNC(efsctuiz);
SPEFUNC(efsdiv);
SPEFUNC(efsmul);
SPEFUNC(efsnabs);
SPEFUNC(efsneg);
SPEFUNC(efssub);

/* Vector Floating-Point functions */
SPEFUNC(evfsabs);
SPEFUNC(evfsadd);
SPEFUNC(evfscmpeq);
SPEFUNC(evfscmpgt);
SPEFUNC(evfscmplt);
SPEFUNC(evfsctsf);
SPEFUNC(evfsctsi);
SPEFUNC(evfsctsiz);
SPEFUNC(evfsctuf);
SPEFUNC(evfsctui);
SPEFUNC(evfsctuiz);
SPEFUNC(evfsdiv);
SPEFUNC(evfsmul);
SPEFUNC(evfsnabs);
SPEFUNC(evfsneg);
SPEFUNC(evfssub);

/* Scalar DPFP functions */
SPEFUNC(efdabs);
SPEFUNC(efdadd);
SPEFUNC(efdcfs);
SPEFUNC(efdcmpeq);
SPEFUNC(efdcmpgt);
SPEFUNC(efdcmplt);
SPEFUNC(efdctsf);
SPEFUNC(efdctsi);
SPEFUNC(efdctsidz);
SPEFUNC(efdctsiz);
SPEFUNC(efdctuf);
SPEFUNC(efdctui);
SPEFUNC(efdctuidz);
SPEFUNC(efdctuiz);
SPEFUNC(efddiv);
SPEFUNC(efdmul);
SPEFUNC(efdnabs);
SPEFUNC(efdneg);
SPEFUNC(efdsub);

#define VCT		0x4
#define SPFP		0x6
#define DPFP		0x7
#define EFAPU		0x4

#define EFSADD		0x2c0
#define EFSSUB		0x2c1
#define EFSABS		0x2c4
#define EFSNABS		0x2c5
#define EFSNEG		0x2c6
#define EFSMUL		0x2c8
#define EFSDIV		0x2c9
#define EFSCMPGT	0x2cc
#define EFSCMPLT	0x2cd
#define EFSCMPEQ	0x2ce
#define EFSCFD		0x2cf
#define EFSCTUI		0x2d4
#define EFSCTSI		0x2d5
#define EFSCTUF		0x2d6
#define EFSCTSF		0x2d7
#define EFSCTUIZ	0x2d8
#define EFSCTSIZ	0x2da

#define EVFSADD		0x280
#define EVFSSUB		0x281
#define EVFSABS		0x284
#define EVFSNABS	0x285
#define EVFSNEG		0x286
#define EVFSMUL		0x288
#define EVFSDIV		0x289
#define EVFSCMPGT	0x28c
#define EVFSCMPLT	0x28d
#define EVFSCMPEQ	0x28e
#define EVFSCTUI	0x294
#define EVFSCTSI	0x295
#define EVFSCTUF	0x296
#define EVFSCTSF	0x297
#define EVFSCTUIZ	0x298
#define EVFSCTSIZ	0x29a

#define EFDADD		0x2e0
#define EFDSUB		0x2e1
#define EFDABS		0x2e4
#define EFDNABS		0x2e5
#define EFDNEG		0x2e6
#define EFDMUL		0x2e8
#define EFDDIV		0x2e9
#define EFDCTUIDZ	0x2ea
#define EFDCTSIDZ	0x2eb
#define EFDCMPGT	0x2ec
#define EFDCMPLT	0x2ed
#define EFDCMPEQ	0x2ee
#define EFDCFS		0x2ef
#define EFDCTUI		0x2f4
#define EFDCTSI		0x2f5
#define EFDCTUF		0x2f6
#define EFDCTSF		0x2f7
#define EFDCTUIZ	0x2f8
#define EFDCTSIZ	0x2fa

#define AB	2
#define XA	3
#define XB	4
#define XCR	5

static u64 fullgprs[32];
static int insn_decode(struct pt_regs *regs, u32 speinsn);
static int (*func)(void *, void *, void *, void *);
static void *op0 = 0, *op1 = 0, *op2 = 0, *op3 = 0;
static int type = 0, flag;

int
spedata_handler(struct pt_regs *regs)
{
	u32 speinsn;
	int ret = 0;
	int i;

	if (get_user(speinsn, (unsigned int __user *) regs->nip))
		return -EFAULT;
	if ((speinsn >> 26) != 4)
		return -EINVAL;         /* not an spe instruction */

	preempt_disable();

	switch ((speinsn >> 5) & 0x7 ) {
	case SPFP:
		for(i = 0; i < 32; i++) {
			fullgprs[i] = regs->gpr[i];
			fullgprs[i] = fullgprs[i] << 32 | current->thread.evr[i];
		};
		break;
	default:
		for(i = 0; i < 32; i++) {
			fullgprs[i] = current->thread.evr[i];
			fullgprs[i] = (fullgprs[i] << 32) | (regs->gpr[i]);
		};
	}

	if (insn_decode(regs, speinsn) == -ENOSYS) {
		ret = -ENOSYS;
		goto out;
	}
	flag = func(op0, op1, op2, op3);

	switch ((speinsn >> 5) & 0x7 ) {
	case SPFP:
		for (i = 0; i < 32; i++) {
			regs->gpr[i] = fullgprs[i] >> 32;
		};
		break;
	default:
		for (i = 0; i < 32; i++) {
			regs->gpr[i] = fullgprs[i];
			current->thread.evr[i] = fullgprs[i] >> 32;
		};
	}
out:
	preempt_enable();
	return ret;
}

int
speround_handler(struct pt_regs *regs)
{
	u32 speinsn;
	int ret = 0;
	u32 lsb_lo, lsb_hi;
	int s_lo, s_hi;
	int rD;

	if (get_user(speinsn, (unsigned int __user *) regs->nip))
		return -EFAULT;
	if ((speinsn >> 26) != 4)
		return -EINVAL;         /* not an spe instruction */

	preempt_disable();

	rD = (speinsn >> 21) & 0x1f;
	flag = insn_decode(regs, speinsn);
	if (type == XCR) {
		ret = -ENOSYS;
		goto out;
	}

	s_lo = (regs->gpr[rD] & 0x80000000) >> 31;
	s_hi = (current->thread.evr[rD] & 0x80000000) >> 31;
	lsb_lo = regs->gpr[rD];
	lsb_hi = current->thread.evr[rD];
	switch ((speinsn >> 5) & 0x7 ) {
	/* Since SPE instructions on E500 core can handle round to nearest
	 * and round toward zero with IEEE-754 complied, we just need
	 * to handle round toward +Inf and round toward -Inf by software.
	 */
	case SPFP:
		if ((current->thread.spefscr & 0x3) == 0x2) { /* round to +Inf */
			if (!s_lo) lsb_lo++; /* Z > 0, choose Z1 */
		} else { /* round to -Inf */
			if (s_lo) lsb_lo++; /* Z < 0, choose Z2 */
		}
		regs->gpr[rD] = lsb_lo;
		break;
	case DPFP:
		fullgprs[rD] = current->thread.evr[rD];
		fullgprs[rD] = (fullgprs[rD] << 32) | (regs->gpr[rD]);

		if ((current->thread.spefscr & 0x3) == 0x2) { /* round to +Inf */
			if (!s_hi) fullgprs[rD]++; /* Z > 0, choose Z1 */
		} else { /* round to -Inf */
			if (s_hi) fullgprs[rD]++; /* Z < 0, choose Z2 */
		}
		regs->gpr[rD] = fullgprs[rD];
		current->thread.evr[rD] = fullgprs[rD] >> 32;
		break;
	case VCT:
		if ((current->thread.spefscr & 0x3) == 0x2) { /* round to +Inf */
			if (!s_lo) lsb_lo++; /* Z_low > 0, choose Z1 */
			if (!s_hi) lsb_hi++; /* Z_high word > 0, choose Z1 */
		} else { /* round to -Inf */
			if (s_lo) lsb_lo++; /* Z_low < 0, choose Z2 */
			if (s_hi) lsb_hi++; /* Z_high < 0, choose Z2 */
		}
		regs->gpr[rD] = lsb_lo;
		current->thread.evr[rD] = lsb_hi;
		break;
	default:
		ret = -EINVAL;
		goto out;
	}
out:
	preempt_enable();
	return ret;
}

static int insn_decode(struct pt_regs *regs, u32 speinsn)
{
	switch (speinsn >> 26) {

	case EFAPU:
		switch (speinsn & 0x7ff) {
		case EFSABS:	func = efsabs;		type = XA;	break;
		case EFSADD:	func = efsadd;		type = AB;      break;
		case EFSCFD:	func = efscfd;		type = XB;	break;
		case EFSCMPEQ:	func = efscmpeq;	type = XCR;	break;
		case EFSCMPGT:	func = efscmpgt;	type = XCR;	break;
		case EFSCMPLT:	func = efscmplt;	type = XCR;	break;
		case EFSCTSF:	func = efsctsf;		type = XB;	break;
		case EFSCTSI:	func = efsctsi;		type = XB;	break;
		case EFSCTSIZ:	func = efsctsiz;	type = XB;	break;
		case EFSCTUF:	func = efsctuf;		type = XB;	break;
		case EFSCTUI:	func = efsctui;		type = XB;	break;
		case EFSCTUIZ:	func = efsctuiz;	type = XB;	break;
		case EFSDIV:	func = efsdiv;		type = AB;	break;
		case EFSMUL:	func = efsmul;		type = AB;	break;
		case EFSNABS:	func = efsnabs;		type = XA;	break;
		case EFSNEG:	func = efsneg;		type = XA;	break;
		case EFSSUB:	func = efssub;		type = AB;	break;

		case EVFSABS:	func = evfsabs;		type = XA;	break;
		case EVFSADD:	func = evfsadd;		type = AB;      break;
		case EVFSCMPEQ:	func = evfscmpeq;	type = XCR;	break;
		case EVFSCMPGT:	func = evfscmpgt;	type = XCR;	break;
		case EVFSCMPLT:	func = evfscmplt;	type = XCR;	break;
		case EVFSCTSF:	func = evfsctsf;	type = XB;	break;
		case EVFSCTSI:	func = evfsctsi;	type = XB;	break;
		case EVFSCTSIZ:	func = evfsctsiz;	type = XB;	break;
		case EVFSCTUF:	func = evfsctuf;	type = XB;	break;
		case EVFSCTUI:	func = evfsctui;	type = XB;	break;
		case EVFSCTUIZ:	func = evfsctuiz;	type = XB;	break;
		case EVFSDIV:	func = evfsdiv;		type = AB;	break;
		case EVFSMUL:	func = evfsmul;		type = AB;	break;
		case EVFSNABS:	func = evfsnabs;	type = XA;	break;
		case EVFSNEG:	func = evfsneg;		type = XA;	break;
		case EVFSSUB:	func = evfssub;		type = AB;	break;

		case EFDABS:	func = efdabs;		type = XA;	break;
		case EFDADD:	func = efdadd;		type = AB;	break;
		case EFDCFS:	func = efdcfs;		type = XB;	break;
		case EFDCMPEQ:	func = efdcmpeq;	type = XCR;	break;
		case EFDCMPGT:	func = efdcmpgt;	type = XCR;	break;
		case EFDCMPLT:	func = efdcmplt;	type = XCR;	break;
		case EFDCTSF:	func = efdctsf;		type = XB;	break;
		case EFDCTSI:	func = efdctsi;		type = XB;	break;
		case EFDCTSIDZ:	func = efdctsidz;	type = XB;	break;
		case EFDCTSIZ:	func = efdctsiz;	type = XB;	break;
		case EFDCTUF:	func = efdctuf;		type = XB;	break;
		case EFDCTUI:	func = efdctui;		type = XB;	break;
		case EFDCTUIDZ:	func = efdctuidz;	type = XB;	break;
		case EFDCTUIZ:	func = efdctuiz;	type = XB;	break;
		case EFDDIV:	func = efddiv;		type = AB;	break;
		case EFDMUL:	func = efdmul;		type = AB;	break;
		case EFDNABS:	func = efdnabs;		type = XA;	break;
		case EFDNEG:	func = efdneg;		type = XA;	break;
		case EFDSUB:	func = efdsub;		type = AB;	break;
		default:
			goto illegal;
		}
		break;
	default:
		goto illegal;
	}

	switch (type) {
	case AB:
		op0 = &fullgprs[(speinsn >> 21) & 0x1f];
		op1 = &fullgprs[(speinsn >> 16) & 0x1f];
		op2 = &fullgprs[(speinsn >> 11) & 0x1f];
		break;

	case XA:
		op0 = &fullgprs[(speinsn >> 21) & 0x1f];
		op1 = &fullgprs[(speinsn >> 16) & 0x1f];
		break;

	case XB:
		op0 = &fullgprs[(speinsn >> 21) & 0x1f];
		op1 = &fullgprs[(speinsn >> 11) & 0x1f];
		break;

	case XCR:
		op0 = (void *)&regs->ccr;
		op1 = (void *)((speinsn >> 23) & 0x7);
		op2 = &fullgprs[(speinsn >> 16) & 0x1f];
		op3 = &fullgprs[(speinsn >> 11) & 0x1f];
		break;

	default:
		goto illegal;
	}
	return 0;
illegal:
	printk(KERN_ERR "\nOoops! IEEE-754 compliance handler encountered un-supported instruction.\n");
	return -ENOSYS;
}
