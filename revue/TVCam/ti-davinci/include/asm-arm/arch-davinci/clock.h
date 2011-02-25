/*
 * include/asm-arm/arch-davinci/clock.h
 *
 * Clock control driver for DaVinci - header file
 *
 * Author: Vladimir Barinov
 * Copyright (C) 2007-2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */
#ifndef __ASM_ARCH_DAVINCI_CLOCK_H
#define __ASM_ARCH_DAVINCI_CLOCK_H

/* PLL/Reset register offsets */
#define PLLM		0x110
#define PREDIV		0x114
#define PLLDIV2		0x11C
#define POSTDIV		0x128
#define PLLDIV4		0x160
#define PLLDIV5		0x164
#define PLLDIV6		0x168
#define PLLDIV7		0x16C
#define PLLDIV8		0x170

struct clk {
	struct list_head	node;
	struct module		*owner;
	const char		*name;
	unsigned int		*rate;
	int			id;
	s8			usecount;
	u8			flags;
	u8			lpsc;
	u8			ctlr;
};

/* Clock flags */
#define RATE_CKCTL		0x01
#define RATE_FIXED		0x02
#define RATE_PROPAGATES 	0x04
#define VIRTUAL_CLOCK		0x08
#define ALWAYS_ENABLED		0x10
#define ENABLE_REG_32BIT	0x20

extern int  clk_register(struct clk *clk);
extern void clk_unregister(struct clk *clk);
extern int  davinci_clk_init(void);
extern void davinci_psc_register(unsigned long *bases, unsigned num);
extern int  davinci_psc_config(unsigned domain, unsigned ctlr, unsigned id,
			       int enable);
extern int  davinci_enable_clks(struct clk *clk_list, int num_clks);

extern void (*davinci_module_setup)(unsigned ctlr, unsigned id, int enable);

#endif
