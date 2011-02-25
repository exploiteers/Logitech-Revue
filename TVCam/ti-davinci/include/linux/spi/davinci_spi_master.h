/*
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

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/device.h>
#include <asm-arm/arch/hardware.h>
#include <linux/spi/spi.h>
#include <linux/spi/davinci_spi.h>
#include <linux/spi/spi_bitbang.h>

#include <asm/arch-davinci/edma.h>

/*Board specific declarations*/
#define SPI_BUS_FREQ	(4000000)
#define CS_DEFAULT	0xFF
#define SCS0_SELECT	0x01
#define SCS1_SELECT	0x02
#define SCS2_SELECT	0x04
#define SCS3_SELECT	0x08
#define SCS4_SELECT	0x10
#define SCS5_SELECT	0x20
#define SCS6_SELECT	0x40
#define SCS7_SELECT	0x80

/* Standard values for DAVINCI */
#define DAVINCI_SPI_MAX_CHIPSELECT 7

/* #define SPI_INTERRUPT_MODE 1 */
#define SPI_SPIFMT_PHASE_MASK		(0x00010000u)
#define SPI_SPIFMT_PHASE_SHIFT		(0x00000010u)
#define SPI_SPIFMT_PHASE_RESETVAL	(0x00000000u)

#define SPI_SPIFMT_POLARITY_MASK	(0x00020000u)
#define SPI_SPIFMT_POLARITY_SHIFT	(0x00000011u)
#define SPI_SPIFMT_POLARITY_RESETVAL	(0x00000000u)

#define SPI_SPIFMT_DISTIMER_MASK	(0x00040000u)
#define SPI_SPIFMT_DISTIMER_SHIFT	(0x00000012u)
#define SPI_SPIFMT_DISTIMER_RESETVAL	(0x00000000u)

#define SPI_SPIFMT_SHIFTDIR_MASK	(0x00100000u)
#define SPI_SPIFMT_SHIFTDIR_SHIFT	(0x00000014u)
#define SPI_SPIFMT_SHIFTDIR_RESETVAL	(0x00000000u)

#define SPI_SPIFMT_WAITENA_MASK		(0x00200000u)
#define SPI_SPIFMT_WAITENA_SHIFT	(0x00000015u)
#define SPI_SPIFMT_WAITENA_RESETVAL	(0x00000000u)

#define SPI_SPIFMT_PARITYENA_MASK	(0x00400000u)
#define SPI_SPIFMT_PARITYENA_SHIFT	(0x00000016u)
#define SPI_SPIFMT_PARITYENA_RESETVAL	(0x00000000u)

#define SPI_SPIFMT_ODD_PARITY_MASK	(0x00800000u)
#define SPI_SPIFMT_ODD_PARITY_SHIFT	(0x00000017u)
#define SPI_SPIFMT_ODD_PARITY_RESETVAL	(0x00000000u)

#define SPI_SPIFMT_WDELAY_MASK		(0x3f000000u)
#define SPI_SPIFMT_WDELAY_SHIFT		(0x00000018u)
#define SPI_SPIFMT_WDELAY_RESETVAL	(0x00000000u)

#define SPI_SPIFMT_CHARLEN_MASK		(0x0000001Fu)
#define SPI_SPIFMT_CHARLEN_SHIFT	(0x00000000u)
#define SPI_SPIFMT_CHARLEN_RESETVAL	(0x00000000u)

/* SPIGCR1 */

#define SPI_SPIGCR1_SPIENA_MASK      (0x01000000u)
#define SPI_SPIGCR1_SPIENA_SHIFT     (0x00000018u)
#define SPI_SPIGCR1_SPIENA_RESETVAL  (0x00000000u)

#define SPI_INTLVL_1				 (0x000001FFu)
#define SPI_INTLVL_0				 (0x00000000u)

/* SPIPC0 */

#define SPI_SPIPC0_DIFUN_MASK        (0x00000800u)
#define SPI_SPIPC0_DIFUN_SHIFT       (0x0000000Bu)
#define SPI_SPIPC0_DIFUN_RESETVAL    (0x00000000u)

/*----DIFUN Tokens----*/
#define SPI_SPIPC0_DIFUN_DI          (0x00000001u)

#define SPI_SPIPC0_DOFUN_MASK        (0x00000400u)
#define SPI_SPIPC0_DOFUN_SHIFT       (0x0000000Au)
#define SPI_SPIPC0_DOFUN_RESETVAL    (0x00000000u)

/*----DOFUN Tokens----*/
#define SPI_SPIPC0_DOFUN_DO          (0x00000001u)

