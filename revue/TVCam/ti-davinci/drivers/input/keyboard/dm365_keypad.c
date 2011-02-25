/*
 * linux/drivers/input/keyboard/dm365_keypad.c
 *
 * DaVinci DM365 Keypad Driver
 *
 * Copyright (C) 2009 Texas Instruments, Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/io.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/arch/dm365_keypad.h>
#include <asm/arch/hardware.h>
#include <asm/arch/irqs.h>
#include <asm/arch/io.h>
#include <asm/arch/mux.h>
#include <asm/arch/cpld.h>

struct davinci_kp {
	struct input_dev	*input;
	int irq;
};

static void __iomem	*dm365_kp_base;
static resource_size_t	dm365_kp_pbase;
static size_t		dm365_kp_base_size;
static int *keymap;

void dm365_keypad_initialize(void)
{
	/*Initializing the Keypad Module */

	/* Enable all interrupts */
	dm365_keypad_write(DM365_KEYPAD_INT_ALL, DM365_KEYPAD_INTENA);

	/* Clear interrupts if any */
	dm365_keypad_write(DM365_KEYPAD_INT_ALL, DM365_KEYPAD_INTCLR);

	/* Setup the scan period */
	dm365_keypad_write(0x05, DM365_KEYPAD_STRBWIDTH);
	dm365_keypad_write(0x02, DM365_KEYPAD_INTERVAL);
	dm365_keypad_write(0x01, DM365_KEYPAD_CONTTIME);

	/* Enable Keyscan module and enable */
	dm365_keypad_write(DM365_KEYPAD_AUTODET | DM365_KEYPAD_KEYEN,
			   DM365_KEYPAD_KEYCTRL);
}

static irqreturn_t dm365_kp_interrupt(int irq, void *dev_id,
				      struct pt_regs *regs)
{
	int i;
	unsigned int status, temp, temp1;
	int keycode = KEY_UNKNOWN;
	struct davinci_kp *dm365_kp = dev_id;

	/* Reading the status of the Keypad */
	status = dm365_keypad_read(DM365_KEYPAD_PREVSTATE);

	temp = ~status;

	for (i = 0; i < 16; i++) {
		temp1 = temp >> i;
		if (temp1 & 0x1) {
			keycode = keymap[i];

			input_report_key(dm365_kp->input, keycode, 1);
			input_report_key(dm365_kp->input, keycode, 0);
		}
	}

	/* clearing interrupts */
	dm365_keypad_write(DM365_KEYPAD_INT_ALL, DM365_KEYPAD_INTCLR);

	return IRQ_HANDLED;
}

/*
 * Registers keypad device with input sub system
 * and configures DM365 keypad registers
 */
