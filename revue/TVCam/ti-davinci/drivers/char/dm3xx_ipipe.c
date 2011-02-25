/*
 * Copyright (C) 2008-2009 Texas Instruments Inc
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
 * ipipe module to hold common functionality across DM355 and DM365 */
#include <linux/kernel.h>
#include <linux/io.h>
#include <asm/arch/dm3xx_ipipe.h>

inline u32 regr_if(u32 offset)
{
	if (cpu_is_davinci_dm355())
		return davinci_readl(DM355_IPIPEIF_BASE_ADDR + offset);
	else
		return davinci_readl(DM365_IPIPEIF_BASE_ADDR + offset);
}

inline u32 regw_if(u32 val, u32 offset)
{
	if (cpu_is_davinci_dm355())
		davinci_writel(val, DM355_IPIPEIF_BASE_ADDR + offset);
	else
		davinci_writel(val, DM365_IPIPEIF_BASE_ADDR + offset);
	return val;
}

inline u32 regr_vpss(u32 offset)
{
	return davinci_readl(DM355_VPSSBL_REG_BASE + offset);
}

inline u32 regw_vpss(u32 val, u32 offset)
{
	davinci_writel(val, DM355_VPSSBL_REG_BASE + offset);
	return val;
}

void ipipeif_set_enable(char en, unsigned int mode)
{
	regw_if(1, IPIPEIF_ENABLE);
}

u32 ipipeif_get_enable(void)
{
	return regr_if(IPIPEIF_ENABLE);
}

int ipipeif_set_address(struct ipipeif *params, unsigned int address)
{
	unsigned int utemp, utemp1;
	if (params->source != 0) {
		utemp = ((params->adofs >> 5) & IPIPEIF_ADOFS_LSB_MASK);
		regw_if(utemp, IPIPEIF_ADOFS);

		/* lower sixteen bit */
		utemp = ((address >> IPIPEIF_ADDRL_SHIFT) & IPIPEIF_ADDRL_MASK);
		regw_if(utemp, IPIPEIF_ADDRL);

		/* upper next seven bit */
		utemp1 =
		    ((address >> IPIPEIF_ADDRU_SHIFT) & IPIPEIF_ADDRU_MASK);
		regw_if(utemp1, IPIPEIF_ADDRU);
	} else
		return -1;
	return 0;
}

static void ipipeif_config_dpc(struct ipipeif_dpc *dpc)
{
	u32 utemp = 0;
	if (dpc->en) {
		utemp = ((dpc->en & 1) << IPIPEIF_DPC2_EN_SHIFT);
		utemp |= (dpc->thr & IPIPEIF_DPC2_THR_MASK);
	}
	regw_if(utemp, IPIPEIF_DPC2);
}

/* This function sets up IPIPEIF and is called from
 * ipipe_hw_setup()
 */