#define SPI_SPIPC0_CLKFUN_MASK       (0x00000200u)
#define SPI_SPIPC0_CLKFUN_SHIFT      (0x00000009u)
#define SPI_SPIPC0_CLKFUN_RESETVAL   (0x00000000u)

/*----CLKFUN Tokens----*/
#define SPI_SPIPC0_CLKFUN_CLK        (0x00000001u)

#define SPI_SPIPC0_EN1FUN_MASK       (0x00000002u)
#define SPI_SPIPC0_EN1FUN_SHIFT      (0x00000001u)
#define SPI_SPIPC0_EN1FUN_RESETVAL   (0x00000000u)

/*----EN1FUN Tokens----*/
#define SPI_SPIPC0_EN1FUN_EN1        (0x00000001u)

#define SPI_SPIPC0_EN0FUN_MASK       (0x00000001u)
#define SPI_SPIPC0_EN0FUN_SHIFT      (0x00000000u)
#define SPI_SPIPC0_EN0FUN_RESETVAL   (0x00000000u)

/*----EN0FUN Tokens----*/
#define SPI_SPIPC0_EN0FUN_EN0        (0x00000001u)

#define SPI_SPIPC0_RESETVAL          (0x00000000u)
#define SPI_SPIPC0_SPIENA		 (0x00000001u)
#define SPI_SPIPC0_SPIENA_SHIFT	 (0x00000008u)

#define SPI_SPIINT_MASKALL		(0x0101035F)

/* SPIDAT1 */

#define SPI_SPIDAT1_CSHOLD_MASK      (0x10000000u)
#define SPI_SPIDAT1_CSHOLD_SHIFT     (0x0000001Cu)
#define SPI_SPIDAT1_CSHOLD_RESETVAL  (0x00000000u)

#define SPI_SPIDAT1_CSNR_MASK        (0x00030000u)
#define SPI_SPIDAT1_CSNR_SHIFT       (0x00000010u)
#define SPI_SPIDAT1_CSNR_RESETVAL    (0x00000000u)

#define SPI_SPIDAT1_DFSEL_MASK       (0x03000000u)
#define SPI_SPIDAT1_DFSEL_SHIFT      (0x00000018u)
#define SPI_SPIDAT1_DFSEL_RESETVAL   (0x00000000u)

#define SPI_SPIGCR1_CLKMOD_MASK      (0x00000002u)
#define SPI_SPIGCR1_CLKMOD_SHIFT     (0x00000001u)
#define SPI_SPIGCR1_CLKMOD_RESETVAL  (0x00000000u)

#define SPI_SPIGCR1_MASTER_MASK      (0x00000001u)
#define SPI_SPIGCR1_MASTER_SHIFT     (0x00000000u)
#define SPI_SPIGCR1_MASTER_RESETVAL  (0x00000000u)

#define SPI_SPIGCR1_LOOPBACK_MASK    (0x00010000u)
#define SPI_SPIGCR1_LOOPBACK_SHIFT   (0x00000010u)
#define SPI_SPIGCR1_LOOPBACK_RESETVAL (0x00000000u)

/* SPIBUF */
#define SPI_SPIBUF_TXFULL_MASK		(0x20000000u)
#define SPI_SPIBUF_RXEMPTY_MASK		(0x80000000u)


#define SPI_SPIFLG_DLEN_ERR_MASK	(0x00000001u)
#define SPI_SPIFLG_TIMEOUT_MASK		(0x00000002u)
#define SPI_SPIFLG_PARERR_MASK		(0x00000004u)
#define SPI_SPIFLG_DESYNC_MASK		(0x00000008u)
#define SPI_SPIFLG_BITERR_MASK		(0x00000010u)
#define SPI_SPIFLG_OVRRUN_MASK		(0x00000040u)
#define SPI_SPIFLG_RX_INTR_MASK		(0x00000100u)
#define SPI_SPIFLG_TX_INTR_MASK		(0x00000200u)
#define SPI_SPIFLG_BUF_INIT_ACTIVE_MASK	(0x01000000u)
#define SPI_SPIFLG_MASK			(SPI_SPIFLG_DLEN_ERR_MASK \
		| SPI_SPIFLG_TIMEOUT_MASK | SPI_SPIFLG_PARERR_MASK \
		| SPI_SPIFLG_DESYNC_MASK | SPI_SPIFLG_BITERR_MASK \
		| SPI_SPIFLG_OVRRUN_MASK | SPI_SPIFLG_RX_INTR_MASK \
		| SPI_SPIFLG_TX_INTR_MASK | SPI_SPIFLG_BUF_INIT_ACTIVE_MASK)

