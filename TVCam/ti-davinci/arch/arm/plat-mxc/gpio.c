/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*
 * Implementation based on omap gpio.c
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/arch/irqs.h>
#include <asm/mach/irq.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>

/*!
 * @file plat-mxc/gpio.c
 *
 * @brief This file contains the GPIO implementation details.
 *
 * @ingroup GPIO
 */

/* GPIO related defines */
#if defined(CONFIG_ARCH_MX2)
enum gpio_reg {
	GPIO_DR = 0x1C,
	GPIO_GDIR = 0x00,
	GPIO_PSR = 0x24,
	GPIO_ICR1 = 0x028,
	GPIO_ICR2 = 0x2C,
	GPIO_IMR = 0x30,
	GPIO_ISR = 0x34,
};
#else
enum gpio_reg {
	GPIO_DR = 0x00,
	GPIO_GDIR = 0x04,
	GPIO_PSR = 0x08,
	GPIO_ICR1 = 0x0C,
	GPIO_ICR2 = 0x10,
	GPIO_IMR = 0x14,
	GPIO_ISR = 0x18,
};
#endif

extern struct mxc_gpio_port mxc_gpio_ports[];

struct gpio_port {
	u32 num;		/*!< gpio port number */
	u32 base;		/*!< gpio port base VA */
	u16 irq;		/*!< irq number to the core */
	u16 virtual_irq_start;	/*!< virtual irq start number */
	u32 reserved_map;	/*!< keep track of which pins are in use */
	u32 irq_is_level_map;	/*!< if a pin's irq is level sensitive. default is edge */
	spinlock_t lock;	/*!< lock when operating on the port */
};
static struct gpio_port gpio_port[GPIO_PORT_NUM];

/*
 * Find the pointer to the gpio_port for a given pin.
 * @param gpio		a gpio pin number
 * @return		pointer to \b struc \b gpio_port
 */
static inline struct gpio_port *get_gpio_port(u32 gpio)
{
	return &gpio_port[GPIO_TO_PORT(gpio)];
}

/*
 * Check if a gpio pin is within [0, MXC_MAX_GPIO_LINES -1].
 * @param gpio		a gpio pin number
 * @return		0 if the pin number is valid; -1 otherwise
 */
static int check_gpio(u32 gpio)
{
	if (gpio >= MXC_MAX_GPIO_LINES) {
		printk(KERN_ERR "mxc-gpio: invalid GPIO %d\n", gpio);
		dump_stack();
		return -1;
	}
	return 0;
}

/*
 * Set a GPIO pin's direction
 * @param port		pointer to a gpio_port
 * @param index		gpio pin index value (0~31)
 * @param is_input	0 for output; non-zero for input
 */
static void _set_gpio_direction(struct gpio_port *port, u32 index, int is_input)
{
	u32 reg = port->base + GPIO_GDIR;
	u32 l;

	l = __raw_readl(reg);
	if (is_input)
		l &= ~(1 << index);
	else
		l |= 1 << index;
	__raw_writel(l, reg);
}

/*!
 * Exported function to set a GPIO pin's direction
 * @param pin		a name defined by \b iomux_pin_name_t
 * @param is_input	1 (or non-zero) for input; 0 for output
 */
void mxc_set_gpio_direction(iomux_pin_name_t pin, int is_input)
{
	struct gpio_port *port;
	u32 gpio = IOMUX_TO_GPIO(pin);

	if (check_gpio(gpio) < 0)
		return;
	port = get_gpio_port(gpio);
	spin_lock(&port->lock);
	_set_gpio_direction(port, GPIO_TO_INDEX(gpio), is_input);
	spin_unlock(&port->lock);
}
EXPORT_SYMBOL(mxc_set_gpio_direction);

/*
 * Set a GPIO pin's data output
 * @param port		pointer to a gpio_port
 * @param index		gpio pin index value (0~31)
 * @param data		value to be set (only 0 or 1 is valid)
 */
static void _set_gpio_dataout(struct gpio_port *port, u32 index, u32 data)
{
	u32 reg = port->base + GPIO_DR;
	u32 l = 0;

	l = (__raw_readl(reg) & (~(1 << index))) | (data << index);
	__raw_writel(l, reg);
}

/*!
 * Exported function to set a GPIO pin's data output
 * @param pin		a name defined by \b iomux_pin_name_t
 * @param data		value to be set (only 0 or 1 is valid)
 */
