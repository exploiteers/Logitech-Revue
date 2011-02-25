/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * Based on mach-integrator/integrator_ap.c
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/kgdb.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <linux/spi/spi.h>
#if defined(CONFIG_MTD) || defined(CONFIG_MTD_MODULE)
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <asm/mach/flash.h>
#endif
#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/arch/common.h>
#include <asm/arch/gpio.h>
#include <asm/arch/keypad.h>
#include "gpio_mux.h"

/*!
 * @file mx27ads.c
 * @brief This file contains the board specific initialization routines.
 *
 * @ingroup System
 */

static void mxc_nop_release(struct device *dev)
{
        /* Nothing */
}

/* MTD NOR flash */

#if defined(CONFIG_MTD_MXC) || defined(CONFIG_MTD_MXC_MODULE)

static struct mtd_partition mxc_nor_partitions[] = {
        {
         .name = "Bootloader",
         .size = 512 * 1024,
         .offset = 0x00000000,
         .mask_flags = MTD_WRITEABLE    /* force read-only */
         },
        {
         .name = "nor.Kernel",
         .size = 2 * 1024 * 1024,
         .offset = MTDPART_OFS_APPEND,
         .mask_flags = 0},
        {
         .name = "nor.userfs",
         .size = 14 * 1024 * 1024,
         .offset = MTDPART_OFS_APPEND,
         .mask_flags = 0},
        {
         .name = "nor.rootfs",
         .size = 12 * 1024 * 1024,
         .offset = MTDPART_OFS_APPEND,
         .mask_flags = MTD_WRITEABLE},
        {
         .name = "FIS directory",
         .size = 12 * 1024,
         .offset = 0x01FE0000,
         .mask_flags = MTD_WRITEABLE    /* force read-only */
         },
        {
         .name = "Redboot config",
         .size = MTDPART_SIZ_FULL,
         .offset = 0x01FFF000,
         .mask_flags = MTD_WRITEABLE    /* force read-only */
         },
};


static struct flash_platform_data mxc_flash_data = {
        .map_name = "cfi_probe",
        .width = 2,
        .parts = mxc_nor_partitions,
        .nr_parts = ARRAY_SIZE(mxc_nor_partitions),
};

static struct resource mxc_flash_resource = {
        .start = 0xc0000000,
        .end = 0xc0000000 + 0x02000000 - 1,
        .flags = IORESOURCE_MEM,

};

static struct platform_device mxc_nor_mtd_device = {
        .name = "mxc_nor_flash",
        .id = 0,
        .dev = {
                .release = mxc_nop_release,
                .platform_data = &mxc_flash_data,
                },
        .num_resources = 1,
        .resource = &mxc_flash_resource,
};

static void mxc_init_nor_mtd(void)
{
        (void)platform_device_register(&mxc_nor_mtd_device);
}
#else
static void mxc_init_nor_mtd(void)
{
}
#endif

/* MTD NAND flash */

#if defined(CONFIG_MTD_NAND_MXC) || defined(CONFIG_MTD_NAND_MXC_MODULE)

static struct mtd_partition mxc_nand_partitions[4] = {
	{
	 .name = "IPL-SPL",
	 .offset = 0,
	 .size = 128 * 1024},
	{
	 .name = "nand.kernel",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 4 * 1024 * 1024},
	{
	 .name = "nand.rootfs",
	 .offset = MTDPART_OFS_APPEND,
	 .size = 22 * 1024 * 1024},
	{
	 .name = "nand.userfs",
	 .offset = MTDPART_OFS_APPEND,
	 .size = MTDPART_SIZ_FULL},
};

static struct flash_platform_data mxc_nand_data = {
	.parts = mxc_nand_partitions,
	.nr_parts = ARRAY_SIZE(mxc_nand_partitions),
	.width = 1,
};

static struct platform_device mxc_nand_mtd_device = {
	.name = "mxc_nand_flash",
	.id = 0,
	.dev = {
		.release = mxc_nop_release,
		.platform_data = &mxc_nand_data,
		},
};

static void mxc_init_nand_mtd(void)
{
	(void)platform_device_register(&mxc_nand_mtd_device);
}
#else
static inline void mxc_init_nand_mtd(void)
{
}
#endif

#if defined(CONFIG_FB_MXC_SYNC_PANEL) || defined(CONFIG_FB_MXC_SYNC_PANEL_MODULE)
static const char fb_default_mode[] = "Sharp-QVGA";

