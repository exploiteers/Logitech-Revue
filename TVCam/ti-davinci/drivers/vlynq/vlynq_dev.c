/*******************************************************************************
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ******************************************************************************/
#include <linux/module.h>
#include <asm/arch/irqs.h>
#include <linux/vlynq/vlynq_dev.h>

#define CB_COUNT_PER_DEV        2

struct vlynq_dev_cb {
	vlynq_dev_cb_fn cb_fn;
	void *cb_param;
	struct vlynq_dev_cb *next;
};

static struct vlynq_dev_cb vlynq_dev_cb_list[MAX_DEV_COUNT * CB_COUNT_PER_DEV];
static struct vlynq_dev_cb *gp_free_vlynq_dev_cb;

static int vlynq_dev_cb_init(void)
{
	struct vlynq_dev_cb *p_vlynq_dev_cb = gp_free_vlynq_dev_cb =
	    &vlynq_dev_cb_list[0];
	int i = 0;

	for (i = 0; i < (CB_COUNT_PER_DEV * MAX_DEV_COUNT); i++) {
		p_vlynq_dev_cb->cb_param = NULL;
		p_vlynq_dev_cb->cb_fn = NULL;

		p_vlynq_dev_cb->next =
		    (i ==
		     (CB_COUNT_PER_DEV * MAX_DEV_COUNT) -
		     1) ? NULL : &vlynq_dev_cb_list[i + 1];

		p_vlynq_dev_cb = p_vlynq_dev_cb->next;
	}

	return (VLYNQ_OK);
}

static struct vlynq_dev_cb *vlynq_dev_get_free_dev_cb(void)
{
	struct vlynq_dev_cb *p_vlynq_dev_cb;

	p_vlynq_dev_cb = gp_free_vlynq_dev_cb;
	if (p_vlynq_dev_cb)
		gp_free_vlynq_dev_cb = gp_free_vlynq_dev_cb->next;

	return (p_vlynq_dev_cb);
}

static void vlynq_dev_free_dev_cb(struct vlynq_dev_cb *p_vlynq_dev_cb)
{
	p_vlynq_dev_cb->cb_param = NULL;
	p_vlynq_dev_cb->cb_fn = NULL;
	p_vlynq_dev_cb->next = gp_free_vlynq_dev_cb;
	gp_free_vlynq_dev_cb = p_vlynq_dev_cb;
}

struct vlynq_dev_t {
	char name[32];
	void *vlynq;
	struct vlynq_dev_t *next;
	u8 reset_bit;
	u8 irq_count;
	u8 instance;
	u8 peer;
	s8 irq[8];
	struct vlynq_dev_cb *dev_cb;

};

struct vlynq_dev_t vlynq_dev[MAX_DEV_COUNT];
static struct vlynq_dev_t *gp_free_vlynq_dev_list;
static struct vlynq_dev_t *gp_used_vlynq_dev_list;

static struct vlynq_dev_t *vlynq_dev_get_free_dev(void)
{
	struct vlynq_dev_t *p_vlynq_dev;

	p_vlynq_dev = gp_free_vlynq_dev_list;
	if (p_vlynq_dev) {
		gp_free_vlynq_dev_list = gp_free_vlynq_dev_list->next;
		p_vlynq_dev->next = gp_used_vlynq_dev_list;
		gp_used_vlynq_dev_list = p_vlynq_dev;
	}

	return (p_vlynq_dev);
}

static void vlynq_dev_free_dev(struct vlynq_dev_t *p_vlynq_dev)
{
	struct vlynq_dev_t *p_tmp_vlynq_dev;
	struct vlynq_dev_t *p_last_vlynq_dev;

	for (p_tmp_vlynq_dev = gp_used_vlynq_dev_list, p_last_vlynq_dev = NULL;
	     p_tmp_vlynq_dev != NULL;
	     p_last_vlynq_dev = p_tmp_vlynq_dev, p_tmp_vlynq_dev =
	     p_tmp_vlynq_dev->next) {
		if (p_tmp_vlynq_dev == p_vlynq_dev) {
			if (!p_last_vlynq_dev)
				gp_used_vlynq_dev_list =
				    gp_used_vlynq_dev_list->next;
			else
				p_last_vlynq_dev->next = p_tmp_vlynq_dev->next;

			break;
		}
	}

	if (!p_tmp_vlynq_dev)
		goto vlynq_dev_free_dev_exit;

	p_vlynq_dev->next = gp_free_vlynq_dev_list;
	gp_free_vlynq_dev_list = p_vlynq_dev;

vlynq_dev_free_dev_exit:

	return;
}