void mxc_set_gpio_dataout(iomux_pin_name_t pin, u32 data)
{
	struct gpio_port *port;
	u32 gpio = IOMUX_TO_GPIO(pin);

	if (check_gpio(gpio) < 0)
		return;

	port = get_gpio_port(gpio);
	spin_lock(&port->lock);
	_set_gpio_dataout(port, GPIO_TO_INDEX(gpio), (data == 0) ? 0 : 1);
	spin_unlock(&port->lock);
}
EXPORT_SYMBOL(mxc_set_gpio_dataout);

/*!
 * Return the data value of a GPIO signal.
 * @param pin	a name defined by \b iomux_pin_name_t
 *
 * @return 	value (0 or 1) of the GPIO signal; -1 if pass in invalid pin
 */
int mxc_get_gpio_datain(iomux_pin_name_t pin)
{
	struct gpio_port *port;
	u32 gpio = IOMUX_TO_GPIO(pin);

	if (check_gpio(gpio) < 0)
		return -1;

	port = get_gpio_port(gpio);

	return (__raw_readl(port->base + GPIO_DR) >> GPIO_TO_INDEX(gpio)) & 1;
}
EXPORT_SYMBOL(mxc_get_gpio_datain);

/*
 * Clear a GPIO signal's interrupt status
 *
 * @param port		pointer to a gpio_port
 * @param index		gpio pin index value (0~31)
 */
static inline void _clear_gpio_irqstatus(struct gpio_port *port, u32 index)
{
	__raw_writel(1 << index, port->base + GPIO_ISR);
}

/*
 * Set a GPIO pin's interrupt edge
 * @param port		pointer to a gpio_port
 * @param index		gpio pin index value (0~31)
 * @param icr		one of the values defined in \b gpio_int_cfg
 *                      to indicate how to generate an interrupt
 */
static void _set_gpio_edge_ctrl(struct gpio_port *port, u32 index,
				enum gpio_int_cfg edge)
{
	u32 reg = port->base;
	u32 l, sig;

	reg += (index <= 15) ? GPIO_ICR1 : GPIO_ICR2;
	sig = (index <= 15) ? index : (index - 16);
	l = __raw_readl(reg);
	l = (l & (~(0x3 << (sig * 2)))) | (edge << (sig * 2));
	__raw_writel(l, reg);
	_clear_gpio_irqstatus(port, index);
}

/*
 * Enable/disable a GPIO signal's interrupt.
 *
 * @param port		pointer to a gpio_port
 * @param index		gpio pin index value (0~31)
 * @param enable	\b #true for enabling the interrupt; \b #false otherwise
 */
static inline void _set_gpio_irqenable(struct gpio_port *port, u32 index,
				       bool enable)
{
	u32 reg = port->base + GPIO_IMR;
	u32 mask = (!enable) ? 0 : 1;
	u32 l;

	l = __raw_readl(reg);
	l = (l & (~(1 << index))) | (mask << index);
	__raw_writel(l, reg);
}

static inline int _request_gpio(struct gpio_port *port, u32 index)
{
	spin_lock(&port->lock);
	if (port->reserved_map & (1 << index)) {
		printk(KERN_ERR
		       "GPIO port %d (0-based), pin %d is already reserved!\n",
		       port->num, index);
		dump_stack();
		spin_unlock(&port->lock);
		return -1;
	}
	port->reserved_map |= (1 << index);
	spin_unlock(&port->lock);
	return 0;
}

/*!
 * Request ownership for a GPIO pin. The caller has to check the return value
 * of this function to make sure it returns 0 before make use of that pin.
 * @param pin		a name defined by \b iomux_pin_name_t
 * @return		0 if successful; Non-zero otherwise
 */
int mxc_request_gpio(iomux_pin_name_t pin)
{
	struct gpio_port *port;
	u32 index, gpio = IOMUX_TO_GPIO(pin);

	if (check_gpio(gpio) < 0)
		return -EINVAL;

	port = get_gpio_port(gpio);
	index = GPIO_TO_INDEX(gpio);

	return _request_gpio(port, index);
}
EXPORT_SYMBOL(mxc_request_gpio);

/*!
 * Release ownership for a GPIO pin
 * @param pin		a name defined by \b iomux_pin_name_t
 */