/* mxc lcd driver */
static struct platform_device mxc_fb_device = {
        .name = "mxc_sdc_fb",
        .id = 0,
        .dev = {
                .release = mxc_nop_release,
                .platform_data = &fb_default_mode,
                .coherent_dma_mask = 0xFFFFFFFF,
                },
};

static void mxc_init_fb(void)
{
        (void)platform_device_register(&mxc_fb_device);
}
#else
static inline void mxc_init_fb(void)
{
}
#endif

static struct spi_board_info mxc_spi_board_info[] __initdata = {
        {
         .modalias = "pmic_spi",
         .irq = IOMUX_TO_IRQ(MX27_PIN_TOUT),
         .max_speed_hz = 4000000,
         .bus_num = 1,
         .chip_select = 0,
         },
};

static const int pbc_card_bit[4][3] = {
	/* BSTAT            IMR enable       IMR removal */
	{PBC_BSTAT_SD2_DET, PBC_INTR_SD2_EN, PBC_INTR_SD2_R_EN},
	{PBC_BSTAT_SD3_DET, PBC_INTR_SD3_EN, PBC_INTR_SD3_R_EN},
	{PBC_BSTAT_MS_DET, PBC_INTR_MS_EN, PBC_INTR_MS_R_EN},
	{PBC_BSTAT_SD1_DET, PBC_INTR_SD1_EN, PBC_INTR_SD1_R_EN},
};

static int mxc_card_status;

/*!
 * Check if a SD card has been inserted or not.
 *
 * @param  num		a card number as defined in \b enum \b mxc_card_no
 * @return 0 if a card is not present; non-zero otherwise.
 */
int mxc_card_detected(enum mxc_card_no num)
{
	u32 status;

	status = __raw_readw(PBC_BSTAT1_REG);
	return ((status & MXC_BSTAT_BIT(num)) == 0);
}

/*
 * Check if there is any state change by reading the IMR register and the
 * previous and current states of the board status register (offset 0x28).
 * A state change is defined to be card insertion OR removal. So the driver
 * may have to call the mxc_card_detected() function to see if it is card
 * insertion or removal.
 *
 * @param  mask		current IMR value
 * @param  s0		previous status register value (offset 0x28)
 * @param  s1		current status register value (offset 0x28)
 *
 * @return 0 if no card status change OR the corresponding bits in the IMR
 *           (passed in as 'mask') is NOT set.
 *         A non-zero value indicates some card state changes. For example,
 *         0b0001 means SD3 has a card state change (bit0 is set) AND its
 *               associated insertion or removal bits in IMR is SET.
 *         0b0100 means SD1 has a card state change (bit2 is set) AND its
 *               associated insertion or removal bits in IMR is SET.
 *         0b1001 means both MS and SD3 have state changes
 */
static u32 mxc_card_state_changed(u32 mask, u32 s0, u32 s1)
{
	u32 i, retval = 0;
	u32 stat = (s0 ^ s1) & 0x7800;

	if (stat == 0)
		return 0;

	for (i = MXC_CARD_MIN; i <= MXC_CARD_MAX; i++) {
		if ((stat & pbc_card_bit[i][0]) != 0 &&
		    (mask & (pbc_card_bit[i][1] | pbc_card_bit[i][2])) != 0) {
			retval |= 1 << i;
		}
	}
	return retval;
}

/*!
 * Interrupt handler for the expio (CPLD) to deal with interrupts from
 * FEC, external UART, CS8900 Ethernet and SD cards, etc.
 */
static void mxc_expio_irq_handler(u32 irq, struct irq_desc *desc,
		struct pt_regs *regs)
{
	u32 imr, card_int, i;
	u32 int_valid;
	u32 expio_irq;
	u32 stat = __raw_readw(PBC_BSTAT1_REG);

	desc->chip->mask(irq);	/* irq = gpio irq number */

	imr = __raw_readw(PBC_INTMASK_SET_REG);

	card_int = mxc_card_state_changed(imr, mxc_card_status, stat);
	mxc_card_status = stat;

	if (card_int != 0) {
		for (i = MXC_CARD_MIN; i <= MXC_CARD_MAX - 1; i++) {
			if ((card_int & (1 << i)) != 0) {
				pr_info("card no %d state changed\n", i);
			}
		}
	}