int ipipeif_hw_setup(struct ipipeif *params)
{
	unsigned int utemp = 0, utemp1 = 0x7;
	enum ccdc_hw_if_type isif_port_if;

	if (NULL == params)
		return -1;
	/* Enable clock to IPIPEIF and IPIPE */
	if (cpu_is_davinci_dm365())
		vpss_enable_clock(VPSS_IPIPEIF_CLOCK, 1);
	/* Combine all the fields to make CFG1 register of IPIPEIF */
	utemp = params->mode << ONESHOT_SHIFT;
	utemp |= params->source << INPSRC_SHIFT;
	utemp |= params->clock_select << CLKSEL_SHIFT;
	utemp |= params->avg_filter << AVGFILT_SHIFT;
	utemp |= params->decimation << DECIM_SHIFT;

	if (cpu_is_davinci_dm355()) {
		utemp |= params->var.if_base.ialaw << IALAW_SHIFT;
		utemp |= params->var.if_base.pack_mode << PACK8IN_SHIFT;
		utemp |= params->var.if_base.clk_div << CLKDIV_SHIFT;
		utemp |= params->var.if_base.data_shift << DATASFT_SHIFT;
	} else {
		/* DM365 IPIPE 5.1 */
		utemp |= params->var.if_5_1.pack_mode << PACK8IN_SHIFT;
		utemp |= params->var.if_5_1.source1 << INPSRC1_SHIFT;
		if (params->source != SDRAM_YUV)
			utemp |= params->var.if_5_1.data_shift << DATASFT_SHIFT;
		else
			utemp &= (~(utemp1 << DATASFT_SHIFT));
	}
	regw_if(utemp, IPIPEIF_GFG1);

	switch (params->source) {
	case CCDC:
		{
			regw_if(params->gain, IPIPEIF_GAIN);
			break;
		}
	case SDRAM_RAW:
	case CCDC_DARKFM:
		{
			regw_if(params->gain, IPIPEIF_GAIN);
			/* fall through */
		}
	case SDRAM_YUV:
		{
			utemp |= params->var.if_5_1.data_shift << DATASFT_SHIFT;
			regw_if(params->glob_hor_size, IPIPEIF_PPLN);
			regw_if(params->glob_ver_size, IPIPEIF_LPFR);
			regw_if(params->hnum, IPIPEIF_HNUM);
			regw_if(params->vnum, IPIPEIF_VNUM);
			if (cpu_is_davinci_dm355()) {
				utemp = regr_vpss(DM355_VPSSBL_PCR);
				/* IPIPEIF read master */
				RESETBIT(utemp, 4);
				RESETBIT(utemp, 5);
				regw_vpss(utemp, DM355_VPSSBL_PCR);
			}
			break;
		}
	default:
		/* Do nothing */
		;
	}

	/*check if decimation is enable or not */
	if (params->decimation)
		regw_if(params->rsz, IPIPEIF_RSZ);
	if (cpu_is_davinci_dm365()) {
		/* Setup sync alignment and initial rsz position */
		utemp = params->var.if_5_1.align_sync & 1;
		utemp <<= IPIPEIF_INIRSZ_ALNSYNC_SHIFT;
		utemp |= (params->var.if_5_1.rsz_start & IPIPEIF_INIRSZ_MASK);
		regw_if(utemp, IPIPEIF_INIRSZ);

		/* Enable DPCM decompression */
		switch (params->source) {
		case SDRAM_RAW:
			{
				utemp = 0;
				if (params->var.if_5_1.dpcm.en) {
					utemp = params->var.if_5_1.dpcm.en & 1;
					utemp |= (params->var.if_5_1.dpcm.type
						  & 1)
					    << IPIPEIF_DPCM_BITS_SHIFT;
					utemp |=
					    (params->var.if_5_1.dpcm.pred & 1)
					    << IPIPEIF_DPCM_PRED_SHIFT;
				}
				regw_if(utemp, IPIPEIF_DPCM);

				/* set DPC */
				ipipeif_config_dpc(&params->var.if_5_1.dpc);

				regw_if(params->var.if_5_1.clip, IPIPEIF_OCLIP);
				/* fall through for SDRAM YUV mode */
			}
		case SDRAM_YUV:
			{
				/* Set clock divider */
				if (params->clock_select == SDRAM_CLK) {
					utemp |=
					    ((params->var.if_5_1.clk_div.m - 1)
					     << IPIPEIF_CLKDIV_M_SHIFT);
					utemp |=
					    (params->var.if_5_1.clk_div.n - 1);
					regw_if(utemp, IPIPEIF_CLKDIV);
				}
				break;
			}
		case CCDC:
		case CCDC_DARKFM:
			{
				/* set DPC */
				ipipeif_config_dpc(&params->var.if_5_1.dpc);

				/* Set DF gain & threshold control */
				utemp = 0;
				if (params->var.if_5_1.df_gain_en) {
					utemp = (params->var.if_5_1.df_gain_thr
						 & IPIPEIF_DF_GAIN_THR_MASK);
					regw_if(utemp, IPIPEIF_DFSGTH);
					utemp = ((params->var.if_5_1.df_gain_en
						  & 1)
						 << IPIPEIF_DF_GAIN_EN_SHIFT);
					utemp |= (params->var.if_5_1.df_gain
						  & IPIPEIF_DF_GAIN_MASK);
				}
				regw_if(utemp, IPIPEIF_DFSGVL);

				isif_port_if =
				    params->var.if_5_1.isif_port.if_type;
				/* configure CFG2 */
				utemp =
				    params->var.if_5_1.isif_port.hdpol
				    << IPIPEIF_CFG2_HDPOL_SHIFT;
				utemp |=
				    params->var.if_5_1.isif_port.vdpol
				    << IPIPEIF_CFG2_VDPOL_SHIFT;
				switch (isif_port_if) {
				case CCDC_YCBCR_SYNC_16:
				case CCDC_BT1120:
					{
						utemp |=
						    (0 <<
						     IPIPEIF_CFG2_YUV8_SHIFT);
						utemp |=
						    (1 <<
						     IPIPEIF_CFG2_YUV16_SHIFT);
						break;
					}
				case CCDC_BT656:
				case CCDC_YCBCR_SYNC_8:
				case CCDC_BT656_10BIT:
					{
						utemp |=
						    (1 <<
						     IPIPEIF_CFG2_YUV8_SHIFT);
						utemp |=
						    (1 <<
						     IPIPEIF_CFG2_YUV16_SHIFT);
						utemp |=
						    ((params->var.if_5_1.
						      pix_order)
						     <<
						     IPIPEIF_CFG2_YUV8P_SHIFT);
						break;
					}
				default:
					{
						/* Bayer */
						regw_if(params->var.if_5_1.clip,
							IPIPEIF_OCLIP);
						utemp |=
						    (0 <<
						     IPIPEIF_CFG2_YUV16_SHIFT);
					}
				}
				regw_if(utemp, IPIPEIF_CFG2);
				break;
			}
		default:
			/* do nothing */
			;
		}
	}
	return 0;
}