void mxc_free_gpio(iomux_pin_name_t pin)
{
	struct gpio_port *port;
	u32 index, gpio = IOMUX_TO_GPIO(pin);

	if (check_gpio(gpio) < 0)
		return;

	port = get_gpio_port(gpio);
	index = GPIO_TO_INDEX(gpio);

	spin_lock(&port->lock);
	if ((!(port->reserved_map & (1 << index)))) {
		printk(KERN_ERR "GPIO port %d, pin %d wasn't reserved!\n",
		       port->num, index);
		dump_stack();
		spin_unlock(&port->lock);
		return;
	}
	port->reserved_map &= ~(1 << index);
	port->irq_is_level_map &= ~(1 << index);
	_set_gpio_direction(port, index, 1);
	_set_gpio_irqenable(port, index, 0);
	_clear_gpio_irqstatus(port, index);
	spin_unlock(&port->lock);
}
EXPORT_SYMBOL(mxc_free_gpio);

/*
 * We need to unmask the GPIO port interrupt as soon as possible to
 * avoid missing GPIO interrupts for other lines in the port.
 * Then we need to mask-read-clear-unmask the triggered GPIO lines
 * in the port to avoid missing nested interrupts for a GPIO line.
 * If we wait to unmask individual GPIO lines in the port after the
 * line's interrupt handler has been run, we may miss some nested
 * interrupts.
 */
static void mxc_gpio_irq_handler(u32 irq, struct irq_desc *desc,
		struct pt_regs *regs)
{
	u32 isr_reg = 0, imr_reg = 0, imr_val;
	u32 int_valid;
	u32 gpio_irq;
	struct gpio_port *port;

	port = (struct gpio_port *)get_irq_data(irq);
	isr_reg = port->base + GPIO_ISR;
	imr_reg = port->base + GPIO_IMR;

	imr_val = __raw_readl(imr_reg);
	int_valid = __raw_readl(isr_reg) & imr_val;

	if (unlikely(!int_valid)) {
		printk(KERN_ERR "\nGPIO port: %d Spurious interrupt:0x%0x\n\n",
		       port->num, int_valid);
		BUG();		/* oops */
	}

	gpio_irq = port->virtual_irq_start;
	for (; int_valid != 0; int_valid >>= 1, gpio_irq++) {
		struct irq_desc *d;

		if ((int_valid & 1) == 0)
			continue;
		d = irq_desc + gpio_irq;
		if (unlikely(!(d->handle_irq))) {
			printk(KERN_ERR "\nGPIO port: %d irq: %d unhandeled\n",
			       port->num, gpio_irq);
			BUG();	/* oops */
		}
		d->handle_irq(gpio_irq, d, regs);
	}
}

#ifdef MXC_MUX_GPIO_INTERRUPTS
static void mxc_gpio_mux_irq_handler(u32 irq, struct irq_desc *desc,
		struct pt_regs *regs)
{
	int i;
	u32 isr_reg = 0, imr_reg = 0, imr_val;
	u32 int_valid;
	struct gpio_port *port;

	for (i = 0; i < GPIO_PORT_NUM; i++) {
		port = &gpio_port[i];
		isr_reg = port->base + GPIO_ISR;
		imr_reg = port->base + GPIO_IMR;

		imr_val = __raw_readl(imr_reg);
		int_valid = __raw_readl(isr_reg) & imr_val;

		if (int_valid) {
			set_irq_data(irq, (void *)port);
			mxc_gpio_irq_handler(irq, desc, regs);
		}
	}
}
#endif

/*
 * Disable a gpio pin's interrupt by setting the bit in the imr.
 * @param irq		a gpio virtual irq number
 */
static void gpio_mask_irq(u32 irq)
{
	u32 gpio = MXC_IRQ_TO_GPIO(irq);
	struct gpio_port *port = get_gpio_port(gpio);

	_set_gpio_irqenable(port, GPIO_TO_INDEX(gpio), 0);
}

/*
 * Acknowledge a gpio pin's interrupt by clearing the bit in the isr.
 * If the GPIO interrupt is level triggered, it also disables the interrupt.
 * @param irq		a gpio virtual irq number
 */
static void gpio_ack_irq(u32 irq)
{
	u32 gpio = MXC_IRQ_TO_GPIO(irq);
	u32 index = GPIO_TO_INDEX(gpio);
	struct gpio_port *port = get_gpio_port(gpio);

	_clear_gpio_irqstatus(port, GPIO_TO_INDEX(gpio));
	if (port->irq_is_level_map & (1 << index)) {
		gpio_mask_irq(irq);
	}
}