	/* Bits defined in PBC_INTSTATUS_REG at 0x2C */
	int_valid = __raw_readw(PBC_INTSTATUS_REG) & imr;
	/*  combined with the card interrupt valid information */
	int_valid = (int_valid & 0x0F8E) | (card_int << PBC_INTR_SD2_EN_BIT);

	if (unlikely(!int_valid)) {
		printk(KERN_ERR "\nEXPIO: Spurious interrupt:0x%0x\n\n",
		       int_valid);
		pr_info("CPLD IMR(0x38)=0x%x, BSTAT1(0x28)=0x%x\n", imr, stat);
		goto out;
	}

	expio_irq = MXC_EXP_IO_BASE;
	for (; int_valid != 0; int_valid >>= 1, expio_irq++) {
		struct irqdesc *d;
		if ((int_valid & 1) == 0)
			continue;
		d = irq_desc + expio_irq;
		if (unlikely(!(d->handle_irq))) {
			printk(KERN_ERR "\nEXPIO irq: %d unhandeled\n",
			       expio_irq);
			BUG();	/* oops */
		}
		d->handle_irq(expio_irq, d, regs);
	}

out:
	desc->chip->ack(irq);
	desc->chip->unmask(irq);
}

/*
 * Disable an expio pin's interrupt by setting the bit in the imr.
 * @param irq		an expio virtual irq number
 */
static void expio_mask_irq(u32 irq)
{
	u32 expio = MXC_IRQ_TO_EXPIO(irq);

	/* mask the interrupt */
	if (irq < EXPIO_INT_SD2_EN) {
		__raw_writew(1 << expio, PBC_INTMASK_CLEAR_REG);
	} else {
		irq -= EXPIO_INT_SD2_EN;
		/* clear both SDx_EN and SDx_R_EN bits */
		__raw_writew((pbc_card_bit[irq][1] | pbc_card_bit[irq][2]),
			     PBC_INTMASK_CLEAR_REG);
	}
}

/*
 * Acknowledge an expanded io pin's interrupt by clearing the bit in the isr.
 * @param irq		an expanded io virtual irq number
 */
static void expio_ack_irq(u32 irq)
{
	u32 expio = MXC_IRQ_TO_EXPIO(irq);
	/* clear the interrupt status */
	__raw_writew(1 << expio, PBC_INTSTATUS_REG);
	/* mask the interrupt */
	expio_mask_irq(irq);
}

/*
 * Enable a expio pin's interrupt by clearing the bit in the imr.
 * @param irq		an expio virtual irq number
 */
static void expio_unmask_irq(u32 irq)
{
	u32 expio = MXC_IRQ_TO_EXPIO(irq);

	/* unmask the interrupt */
	if (irq < EXPIO_INT_SD2_EN) {
		if (irq == EXPIO_INT_XUART_INTA) {
			/* Set 8250 MCR register bit 3 - Forces the INT (A-B
			 * outputs to the active mode and sets OP2 to logic 0.
			 * This is needed to avoid spurious int caused by the
			 * internal CPLD pull-up for the interrupt pin.
			 */
			u16 val = __raw_readw(MXC_LL_EXTUART_VADDR + 8);
			__raw_writew(val | 0x8, MXC_LL_EXTUART_VADDR + 8);
		}
		__raw_writew(1 << expio, PBC_INTMASK_SET_REG);
	} else {
		irq -= EXPIO_INT_SD2_EN;

		if (mxc_card_detected(irq)) {
			__raw_writew(pbc_card_bit[irq][2], PBC_INTMASK_SET_REG);
		} else {
			__raw_writew(pbc_card_bit[irq][1], PBC_INTMASK_SET_REG);
		}
	}
}

static struct irqchip expio_irq_chip = {
	.ack = expio_ack_irq,
	.mask = expio_mask_irq,
	.unmask = expio_unmask_irq,
};