static int dm365_kp_probe(struct platform_device *pdev)
{
	struct davinci_kp *dm365_kp;
	struct input_dev *key_dev;
	struct resource *res, *mem;
	int ret, i;
	unsigned int val;
	struct dm365_kp_platform_data *pdata = pdev->dev.platform_data;

	/* Enabling pins for Keypad */
	davinci_cfg_reg(DM365_KEYPAD, PINMUX_RESV);

	/* Enabling the Kepad Module */
	val = keypad_read(DM365_CPLD_REGISTER3);
	val |= 0x80808080;
	keypad_write(val, DM365_CPLD_REGISTER3);

	if (!pdata->keymap) {
		printk(KERN_ERR "No keymap from pdata\n");
		return -EINVAL;
	}

	dm365_kp = kzalloc(sizeof *dm365_kp, GFP_KERNEL);
	key_dev = input_allocate_device();

	if (!dm365_kp || !key_dev) {
		printk(KERN_ERR "Could not allocate input device\n");
		return -EINVAL;
	}

	platform_set_drvdata(pdev, dm365_kp);

	dm365_kp->input = key_dev;

	dm365_kp->irq = platform_get_irq(pdev, 0);
	if (dm365_kp->irq <= 0) {
		pr_debug("%s: No DM365 Keypad irq\n", pdev->name);
		goto fail1;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res && res->start != DM365_KEYSCAN_BASE) {
		pr_debug("%s: KEYSCAN registers at %08x, expected %08x\n",
			 pdev->name, (unsigned)res->start, DM365_KEYSCAN_BASE);
		goto fail1;
	}

	dm365_kp_pbase = res->start;
	dm365_kp_base_size = res->end - res->start + 1;

	if (res)
		mem = request_mem_region(res->start,
					 dm365_kp_base_size, pdev->name);
	else
		mem = NULL;

	if (!mem) {
		pr_debug("%s: KEYSCAN registers at %08x are not free\n",
			 pdev->name, DM365_KEYSCAN_BASE);
		goto fail1;
	}

	dm365_kp_base = ioremap(res->start, dm365_kp_base_size);
	if (dm365_kp_base == NULL) {
		pr_debug("%s: Can't ioremap MEM resource.\n", pdev->name);
		goto fail2;
	}

	/* Enable auto repeat feature of Linux input subsystem */
	if (pdata->rep)
		set_bit(EV_REP, key_dev->evbit);

	/* setup input device */
	set_bit(EV_KEY, key_dev->evbit);

	/* Setup the keymap */
	keymap = pdata->keymap;

	for (i = 0; i < 16; i++)
		set_bit(keymap[i], key_dev->keybit);

	key_dev->name = "dm365_keypad";
	key_dev->phys = "dm365_keypad/input0";
	key_dev->cdev.dev = &pdev->dev;
	key_dev->private = dm365_kp;
	key_dev->id.bustype = BUS_HOST;
	key_dev->id.vendor = 0x0001;
	key_dev->id.product = 0x0365;
	key_dev->id.version = 0x0001;

	key_dev->keycode = keymap;
	key_dev->keycodesize = sizeof(unsigned int);
	key_dev->keycodemax = pdata->keymapsize;

	ret = input_register_device(dm365_kp->input);
	if (ret < 0) {
		printk(KERN_ERR
		       "Unable to register DaVinci DM365 keypad device\n");
		goto fail3;
	}

	ret = request_irq(dm365_kp->irq, dm365_kp_interrupt, IRQF_DISABLED,
			  "dm365_keypad", dm365_kp);
	if (ret < 0) {
		printk(KERN_ERR
		       "Unable to register DaVinci DM365 keypad Interrupt\n");
		goto fail4;
	}

	dm365_keypad_initialize();

	return 0;
fail4:
	input_unregister_device(dm365_kp->input);
fail3:
	iounmap(dm365_kp_base);
fail2:
	release_mem_region(dm365_kp_pbase, dm365_kp_base_size);
fail1:
	kfree(dm365_kp);
	input_free_device(key_dev);

	/* Freeing Keypad Pins */
	davinci_cfg_reg(DM365_KEYPAD, PINMUX_FREE);

	/* Re enabling other modules */
	keypad_write(0x0, DM365_CPLD_REGISTER3);

	return -EINVAL;
}

static int __devexit dm365_kp_remove(struct platform_device *pdev)
{
	struct davinci_kp *dm365_kp = platform_get_drvdata(pdev);

	input_unregister_device(dm365_kp->input);
	free_irq(dm365_kp->irq, dm365_kp);

	kfree(dm365_kp);

	iounmap(dm365_kp_base);
	release_mem_region(dm365_kp_pbase, dm365_kp_base_size);

	platform_set_drvdata(pdev, NULL);

	/* Freeing Keypad Pins */
	davinci_cfg_reg(DM365_KEYPAD, PINMUX_FREE);

	/* Re enabling other modules */
	keypad_write(0x0, DM365_CPLD_REGISTER3);

	return 0;
}

static struct platform_driver dm365_kp_driver = {
	.probe = dm365_kp_probe,
	.remove = __devexit_p(dm365_kp_remove),
	.driver = {
		   .name = "dm365_keypad",
		   .owner = THIS_MODULE,
		   },
};

static int __init dm365_kp_init(void)
{
	printk(KERN_INFO "DaVinci DM365 Keypad Driver\n");

	return platform_driver_register(&dm365_kp_driver);
}

static void __exit dm365_kp_exit(void)
{
	platform_driver_unregister(&dm365_kp_driver);
}

module_init(dm365_kp_init);
module_exit(dm365_kp_exit);

MODULE_AUTHOR("Sandeep Paulraj");
MODULE_DESCRIPTION("Texas Instruments DaVinci DM365 Keypad Driver");
MODULE_LICENSE("GPL");
