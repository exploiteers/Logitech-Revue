/*
 *  linux/drivers/serial/cpm_uart.c
 *
 *  Driver for CPM (SCC/SMC) serial ports; CPM1 definitions
 *
 *  Maintainer: Kumar Gala (galak@kernel.crashing.org) (CPM2)
 *              Pantelis Antoniou (panto@intracom.gr) (CPM1)
 *
 *  Copyright (C) 2004 Freescale Semiconductor, Inc.
 *            (C) 2004 Intracom, S.A.
 *            (C) 2006 MontaVista Software, Inc.
 * 		Vitaly Bordug <vbordug@ru.mvista.com>
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
 *
 */

#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/serial.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/device.h>
#include <linux/bootmem.h>
#include <linux/dma-mapping.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/fs_pd.h>
#include <asm/prom.h>

#include <linux/serial_core.h>
#include <linux/kernel.h>

#ifdef CONFIG_MPC885ADS
#include <platforms/8xx/mpc885ads.h>
#endif

#ifdef CONFIG_PPC_EP88XC
#include <platforms/8xx/ep88xc.h>
#endif

#ifdef CONFIG_MPC86XADS
#include <platforms/8xx/mpc86xads.h>
#endif

#include "cpm_uart.h"

extern void m8xx_cpm_dpinit(void);

/**************************************************************/

void cpm_line_cr_cmd(int line, int cmd)
{
	ushort val;
	volatile cpm8xx_t *cp = (cpm8xx_t *)immr_map(im_cpm);

	switch (line) {
	case UART_SMC1:
		val = mk_cr_cmd(CPM_CR_CH_SMC1, cmd) | CPM_CR_FLG;
		break;
	case UART_SMC2:
		val = mk_cr_cmd(CPM_CR_CH_SMC2, cmd) | CPM_CR_FLG;
		break;
	case UART_SCC1:
		val = mk_cr_cmd(CPM_CR_CH_SCC1, cmd) | CPM_CR_FLG;
		break;
	case UART_SCC2:
		val = mk_cr_cmd(CPM_CR_CH_SCC2, cmd) | CPM_CR_FLG;
		break;
	case UART_SCC3:
		val = mk_cr_cmd(CPM_CR_CH_SCC3, cmd) | CPM_CR_FLG;
		break;
	case UART_SCC4:
		val = mk_cr_cmd(CPM_CR_CH_SCC4, cmd) | CPM_CR_FLG;
		break;
	default:
		return;

	}
	cp->cp_cpcr = val;
	while (cp->cp_cpcr & CPM_CR_FLG) ;
}

void smc1_lineif(struct uart_cpm_port *pinfo)
{
	/* XXX SMC1: insert port configuration here */
	unsigned *bcsr_io;
	cpm8xx_t *cp;
	pinfo->brg = 1;

#if defined (CONFIG_MPC885ADS) || defined (CONFIG_MPC86XADS)
#if defined(CONFIG_MPC885ADS)
	cp = (cpm8xx_t *)immr_map(im_cpm);
	setbits32(&cp->cp_pepar, 0x000000c0);
	clrbits32(&cp->cp_pedir, 0x000000c0);
	clrbits32(&cp->cp_peso, 0x00000040);
	setbits32(&cp->cp_peso, 0x00000080);
	immr_unmap(cp);
#endif

	bcsr_io = ioremap(BCSR1, sizeof(unsigned long));

	if (bcsr_io == NULL) {
		printk(KERN_CRIT "Could not remap BCSR\n");
		return;
	}
	out_be32(bcsr_io, in_be32(bcsr_io) & ~BCSR1_RS232EN_1);
	iounmap(bcsr_io);
#endif
}

