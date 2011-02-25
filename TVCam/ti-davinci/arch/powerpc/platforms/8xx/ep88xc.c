/*arch/ppc/platforms/ep88xc.c
 *
 * Platform setup for the Embedded Planet EP88xC board
 *
 * Author: Scott Wood <scottwood@freescale.com>
 * Copyright 2007 Freescale Semiconductor, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/param.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/root_dev.h>

#include <linux/fs_enet_pd.h>
#include <linux/fs_uart_pd.h>
#include <linux/platform_device.h>
#include <linux/fsl_devices.h>
#include <linux/mii.h>

#include <asm/delay.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <asm/system.h>
#include <asm/time.h>
#include <asm/ppcboot.h>
#include <asm/udbg.h>
#include <asm/mpc8xx.h>
#include <asm/8xx_immap.h>
#include <asm/commproc.h>
#include <asm/fs_pd.h>
#include <asm/prom.h>

extern void mpc8xx_show_cpuinfo(struct seq_file *);
extern void mpc8xx_restart(char *cmd);
extern void mpc8xx_calibrate_decr(void);
extern int mpc8xx_set_rtc_time(struct rtc_time *tm);
extern void mpc8xx_get_rtc_time(struct rtc_time *tm);
extern void m8xx_pic_init(void);
extern unsigned int mpc8xx_get_irq(struct pt_regs *);
extern void mpc8xx_map_io(void);

void init_fec_ioports(struct fs_platform_info *fpi) {}
void init_scc_ioports(struct fs_platform_info *fpi) {}

int platform_device_skip(char *model, int id)
{
	return 0;
}

struct cpm_pin {
	int port, pin, flags;
};

static struct cpm_pin ep88xc_pins[] = {
	/* SMC1 */
	{1, 24, CPM_PIN_INPUT}, /* RX */
	{1, 25, CPM_PIN_INPUT | CPM_PIN_SECONDARY}, /* TX */

	/* SCC2 */
	{0, 12, CPM_PIN_INPUT}, /* TX */
	{0, 13, CPM_PIN_INPUT}, /* RX */
	{2, 8, CPM_PIN_INPUT | CPM_PIN_SECONDARY | CPM_PIN_GPIO}, /* CD */
	{2, 9, CPM_PIN_INPUT | CPM_PIN_SECONDARY | CPM_PIN_GPIO}, /* CTS */
	{2, 14, CPM_PIN_INPUT}, /* RTS */

	/* MII1 */
	{0, 0, CPM_PIN_INPUT},
	{0, 1, CPM_PIN_INPUT},
	{0, 2, CPM_PIN_INPUT},
	{0, 3, CPM_PIN_INPUT},
	{0, 4, CPM_PIN_OUTPUT},
	{0, 10, CPM_PIN_OUTPUT},
	{0, 11, CPM_PIN_OUTPUT},
	{1, 19, CPM_PIN_INPUT},
	{1, 31, CPM_PIN_INPUT},
	{2, 12, CPM_PIN_INPUT},
	{2, 13, CPM_PIN_INPUT},
	{3, 8, CPM_PIN_INPUT},
	{4, 30, CPM_PIN_OUTPUT},
	{4, 31, CPM_PIN_OUTPUT},

	/* MII2 */
	{4, 14, CPM_PIN_OUTPUT | CPM_PIN_SECONDARY},
	{4, 15, CPM_PIN_OUTPUT | CPM_PIN_SECONDARY},
	{4, 16, CPM_PIN_OUTPUT},
	{4, 17, CPM_PIN_OUTPUT | CPM_PIN_SECONDARY},
	{4, 18, CPM_PIN_OUTPUT | CPM_PIN_SECONDARY},
	{4, 19, CPM_PIN_OUTPUT | CPM_PIN_SECONDARY},
	{4, 20, CPM_PIN_OUTPUT | CPM_PIN_SECONDARY},
	{4, 21, CPM_PIN_OUTPUT},
	{4, 22, CPM_PIN_OUTPUT},
	{4, 23, CPM_PIN_OUTPUT},
	{4, 24, CPM_PIN_OUTPUT},
	{4, 25, CPM_PIN_OUTPUT},
	{4, 26, CPM_PIN_OUTPUT},
	{4, 27, CPM_PIN_OUTPUT},
	{4, 28, CPM_PIN_OUTPUT},
	{4, 29, CPM_PIN_OUTPUT},

	/* USB */
	{0, 6, CPM_PIN_INPUT},  /* CLK2 */
	{0, 14, CPM_PIN_INPUT}, /* USBOE */
	{0, 15, CPM_PIN_INPUT}, /* USBRXD */
	{2, 6, CPM_PIN_OUTPUT}, /* USBTXN */
	{2, 7, CPM_PIN_OUTPUT}, /* USBTXP */
	{2, 10, CPM_PIN_INPUT | CPM_PIN_GPIO | CPM_PIN_SECONDARY}, /* USBRXN */
	{2, 11, CPM_PIN_INPUT | CPM_PIN_GPIO | CPM_PIN_SECONDARY}, /* USBRXP */

	/* I2C */
	{1, 26, CPM_PIN_OUTPUT | CPM_PIN_OPENDRAIN},
	{1, 27, CPM_PIN_OUTPUT | CPM_PIN_OPENDRAIN},
};

