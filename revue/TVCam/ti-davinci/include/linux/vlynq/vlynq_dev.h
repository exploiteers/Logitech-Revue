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

#ifndef __VLYNQ_DEV_H__
#define __VLYNQ_DEV_H__

#include "vlynq.h"

#define VLYNQ_EVENT_LOCAL_ERROR             0
#define VLYNQ_EVENT_PEER_ERROR              1

#define VLYNQ_DEV_ADD_IRQ                   0
#define VLYNQ_DEV_REMOVE_IRQ                1

typedef int (*vlynq_dev_cb_fn) (void *data, u32 condition, u32 value);

int vlynq_dev_cb_register(void *vlynq_dev,
			  vlynq_dev_cb_fn cb_fn, void *this_driver);

int vlynq_dev_cb_unregister(void  *vlynq_dev, void *this_driver);

void *vlynq_dev_find(char *name, u8 instance);

void *vlynq_dev_get_vlynq(void *vlynq_dev_A);

int vlynq_dev_find_irq(void *vlynq_dev, u8 *irq, u32 num_irq);

int vlynq_dev_get_reset_bit(void *vlynq_dev, u32 *reset_bit);

void *vlynq_dev_create(void *vlynq, char *name,
				u32 instance, int reset_bit, u8 peer);

int vlynq_dev_destroy(void *vlynq_dev);

/* Protected API(s) to be used only by pal_vlynq.c */
int vlynq_dev_init(void);

int vlynq_dev_handle_event(void *vlynq_dev, u32 cmd, u32 val);

int vlynq_dev_ioctl(void *vlynq_dev, u32 cmd, u32 cmd_val);

#endif				/* #ifndef __VLYNQ_DEV_H__ */
