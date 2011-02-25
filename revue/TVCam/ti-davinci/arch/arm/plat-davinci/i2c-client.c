/*
 * arch/arm/plat-davinci/i2c-client.c
 *
 * Copyright (C) 2006 Texas Instruments Inc
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/i2c.h>
#include <asm/arch/i2c-client.h>

static struct i2c_client davinci_i2c_client;

/* This function is used for internal initialization */
int davinci_i2c_read(u8 size, u8 *val, u16 client_addr)
{
	struct i2c_client *client = &davinci_i2c_client;
	struct i2c_msg msg;
	int err;

	if (client->adapter == NULL)
		return -ENODEV;

	msg.addr = client_addr;
	msg.flags = I2C_M_RD;
	msg.len = size;
	msg.buf = val;

	err = i2c_transfer(client->adapter, &msg, 1);
	return err < 0 ? err : 0;
}
EXPORT_SYMBOL(davinci_i2c_read);

/* This function is used for internal initialization */
int davinci_i2c_write(u8 size, u8 *val, u16 client_addr)
{
	struct i2c_client *client = &davinci_i2c_client;
	struct i2c_msg msg;
	int err;

	if (client->adapter == NULL)
		return -ENODEV;

	msg.addr = client_addr;
	msg.flags = 0;
	msg.len = size;
	msg.buf = val;

	err = i2c_transfer(client->adapter, &msg, 1);
	return err < 0 ? err : 0;
}
EXPORT_SYMBOL(davinci_i2c_write);

#ifdef CONFIG_DAVINCI_I2C_EXPANDER

struct davinci_i2c_expander davinci_i2c_expander;
static DEFINE_MUTEX(expander_lock);

int davinci_i2c_expander_op(enum i2c_expander_pins pin, unsigned val)
{
	u16 address = davinci_i2c_expander.address;
	u8 data = 0;
	int err;

	if (val > 1)
		return -EINVAL;

	if (davinci_i2c_expander.validate != NULL) {
		err = davinci_i2c_expander.validate(pin);
		if (err)
			return err;
	}

	mutex_lock(&expander_lock);

	err = davinci_i2c_read(1, &data, address);
	if (err)
		goto exit;

	if (davinci_i2c_expander.special != NULL) {
		err = davinci_i2c_expander.special(pin, val);
		if (err)
			goto exit;
	}

	data &= ~(1 << pin);
	data |= val << pin;

	err = davinci_i2c_write(1, &data, address);

exit:
	mutex_unlock(&expander_lock);
	return err;
}
EXPORT_SYMBOL(davinci_i2c_expander_op);
#endif /* CONFIG_DAVINCI_I2C_EXPANDER */

static int davinci_i2c_attach_adapter(struct i2c_adapter *adapter)
{
	struct i2c_client *client = &davinci_i2c_client;
	int err;

	if (client->adapter != NULL)
		return -EBUSY;		/* our client is already attached */

	client->adapter = adapter;

#ifdef	CONFIG_DAVINCI_I2C_EXPANDER
	/*
	 * We may have the I2C expander support enabled without actual
	 * expander present on the board...
	 */
	if (davinci_i2c_expander.name == NULL)
		return -ENODEV;

	client->addr = davinci_i2c_expander.address;
	strlcpy(client->name, davinci_i2c_expander.name, I2C_NAME_SIZE);

	err = i2c_attach_client(client);
	if (err) {
		client->adapter = NULL;
		return err;
	}

	davinci_i2c_write(1, &davinci_i2c_expander.init_data,
			  davinci_i2c_expander.address);

	if (davinci_i2c_expander.setup != NULL)
		davinci_i2c_expander.setup();
#endif
	return 0;
}

static int davinci_i2c_detach_adapter(struct i2c_adapter *adapter)
{
	struct i2c_client *client = &davinci_i2c_client;

	if (client->adapter == adapter)
		client->adapter = NULL;

	return 0;
}

static int davinci_i2c_detach_client(struct i2c_client *client)
{
#ifdef	CONFIG_DAVINCI_I2C_EXPANDER
	if (client->adapter == NULL)
		return -ENODEV; 	/* our client isn't attached */

	return i2c_detach_client(client);
#else
	return -ENODEV;
#endif
}

/* This is the driver that will be inserted */
static struct i2c_driver davinci_evm_driver = {
	.driver = {
		.name	= "davinci_evm",
	},
	.attach_adapter	= davinci_i2c_attach_adapter,
	.detach_adapter	= davinci_i2c_detach_adapter,
	.detach_client	= davinci_i2c_detach_client,
};

static struct i2c_client davinci_i2c_client = {
	.driver 	= &davinci_evm_driver,
};

static int __init davinci_i2c_client_init(void)
{
	return i2c_add_driver(&davinci_evm_driver);
}

static void __exit davinci_i2c_client_exit(void)
{
	i2c_del_driver(&davinci_evm_driver);
}

module_init(davinci_i2c_client_init);
module_exit(davinci_i2c_client_exit);