static int __init mxc_expio_init(void)
{
	int i, ver;

	ver = (__raw_readw(PBC_VERSION_REG) >> 8) & 0xFF;
	if ((ver & 0x80) != 0) {
		pr_info("MX27 ADS EXPIO(CPLD) hardware\n");
		pr_info("CPLD version: 0x%x\n", ver);
	} else {
		ver &= 0x0F;
		pr_info("MX27 EVB EXPIO(CPLD) hardware\n");
		if (ver == 0xF || ver <= MXC_CPLD_VER_1_50)
			pr_info("Wrong CPLD version: %d\n", ver);
		else {
			pr_info("CPLD version: %d\n", ver);
		}
	}

	mxc_card_status = __raw_readw(PBC_BSTAT1_REG);

	/*
	 * Configure INT line as GPIO input
	 */
	gpio_config_mux(MX27_PIN_TIN, GPIO_MUX_GPIO);
	mxc_set_gpio_direction(MX27_PIN_TIN, 1);

	/* disable the interrupt and clear the status */
	__raw_writew(0xFFFF, PBC_INTMASK_CLEAR_REG);
	__raw_writew(0xFFFF, PBC_INTSTATUS_REG);

	for (i = MXC_EXP_IO_BASE; i < (MXC_EXP_IO_BASE + MXC_MAX_EXP_IO_LINES);
	     i++) {
		set_irq_chip(i, &expio_irq_chip);
		set_irq_handler(i, do_level_IRQ);
		set_irq_flags(i, IRQF_VALID);
	}
	set_irq_type(EXPIO_PARENT_INT, IRQT_HIGH);
	set_irq_chained_handler(EXPIO_PARENT_INT, mxc_expio_irq_handler);

	return 0;
}


#if defined(CONFIG_SERIAL_8250) || defined(CONFIG_SERIAL_8250_MODULE) || \
	defined(CONFIG_KGDB_8250)
/*!
 * The serial port definition structure.
 */
static struct plat_serial8250_port serial_platform_data[] = {
	{
		.membase  = (void __iomem *)(CS4_BASE_ADDR_VIRT + 0x20000),
		.mapbase  = (unsigned long)(CS4_BASE_ADDR + 0x20000),
		.irq      = EXPIO_INT_XUART_INTA,
		.uartclk  = 3686400,
		.regshift = 1,
		.iotype   = UPIO_MEM,
		.flags    = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST | UPF_AUTO_IRQ,
	},
	{},
};
#endif

#if defined(CONFIG_SERIAL_8250) || defined(CONFIG_SERIAL_8250_MODULE)
static struct platform_device serial_device = {
	.name = "serial8250",
	.id   = 0,
	.dev  = {
		.platform_data = serial_platform_data,
	},
};
#endif

static int __init mxc_init_extuart(void)
{
	/* Toggle the UART reset line */

	__raw_writew(PBC_BCTRL1_URST, PBC_BCTRL1_SET_REG);
	udelay(1000);
	__raw_writew(PBC_BCTRL1_URST, PBC_BCTRL1_CLEAR_REG);
#ifdef CONFIG_KGDB_8250
	kgdb8250_add_platform_port(0, serial_platform_data);
#endif
#if defined(CONFIG_SERIAL_8250) || defined(CONFIG_SERIAL_8250_MODULE)
	return platform_device_register(&serial_device);
#else
	return 0;
#endif
}

#if defined(CONFIG_MXC_PMIC_MC13783) && defined(CONFIG_SND_MXC_PMIC)
extern void gpio_ssi_active(int ssi_num);

static void __init mxc_init_pmic_audio(void)
{
	struct clk *ssi_clk;
	struct clk *ckih_clk;
	struct clk *cko_clk;

	/* Enable 26 mhz clock on CKO1 for PMIC audio */
	ckih_clk = clk_get(NULL, "ckih");
	cko_clk = clk_get(NULL, "clko_clk");
	if (IS_ERR(ckih_clk) || IS_ERR(cko_clk)) {
		printk(KERN_ERR "Unable to set CLKO output to CKIH\n");
	} else {
		clk_set_parent(cko_clk, ckih_clk);
		clk_set_rate(cko_clk, clk_get_rate(ckih_clk));
		clk_enable(cko_clk);
	}
	clk_put(ckih_clk);
	clk_put(cko_clk);

	ssi_clk = clk_get(NULL, "ssi_clk.0");
	clk_enable(ssi_clk);
	clk_put(ssi_clk);
	ssi_clk = clk_get(NULL, "ssi_clk.1");
	clk_enable(ssi_clk);
	clk_put(ssi_clk);

	gpio_ssi_active(0);
	gpio_ssi_active(1);
}
#else
static void __inline mxc_init_pmic_audio(void)
{
}
#endif

#if defined(CONFIG_KEYBOARD_MXC) || defined(CONFIG_KEYBOARD_MXC_MODULE)

