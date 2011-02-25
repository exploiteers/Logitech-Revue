/*
 * MPC8349E-mITX/MPC837x-RDB Power Management and GPIO expander driver
 *
 * On the MPC8349E-mITX and MPC837x-RDB boards there is MC9S08QG8 (MCU) chip
 * with the custom firmware pre-programmed. This firmware offers to control
 * some of the MCU GPIO pins (used for LEDs and soft power-off) via I2C. MCU
 * have some other functions, but these are not implemented yet.
 *
 * Copyright (c) 2008  MontaVista Software, Inc.
 *
 * Author: Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/i2c.h>
#include <asm/of_device.h>
#include <asm/gpio.h>
#include <asm/machdep.h>

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x0a, I2C_CLIENT_END };

static struct i2c_client_address_data addr_data = {
	.normal_i2c = normal_addr,
	.probe = ignore,
	.ignore = ignore,
};

static struct i2c_driver mcu_driver;

/*
 * I don't have specifications for the MCU firmware that is used on the
 * MPC8349E-mITX/MPC837XRDB boards, I found this register and bits positions
 * by the trial&error method.
 */
#define MCU_REG_CTRL	0x20
#define MCU_CTRL_POFF	0x40
#define MCU_NUM_GPIO	2

struct mcu {
	struct mutex lock;
	struct device_node *np;
	struct i2c_client client;
	struct of_gpio_chip of_gc;
	u8 reg_ctrl;
};

static struct mcu *glob_mcu;

static void mcu_power_off(void)
{
	struct mcu *mcu = glob_mcu;

	pr_info("Sending power-off request to the MCU...\n");
	mutex_lock(&mcu->lock);
	i2c_smbus_write_byte_data(&glob_mcu->client, MCU_REG_CTRL,
				  mcu->reg_ctrl | MCU_CTRL_POFF);
	mutex_unlock(&mcu->lock);
}

static void mcu_gpio_set(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct of_gpio_chip *of_gc = to_of_gpio_chip(gc);
	struct mcu *mcu = container_of(of_gc, struct mcu, of_gc);
	u8 bit = 1 << (4 + gpio);

	mutex_lock(&mcu->lock);
	if (val)
		mcu->reg_ctrl |= bit;
	else
		mcu->reg_ctrl &= ~bit;

	i2c_smbus_write_byte_data(&mcu->client, MCU_REG_CTRL, mcu->reg_ctrl);
	mutex_unlock(&mcu->lock);
}

static int mcu_gpio_dir_out(struct gpio_chip *gc, unsigned int gpio, int val)
{
	mcu_gpio_set(gc, gpio, val);
	return 0;
}

static int mcu_gpiochip_add(struct mcu *mcu)
{
	struct device_node *np;
	struct of_gpio_chip *of_gc = &mcu->of_gc;
	struct gpio_chip *gc = &of_gc->gc;

	np = of_find_compatible_node(NULL, NULL, "fsl,mcu-mpc8349emitx");
	if (!np)
		return -ENODEV;

	gc->owner = THIS_MODULE;
	gc->label = np->full_name;
	gc->can_sleep = 1;
	gc->ngpio = MCU_NUM_GPIO;
	gc->base = -1;
	gc->set = mcu_gpio_set;
	gc->direction_output = mcu_gpio_dir_out;
	of_gc->gpio_cells = 1;
	of_gc->xlate = of_gpio_simple_xlate;

	np->data = of_gc;
	mcu->np = np;

	/*
	 * We don't want to lose the node, its ->data and ->full_name...
	 * So, there is no of_node_put(np); here.
	 */
	return gpiochip_add(gc);
}

static int mcu_gpiochip_remove(struct mcu *mcu)
{
	int ret;

	ret = gpiochip_remove(&mcu->of_gc.gc);
	if (ret)
		return ret;
	of_node_put(mcu->np);

	return 0;
}

static int mcu_probe(struct i2c_adapter *adap, int addr, int kind)
{
	struct mcu *mcu;
	int ret;

	mcu = kzalloc(sizeof(*mcu), GFP_KERNEL);
	if (!mcu)
		return -ENOMEM;

	mutex_init(&mcu->lock);
	mcu->client.addr = addr;
	mcu->client.adapter = adap;
	mcu->client.driver = &mcu_driver;
	mcu->client.flags = 0;
	strlcpy(mcu->client.name, "mcu-mpc8349emitx", I2C_NAME_SIZE);

	i2c_set_clientdata(&mcu->client, mcu);

	ret = i2c_attach_client(&mcu->client);
	if (ret)
		goto err_iic_attach;

	ret = i2c_smbus_read_byte_data(&mcu->client, MCU_REG_CTRL);
	if (ret < 0)
		goto err_iic_read;
	mcu->reg_ctrl = ret;

	ret = mcu_gpiochip_add(mcu);
	if (ret)
		goto err_gpio;

	/* XXX: this is potentionally racy, but there is no ppc_md lock */
	if (!ppc_md.power_off) {
		glob_mcu = mcu;
		ppc_md.power_off = mcu_power_off;
		dev_info(&mcu->client.dev, "will provide power-off service\n");
	}

	return 0;
err_gpio:
	i2c_detach_client(&mcu->client);
err_iic_attach:
	mcu_gpiochip_remove(mcu);
err_iic_read:
	kfree(mcu);
	return ret;
}

static int mcu_attach(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, mcu_probe);
}

static int mcu_detach(struct i2c_client *client)
{
	struct mcu *mcu = i2c_get_clientdata(client);
	int ret;

	if (glob_mcu == mcu) {
		ppc_md.power_off = NULL;
		glob_mcu = NULL;
	}

	ret = i2c_detach_client(&mcu->client);
	if (ret)
		return ret;

	ret = mcu_gpiochip_remove(mcu);
	if (ret)
		return ret;

	i2c_set_clientdata(client, NULL);
	kfree(mcu);
	return 0;
}

static struct i2c_driver mcu_driver = {
	.driver = {
		.name = "mcu-mpc8349emitx",
		.owner = THIS_MODULE,
	},
	.id = I2C_DRIVERID_MCU_MPC8349EMITX,
	.attach_adapter = mcu_attach,
	.detach_client	= mcu_detach,
};

static int __init mcu_init(void)
{
	return i2c_add_driver(&mcu_driver);
}
module_init(mcu_init);

static void __exit mcu_exit(void)
{
	i2c_del_driver(&mcu_driver);
}
module_exit(mcu_exit);

MODULE_DESCRIPTION("MPC8349E-mITX/MPC837x-RDB Power Management and GPIO "
		   "expander driver");
MODULE_AUTHOR("Anton Vorontsov <avorontsov@ru.mvista.com>");
MODULE_LICENSE("GPL");