#ifdef CONFIG_IMP_DEBUG
void ipipeif_dump_register(void)
{
	u32 utemp;
	printk(KERN_NOTICE "IPIPEIF Registers\n");
	printk(KERN_NOTICE
	       "**************************************************\n");
	utemp = regr_if(IPIPEIF_ENABLE);
	printk(KERN_NOTICE "IPIPEIF ENABLE = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_GFG1);
	printk(KERN_NOTICE "IPIPEIF CFG = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_PPLN);
	printk(KERN_NOTICE "IPIPEIF PPLN = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_LPFR);
	printk(KERN_NOTICE "IPIPEIF LPFR = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_HNUM);
	printk(KERN_NOTICE "IPIPEIF HNUM = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_VNUM);
	printk(KERN_NOTICE "IPIPEIF VNUM = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_ADDRU);
	printk(KERN_NOTICE "IPIPEIF ADDRU = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_ADDRL);
	printk(KERN_NOTICE "IPIPEIF ADDRL = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_ADOFS);
	printk(KERN_NOTICE "IPIPEIF ADOFS = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_RSZ);
	printk(KERN_NOTICE "IPIPEIF RSZ = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_GAIN);
	printk(KERN_NOTICE "IPIPEIF GAIN = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_DPCM);
	printk(KERN_NOTICE "IPIPEIF DPCM = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_CFG2);
	printk(KERN_NOTICE "IPIPEIF CFG2 = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_INIRSZ);
	printk(KERN_NOTICE "IPIPEIF INIRSZ = 0x%x\n", utemp);
	utemp = regr_if(IPIPEIF_OCLIP);
	printk(KERN_NOTICE "IPIPEIF OCLIP = %x\n", utemp);
	utemp = regr_if(IPIPEIF_DTUDF);
	printk(KERN_NOTICE "IPIPEIF DTUDF = %x\n", utemp);
	utemp = regr_if(IPIPEIF_CLKDIV);
	printk(KERN_NOTICE "IPIPEIF CLKDIV = %x\n", utemp);
	utemp = regr_if(IPIPEIF_DPC1);
	printk(KERN_NOTICE "IPIPEIF DPC1 = %x\n", utemp);
	utemp = regr_if(IPIPEIF_DPC2);
	printk(KERN_NOTICE "IPIPEIF DPC2  = %x\n", utemp);
	utemp = regr_if(IPIPEIF_DFSGVL);
	printk(KERN_NOTICE "IPIPEIF DFSGVL  = %x\n", utemp);
	utemp = regr_if(IPIPEIF_DFSGTH);
	printk(KERN_NOTICE "IPIPEIF DFSGTH  = %x\n", utemp);
	utemp = regr_if(IPIPEIF_RSZ3A);
	printk(KERN_NOTICE "IPIPEIF RSZ3A  = %x\n", utemp);
	utemp = regr_if(IPIPEIF_INIRSZ3A);
	printk(KERN_NOTICE "IPIPEIF INIRSZ3A  = %x\n", utemp);
	printk(KERN_NOTICE "IPIPE Registers\n");
	printk(KERN_NOTICE
	       "**************************************************\n");
}
#else
void ipipeif_dump_register(void) {};
#endif
