/*
 * Copyright (C) 2008 MontaVista Software Inc.
 * Copyright (C) 2008 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef __DA8XX_LCDC_H__
#define __DA8XX_LCDC_H__

#define DA8XX_LCDC_REVISION	  0x4C100100

/* LCD Status Register */
#define LCD_END_OF_FRAME1           (1 << 9)
#define LCD_END_OF_FRAME0           (1 << 8)
#define LCD_PALETTE_LOADED          (1 << 6)
#define LCD_FIFO_UNDERFLOW          (1 << 5)
#define LCD_AC_BIAS_COUNT_STATUS    (1 << 3)
#define LCD_SYNC_LOST               (1 << 2)
#define LCD_FRAME_DONE              (1 << 0)

/* LCD DMA Control Register */
#define LCD_DMA_BURST_SIZE(x)       ((x) << 4)
#define LCD_DMA_BURST_1             0x0
#define LCD_DMA_BURST_2             0x1
#define LCD_DMA_BURST_4             0x2
#define LCD_DMA_BURST_8             0x3
#define LCD_DMA_BURST_16            0x4
#define LCD_DMA_BIG_ENDIAN          (1 << 1)
#define LCD_END_OF_FRAME_INT_ENA    (1 << 2)
#define LCD_DUAL_FRAME_BUFFER_ENABLE  (1 << 0)

/* LCD Control Register */
#define LCD_CLK_DIVISOR(x)          ((x) << 8)
#define LCD_RASTER_MODE             0x01

/* LCD Raster Control Register */
#define LCD_PALETTE_LOAD_MODE(x)    ((x) << 20)
#define PALETTE_AND_DATA	    0x00
#define PALETTE_ONLY                0x01
#define DATA_ONLY                   0x02

#define LCD_MONO_8BIT_MODE          (1 << 9)
#define LCD_RASTER_ORDER            (1 << 8)
#define LCD_TFT_MODE                (1 << 7)
#define LCD_UNDERFLOW_INT_ENA       (1 << 6)
#define LCD_SYNC_LOST_INT_ENA       (1 << 5)
#define LCD_LOAD_DONE_INT_ENA       (1 << 4)
#define LCD_FRAME_DONE_INT_ENA      (1 << 3)
#define LCD_AC_BIAS_INT_ENA         (1 << 2)
#define LCD_MONOCHROME_MODE         (1 << 1)
#define LCD_RASTER_ENABLE           (1 << 0)
#define LCD_NIBBLE_MODE_ENABLE      (1 << 22)
#define LCD_TFT_ALT_ENABLE	    (1 << 23)
#define LCD_STN_565_ENABLE	    (1 << 24)

/* LCD Interrupt Status */
#define LCD_ALL_STATUS_INTS         (LCD_END_OF_FRAME1 | LCD_END_OF_FRAME0 | \
				     LCD_PALETTE_LOADED | LCD_FIFO_UNDERFLOW | \
				     LCD_AC_BIAS_COUNT_STATUS | LCD_SYNC_LOST |\
				     LCD_FRAME_DONE)

/* LCD Raster Timing 2 Register */
#define LCD_AC_BIAS_TRANSITIONS_PER_INT(x)      ((x) << 16)
#define LCD_AC_BIAS_FREQUENCY(x)    ((x) << 8)
#define LCD_SYNC_CTRL               (1 << 25)
#define LCD_SYNC_EDGE               (1 << 24)
#define LCD_INVERT_PIXEL_CLOCK      (1 << 22)
#define LCD_INVERT_LINE_CLOCK       (1 << 21)
#define LCD_INVERT_FRAME_CLOCK      (1 << 20)

/* System Reset Disable Control Register */
#define LCD_RASTER_ENABLE           (1 << 0)
#define RST_LCD                     (1 << 24)

/* System Clock Disable Control Register */
#define LCD_CLK_DSBL                (1 << 25)
/* LCD Block */
#define  LCD_BLK_REV_REG                      0x0
#define  LCD_CTRL_REG                         0x4
#define  LCD_STAT_REG                         0x8
#define  LCD_LIDD_CTRL_REG                     0xC
#define  LCD_LIDD_CS0_CONFIG_REG                0x10
#define  LCD_LIDD_CS0_ADDR_REG                  0x14
#define  LCD_LIDD_CS0_DATA_REG                  0x18
#define  LCD_LIDD_CS1_CONFIG_REG                0x1C
#define  LCD_LIDD_CS1_ADDR_REG                  0x20
#define  LCD_LIDD_CS1_DATA_REG                  0x24
#define  LCD_RASTER_CTRL_REG                    0x28
#define  LCD_RASTER_TIMING_0_REG                0x2C
#define  LCD_RASTER_TIMING_1_REG                0x30
#define  LCD_RASTER_TIMING_2_REG                0x34
#define  LCD_RASTER_SUBPANEL_DISP_REG           0x38
#define  LCD_DMA_CTRL_REG                       0x40
#define  LCD_DMA_FRM_BUF_BASE_ADDR_0_REG        0x44
#define  LCD_DMA_FRM_BUF_CEILING_ADDR_0_REG     0x48
#define  LCD_DMA_FRM_BUF_BASE_ADDR_1_REG        0x4C
#define  LCD_DMA_FRM_BUF_CEILING_ADDR_1_REG     0x50
#endif /*__DA8XX_LCDC_H__*/
