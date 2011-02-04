/*
 * Ctrl-Alt-Del input handler
 *
 * Copyright (C) 2010 Google, Inc.
 * Author: Eugene Surovegin <es@google.com>
 *
 * based on keyboard.c
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 */

#include <linux/init.h>
#include <linux/input.h>
#include <linux/module.h>

#define DRV_NAME        	"cad"
#define DRV_VERSION     	"0.3"
#define DRV_DESC        	"Simple Ctrl-Alt-Del input handler"

MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("Eugene Surovegin <es@google.com>");
MODULE_LICENSE("GPL");

extern void ctrl_alt_del(void);

enum {
	LEFT_CTRL,
	RIGHT_CTRL,
	LEFT_ALT,
	RIGHT_ALT,
	DELETE,
	BACKSPACE,
	LEFT_SHIFT,
	RIGHT_SHIFT,
	HOME,
};

static void cad_event(struct input_handle *handle, unsigned int event_type,
		      unsigned int event_code, int down)
{
	unsigned long *mask;
	int bit;

	if (event_type != EV_KEY)
		return;

	/* translate keycode to bits we use internaly so we can fit
	 * all them into unsigned long
	 */
	switch (event_code) {
	case KEY_LEFTCTRL:	bit = LEFT_CTRL; break;
	case KEY_RIGHTCTRL:	bit = RIGHT_CTRL; break;
	case KEY_LEFTALT:	bit = LEFT_ALT; break;
	case KEY_RIGHTALT:	bit = RIGHT_ALT; break;
	case KEY_LEFTSHIFT:	bit = LEFT_SHIFT; break;
	case KEY_RIGHTSHIFT:	bit = RIGHT_SHIFT; break;
	case KEY_DELETE:	bit = DELETE; break;
	case KEY_BACKSPACE:	bit = BACKSPACE; break;
	case KEY_HOME:		bit = HOME; break;
	default:
		return;
	}

	/* we use handle->private to store bitmask of currently pressed keys */
	mask = (unsigned long*)&handle->private;
	if (down) {
		set_bit(bit, mask);
	} else {
		clear_bit(bit, mask);
		return;
	}

	/* CTRL-ALT-DEL */
	if ((test_bit(LEFT_CTRL, mask) || test_bit(RIGHT_CTRL, mask)) &&
	    (test_bit(LEFT_ALT, mask) || test_bit(RIGHT_ALT, mask)) &&
	    test_bit(DELETE, mask)) {
		ctrl_alt_del();
	}

#ifdef CONFIG_CAD_HANDLER_CTRL_ALT_BACKSPACE
	/* CTRL-ALT-BACKSPACE */
	if ((test_bit(LEFT_CTRL, mask) || test_bit(RIGHT_CTRL, mask)) &&
	    (test_bit(LEFT_ALT, mask) || test_bit(RIGHT_ALT, mask)) &&
	    test_bit(BACKSPACE, mask)) {
		ctrl_alt_del();
	}
#endif

#ifdef CONFIG_CAD_HANDLER_ALT_SHIFT_HOME
	/* ALT-SHIFT-HOME */
	if ((test_bit(LEFT_ALT, mask) || test_bit(RIGHT_ALT, mask)) &&
	    (test_bit(LEFT_SHIFT, mask) || test_bit(RIGHT_SHIFT, mask)) &&
	    test_bit(HOME, mask)) {
		ctrl_alt_del();
	}
#endif
}

static int cad_connect(struct input_handler *handler, struct input_dev *dev,
			const struct input_device_id *id)
{
	struct input_handle *handle;
	int err;
	int have_combo = 0;

	/* We are only interested in devices that support one of the configured
	 * combos.
	 */

	if ((test_bit(KEY_LEFTCTRL, dev->keybit) ||
	     test_bit(KEY_RIGHTCTRL, dev->keybit)) &&
	    (test_bit(KEY_LEFTALT, dev->keybit) ||
	     test_bit(KEY_RIGHTALT, dev->keybit)) &&
	    test_bit(KEY_DELETE, dev->keybit))
		have_combo = 1;

#ifdef CONFIG_CAD_HANDLER_CTRL_ALT_BACKSPACE
	if ((test_bit(KEY_LEFTCTRL, dev->keybit) ||
	     test_bit(KEY_RIGHTCTRL, dev->keybit)) &&
	    (test_bit(KEY_LEFTALT, dev->keybit) ||
	     test_bit(KEY_RIGHTALT, dev->keybit)) &&
	    test_bit(KEY_BACKSPACE, dev->keybit))
		have_combo = 1;
#endif

#ifdef CONFIG_CAD_HANDLER_ALT_SHIFT_HOME
	if ((test_bit(KEY_LEFTALT, dev->keybit) ||
	     test_bit(KEY_RIGHTALT, dev->keybit)) &&
	    (test_bit(KEY_LEFTSHIFT, dev->keybit) ||
	     test_bit(KEY_RIGHTSHIFT, dev->keybit)) &&
	    test_bit(KEY_HOME, dev->keybit))
		have_combo = 1;
#endif

	if (!have_combo)
		return -ENODEV;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (unlikely(!handle))
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = DRV_NAME;

	err = input_register_handle(handle);
	if (unlikely(err))
		goto out_free;

	err = input_open_device(handle);
	if (unlikely(err))
		goto out_unregister;

	return 0;

  out_unregister:
	input_unregister_handle(handle);
  out_free:
	kfree(handle);
	return err;
}

static void cad_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static struct input_handler cad_handler = {
	.event		= cad_event,
	.connect	= cad_connect,
	.disconnect	= cad_disconnect,
	.name		= DRV_NAME,
	.id_table	= (const struct input_device_id[]) {
		{
			.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
			.evbit = { BIT(EV_KEY) },
		},
		{ },    /* sentinel */
	}
};

static int __init cad_init(void)
{
	return input_register_handler(&cad_handler);
}

static void __exit cad_exit(void)
{
	input_unregister_handler(&cad_handler);
}

module_init(cad_init);
module_exit(cad_exit);