void smc2_lineif(struct uart_cpm_port *pinfo)
{
	/* XXX SMC2: insert port configuration here */
	unsigned *bcsr_io;
	cpm8xx_t *cp;
	pinfo->brg = 2;

#if defined (CONFIG_MPC885ADS) || defined (CONFIG_MPC86XADS)
#if defined(CONFIG_MPC885ADS)
	cp = (cpm8xx_t *)immr_map(im_cpm);
	setbits32(&cp->cp_pepar, 0x00000c00);
	clrbits32(&cp->cp_pedir, 0x00000c00);
	clrbits32(&cp->cp_peso, 0x00000400);
	setbits32(&cp->cp_peso, 0x00000800);
	immr_unmap(cp);
#endif

	bcsr_io = ioremap(BCSR1, sizeof(unsigned long));

	if (bcsr_io == NULL) {
		printk(KERN_CRIT "Could not remap BCSR\n");
		return;
	}
        out_be32(bcsr_io, in_be32(bcsr_io) & ~BCSR1_RS232EN_2);
	iounmap(bcsr_io);
#endif
}

void scc1_lineif(struct uart_cpm_port *pinfo)
{
	pinfo->brg = 1;
}

void scc2_lineif(struct uart_cpm_port *pinfo)
{
	pinfo->brg = 2;
}

void scc3_lineif(struct uart_cpm_port *pinfo)
{
	/* XXX SCC3: insert port configuration here */
	pinfo->brg = 3;
}

void scc4_lineif(struct uart_cpm_port *pinfo)
{
	/* XXX SCC4: insert port configuration here */
	pinfo->brg = 4;
}

/*
 * Allocate DP-Ram and memory buffers. We need to allocate a transmit and
 * receive buffer descriptors from dual port ram, and a character
 * buffer area from host mem. If we are allocating for the console we need
 * to do it from bootmem
 */
int cpm_uart_allocbuf(struct uart_cpm_port *pinfo, unsigned int is_con)
{
	int dpmemsz, memsz;
	u8 *dp_mem;
	unsigned long dp_offset;
	u8 *mem_addr;
	dma_addr_t dma_addr = 0;

	pr_debug("CPM uart[%d]:allocbuf\n", pinfo->port.line);

	dpmemsz = sizeof(cbd_t) * (pinfo->rx_nrfifos + pinfo->tx_nrfifos);
	dp_offset = cpm_dpalloc(dpmemsz, 8);
	if (IS_ERR_VALUE(dp_offset)) {
		printk(KERN_ERR
		       "cpm_uart_cpm1.c: could not allocate buffer descriptors\n");
		return -ENOMEM;
	}
	dp_mem = cpm_dpram_addr(dp_offset);

	memsz = L1_CACHE_ALIGN(pinfo->rx_nrfifos * pinfo->rx_fifosize) +
	    L1_CACHE_ALIGN(pinfo->tx_nrfifos * pinfo->tx_fifosize);
	if (is_con) {
		/* was hostalloc but changed cause it blows away the */
		/* large tlb mapping when pinning the kernel area    */
		mem_addr = (u8 *) cpm_dpram_addr(cpm_dpalloc(memsz, 8));
		dma_addr = (u32)cpm_dpram_phys(mem_addr);
	} else
		mem_addr = dma_alloc_coherent(NULL, memsz, &dma_addr,
					      GFP_KERNEL);

	if (mem_addr == NULL) {
		cpm_dpfree(dp_offset);
		printk(KERN_ERR
		       "cpm_uart_cpm1.c: could not allocate coherent memory\n");
		return -ENOMEM;
	}

	pinfo->dp_addr = dp_offset;
	pinfo->mem_addr = mem_addr;             /*  virtual address*/
	pinfo->dma_addr = dma_addr;             /*  physical address*/
	pinfo->mem_size = memsz;

	pinfo->rx_buf = mem_addr;
	pinfo->tx_buf = pinfo->rx_buf + L1_CACHE_ALIGN(pinfo->rx_nrfifos
						       * pinfo->rx_fifosize);

	pinfo->rx_bd_base = (volatile cbd_t *)dp_mem;
	pinfo->tx_bd_base = pinfo->rx_bd_base + pinfo->rx_nrfifos;

	return 0;
}

