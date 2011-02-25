/*
 * Author: MontaVista Software, Inc.
 *       <source@mvista.com>
 *
 * Based on the OMAP devices.c
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under the
 * terms of the GNU General Public License version 2. This program is
 * licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * Copyright 2006-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <asm/hardware.h>
#include <asm/arch/mxc_uart.h>
#include <linux/serial.h>
#include <asm/arch/gpio.h>
#include "serial.h"
#include <asm/arch/mxc_i2c.h>

/*!
 * @file mach-mx27/devices.c
 * @brief device configurations for mx27.
 */

static void mxc_nop_release(struct device *dev)
{
        /* Nothing */
}

#if defined(CONFIG_SERIAL_MXC) || defined(CONFIG_SERIAL_MXC_MODULE)

/*!
 * This is an array where each element holds information about a UART port,
 * like base address of the UART, interrupt numbers etc. This structure is
 * passed to the serial_core.c file. Based on which UART is used, the core file
 * passes back the appropriate port structure as an argument to the control
 * functions.
 */
static uart_mxc_port mxc_ports[] = {
	[0] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART1_BASE_ADDR),
			.mapbase = UART1_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART1_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 0,
			},
	       .ints_muxed = UART1_MUX_INTS,
	       .irqs = {UART1_INT2, UART1_INT3},
	       .mode = UART1_MODE,
	       .ir_mode = UART1_IR,
	       .enabled = UART1_ENABLED,
	       .hardware_flow = UART1_HW_FLOW,
	       .cts_threshold = UART1_UCR4_CTSTL,
	       .dma_enabled = UART1_DMA_ENABLE,
	       .dma_rxbuf_size = UART1_DMA_RXBUFSIZE,
	       .rx_threshold = UART1_UFCR_RXTL,
	       .tx_threshold = UART1_UFCR_TXTL,
	       .shared = UART1_SHARED_PERI,
	       .dma_tx_id = MXC_DMA_UART1_TX,
	       .dma_rx_id = MXC_DMA_UART1_RX,
	       .rxd_mux = MXC_UART_RXDMUX,
	       },
	[1] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART2_BASE_ADDR),
			.mapbase = UART2_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART2_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 1,
			},
	       .ints_muxed = UART2_MUX_INTS,
	       .irqs = {UART2_INT2, UART2_INT3},
	       .mode = UART2_MODE,
	       .ir_mode = UART2_IR,
	       .enabled = UART2_ENABLED,
	       .hardware_flow = UART2_HW_FLOW,
	       .cts_threshold = UART2_UCR4_CTSTL,
	       .dma_enabled = UART2_DMA_ENABLE,
	       .dma_rxbuf_size = UART2_DMA_RXBUFSIZE,
	       .rx_threshold = UART2_UFCR_RXTL,
	       .tx_threshold = UART2_UFCR_TXTL,
	       .shared = UART2_SHARED_PERI,
	       .dma_tx_id = MXC_DMA_UART2_TX,
	       .dma_rx_id = MXC_DMA_UART2_RX,
	       .rxd_mux = MXC_UART_RXDMUX,
	       },
	[2] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART3_BASE_ADDR),
			.mapbase = UART3_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART3_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 2,
			},
	       .ints_muxed = UART3_MUX_INTS,
	       .irqs = {UART3_INT2, UART3_INT3},
	       .mode = UART3_MODE,
	       .ir_mode = UART3_IR,
	       .enabled = UART3_ENABLED,
	       .hardware_flow = UART3_HW_FLOW,
	       .cts_threshold = UART3_UCR4_CTSTL,
	       .dma_enabled = UART3_DMA_ENABLE,
	       .dma_rxbuf_size = UART3_DMA_RXBUFSIZE,
	       .rx_threshold = UART3_UFCR_RXTL,
	       .tx_threshold = UART3_UFCR_TXTL,
	       .shared = UART3_SHARED_PERI,
	       .dma_tx_id = MXC_DMA_UART3_TX,
	       .dma_rx_id = MXC_DMA_UART3_RX,
	       .rxd_mux = MXC_UART_IR_RXDMUX,
	       },
	[3] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART4_BASE_ADDR),
			.mapbase = UART4_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART4_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 3,
			},
	       .ints_muxed = UART4_MUX_INTS,
	       .irqs = {UART4_INT2, UART4_INT3},
	       .mode = UART4_MODE,
	       .ir_mode = UART4_IR,
	       .enabled = UART4_ENABLED,
	       .hardware_flow = UART4_HW_FLOW,
	       .cts_threshold = UART4_UCR4_CTSTL,
	       .dma_enabled = UART4_DMA_ENABLE,
	       .dma_rxbuf_size = UART4_DMA_RXBUFSIZE,
	       .rx_threshold = UART4_UFCR_RXTL,
	       .tx_threshold = UART4_UFCR_TXTL,
	       .shared = UART4_SHARED_PERI,
	       .dma_tx_id = MXC_DMA_UART4_TX,
	       .dma_rx_id = MXC_DMA_UART4_RX,
	       .rxd_mux = MXC_UART_RXDMUX,
	       },
	[4] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART5_BASE_ADDR),
			.mapbase = UART5_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART5_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 4,
			},
	       .ints_muxed = UART5_MUX_INTS,
	       .irqs = {UART5_INT2, UART5_INT3},
	       .mode = UART5_MODE,
	       .ir_mode = UART5_IR,
	       .enabled = UART5_ENABLED,
	       .hardware_flow = UART5_HW_FLOW,
	       .cts_threshold = UART5_UCR4_CTSTL,
	       .dma_enabled = UART5_DMA_ENABLE,
	       .dma_rxbuf_size = UART5_DMA_RXBUFSIZE,
	       .rx_threshold = UART5_UFCR_RXTL,
	       .tx_threshold = UART5_UFCR_TXTL,
	       .shared = UART5_SHARED_PERI,
	       .dma_tx_id = MXC_DMA_UART5_TX,
	       .dma_rx_id = MXC_DMA_UART5_RX,
	       .rxd_mux = MXC_UART_RXDMUX,
	       },
	[5] = {
	       .port = {
			.membase = (void *)IO_ADDRESS(UART6_BASE_ADDR),
			.mapbase = UART6_BASE_ADDR,
			.iotype = SERIAL_IO_MEM,
			.irq = UART6_INT1,
			.fifosize = 32,
			.flags = ASYNC_BOOT_AUTOCONF,
			.line = 5,
			},
	       .ints_muxed = UART6_MUX_INTS,
	       .irqs = {UART6_INT2, UART6_INT3},
	       .mode = UART6_MODE,
	       .ir_mode = UART6_IR,
	       .enabled = UART6_ENABLED,
	       .hardware_flow = UART6_HW_FLOW,
	       .cts_threshold = UART6_UCR4_CTSTL,
	       .dma_enabled = UART6_DMA_ENABLE,
	       .dma_rxbuf_size = UART6_DMA_RXBUFSIZE,
	       .rx_threshold = UART6_UFCR_RXTL,
	       .tx_threshold = UART6_UFCR_TXTL,
	       .shared = UART6_SHARED_PERI,
	       .dma_tx_id = MXC_DMA_UART6_TX,
	       .dma_rx_id = MXC_DMA_UART6_RX,
	       .rxd_mux = MXC_UART_RXDMUX,
	       },
};