/*
 * Enable a gpio pin's interrupt by clearing the bit in the imr.
 * @param irq		a gpio virtual irq number
 */
static void gpio_unmask_irq(u32 irq)
{
	u32 gpio = MXC_IRQ_TO_GPIO(irq);
	struct gpio_port *port = get_gpio_port(gpio);

	_set_gpio_irqenable(port, GPIO_TO_INDEX(gpio), 1);
}

/*
 * Enable a gpio pin's interrupt by clearing the bit in the imr.
 * @param irq		a gpio virtual irq number
 */
static int gpio_set_irq_type(u32 irq, u32 type)
{
	u32 gpio = MXC_IRQ_TO_GPIO(irq);
	struct gpio_port *port = get_gpio_port(gpio);

	switch (type) {
	case IRQT_RISING:
		_set_gpio_edge_ctrl(port, GPIO_TO_INDEX(gpio),
				    GPIO_INT_RISE_EDGE);
		set_irq_handler(irq, do_edge_IRQ);
		port->irq_is_level_map &= ~(1 << GPIO_TO_INDEX(gpio));
		break;
	case IRQT_FALLING:
		_set_gpio_edge_ctrl(port, GPIO_TO_INDEX(gpio),
				    GPIO_INT_FALL_EDGE);
		set_irq_handler(irq, do_edge_IRQ);
		port->irq_is_level_map &= ~(1 << GPIO_TO_INDEX(gpio));
		break;
	case IRQT_LOW:
		_set_gpio_edge_ctrl(port, GPIO_TO_INDEX(gpio),
				    GPIO_INT_LOW_LEV);
		set_irq_handler(irq, do_level_IRQ);
		port->irq_is_level_map |= 1 << GPIO_TO_INDEX(gpio);
		break;
	case IRQT_HIGH:
		_set_gpio_edge_ctrl(port, GPIO_TO_INDEX(gpio),
				    GPIO_INT_HIGH_LEV);
		set_irq_handler(irq, do_level_IRQ);
		port->irq_is_level_map |= 1 << GPIO_TO_INDEX(gpio);
		break;
	case IRQT_BOTHEDGE:
	default:
		return -EINVAL;
		break;
	}
	return 0;
}

static struct irq_chip gpio_irq_chip = {
	.ack = gpio_ack_irq,
	.mask = gpio_mask_irq,
	.unmask = gpio_unmask_irq,
	.set_type = gpio_set_irq_type,
};

static int initialized /*= 0*/;

static int __init _mxc_gpio_init(void)
{
	int i;
	struct gpio_port *port;

	initialized = 1;

	printk(KERN_INFO "MXC GPIO hardware\n");

	for (i = 0; i < GPIO_PORT_NUM; i++) {
		int j, gpio_count = GPIO_NUM_PIN;

		port = &gpio_port[i];
		port->base = mxc_gpio_ports[i].base;
		port->num = mxc_gpio_ports[i].num;
		port->irq = mxc_gpio_ports[i].irq;
		port->virtual_irq_start = mxc_gpio_ports[i].virtual_irq_start;

		port->reserved_map = 0;
		spin_lock_init(&port->lock);

		/* disable the interrupt and clear the status */
		__raw_writel(0, port->base + GPIO_IMR);
		__raw_writel(0xFFFFFFFF, port->base + GPIO_ISR);
		for (j = port->virtual_irq_start;
		     j < port->virtual_irq_start + gpio_count; j++) {
			set_irq_chip(j, &gpio_irq_chip);
			set_irq_handler(j, do_edge_IRQ);
			set_irq_flags(j, IRQF_VALID);
		}
#ifndef MXC_MUX_GPIO_INTERRUPTS
		set_irq_chained_handler(port->irq, mxc_gpio_irq_handler);
		set_irq_data(port->irq, port);
#endif
	}

#ifdef MXC_MUX_GPIO_INTERRUPTS
	set_irq_chained_handler(port->irq, mxc_gpio_mux_irq_handler);
	set_irq_data(mxc_gpio_ports[0].irq, gpio_port);
#endif

	return 0;
}

/*
 * This may get called early from board specific init
 */
int mxc_gpio_init(void)
{
	if (!initialized)
		return _mxc_gpio_init();
	else
		return 0;
}

postcore_initcall(mxc_gpio_init);
