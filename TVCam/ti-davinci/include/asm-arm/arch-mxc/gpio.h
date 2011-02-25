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
#ifndef __ASM_ARCH_MXC_GPIO_H__
#define __ASM_ARCH_MXC_GPIO_H__

/*!
 * @defgroup GPIO General Purpose Input Output (GPIO)
 */

/*!
 * @file arch-mxc/gpio.h
 * @brief This file contains the GPIO API functions.
 *
 * @ingroup GPIO
 */

#include <asm/sizes.h>
#include <linux/interrupt.h>
#include <asm/hardware.h>

/* gpio related defines */

/*!
 * gpio port structure
 */
struct mxc_gpio_port {
	u32 num;		/*!< gpio port number */
	u32 base;		/*!< gpio port base VA */
	u16 irq;		/*!< irq number to the core */
	u16 virtual_irq_start;	/*!< virtual irq start number */
};

/*!
 * This enumeration data type defines various different ways for interrupting
 * the ARM core from GPIO signals. The way to interrupt the core is dictated
 * by the external hardware.
 */
enum gpio_int_cfg {
#if defined(CONFIG_ARCH_MX2)
	GPIO_INT_LOW_LEV = 0x3,	/*!< low level sensitive */
	GPIO_INT_HIGH_LEV = 0x2,	/*!< high level sensitive */
	GPIO_INT_RISE_EDGE = 0x0,	/*!< rising edge sensitive */
	GPIO_INT_FALL_EDGE = 0x1,	/*!< falling edge sensitive */
	GPIO_INT_NONE = 0x4	/*!< No interrupt */
#else
#error gpio_int_cfg values are not defined for this config
#endif
};

/*!
 * Request ownership for a GPIO pin. The caller has to check the return value
 * of this function to make sure it returns 0 before make use of that pin.
 * @param pin		a name defined by \b iomux_pin_name_t
 * @return		0 if successful; Non-zero otherwise
 */
extern int mxc_request_gpio(iomux_pin_name_t pin);

/*!
 * Release ownership for a GPIO pin
 * @param pin		a name defined by \b iomux_pin_name_t
 */
extern void mxc_free_gpio(iomux_pin_name_t pin);

/*!
 * Exported function to set a GPIO pin's direction
 * @param pin		a name defined by \b iomux_pin_name_t
 * @param is_input	1 (or non-zero) for input; 0 for output
 */
extern void mxc_set_gpio_direction(iomux_pin_name_t pin, int is_input);

/*!
 * Exported function to set a GPIO pin's data output
 * @param pin		a name defined by \b iomux_pin_name_t
 * @param data		value to be set (only 0 or 1 is valid)
 */
extern void mxc_set_gpio_dataout(iomux_pin_name_t pin, u32 data);

/*!
 * Return the data value of a GPIO signal.
 * @param pin	a name defined by \b iomux_pin_name_t
 *
 * @return 	value (0 or 1) of the GPIO signal; -1 if pass in invalid pin
 */
extern int mxc_get_gpio_datain(iomux_pin_name_t pin);

/*!
 * GPIO driver initialization
 * @return    always 0
 */
extern int mxc_gpio_init(void);

#endif				/* __ASM_ARCH_MXC_GPIO_H__ */
