/*
 * ALSA SoC I2S Audio Layer for TI DAVINCI processor
 *
 * Copyright (C) 2008 Texas Instruments.
 *
 * Based on sound/soc/davinci/davinci-i2s-mcbsp.h by Vladimir Barinov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DAVINCI_VCIF_H
#define _DAVINCI_VCIF_H

#define DAVINCI_VCIF_PID_REG	0x00
#define DAVINCI_VCIF_CTRL_REG	0x04
#define DAVINCI_VCIF_INTEN_REG	0x08
#define DAVINCI_VCIF_INTSTATUS_REG	0x0c
#define DAVINCI_VCIF_INTCLR_REG	0x10
#define DAVINCI_VCIF_EMUL_CTRL_REG	0x14
#define DAVINCI_VCIF_RFIFO_REG	0x20
#define DAVINCI_VCIF_WFIFO_REG	0x24
#define DAVINCI_VCIF_FIFOSTAT_REG	0x28
#define DAVINCI_VCIF_TST_CTRL_REG	0x2C

#define DAVINCI_VCIF_CTRL_MASK		0x5500
#define DAVINCI_VCIF_CTRL_RSTADC	(1 << 0)
#define DAVINCI_VCIF_CTRL_RSTDAC	(1 << 1)
#define DAVINCI_VCIF_CTRL_RD_BITS_8 (1 << 4)
#define DAVINCI_VCIF_CTRL_RD_UNSIGNED	(1 << 5)
#define DAVINCI_VCIF_CTRL_WD_BITS_8 (1 << 6)
#define DAVINCI_VCIF_CTRL_WD_UNSIGNED	(1 << 7)
#define DAVINCI_VCIF_CTRL_RFIFOEN	(1 << 8)
#define DAVINCI_VCIF_CTRL_RFIFOCL	(1 << 9)
#define DAVINCI_VCIF_CTRL_RFIFOMD_WORD_1	(1 << 10)
#define DAVINCI_VCIF_CTRL_WFIFOEN	(1 << 12)
#define DAVINCI_VCIF_CTRL_WFIFOCL	(1 << 13)
#define DAVINCI_VCIF_CTRL_WFIFOMD_WORD_1	(1 << 14)

#define DAVINCI_VCIF_INT_MASK	0x3F
#define DAVINCI_VCIF_INT_RDRDY_MASK	(1 << 0)
#define DAVINCI_VCIF_INT_RERROVF_MASK	(1 << 1)
#define DAVINCI_VCIF_INT_RERRUDR_MASK	(1 << 2)
#define DAVINCI_VCIF_INT_WDREQ_MASK	(1 << 3)
#define DAVINCI_VCIF_INT_WERROVF_MASK	(1 << 4)
#define DAVINCI_VCIF_INT_WERRUDR_MASK	(1 << 5)


#if defined(CONFIG_SND_DAVINCI_SOC_I2S_VCIF) || \
    defined(CONFIG_SND_DAVINCI_SOC_I2S_VCIF_MODULE)
extern struct snd_soc_cpu_dai vcif_i2s_dai[];
#else
#define vcif_i2s_dai		NULL
#endif

#endif