/*!
 * This array is used for mapping mx27 ADS keypad scancodes to input keyboard
 * keycodes.
 */
static u16 mxckpd_keycodes[(MXC_KBD_MAXROW * MXC_KBD_MAXCOL)] = {
	KEY_KP9, KEY_LEFTSHIFT, KEY_0, KEY_KPASTERISK, KEY_RECORD, KEY_POWER,
	KEY_KP8, KEY_9, KEY_8, KEY_7, KEY_KP5, KEY_VOLUMEDOWN,
	KEY_KP7, KEY_6, KEY_5, KEY_4, KEY_KP4, KEY_VOLUMEUP,
	KEY_KP6, KEY_3, KEY_2, KEY_1, KEY_KP3, KEY_DOWN,
	KEY_BACK, KEY_RIGHT, KEY_ENTER, KEY_LEFT, KEY_HOME, KEY_KP2,
	KEY_END, KEY_F2, KEY_UP, KEY_F1, KEY_F4, KEY_KP1,
};

static struct mxc_keypad_data evb_6_by_6_keypad = {
	.rowmax = 6,
	.colmax = 6,
	.irq = MXC_INT_KPP,
	.learning = 0,
	.delay = 2,
	.matrix = mxckpd_keycodes,
};

static struct resource mxc_kpp_resources[] = {
	[0] = {
	       .start = MXC_INT_KPP,
	       .flags = IORESOURCE_IRQ,
	       }
};

/* mxc keypad driver */
static struct platform_device mxc_keypad_device = {
	.name = "mxc_keypad",
	.id = 0,
	.num_resources = ARRAY_SIZE(mxc_kpp_resources),
	.resource = mxc_kpp_resources,
	.dev = {
		.platform_data = &evb_6_by_6_keypad,
		},
};

static void mxc_init_keypad(void)
{
	(void)platform_device_register(&mxc_keypad_device);
}
#else
static inline void mxc_init_keypad(void)
{
}
#endif

#ifdef CONFIG_VIDEO_MXC_EMMA_OUTPUT
static void camera_platform_release(struct device *device)
{
}

static struct platform_device mxc_v4l2out_device = {
	.name = "MXC Video Output",
	.dev = {
		.release = camera_platform_release,
		},
	.id = 0,
};

static void mxc_init_video_output(void)
{
	(void)platform_device_register(&mxc_v4l2out_device);
}
#else
static inline void mxc_init_video_output(void)
{
}
#endif

/*!
 * This structure defines static mappings for the i.MX27ADS board.
 */
static struct map_desc mx27ads_io_desc[] __initdata = {
	{
		.virtual = CS4_BASE_ADDR_VIRT,
		.pfn     = __phys_to_pfn(CS4_BASE_ADDR),
		.length  = CS4_SIZE,
		.type    = MT_DEVICE,
	}
};

/*!
 * Set up static virtual mappings.
 */
void __init mx27ads_map_io(void)
{
	mxc_map_io();
	iotable_init(mx27ads_io_desc, ARRAY_SIZE(mx27ads_io_desc));
}

static void __init mx27ads_board_init(void)
{
	pr_info("AIPI VA base: 0x%x\n", IO_ADDRESS(AIPI_BASE_ADDR));

	mxc_clocks_init();
	mxc_gpio_init();
	mxc_expio_init();
        mxc_init_nor_mtd();
	mxc_init_nand_mtd();
	mxc_init_extuart();
	mxc_init_keypad();
	mxc_init_video_output();
	mxc_init_pmic_audio();

        spi_register_board_info(mxc_spi_board_info,
                                ARRAY_SIZE(mxc_spi_board_info));

	mxc_init_fb();
}

/*
 * The following uses standard kernel macros define in arch.h in order to
 * initialize __mach_desc_MX27ADS data structure.
 */
/* *INDENT-OFF* */
MACHINE_START(MX27ADS, "Freescale i.MX27ADS")
	/* maintainer: Freescale Semiconductor, Inc. */
	.phys_io        = AIPI_BASE_ADDR,
	.io_pg_offst    = ((AIPI_BASE_ADDR_VIRT) >> 18) & 0xfffc,
	.boot_params    = PHYS_OFFSET + 0x100,
	.map_io         = mx27ads_map_io,
	.init_irq       = mxc_init_irq,
	.init_machine   = mx27ads_board_init,
	.timer          = &mxc_timer,
MACHINE_END