void __init init_ioports(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ep88xc_pins); i++) {
		struct cpm_pin *pin = &ep88xc_pins[i];
		cpm1_set_pin(pin->port, pin->pin, pin->flags);
	}

#ifndef CONFIG_KGDB_CPM_UART_SMC1
	cpm1_clk_setup(CPM_CLK_SMC1, CPM_BRG1, CPM_CLK_RTX);
#endif
	cpm1_clk_setup(CPM_CLK_SCC1, CPM_CLK2, CPM_CLK_TX); /* USB */
	cpm1_clk_setup(CPM_CLK_SCC1, CPM_CLK2, CPM_CLK_RX);
#ifndef CONFIG_KGDB_CPM_UART_SCC2
	cpm1_clk_setup(CPM_CLK_SCC2, CPM_BRG2, CPM_CLK_TX);
	cpm1_clk_setup(CPM_CLK_SCC2, CPM_BRG2, CPM_CLK_RX);
#endif
}

#define BCSR2_FLASH_UNPROTECT 0x08

#define BCSR7_SCC2_ENABLE 0x10

#define BCSR8_PHY1_ENABLE 0x80
#define BCSR8_PHY1_POWER  0x40
#define BCSR8_PHY2_ENABLE 0x20
#define BCSR8_PHY2_POWER  0x10

#define BCSR9_USB_ENABLE  0x80
#define BCSR9_USB_POWER   0x40
#define BCSR9_USB_HOST    0x20
#define BCSR9_USB_FULL_SPEED_TARGET 0x10

static void __init ep88xc_setup_arch(void)
{
	struct device_node *np;
	u8 __iomem *ep88xc_bcsr;

	np = of_find_node_by_type(NULL, "cpu");
	if (np != 0) {
		const unsigned int *fp;

		fp = get_property(np, "clock-frequency", NULL);
		if (fp != 0)
			loops_per_jiffy = *fp / HZ;
		else
			loops_per_jiffy = 50000000 / HZ;
		of_node_put(np);
	}

#if !defined(CONFIG_KGDB_CPM_UART_SMC1) && !defined(CONFIG_KGDB_CPM_UART_SCC2)
	cpm_reset();
	init_ioports();
#endif

	np = of_find_compatible_node(NULL, NULL, "fsl,ep88xc-bcsr");
	if (!np) {
		printk(KERN_CRIT "Could not find fsl,ep88xc-bcsr node\n");
		return;
	}

	ep88xc_bcsr = of_iomap(np, 0);
	of_node_put(np);

	if (!ep88xc_bcsr) {
		printk(KERN_CRIT "Could not remap BCSR\n");
		return;
	}

	setbits8(&ep88xc_bcsr[2], BCSR2_FLASH_UNPROTECT);

	setbits8(&ep88xc_bcsr[7], BCSR7_SCC2_ENABLE);
	setbits8(&ep88xc_bcsr[8], BCSR8_PHY1_ENABLE | BCSR8_PHY1_POWER |
	                          BCSR8_PHY2_ENABLE | BCSR8_PHY2_POWER);
	clrsetbits_8(&ep88xc_bcsr[9], 0xFF, BCSR9_USB_ENABLE |
				      BCSR9_USB_FULL_SPEED_TARGET);
	printk(KERN_INFO "Board revision: %02x\n", in_8(&ep88xc_bcsr[16]));
	ROOT_DEV = Root_NFS;
}

static int __init ep88xc_probe(void)
{
	char *model = of_get_flat_dt_prop(of_get_flat_dt_root(),
					  "model", NULL);
	if (model == NULL)
		return 0;
	if (strcmp(model, "EP88XC"))
		return 0;

	return 1;
}

define_machine(ep88xc)
{
	.name			= "EP88XC",
	.probe			= ep88xc_probe,
	.setup_arch		= ep88xc_setup_arch,
	.init_IRQ		= m8xx_pic_init,
	.show_cpuinfo		= mpc8xx_show_cpuinfo,
	.get_irq		= mpc8xx_get_irq,
	.restart		= mpc8xx_restart,
	.calibrate_decr		= mpc8xx_calibrate_decr,
	.set_rtc_time		= mpc8xx_set_rtc_time,
	.get_rtc_time		= mpc8xx_get_rtc_time,
	.setup_io_mappings	= mpc8xx_map_io,
	.progress		= udbg_progress,
};