static struct platform_device mxc_uart_device1 = {
	.name = "mxcintuart",
	.id = 0,
	.dev = {
		.platform_data = &mxc_ports[0],
		},
};

static struct platform_device mxc_uart_device2 = {
	.name = "mxcintuart",
	.id = 1,
	.dev = {
		.platform_data = &mxc_ports[1],
		},
};

static struct platform_device mxc_uart_device3 = {
	.name = "mxcintuart",
	.id = 2,
	.dev = {
		.platform_data = &mxc_ports[2],
		},
};

static struct platform_device mxc_uart_device4 = {
	.name = "mxcintuart",
	.id = 3,
	.dev = {
		.platform_data = &mxc_ports[3],
		},
};
static struct platform_device mxc_uart_device5 = {
	.name = "mxcintuart",
	.id = 4,
	.dev = {
		.platform_data = &mxc_ports[4],
		},
};
static struct platform_device mxc_uart_device6 = {
	.name = "mxcintuart",
	.id = 5,
	.dev = {
		.platform_data = &mxc_ports[5],
		},
};

static int __init mxc_init_uart(void)
{
	/* Register all the MXC UART platform device structures */
	platform_device_register(&mxc_uart_device1);
	platform_device_register(&mxc_uart_device2);
#ifndef CONFIG_MXC_IRDA
	platform_device_register(&mxc_uart_device3);
#endif
	platform_device_register(&mxc_uart_device4);

	platform_device_register(&mxc_uart_device5);
	platform_device_register(&mxc_uart_device6);
	return 0;
}

