/*
 * OF helpers for the GPIO API
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/io.h>
#include <asm/of_platform.h>
#include <asm/prom.h>
#include <asm/gpio.h>

int of_get_gpio(struct device_node *np, int index)
{
	int ret = -EINVAL;
	struct device_node *gc;
	struct of_gpio_chip *of_gc = NULL;
	int size;
	const u32 *gpios;
	u32 nr_cells;
	int i;
	const void *gpio_spec;
	const u32 *gpio_cells;
	int gpio_index = 0;

	gpios = of_get_property(np, "gpios", &size);
	if (!gpios) {
		ret = -ENOENT;
		goto err0;
	}
	nr_cells = size / sizeof(u32);

	for (i = 0; i < nr_cells;) {
		const phandle *gpio_phandle;

		gpio_phandle = gpios + i;
		gpio_spec = gpio_phandle + 1;

		/* one cell hole in the gpios = <>; */
		if (!*gpio_phandle) {
			if (gpio_index == index)
				return -ENOENT;
			i++;
			gpio_index++;
			continue;
		}

		gc = of_find_node_by_phandle(*gpio_phandle);
		if (!gc) {
			pr_debug("%s: could not find phandle for gpios\n",
				 np->full_name);
			goto err0;
		}

		of_gc = gc->data;
		if (!of_gc) {
			pr_debug("%s: gpio controller %s isn't registered\n",
				 np->full_name, gc->full_name);
			goto err1;
		}

		gpio_cells = of_get_property(gc, "#gpio-cells", &size);
		if (!gpio_cells || size != sizeof(*gpio_cells) ||
				*gpio_cells != of_gc->gpio_cells) {
			pr_debug("%s: wrong #gpio-cells for %s\n",
				 np->full_name, gc->full_name);
			goto err1;
		}

		/* Next phandle is at phandle cells + #gpio-cells */
		i += sizeof(*gpio_phandle) / sizeof(u32) + *gpio_cells;
		if (i >= nr_cells + 1) {
			pr_debug("%s: insufficient gpio-spec length\n",
				 np->full_name);
			goto err1;
		}

		if (gpio_index == index)
			break;

		of_gc = NULL;
		of_node_put(gc);
		gpio_index++;
	}

	if (!of_gc) {
		ret = -ENOENT;
		goto err0;
	}

	ret = of_gc->xlate(of_gc, np, gpio_spec);
	if (ret < 0)
		goto err1;

	ret += of_gc->gc.base;
err1:
	of_node_put(gc);
err0:
	pr_debug("%s exited with status %d\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL(of_get_gpio);

int of_gpio_simple_xlate(struct of_gpio_chip *of_gc, struct device_node *np,
			 const void *gpio_spec)
{
	const u32 *gpio = gpio_spec;

	if (*gpio > of_gc->gc.ngpio)
		return -EINVAL;

	return *gpio;
}
EXPORT_SYMBOL(of_gpio_simple_xlate);

static int of_get_gpiochip_base(struct device_node *np)
{
	struct device_node *gc = NULL;
	int gpiochip_base = 0;

	while ((gc = of_find_all_nodes(gc))) {
		if (!of_get_property(gc, "gpio-controller", NULL))
			continue;

		if (gc != np) {
			gpiochip_base += ARCH_OF_GPIOS_PER_CHIP;
			continue;
		}

		of_node_put(gc);

		if (gpiochip_base >= ARCH_OF_GPIOS_END)
			return -ENOSPC;

		return gpiochip_base;
	}

	return -ENOENT;
}

int of_mm_gpiochip_add(struct device_node *np,
		       struct of_mm_gpio_chip *mm_gc)
{
	int ret = -ENOMEM;
	struct of_gpio_chip *of_gc = &mm_gc->of_gc;

	if (of_gc->gc.ngpio > ARCH_OF_GPIOS_PER_CHIP) {
		ret = -ENOSPC;
		goto err;
	}

	mm_gc->of_gc.gc.label = kstrdup(np->full_name, GFP_KERNEL);
	if (!mm_gc->of_gc.gc.label)
		goto err;

	mm_gc->regs = of_iomap(np, 0);
	if (!mm_gc->regs)
		goto err1;

	ret = of_get_gpiochip_base(np);
	if (ret < 0)
		goto err2;
	mm_gc->of_gc.gc.base = ret;

	if (!of_gc->xlate)
		of_gc->xlate = of_gpio_simple_xlate;

	if (mm_gc->save_regs)
		mm_gc->save_regs(mm_gc);

	np->data = &mm_gc->of_gc;

	ret = gpiochip_add(&mm_gc->of_gc.gc);
	if (ret)
		goto err3;

	/* We don't want to lose the node and its ->data */
	of_node_get(np);

	pr_debug("%s: registered as generic GPIO chip, base is %d\n",
		 np->full_name, mm_gc->of_gc.gc.base);
	return 0;
err3:
	np->data = NULL;
err2:
	iounmap(mm_gc->regs);
err1:
	kfree(mm_gc->of_gc.gc.label);
err:
	pr_err("%s: GPIO chip registration failed with status %d\n",
	       np->full_name, ret);
	return ret;
}
EXPORT_SYMBOL(of_mm_gpiochip_add);