int vlynq_dev_cb_register(void *vlynq_dev,
			  vlynq_dev_cb_fn cb_fn, void *this_driver)
{
	struct vlynq_dev_t *p_vlynq_dev = (struct vlynq_dev_t *) vlynq_dev;
	struct vlynq_dev_cb *p_dev_cb;
	unsigned long flags;

	if (!p_vlynq_dev || !this_driver)
		return (VLYNQ_INVALID_PARAM);

	local_irq_save(flags);

	p_dev_cb = vlynq_dev_get_free_dev_cb();

	local_irq_restore(flags);

	if (!p_dev_cb)
		return (VLYNQ_INTERNAL_ERROR);

	p_dev_cb->cb_fn = cb_fn;
	p_dev_cb->cb_param = this_driver;

	p_dev_cb->next = p_vlynq_dev->dev_cb;
	p_vlynq_dev->dev_cb = p_dev_cb;

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_dev_cb_register);

int vlynq_dev_cb_unregister(void *vlynq_dev, void *this_driver)
{
	int ret_val = VLYNQ_INVALID_PARAM;
	struct vlynq_dev_t *p_vlynq_dev = vlynq_dev;
	struct vlynq_dev_cb *p_dev_cb, *p_last_dev_cb;

	if (!p_vlynq_dev)
		return (ret_val);

	for (p_dev_cb = p_vlynq_dev->dev_cb, p_last_dev_cb = NULL;
	     p_dev_cb != NULL;
	     p_last_dev_cb = p_dev_cb, p_dev_cb = p_dev_cb->next) {
		if (p_dev_cb->cb_param == this_driver) {
			unsigned long flags;

			if (!p_last_dev_cb)
				p_vlynq_dev->dev_cb = NULL;
			else
				p_last_dev_cb->next = p_dev_cb->next;

			local_irq_save(flags);
			vlynq_dev_free_dev_cb(p_dev_cb);
			local_irq_restore(flags);

			ret_val = VLYNQ_OK;
			break;
		}
	}

	return (ret_val);
}
EXPORT_SYMBOL(vlynq_dev_cb_unregister);

void *vlynq_dev_find(char *name, u8 instance)
{
	struct vlynq_dev_t *p_vlynq_dev;

	for (p_vlynq_dev = gp_used_vlynq_dev_list;
	     p_vlynq_dev != NULL; p_vlynq_dev = p_vlynq_dev->next) {
		if (!strcmp(p_vlynq_dev->name, name) &&
		    (p_vlynq_dev->instance == instance))
			break;
	}

	return (p_vlynq_dev);
}
EXPORT_SYMBOL(vlynq_dev_find);

void *vlynq_dev_get_vlynq(void *vlynq_dev)
{
	struct vlynq_dev_t *p_vlynq_dev = (void *) vlynq_dev;

	if (p_vlynq_dev)
		return (p_vlynq_dev->vlynq);
	else
		return (NULL);
}
EXPORT_SYMBOL(vlynq_dev_get_vlynq);

int vlynq_dev_ioctl(void *vlynq_dev, u32 cmd, u32 cmd_val)
{
	struct vlynq_dev_t *p_vlynq_dev = vlynq_dev;

	if (!p_vlynq_dev)
		return (VLYNQ_INVALID_PARAM);

	switch (cmd) {
	case VLYNQ_DEV_ADD_IRQ:
		{
			int i;

			if (p_vlynq_dev->irq_count == MAX_IRQ_PER_VLYNQ)
				return (-2);

			for (i = 0; i < MAX_IRQ_PER_VLYNQ; i++) {
				if (p_vlynq_dev->irq[i] == -1) {
					p_vlynq_dev->irq[i] = cmd_val;
					p_vlynq_dev->irq_count++;
					break;
				}
			}
		}
		break;

	case VLYNQ_DEV_REMOVE_IRQ:
		{
			int i;

			if (p_vlynq_dev->irq_count == 0)
				return (-2);

			for (i = 0; i < MAX_IRQ_PER_VLYNQ; i++) {
				if (p_vlynq_dev->irq[i] == cmd_val) {
					p_vlynq_dev->irq[i] = -1;
					p_vlynq_dev->irq_count--;
					break;
				}
			}

			if (i == MAX_IRQ_PER_VLYNQ)
				return (VLYNQ_INTERNAL_ERROR);
		}
		break;

	default:
		break;
	}

	return (VLYNQ_OK);
}

