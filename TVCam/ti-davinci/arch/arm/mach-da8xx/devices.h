/*
 * Common controller/device info
 *
 * Author: Mark A. Greer <mgreer@mvista.com>
 *
 * Copyright (C) 2008 MontaVista Software, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */
#ifndef __ARCH_ARM_DA8XX_DEVICES_H
#define __ARCH_ARM_DA8XX_DEVICES_H

enum {
	DA8XX_PDEV_SERIAL,
	DA8XX_PDEV_I2C_0,
	DA8XX_PDEV_I2C_1,
	DA8XX_PDEV_WATCHDOG,
	DA8XX_PDEV_USB_20,
	DA8XX_PDEV_USB_11,
	DA8XX_PDEV_EMAC,
	DA8XX_PDEV_MMC,
	DA8XX_PDEV_RTC,
	DA8XX_PDEV_EQEP_0,
	DA8XX_PDEV_EQEP_1,
	DA8XX_PDEV_LCDC,
	DA8XX_PDEV_COUNT
};

int da8xx_add_devices(void *pdata[DA8XX_PDEV_COUNT]);
void da8xx_irq_init(u8 *irq_prio);
void da8xx_uart_clk_init(u32 uartclk);

#ifdef CONFIG_KGDB_8250
void da8xx_kgdb_init(void);
#else
#define da8xx_kgdb_init()
#endif


#endif /* __ARCH_ARM_DA8XX_DEVICES_H */