void cpm_uart_freebuf(struct uart_cpm_port *pinfo)
{
	dma_free_coherent(NULL, L1_CACHE_ALIGN(pinfo->rx_nrfifos *
					       pinfo->rx_fifosize) +
			  L1_CACHE_ALIGN(pinfo->tx_nrfifos *
					 pinfo->tx_fifosize), pinfo->mem_addr,
			  pinfo->dma_addr);

	cpm_dpfree(pinfo->dp_addr);
}

/* Setup any dynamic params in the uart desc */
int __init cpm_uart_init_portdesc(void)
{
	cpm8xx_t *cpmp = (cpm8xx_t *)immr_map(im_cpm);

	pr_debug("CPM uart[-]:init portdesc\n");

	/* Check if we have called this yet. This may happen if early kgdb
	breakpoint is on */
	if(cpm_uart_nr)
		return 0;
	cpm_uart_nr = 0;
#ifdef CONFIG_SERIAL_CPM_SMC1
	cpm_uart_ports[UART_SMC1].smcp = &cpmp->cp_smc[0];
/*
 *  Is SMC1 being relocated?
 */
# ifdef CONFIG_I2C_SPI_SMC1_UCODE_PATCH
	cpm_uart_ports[UART_SMC1].smcup =
	    (smc_uart_t *) & cpmp->cp_dparam[0x3C0];
# else
	cpm_uart_ports[UART_SMC1].smcup =
	    (smc_uart_t *) & cpmp->cp_dparam[PROFF_SMC1];
# endif
	cpm_uart_ports[UART_SMC1].port.mapbase =
	    (unsigned long)&cpmp->cp_smc[0];
	cpm_uart_ports[UART_SMC1].smcp->smc_smcm |= (SMCM_RX | SMCM_TX);
	cpm_uart_ports[UART_SMC1].smcp->smc_smcmr &= ~(SMCMR_REN | SMCMR_TEN);
	cpm_uart_ports[UART_SMC1].port.uartclk = uart_clock();
	cpm_uart_port_map[cpm_uart_nr++] = UART_SMC1;
#endif

#ifdef CONFIG_SERIAL_CPM_SMC2
	cpm_uart_ports[UART_SMC2].smcp = &cpmp->cp_smc[1];
	cpm_uart_ports[UART_SMC2].smcup =
	    (smc_uart_t *) & cpmp->cp_dparam[PROFF_SMC2];
	cpm_uart_ports[UART_SMC2].port.mapbase =
	    (unsigned long)&cpmp->cp_smc[1];
	cpm_uart_ports[UART_SMC2].smcp->smc_smcm |= (SMCM_RX | SMCM_TX);
	cpm_uart_ports[UART_SMC2].smcp->smc_smcmr &= ~(SMCMR_REN | SMCMR_TEN);
	cpm_uart_ports[UART_SMC2].port.uartclk = uart_clock();
	cpm_uart_port_map[cpm_uart_nr++] = UART_SMC2;
#endif

#ifdef CONFIG_SERIAL_CPM_SCC1
	cpm_uart_ports[UART_SCC1].sccp = &cpmp->cp_scc[0];
	cpm_uart_ports[UART_SCC1].sccup =
	    (scc_uart_t *) & cpmp->cp_dparam[PROFF_SCC1];
	cpm_uart_ports[UART_SCC1].port.mapbase =
	    (unsigned long)&cpmp->cp_scc[0];
	cpm_uart_ports[UART_SCC1].sccp->scc_sccm &=
	    ~(UART_SCCM_TX | UART_SCCM_RX);
	cpm_uart_ports[UART_SCC1].sccp->scc_gsmrl &=
	    ~(SCC_GSMRL_ENR | SCC_GSMRL_ENT);
	cpm_uart_ports[UART_SCC1].port.uartclk = uart_clock();
	cpm_uart_port_map[cpm_uart_nr++] = UART_SCC1;
#endif

#ifdef CONFIG_SERIAL_CPM_SCC2
	cpm_uart_ports[UART_SCC2].sccp = &cpmp->cp_scc[1];
	cpm_uart_ports[UART_SCC2].sccup =
	    (scc_uart_t *) & cpmp->cp_dparam[PROFF_SCC2];
	cpm_uart_ports[UART_SCC2].port.mapbase =
	    (unsigned long)&cpmp->cp_scc[1];
	cpm_uart_ports[UART_SCC2].sccp->scc_sccm &=
	    ~(UART_SCCM_TX | UART_SCCM_RX);
	cpm_uart_ports[UART_SCC2].sccp->scc_gsmrl &=
	    ~(SCC_GSMRL_ENR | SCC_GSMRL_ENT);
	cpm_uart_ports[UART_SCC2].port.uartclk = uart_clock();
	cpm_uart_port_map[cpm_uart_nr++] = UART_SCC2;
#endif

#ifdef CONFIG_SERIAL_CPM_SCC3
	cpm_uart_ports[UART_SCC3].sccp = &cpmp->cp_scc[2];
	cpm_uart_ports[UART_SCC3].sccup =
	    (scc_uart_t *) & cpmp->cp_dparam[PROFF_SCC3];
	cpm_uart_ports[UART_SCC3].port.mapbase =
	    (unsigned long)&cpmp->cp_scc[2];
	cpm_uart_ports[UART_SCC3].sccp->scc_sccm &=
	    ~(UART_SCCM_TX | UART_SCCM_RX);
	cpm_uart_ports[UART_SCC3].sccp->scc_gsmrl &=
	    ~(SCC_GSMRL_ENR | SCC_GSMRL_ENT);
	cpm_uart_ports[UART_SCC3].port.uartclk = uart_clock();
	cpm_uart_port_map[cpm_uart_nr++] = UART_SCC3;
#endif

#ifdef CONFIG_SERIAL_CPM_SCC4
	cpm_uart_ports[UART_SCC4].sccp = &cpmp->cp_scc[3];
	cpm_uart_ports[UART_SCC4].sccup =
	    (scc_uart_t *) & cpmp->cp_dparam[PROFF_SCC4];
	cpm_uart_ports[UART_SCC4].port.mapbase =
	    (unsigned long)&cpmp->cp_scc[3];
	cpm_uart_ports[UART_SCC4].sccp->scc_sccm &=
	    ~(UART_SCCM_TX | UART_SCCM_RX);
	cpm_uart_ports[UART_SCC4].sccp->scc_gsmrl &=
	    ~(SCC_GSMRL_ENR | SCC_GSMRL_ENT);
	cpm_uart_ports[UART_SCC4].port.uartclk = uart_clock();
	cpm_uart_port_map[cpm_uart_nr++] = UART_SCC4;
#endif
	return 0;
}

