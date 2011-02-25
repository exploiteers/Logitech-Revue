/*
 * TI DaVinci GPIO Support
 *
 * Copyright (c) 2006 David Brownell
 * Copyright (c) 2007, MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>

#include <asm/arch/irqs.h>
#include <asm/arch/hardware.h>
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>
#include <linux/proc_fs.h>


/* If a new chip is added with number of GPIO greater than 128, please
   update DAVINCI_MAX_N_GPIO in include/asm-arm/arch-davinci/irqs.h */
#define DM646x_N_GPIO	48
#define DM644x_N_GPIO	71
#define DM3xx_N_GPIO	104

#if defined(CONFIG_ARCH_DAVINCI644x) || defined(CONFIG_ARCH_DAVINCI_DM357)
static DECLARE_BITMAP(dm644x_gpio_in_use, DM644x_N_GPIO);
struct gpio_bank gpio_bank_dm6446 = {
	.base		= DAVINCI_GPIO_BASE,
	.num_gpio	= DM644x_N_GPIO,
	.irq_num	= IRQ_DM644X_GPIOBNK0,
	.irq_mask	= 0x1f,
	.in_use		= dm644x_gpio_in_use,
};
#endif
#ifdef CONFIG_ARCH_DAVINCI_DM355
static DECLARE_BITMAP(dm355_gpio_in_use, DM3xx_N_GPIO);
struct gpio_bank gpio_bank_dm355 = {
	.base		= DAVINCI_GPIO_BASE,
	.num_gpio	= DM3xx_N_GPIO,
	.irq_num	= IRQ_DM355_GPIOBNK0,
	.irq_mask	= 0x7f,
	.in_use		= dm355_gpio_in_use,
};
#endif
#ifdef CONFIG_ARCH_DAVINCI_DM365
static DECLARE_BITMAP(dm365_gpio_in_use, DM3xx_N_GPIO);
struct gpio_bank gpio_bank_dm365 = {
	.base		= DAVINCI_GPIO_BASE,
	.num_gpio	= DM3xx_N_GPIO,
	.irq_num	= 0,
	.irq_mask	= 0x7f,
	.in_use		= dm365_gpio_in_use,
};
#endif
#ifdef CONFIG_ARCH_DAVINCI_DM646x
static DECLARE_BITMAP(dm646x_gpio_in_use, DM646x_N_GPIO);
struct gpio_bank gpio_bank_dm646x = {
	.base		= DAVINCI_GPIO_BASE,
	.num_gpio	= DM646x_N_GPIO,
	.irq_num	= IRQ_DM646X_GPIOBNK0,
	.irq_mask	= 0x7,
	.in_use		= dm646x_gpio_in_use,
};
#endif

void davinci_gpio_init(void)
{
	struct gpio_bank *gpio_bank;

#if defined(CONFIG_ARCH_DAVINCI644x) || defined(CONFIG_ARCH_DAVINCI_DM357)
	if (cpu_is_davinci_dm644x() || cpu_is_davinci_dm357())
		gpio_bank = &gpio_bank_dm6446;
#endif
#ifdef CONFIG_ARCH_DAVINCI_DM355
	if (cpu_is_davinci_dm355())
		gpio_bank = &gpio_bank_dm355;
#endif
#ifdef CONFIG_ARCH_DAVINCI_DM365
	if (cpu_is_davinci_dm365())
		gpio_bank = &gpio_bank_dm365;
#endif
#ifdef CONFIG_ARCH_DAVINCI_DM646x
	if (cpu_is_davinci_dm6467())
		gpio_bank = &gpio_bank_dm646x;
#endif
	if (!gpio_bank)
		BUG();

	davinci_gpio_irq_setup(gpio_bank);
}
#if CONFIG_PROC_FS

#define MAX_BUF		4*1024
static struct proc_dir_entry *gio_procdir;
static struct proc_dir_entry *gio_proc[DM3xx_N_GPIO];

