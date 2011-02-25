/*
 * Generic GPIO API implementation for PowerPC.
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __ASM_POWERPC_GPIO_H
#define __ASM_POWERPC_GPIO_H

#include <asm-generic/gpio.h>

#ifdef CONFIG_HAVE_GPIO_LIB

#define ARCH_OF_GPIOS_PER_CHIP	32
#define ARCH_OF_GPIOS_BASE	0
#define ARCH_OF_GPIOS_END	(ARCH_OF_GPIOS_PER_CHIP * 7)
#define ARCH_NON_OF_GPIOS_BASE	ARCH_OF_GPIOS_END
#define ARCH_NON_OF_GPIOS_END	ARCH_NR_GPIOS

#if ARCH_NON_OF_GPIOS_BASE >= ARCH_NON_OF_GPIOS_END
#error "Default ARCH_NR_GPIOS isn't sufficient, define yours."
#endif

/*
 * We don't (yet) implement inlined/rapid versions for on-chip gpios.
 * Just call gpiolib.
 */
static inline int gpio_get_value(unsigned int gpio)
{
	return __gpio_get_value(gpio);
}

static inline void gpio_set_value(unsigned int gpio, int value)
{
	__gpio_set_value(gpio, value);
}

static inline int gpio_cansleep(unsigned int gpio)
{
	return __gpio_cansleep(gpio);
}

/*
 * Not implemented, yet.
 */
static inline int gpio_to_irq(unsigned int gpio)
{
	return -ENOSYS;
}

static inline int irq_to_gpio(unsigned int irq)
{
	return -EINVAL;
}

/*
 * Generic OF GPIO chip
 */
struct of_gpio_chip {
	struct gpio_chip gc;
	int gpio_cells;
	int (*xlate)(struct of_gpio_chip *of_gc, struct device_node *np,
		     const void *gpio_spec);
};

#define to_of_gpio_chip(x) container_of(x, struct of_gpio_chip, gc)

/*
 * OF GPIO chip for memory mapped banks
 */
struct of_mm_gpio_chip {
	struct of_gpio_chip of_gc;
	void (*save_regs)(struct of_mm_gpio_chip *mm_gc);
	void __iomem *regs;
};

#define to_of_mm_gpio_chip(x) container_of(to_of_gpio_chip(x), \
					   struct of_mm_gpio_chip, of_gc)

/**
 * of_get_gpio - Get a GPIO number from the device tree to use with GPIO API
 * @np:		device node to get GPIO from
 * @index:	index of the GPIO
 *
 * Returns GPIO number to use with Linux generic GPIO API, or one of the errno
 * value on the error condition.
 */
extern int of_get_gpio(struct device_node *np, int index);

/**
 * of_mm_gpiochip_add - Add memory mapped GPIO chip (bank)
 * @np:		device node of the GPIO chip
 * @mm_gc:	pointer to the of_mm_gpio_chip allocated structure
 *
 * To use this function you should allocate and fill mm_gc with:
 *
 * 1) In the gpio_chip structure:
 *    a) all the callbacks
 *    b) ngpios (GPIOs per bank)
 *
 * 2) In the of_gpio_chip structure:
 *    a) gpio_cells
 *    b) xlate callback (optional)
 *
 * 3) In the of_mm_gpio_chip structure:
 *    a) save_regs callback (optional)
 *
 * If succeeded, this function will map bank's memory and will
 * do all necessary work for you. Then you'll able to use .regs
 * to manage GPIOs from the callbacks.
 */
extern int of_mm_gpiochip_add(struct device_node *np,
			      struct of_mm_gpio_chip *mm_gc);

/**
 * of_gpio_simple_xlate - translate gpio_spec to the GPIO number
 * @of_gc:	pointer to the of_gpio_chip structure
 * @np:		device node of the GPIO chip
 * @gpio_spec:	gpio specifier as found in the device tree
 *
 * This is simple translation function, suitable for the most 1:1 mapped
 * gpio chips. This function performs only one sanity check: whether gpio
 * is less than ngpios (that is specified in the gpio_chip).
 */
extern int of_gpio_simple_xlate(struct of_gpio_chip *of_gc,
				struct device_node *np,
				const void *gpio_spec);

#endif /* CONFIG_HAVE_GPIO_LIB */

#endif /* __ASM_POWERPC_GPIO_H */