static char tmp_sw_page[0x1800];
#ifdef CONFIG_PPC_8xx
extern unsigned long __initial_memory_limit;
#endif

void kgdb_params_early_init(void)
{
	mtspr(SPRN_M_TWB, virt_to_phys(tmp_sw_page));
#ifdef CONFIG_PPC_8xx
	__initial_memory_limit = 0x800000;
#endif
	get_from_flat_dt("cpu", "clock-frequency", &ppc_proc_freq);
	get_from_flat_dt("soc", "reg", &immrbase);
	get_from_flat_dt("cpm", "brg-frequency", &brgfreq);

#ifdef CONFIG_PPC_EP88XC
	cpm_reset();
	init_ioports();
#ifdef CONFIG_KGDB_CPM_UART_SMC1
	cpm1_clk_setup(CPM_CLK_SMC1, CPM_BRG1, CPM_CLK_RTX);
#endif
#ifdef CONFIG_KGDB_CPM_UART_SCC2
	cpm1_clk_setup(CPM_CLK_SCC2, CPM_BRG2, CPM_CLK_TX);
	cpm1_clk_setup(CPM_CLK_SCC2, CPM_BRG2, CPM_CLK_RX);
#endif
#else
	/* early dpmem init */
	m8xx_cpm_dpinit();
#endif
}