static int proc_gio(char *page, char **start, off_t off,
		    int count, int *eof, void *data)
{
	int len = 0;
	char buffer[MAX_BUF];
	volatile unsigned long *reg;

	if (!count)
		return 0;

	reg = (unsigned long *)IO_ADDRESS(DAVINCI_GPIO_BASE);
	len += sprintf(&buffer[len], "GPIO Module:\n");
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x PID                \n", reg,
		    (int)reg[0]);
	reg += 2;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x BINTEN             \n", reg,
		    (int)reg[0]);

	//BANK 0 AND BANK 1
	len += sprintf(&buffer[len], "GPIO Bank0 and Bank1:\n");
	reg = (unsigned long *)IO_ADDRESS(DAVINCI_GPIO_BASE + 0x10);
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Direction          \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Output Data        \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Set Data           \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Clear Data         \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Input Data         \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Set Rising edge   \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Clear Rising edge \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Set Falling edge  \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Clear Falling edge\n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Interrupt Status   \n", reg,
		    (int)reg[0]);

	//BANK 2 AND BANK 3
	len += sprintf(&buffer[len], "GPIO Bank2 and Bank3:\n");
	reg = (unsigned long *)IO_ADDRESS(DAVINCI_GPIO_BASE + 0x38);
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Direction          \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Output Data        \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Set Data           \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Clear Data         \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Input Data         \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Set Rising edge   \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Clear Rising edge \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Set Falling edge  \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Clear Falling edge\n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Interrupt Status   \n", reg,
		    (int)reg[0]);

	//BANK 4 AND BANK 5 for DM355 and BANK 4 for DM6446
	if (cpu_is_davinci_dm355())
		len += sprintf(&buffer[len], "GPIO Bank4 and Bank5:\n");
	else
		len += sprintf(&buffer[len], "GPIO Bank4:\n");
	reg = (unsigned long *)IO_ADDRESS(DAVINCI_GPIO_BASE + 0x60);
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Direction          \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Output Data        \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Set Data           \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Clear Data         \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Input Data         \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Set Rising edge   \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Clear Rising edge \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Set Falling edge  \n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Clear Falling edge\n", reg,
		    (int)reg[0]);
	reg++;
	len +=
	    sprintf(&buffer[len], "  0x%p: 0x%08x Interrupt Status   \n", reg,
		    (int)reg[0]);

	//BANK 6
	if (cpu_is_davinci_dm355()) 
	{
		len += sprintf(&buffer[len], "GPIO Bank6:\n");
		reg = (unsigned long *)IO_ADDRESS(DAVINCI_GPIO_BASE + 0x88);
		len +=
		    sprintf(&buffer[len], "  0x%p: 0x%08x Direction          \n", reg,
			    (int)reg[0]);
		reg++;
		len +=
	    	sprintf(&buffer[len], "  0x%p: 0x%08x Output Data        \n", reg,
		    	(int)reg[0]);
		reg++;
		len +=
	    	sprintf(&buffer[len], "  0x%p: 0x%08x Set Data           \n", reg,
			    (int)reg[0]);
		reg++;
		len +=
		    sprintf(&buffer[len], "  0x%p: 0x%08x Clear Data         \n", reg,
			    (int)reg[0]);
		reg++;
		len +=
	    	sprintf(&buffer[len], "  0x%p: 0x%08x Input Data         \n", reg,
		    	(int)reg[0]);
		reg++;
		len +=
	    	sprintf(&buffer[len], "  0x%p: 0x%08x Set Rising edge   \n", reg,
		    	(int)reg[0]);
		reg++;
		len +=
	    	sprintf(&buffer[len], "  0x%p: 0x%08x Clear Rising edge \n", reg,
		    	(int)reg[0]);
		reg++;
		len +=
	    	sprintf(&buffer[len], "  0x%p: 0x%08x Set Falling edge  \n", reg,
		    	(int)reg[0]);
		reg++;
		len +=
	    	sprintf(&buffer[len], "  0x%p: 0x%08x Clear Falling edge\n", reg,
		    	(int)reg[0]);
		reg++;
		len +=
	    	sprintf(&buffer[len], "  0x%p: 0x%08x Interrupt Status   \n", reg,
		    	(int)reg[0]);
	}
	if (count > len)
		count = len;
	memcpy(page, &buffer[off], count);

	*eof = 1;
	*start = NULL;
	return len;
}

static int proc_gio_read(char *page, char **start, off_t off,
			 int count, int *eof, void *data)
{
	int len;

	if (count == 0)
		return 0;

	gpio_direction_input((int)data);//gpio_set_direction((int)data, GIO_DIR_INPUT);
	len = sprintf(page, "%d\n", gpio_get_value((int)data));

	*eof = 1;
	*start = 0;

	return len;
}

static int proc_gio_write(struct file *file, const char __user * buffer,
			  unsigned long count, void *data)
{
	int i;

	if (!count)
		return 0;

	//gpio_set_direction((int)data, GIO_DIR_OUTPUT);

	for (i = 0; i < count; i++) {
		if (buffer[i] == '0')
		{
			gpio_direction_output((int)data,0);//gpio_set_value((int)data, 0);
		}
		if (buffer[i] == '1')
		{
			gpio_direction_output((int)data,1);//gpio_set_value((int)data, 1);
		}
	}

	return count;
}

int __init gio_proc_client_create(void)
{
	int i;
	char name[16];
	struct proc_dir_entry *ent;
	u32 max_gpio = 0;

	/*if (cpu_is_davinci_dm355())
		max_gpio = DM355_NUM_GIOS;
	else if (cpu_is_davinci_dm6443())
		max_gpio = DM644x_NUM_GIOS;
	*/
	max_gpio = DM3xx_N_GPIO;

	gio_procdir = proc_mkdir("gio", 0);
	if (gio_procdir == NULL) {
		printk(KERN_ERR "gio: failed to register proc directory gio\n");
		return -ENOMEM;
	}

	ent = create_proc_read_entry("registers", 0, gio_procdir, proc_gio, 0);
	if (ent == NULL) {
		printk(KERN_ERR
		       "gio: failed to register proc device: registers\n");
		return -ENOMEM;
	}

	for (i = 0; i < max_gpio; i++) {
		sprintf(name, "gio%d", i);
		gio_proc[i] = create_proc_entry(name, 0, gio_procdir);
		if (gio_proc[i] == NULL) {
			printk(KERN_ERR
			       "gio: failed to register proc device: %s\n",
			       name);
			return -ENOMEM;
		}
		gio_proc[i]->data = (void *)i;
		gio_proc[i]->read_proc = proc_gio_read;
		gio_proc[i]->write_proc = proc_gio_write;
	}

	return 0;
}

#else				/* CONFIG_PROC_FS */

#define gio_proc_client_create() do {} while(0);

#endif				/* CONFIG_PROC_FS */

module_init(gio_proc_client_create);