#else
static int __init mxc_init_uart(void)
{
	return 0;
}
#endif

/* SPI controller and device data */
#if defined(CONFIG_SPI_MXC) || defined(CONFIG_SPI_MXC_MODULE)

#ifdef CONFIG_SPI_MXC_SELECT1
/*!
 * Resource definition for the CSPI1
 */
static struct resource mxcspi1_resources[] = {
        [0] = {
               .start = CSPI1_BASE_ADDR,
               .end = CSPI1_BASE_ADDR + SZ_4K - 1,
               .flags = IORESOURCE_MEM,
               },
        [1] = {
               .start = MXC_INT_CSPI1,
               .end = MXC_INT_CSPI1,
               .flags = IORESOURCE_IRQ,
               },
};

/*! Platform Data for MXC CSPI1 */
static struct mxc_spi_master mxcspi1_data = {
        .maxchipselect = 4,
        .spi_version = 0,
};

/*! Device Definition for MXC CSPI1 */
static struct platform_device mxcspi1_device = {
        .name = "mxc_spi",
        .id = 0,
        .dev = {
                .release = mxc_nop_release,
                .platform_data = &mxcspi1_data,
                },
        .num_resources = ARRAY_SIZE(mxcspi1_resources),
        .resource = mxcspi1_resources,
};

#endif                          /* CONFIG_SPI_MXC_SELECT1 */

#ifdef CONFIG_SPI_MXC_SELECT2
/*!
 * Resource definition for the CSPI2
 */
static struct resource mxcspi2_resources[] = {
        [0] = {
               .start = CSPI2_BASE_ADDR,
               .end = CSPI2_BASE_ADDR + SZ_4K - 1,
               .flags = IORESOURCE_MEM,
               },
        [1] = {
               .start = MXC_INT_CSPI2,
               .end = MXC_INT_CSPI2,
               .flags = IORESOURCE_IRQ,
               },
};

/*! Platform Data for MXC CSPI2 */
static struct mxc_spi_master mxcspi2_data = {
        .maxchipselect = 4,
        .spi_version = 0,
};

/*! Device Definition for MXC CSPI2 */
static struct platform_device mxcspi2_device = {
        .name = "mxc_spi",
        .id = 1,
        .dev = {
                .release = mxc_nop_release,
                .platform_data = &mxcspi2_data,
                },
        .num_resources = ARRAY_SIZE(mxcspi2_resources),
        .resource = mxcspi2_resources,
};
#endif                          /* CONFIG_SPI_MXC_SELECT2 */

#ifdef CONFIG_SPI_MXC_SELECT3
/*!
 * Resource definition for the CSPI3
 */
static struct resource mxcspi3_resources[] = {
        [0] = {
               .start = CSPI3_BASE_ADDR,
               .end = CSPI3_BASE_ADDR + SZ_4K - 1,
               .flags = IORESOURCE_MEM,
               },
        [1] = {
               .start = INT_CSPI3,
               .end = INT_CSPI3,
               .flags = IORESOURCE_IRQ,
               },
};

/*! Platform Data for MXC CSPI3 */
static struct mxc_spi_master mxcspi3_data = {
        .maxchipselect = 4,
        .spi_version = 0,
};

/*! Device Definition for MXC CSPI3 */
static struct platform_device mxcspi3_device = {
        .name = "mxc_spi",
        .id = 2,
        .dev = {
                .release = mxc_nop_release,
                .platform_data = &mxcspi3_data,
                },
        .num_resources = ARRAY_SIZE(mxcspi3_resources),
        .resource = mxcspi3_resources,
};
#endif                          /* CONFIG_SPI_MXC_SELECT3 */

static inline void mxc_init_spi(void)
{
#ifdef CONFIG_SPI_MXC_SELECT1
        if (platform_device_register(&mxcspi1_device) < 0)
                printk(KERN_ERR "Registering the SPI Controller_1\n");
#endif                          /* CONFIG_SPI_MXC_SELECT1 */
#ifdef CONFIG_SPI_MXC_SELECT2
        if (platform_device_register(&mxcspi2_device) < 0)
                printk(KERN_ERR "Registering the SPI Controller_2\n");
#endif                          /* CONFIG_SPI_MXC_SELECT2 */
#ifdef CONFIG_SPI_MXC_SELECT3
        if (platform_device_register(&mxcspi3_device) < 0)
                printk(KERN_ERR "Registering the SPI Controller_3\n");
#endif                          /* CONFIG_SPI_MXC_SELECT3 */
}
#else
static inline void mxc_init_spi(void)
{
}
#endif