int vlynq_dev_find_irq(void *vlynq_dev, u8 *irq, u32 num_irq)
{
	struct vlynq_dev_t *p_vlynq_dev = vlynq_dev;
	int i = 0;
	s8 *p_dev_irq;

	if (!p_vlynq_dev)
		return (VLYNQ_INVALID_PARAM);

	if (p_vlynq_dev->irq_count != num_irq)
		return (VLYNQ_INTERNAL_ERROR);

	p_dev_irq = p_vlynq_dev->irq;

	for (i = 0; i < MAX_IRQ_PER_VLYNQ; i++) {
		if (p_dev_irq[i] != -1)
			irq[--num_irq] = p_dev_irq[i];
	}

	return (VLYNQ_OK);

}
EXPORT_SYMBOL(vlynq_dev_find_irq);

void *vlynq_dev_create(void *vlynq, char *name,
				u32 instance, int reset_bit, u8 peer)
{
	struct vlynq_dev_t *p_vlynq_dev = NULL;

	p_vlynq_dev = vlynq_dev_get_free_dev();
	if (!p_vlynq_dev || !vlynq)
		return (NULL);

	strcpy(p_vlynq_dev->name, name);
	p_vlynq_dev->instance = instance;
	p_vlynq_dev->reset_bit = reset_bit;
	p_vlynq_dev->vlynq = vlynq;
	p_vlynq_dev->peer = peer;

	return (p_vlynq_dev);
}
EXPORT_SYMBOL(vlynq_dev_create);

int vlynq_dev_destroy(void *vlynq_dev)
{
	struct vlynq_dev_t *p_vlynq_dev = vlynq_dev;
	int j;

	if (!p_vlynq_dev)
		return (VLYNQ_INVALID_PARAM);

	if (p_vlynq_dev->dev_cb)
		return (VLYNQ_INTERNAL_ERROR);

	p_vlynq_dev->vlynq = NULL;
	p_vlynq_dev->irq_count = 0;

	for (j = 0; j < MAX_IRQ_PER_VLYNQ; j++)
		p_vlynq_dev->irq[j] = -1;

	vlynq_dev_free_dev(p_vlynq_dev);

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_dev_destroy);

int vlynq_dev_get_reset_bit(void *vlynq_dev, u32 *reset_bit)
{
	struct vlynq_dev_t *p_vlynq_dev = vlynq_dev;

	if (!p_vlynq_dev)
		return (VLYNQ_INVALID_PARAM);

	*reset_bit = p_vlynq_dev->reset_bit;

	return (VLYNQ_OK);
}
EXPORT_SYMBOL(vlynq_dev_get_reset_bit);

int vlynq_dev_init(void)
{
	struct vlynq_dev_t *p_vlynq_dev =
				gp_free_vlynq_dev_list = &vlynq_dev[0];
	u32 i, j;

	for (i = 0; i < MAX_DEV_COUNT; i++) {
		p_vlynq_dev->vlynq = NULL;
		p_vlynq_dev->irq_count = 0;
		p_vlynq_dev->dev_cb = NULL;

		for (j = 0; j < MAX_IRQ_PER_VLYNQ; j++)
			p_vlynq_dev->irq[j] = -1;

		p_vlynq_dev->next =
		    (i == MAX_DEV_COUNT - 1) ? NULL : &vlynq_dev[i + 1];
		p_vlynq_dev = p_vlynq_dev->next;
	}

	vlynq_dev_cb_init();

	return (VLYNQ_OK);
}

int vlynq_dev_handle_event(void *vlynq_dev, u32 cmd, u32 val)
{
	struct vlynq_dev_t *p_vlynq_dev = vlynq_dev;
	int ret_val = VLYNQ_INTERNAL_ERROR;

	if (p_vlynq_dev) {
		struct vlynq_dev_cb *p_dev_cb;

		for (p_dev_cb = p_vlynq_dev->dev_cb;
		     p_dev_cb != NULL; p_dev_cb = p_dev_cb->next)
			p_dev_cb->cb_fn(p_dev_cb->cb_param, cmd, val);

		ret_val = VLYNQ_OK;
	}

	return (ret_val);
}

int vlynq_dev_dump_dev(void *vlynq_dev, char *buf, int limit,
		       int *eof)
{
	int irq_count, len = 0;
	struct vlynq_dev_t *p_vlynq_dev = vlynq_dev;

	if (!p_vlynq_dev)
		return (len);

	len += snprintf(buf + len, limit - len, "    %s%d\n",
			p_vlynq_dev->name, p_vlynq_dev->instance);

	len += snprintf(buf + len, limit - len, "      IRQ-id(s): ");

	for (irq_count = 0; irq_count < MAX_IRQ_PER_VLYNQ; irq_count++) {
		int val;

		if (p_vlynq_dev->irq[irq_count] == -1)
			continue;

		val = p_vlynq_dev->irq[irq_count];

		len += snprintf(buf + len, limit - len, "%d, ", val);
	}

	len += snprintf(buf + len, limit - len, "\n");

	return (len);
}
