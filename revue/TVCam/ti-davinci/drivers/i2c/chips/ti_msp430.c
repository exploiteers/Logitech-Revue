/*
 * I2C client/driver for the MSP430 chip on the TI development boards
 * Author: Aleksey Makarov, MontaVista Software, Inc. <source@mvista.com>
 *
 * 2008 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/i2c.h>

#include <asm/mach-types.h>

#define	DRIVER_NAME "ti_msp430"

static struct i2c_driver ti_msp430_driver;
static struct i2c_client *client;
static DEFINE_MUTEX(lock);

int
ti_msp430_read(u16 reg, u8 * value)
{
	s32 err;

	if (!client)
		return -ENODEV;

	mutex_lock(&lock);

	err = i2c_smbus_read_byte_data(client, reg);

	mutex_unlock(&lock);

	if (err == -1)
		return -EIO;

	*value = err;

	return 0;
}

EXPORT_SYMBOL(ti_msp430_read);

int
ti_msp430_write(u16 reg, u8 value)
{
	s32 err;

	if (!client)
		return -ENODEV;

	mutex_lock(&lock);

	err = i2c_smbus_write_byte_data(client, reg, value);

	mutex_unlock(&lock);

	return err;
}

EXPORT_SYMBOL(ti_msp430_write);

static int
ti_msp430_attach(struct i2c_adapter *adap)
{
	int err = -ENODEV;

	client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (!client)
		return -ENOMEM;

	strncpy(client->name, DRIVER_NAME, I2C_NAME_SIZE);

	if (machine_is_davinci_dm355_evm())
		client->addr = 0x25;
	else if(machine_is_davinci_evm())
		client->addr = 0x23;
	else
		goto err;

	client->adapter = adap;
	client->driver = &ti_msp430_driver;

	err = i2c_attach_client(client);
	if (err)
		goto err;

	return 0;
err:
	kfree(client);
	client = 0;
	return err;
}

static int
ti_msp430_detach(struct i2c_client *client)
{
	i2c_detach_client(client);
	client = 0;
	return 0;
}

static struct i2c_driver ti_msp430_driver = {
	.driver = {
		.name	= DRIVER_NAME,
	},
	.id = I2C_DRIVERID_TI_MSP430,
	.attach_adapter = ti_msp430_attach,
	.detach_client = ti_msp430_detach,
};

static int __init
ti_msp430_init(void)
{
	return i2c_add_driver(&ti_msp430_driver);
}

static void __exit
ti_msp430_exit(void)
{
	i2c_del_driver(&ti_msp430_driver);
}

module_init(ti_msp430_init);
module_exit(ti_msp430_exit);

MODULE_AUTHOR("Aleksey Makarov <amakarov@ru.mvista.com>");
MODULE_DESCRIPTION("Driver for the MSP430 chip on the TI development boards");
MODULE_LICENSE("GPL");
