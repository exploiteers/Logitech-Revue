#
# Makefile for the Linux lcd driver.
#

obj-$(CONFIG_DA8XX_LCD) += da8xx_lcd.o

da8xx_lcd-objs := lcd_drv.o  lidd_hal.o  hd44780_hal.o

da8xx_lcd.o:      $(da8xx_lcd-objs)
	$(LD) -r -o $@ $(da8xx_lcd-objs)