#define SPI_SPIINT_DLEN_ERR_INTR	(0x00000001u)
#define SPI_SPIINT_TIMEOUT_INTR		(0x00000002u)
#define SPI_SPIINT_PARERR_INTR		(0x00000004u)
#define SPI_SPIINT_DESYNC_INTR		(0x00000008u)
#define SPI_SPIINT_BITERR_INTR		(0x00000010u)
#define SPI_SPIINT_OVRRUN_INTR		(0x00000040u)
#define SPI_SPIINT_RX_INTR		(0x00000100u)
#define SPI_SPIINT_TX_INTR		(0x00000200u)
#define SPI_SPIINT_DMA_REQ_EN		(0x00010000u)
#define SPI_SPIINT_ENABLE_HIGHZ		(0x01000000u)

/**< Error return coded */
#define SPI_ERROR_BASE			(-30)
#define SPI_OVERRUN_ERR			(SPI_ERROR_BASE-1)
#define SPI_BIT_ERR			(SPI_ERROR_BASE-2)
#define SPI_DESYNC_ERR			(SPI_ERROR_BASE-3)
#define SPI_PARITY_ERR			(SPI_ERROR_BASE-4)
#define SPI_TIMEOUT_ERR			(SPI_ERROR_BASE-5)
#define SPI_TRANSMIT_FULL_ERR		(SPI_ERROR_BASE-6)
#define SPI_POWERDOWN			(SPI_ERROR_BASE-7)
#define SPI_DLEN_ERR			(SPI_ERROR_BASE-8)
#define SPI_TX_INTR_ERR			(SPI_ERROR_BASE-9)
#define SPI_BUF_INIT_ACTIVE_ERR		(SPI_ERROR_BASE-11)

#define SPI_BYTELENGTH 8u

/******************************************************************/

enum spi_pin_op_mode {
	SPI_OPMODE_3PIN,
	SPI_OPMODE_SPISCS_4PIN,
	SPI_OPMODE_SPIENA_4PIN,
	SPI_OPMODE_5PIN,
};

struct davinci_spi_config_t {
	/* SPIFMT */
	u32	wdelay;
	u32	odd_parity;
	u32	parity_enable;
	u32	wait_enable;
	u32	lsb_first;
	u32	timer_disable;
	u32	clk_high;
	u32	phase_in;
	/* SPIGCR1 */
	u32	clk_internal;
	u32	loop_back;
	/* SPIDAT1 */
	u32	cs_hold;
	/* SPIINTLVL1 */
	u32	intr_level;
	/* Others */
	enum spi_pin_op_mode	pin_op_modes;
	u32	poll_mode;
};

/* SPI Controller registers */

#define SPIGCR0		0x00
#define SPIGCR1		0x04
#define SPIINT		0x08
#define SPILVL		0x0c
#define SPIFLG		0x10
#define SPIPC0		0x14
#define SPIPC1		0x18
#define SPIPC2		0x1c
#define SPIPC3		0x20
#define SPIPC4		0x24
#define SPIPC5		0x28
#define SPIPC6		0x2c
#define SPIPC7		0x30
#define SPIPC8		0x34
#define SPIDAT0		0x38
#define SPIDAT1		0x3c
#define SPIBUF		0x40
#define SPIEMU		0x44
#define SPIDELAY	0x48
#define SPIDEF		0x4c
#define SPIFMT0		0x50
#define SPIFMT1		0x54
#define SPIFMT2		0x58
#define SPIFMT3		0x5c
#define TGINTVEC0	0x60
#define TGINTVEC1	0x64

struct davinci_spi_slave {
	u32	cmd_to_write;
	u32	clk_ctrl_to_write;
	u32	bytes_per_word;
	u8	active_cs;
};

#define SPI_BUFSIZ	(SMP_CACHE_BYTES + 1)

/* We have 2 DMA channels per CS, one for RX and one for TX */
struct davinci_spi_dma {
	int			dma_tx_channel;
	int			dma_rx_channel;
	int			dma_tx_sync_dev;
	int			dma_rx_sync_dev;
	enum dma_event_q	eventq;

	struct completion	dma_tx_completion;
	struct completion	dma_rx_completion;
};

/* SPI Controller driver's private data. */
struct davinci_spi {
	/* bitbang has to be first */
	struct spi_bitbang	bitbang;

	u8			version;
	resource_size_t		pbase;
	void __iomem		*base; /* virtual base */
	size_t			region_size;
	u32			irq;
	struct completion	done;

	const void		*tx;
	void			*rx;
	u8			*tmp_buf;
	int			count;
	struct davinci_spi_dma	*dma_channels;
	struct davinci_spi_platform_data *pdata;

	void			(*get_rx)(u32 rx_data, struct davinci_spi *);
	u32			(*get_tx)(struct davinci_spi *);

	struct davinci_spi_slave slave[DAVINCI_SPI_MAX_CHIPSELECT];
};