#if defined(CONFIG_I2C_MXC) || defined(CONFIG_I2C_MXC_MODULE)

#ifdef CONFIG_I2C_MXC_SELECT1
/*!
 * Resource definition for the I2C1
 */
static struct resource mxci2c1_resources[] = {
	[0] = {
	       .start = I2C_BASE_ADDR,
	       .end = I2C_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_I2C,
	       .end = MXC_INT_I2C,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC I2C */
static struct mxc_i2c_platform_data mxci2c1_data = {
	.i2c_clk = 100000,
};
#endif

#ifdef CONFIG_I2C_MXC_SELECT2
/*!
 * Resource definition for the I2C2
 */
static struct resource mxci2c2_resources[] = {
	[0] = {
	       .start = I2C2_BASE_ADDR,
	       .end = I2C2_BASE_ADDR + SZ_4K - 1,
	       .flags = IORESOURCE_MEM,
	       },
	[1] = {
	       .start = MXC_INT_I2C2,
	       .end = MXC_INT_I2C2,
	       .flags = IORESOURCE_IRQ,
	       },
};

/*! Platform Data for MXC I2C */
static struct mxc_i2c_platform_data mxci2c2_data = {
	.i2c_clk = 100000,
};
#endif

/*! Device Definition for MXC I2C */
static struct platform_device mxci2c_devices[] = {
#ifdef CONFIG_I2C_MXC_SELECT1
	{
	 .name = "mxc_i2c",
	 .id = 0,
	 .dev = {
		 .platform_data = &mxci2c1_data,
		 },
	 .num_resources = ARRAY_SIZE(mxci2c1_resources),
	 .resource = mxci2c1_resources,},
#endif
#ifdef CONFIG_I2C_MXC_SELECT2
	{
	 .name = "mxc_i2c",
	 .id = 1,
	 .dev = {
		 .platform_data = &mxci2c2_data,
		 },
	 .num_resources = ARRAY_SIZE(mxci2c2_resources),
	 .resource = mxci2c2_resources,},
#endif
};

static inline void mxc_init_i2c(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mxci2c_devices); i++) {
		if (platform_device_register(&mxci2c_devices[i]) < 0)
			dev_err(&mxci2c_devices[i].dev,
				"Unable to register I2C device\n");
	}
}
#else
static inline void mxc_init_i2c(void)
{
}
#endif

#ifdef	CONFIG_MXC_VPU
/*! Platform Data for MXC VPU */
static struct platform_device mxcvpu_device = {
	.name = "mxc_vpu",
	.dev = {
		.release = mxc_nop_release,
		},
	.id = 0,
};

static inline void mxc_init_vpu(void)
{
	if (platform_device_register(&mxcvpu_device) < 0)
		printk(KERN_ERR "Error: Registering the VPU.\n");
}
#else
static inline void mxc_init_vpu(void)
{
}
#endif

struct mxc_gpio_port mxc_gpio_ports[GPIO_PORT_NUM] = {
	{
	 .num = 0,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR),
	 .irq = MXC_INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_INT_BASE,
	 },
	{
	 .num = 1,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x100,
	 .irq = MXC_INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_INT_BASE + GPIO_NUM_PIN,
	 },
	{
	 .num = 2,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x200,
	 .irq = MXC_INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_INT_BASE + GPIO_NUM_PIN * 2,
	 },
	{
	 .num = 3,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x300,
	 .irq = MXC_INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_INT_BASE + GPIO_NUM_PIN * 3,
	 },
	{
	 .num = 4,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x400,
	 .irq = MXC_INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_INT_BASE + GPIO_NUM_PIN * 4,
	 },
	{
	 .num = 5,
	 .base = IO_ADDRESS(GPIO_BASE_ADDR) + 0x500,
	 .irq = MXC_INT_GPIO,
	 .virtual_irq_start = MXC_GPIO_INT_BASE + GPIO_NUM_PIN * 5,
	 },
};

static int __init mxc_init_devices(void)
{
        mxc_init_spi();
	mxc_init_uart();
	mxc_init_i2c();
	mxc_init_vpu();
	return 0;
}

arch_initcall(mxc_init_devices);
